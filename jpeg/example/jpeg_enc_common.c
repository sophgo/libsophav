#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "jpeg_enc_common.h"
#include "bm_ioctl.h"
#include "helper/md5.h"

#define HEAP_MASK_1_2 0x06


void ConvertToImageFormat(BmJpuImageFormat *image_format, BmJpuColorFormat color_format, BmJpuChromaFormat cbcr_interleave)
{
    // TODO: support packed format?
    if (color_format == BM_JPU_COLOR_FORMAT_YUV420) {
        if (cbcr_interleave == BM_JPU_CHROMA_FORMAT_CBCR_INTERLEAVE) {
            *image_format = BM_JPU_IMAGE_FORMAT_NV12;
        } else if (cbcr_interleave == BM_JPU_CHROMA_FORMAT_CRCB_INTERLEAVE) {
            *image_format = BM_JPU_IMAGE_FORMAT_NV21;
        } else {
            *image_format = BM_JPU_IMAGE_FORMAT_YUV420P;
        }
    } else if (color_format == BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL) {
        if (cbcr_interleave == BM_JPU_CHROMA_FORMAT_CBCR_INTERLEAVE) {
            *image_format = BM_JPU_IMAGE_FORMAT_NV16;
        } else if (cbcr_interleave == BM_JPU_CHROMA_FORMAT_CRCB_INTERLEAVE) {
            *image_format = BM_JPU_IMAGE_FORMAT_NV61;
        } else {
            *image_format = BM_JPU_IMAGE_FORMAT_YUV422P;
        }
    } else if (color_format == BM_JPU_COLOR_FORMAT_YUV444) {
        // only support planar type
        *image_format = BM_JPU_IMAGE_FORMAT_YUV444P;
    } else {
        *image_format = BM_JPU_IMAGE_FORMAT_GRAY;
    }
}

void* acquire_output_buffer(void *context, size_t size, void **acquired_handle)
{
    printf("acquire_output_buffer callback line = %d, size = %lu.\n", __LINE__, size);

    void *mem;

    UNUSED(context);

    /* in this example, "acquire" a buffer by simply allocating it with malloc() */
    mem = malloc(size);
    *acquired_handle = mem;
    printf("mem = %p, *acquired_handle = %p\n", mem, *acquired_handle);

    return mem;
}

void finish_output_buffer(void *context, void *acquired_handle)
{
    UNUSED(context);

    if (acquired_handle != NULL)
        free(acquired_handle);
}

static int read_and_copy_to_device(bm_handle_t bm_handle, bm_device_mem_t *dev_mem, size_t size, FILE *fp_in, int is_pcie_mode)
{
    int ret = 0;
    uint8_t *virt_addr = NULL;
#ifndef BM_PCIE_MODE
    unsigned long long vaddr = 0;
    ret = bm_mem_mmap_device_mem(bm_handle, dev_mem, &vaddr);
    if (ret != BM_SUCCESS) {
        fprintf(stderr, "bm_mem_mmap_device_mem failed, device_addr: %#lx, size: %zu\n",
                dev_mem->u.device.device_addr, dev_mem->size);
        return ret;
    }
    virt_addr = (uint8_t *)vaddr;
    memset(virt_addr, 0, size);
    fread(virt_addr, sizeof(uint8_t), size, fp_in);
    bm_mem_flush_device_mem(bm_handle, dev_mem);
    bm_mem_unmap_device_mem(bm_handle, virt_addr, size);
#else
    virt_addr = (uint8_t *)malloc(size);
    if (!virt_addr) {
        fprintf(stderr, "can't malloc memory to save input data\n");
        return -1;
    }
    fread(virt_addr, sizeof(uint8_t), size, fp_in);
    bm_memcpy_s2d_partial(bm_handle, *dev_mem, virt_addr, size);
    free(virt_addr);
#endif
    return 0;
}

BmJpuEncReturnCodes start_encode(BmJpuJPEGEncoder *jpeg_encoder, EncParam *enc_params, FILE *fp_in, FILE *fp_out, int inst_idx, const uint8_t *ref_md5)
{
    BmJpuEncReturnCodes ret = BM_JPU_ENC_RETURN_CODE_OK;
    BmJpuFramebuffer framebuffer;
    BmJpuJPEGEncParams jpu_enc_params;
    BmJpuImageFormat image_format;
    void *acquired_handle = NULL;
    size_t output_buffer_size = 0;
    unsigned int rotation_info;
    unsigned int mirror_info;

    unsigned int y_size;
    unsigned int c_size;
    unsigned int total_size;
    uint8_t *virt_addr = NULL;
    bm_handle_t bm_handle;
    bm_status_t bm_ret = BM_SUCCESS;
    bm_device_mem_t *bitstream_buffer = NULL;
    struct timeval tv_start, tv_end;

    y_size = enc_params->y_stride * enc_params->aligned_height;
    switch (enc_params->pix_fmt) {
        case BM_JPU_COLOR_FORMAT_YUV400:
            c_size = 0;
            break;
        case BM_JPU_COLOR_FORMAT_YUV420:
            c_size = enc_params->y_stride / 2 * (enc_params->aligned_height / 2);
            break;
        case BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL:
        case BM_JPU_COLOR_FORMAT_YUV422_VERTICAL:
            c_size = enc_params->y_stride / 2 * enc_params->aligned_height;
            break;
        case BM_JPU_COLOR_FORMAT_YUV444:
            c_size = enc_params->y_stride * enc_params->aligned_height;
            break;
        default:
            fprintf(stderr, "unsupported pixel format: %d\n", enc_params->pix_fmt);
            return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    total_size = y_size + c_size * 2;

    /* Initialisze the input framebuffer */
    memset(&framebuffer, 0, sizeof(BmJpuFramebuffer));
    framebuffer.y_stride = enc_params->y_stride;
    framebuffer.cbcr_stride = enc_params->c_stride;
    framebuffer.y_offset = 0;
    framebuffer.cb_offset = y_size;
    framebuffer.cr_offset = y_size + c_size;

    bm_handle = bm_jpu_enc_get_bm_handle(jpeg_encoder->device_index);

    if (enc_params->yuv_seperate == 0) {
        framebuffer.dma_buffer = (bm_device_mem_t *)malloc(sizeof(bm_device_mem_t));
        ret = bm_malloc_device_byte_heap_mask(bm_handle, framebuffer.dma_buffer, HEAP_MASK_1_2, total_size);
        if (ret != 0) {
            printf("malloc device memory size = %u failed, ret = %d\n", total_size, ret);
            goto finish;
        }
        unsigned long long phys_addr = bm_mem_get_device_addr(*framebuffer.dma_buffer);
        printf("phys addr: %#llx, total_size = %u\n", phys_addr, total_size);

        ret = read_and_copy_to_device(bm_handle, framebuffer.dma_buffer, total_size, fp_in, 0);
        if (ret != 0) goto finish;
    } else {
        framebuffer.dma_buffer_y = (bm_device_mem_t *)malloc(sizeof(bm_device_mem_t));
        ret = bm_malloc_device_byte_heap_mask(bm_handle, framebuffer.dma_buffer_y, HEAP_MASK_1_2, y_size);
        if (ret != 0) {
            printf("malloc device memory size = %u failed, ret = %d\n", y_size, ret);
            goto finish;
        }
        unsigned long long phys_addr = bm_mem_get_device_addr(*framebuffer.dma_buffer_y);
        printf("phys_addr_y: %#llx, y_size = %u\n", phys_addr, y_size);

        ret = read_and_copy_to_device(bm_handle, framebuffer.dma_buffer_y, y_size, fp_in, 0);
        if (ret != 0) goto finish;

        if(enc_params->cbcr_interleave == 0){
            framebuffer.dma_buffer_u = (bm_device_mem_t *)malloc(sizeof(bm_device_mem_t));
            ret = bm_malloc_device_byte_heap_mask(bm_handle, framebuffer.dma_buffer_u, HEAP_MASK_1_2, c_size);
            if (ret != 0) {
                printf("malloc device memory size = %u failed, ret = %d\n", c_size, ret);
                goto finish;
            }

            framebuffer.dma_buffer_v = (bm_device_mem_t *)malloc(sizeof(bm_device_mem_t));
            ret = bm_malloc_device_byte_heap_mask(bm_handle, framebuffer.dma_buffer_v, HEAP_MASK_1_2, c_size);
            if (ret != 0) {
                printf("malloc device memory size = %u failed, ret = %d\n", c_size, ret);
                goto finish;
            }

            phys_addr = bm_mem_get_device_addr(*framebuffer.dma_buffer_u);
            printf("phys_addr_u: %#llx, c_size * 2 = %u\n", phys_addr, c_size);
            phys_addr = bm_mem_get_device_addr(*framebuffer.dma_buffer_v);
            printf("phys_addr_u: %#llx, c_size * 2 = %u\n", phys_addr, c_size);


            ret = read_and_copy_to_device(bm_handle, framebuffer.dma_buffer_u, c_size, fp_in, 0);
            if (ret != 0) goto finish;

            ret = read_and_copy_to_device(bm_handle, framebuffer.dma_buffer_v, c_size, fp_in, 0);
            if (ret != 0) goto finish;
        }
        else {
            framebuffer.dma_buffer_u = (bm_device_mem_t *)malloc(sizeof(bm_device_mem_t));
            ret = bm_malloc_device_byte_heap_mask(bm_handle, framebuffer.dma_buffer_u, HEAP_MASK_1_2, c_size * 2);
            if (ret != 0) {
                printf("malloc device memory size = %u failed, ret = %d\n", c_size * 2, ret);
                goto finish;
            }
            framebuffer.dma_buffer_v = framebuffer.dma_buffer_u;

            phys_addr = bm_mem_get_device_addr(*framebuffer.dma_buffer_u);
            printf("phys_addr_uv: %#llx, c_size * 2 = %u\n", phys_addr, c_size * 2);

            ret = read_and_copy_to_device(bm_handle, framebuffer.dma_buffer_u, c_size * 2, fp_in, 0);
            if (ret != 0) goto finish;
        }
    }

    ConvertToImageFormat(&image_format, enc_params->pix_fmt, enc_params->cbcr_interleave);
    memset(&jpu_enc_params, 0, sizeof(BmJpuJPEGEncParams));
    jpu_enc_params.frame_width = enc_params->width;
    jpu_enc_params.frame_height = enc_params->height;
    jpu_enc_params.quality_factor = enc_params->quality_factor;
    jpu_enc_params.image_format = image_format;
    jpu_enc_params.acquire_output_buffer = acquire_output_buffer;
    jpu_enc_params.finish_output_buffer = finish_output_buffer;
    jpu_enc_params.output_buffer_context = NULL;
    jpu_enc_params.timeout = enc_params->timeout;
    jpu_enc_params.timeout_count = enc_params->timeout_count;

    if (enc_params->bs_set_on_process) {
        bitstream_buffer = (bm_device_mem_t*)malloc(sizeof(bm_device_mem_t));
        ret = bm_malloc_device_byte_heap(bm_handle, bitstream_buffer, 1, enc_params->bitstream_size);
        if (ret != (BmJpuEncReturnCodes)BM_SUCCESS) {
            fprintf(stderr, "malloc device memory size = %d failed, ret = %d\n", enc_params->bitstream_size, ret);
            if (bitstream_buffer != NULL) {
                free(bitstream_buffer);
                bitstream_buffer = NULL;
            }
            ret = -1;
            goto finish;
        }

        jpu_enc_params.bs_buffer_phys_addr = bitstream_buffer->u.device.device_addr;
        jpu_enc_params.bs_buffer_size = bitstream_buffer->size;
    }

    rotation_info = enc_params->rotate & 0x3;
    if (rotation_info != 0) {
        jpu_enc_params.rotationEnable = 1;
        if (rotation_info == 1) {
            jpu_enc_params.rotationAngle = 90;
        } else if (rotation_info == 2) {
            jpu_enc_params.rotationAngle = 180;
        } else if (rotation_info == 3) {
            jpu_enc_params.rotationAngle = 270;
        }
    }

    mirror_info = enc_params->rotate & 0xc;
    if (mirror_info != 0) {
        jpu_enc_params.mirrorEnable = 1;
        if (mirror_info >> 2 == 1) {
            jpu_enc_params.mirrorDirection = 1;
        } else if (mirror_info >> 2 == 2) {
            jpu_enc_params.mirrorDirection = 2;
        } else {
            jpu_enc_params.mirrorDirection = 3;
        }
    }

    /* Do the actual encoding */
    gettimeofday(&tv_start, NULL);
    ret = bm_jpu_jpeg_enc_encode(jpeg_encoder, &framebuffer, &jpu_enc_params, &acquired_handle, &output_buffer_size);
    if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
        fprintf(stderr, "failed to encode this image\n");
        goto finish;
    }
    gettimeofday(&tv_end, NULL);
    printf("inst %d: encode time is %ld us\n", inst_idx, (tv_end.tv_sec - tv_start.tv_sec) * 1000 * 1000 + (tv_end.tv_usec - tv_start.tv_usec));

    fwrite(acquired_handle, 1, output_buffer_size, fp_out);
    fflush(fp_out);
    printf("write %lu bytes data to output file\n", output_buffer_size);

    if (ref_md5 != NULL) {
        uint8_t *out_md5 = NULL;
        int i;

        out_md5 = (uint8_t *)malloc(sizeof(uint8_t)*MD5_DIGEST_LENGTH);
        MD5(acquired_handle, output_buffer_size, out_md5);

        /* convert md5 value to md5 string */
        char md5_str[MD5_STRING_LENGTH + 1];
        for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
            snprintf(md5_str + i*2, 3, "%02x", out_md5[i]);
        }
        fprintf(stdout, "output file md5: %s\n", md5_str);

        if (memcmp(out_md5, ref_md5, MD5_DIGEST_LENGTH) != 0) {
            fprintf(stderr, "md5 dismatch\n");
        }

        free(out_md5);
    }

finish:
    if (acquired_handle != NULL) {
        finish_output_buffer(NULL, acquired_handle);
    }

    if (framebuffer.dma_buffer != NULL) {
        bm_free_device(bm_handle, *framebuffer.dma_buffer);
        free(framebuffer.dma_buffer);
    }

    if (framebuffer.dma_buffer_y != NULL) {
        bm_free_device(bm_handle, *framebuffer.dma_buffer_y);
        free(framebuffer.dma_buffer_y);
    }

    if (framebuffer.dma_buffer_u != NULL) {
        bm_free_device(bm_handle, *framebuffer.dma_buffer_u);
        free(framebuffer.dma_buffer_u);
    }

    if (bitstream_buffer != NULL) {
        bm_free_device(bm_handle, *bitstream_buffer);
        bitstream_buffer = NULL;
    }
    // bm_dev_free(bm_handle);

    return ret;
}
