#include <sys/time.h>

#include "jpeg_dec_common.h"
#include "bm_ioctl.h"
#include "helper/md5.h"

int save_yuv(FILE *fp, BmJpuJPEGDecInfo *dec_info, uint8_t *virt_addr)
{
    for (unsigned int i = 0; i < dec_info->actual_frame_height; i++) {
        fwrite(virt_addr + dec_info->y_offset + i * dec_info->y_stride, 1, dec_info->actual_frame_width, fp);
    }

    switch (dec_info->image_format) {
        case BM_JPU_IMAGE_FORMAT_NV12:
        case BM_JPU_IMAGE_FORMAT_NV21:
            for (unsigned int i = 0; i < dec_info->actual_frame_height / 2; i++) {
                fwrite(virt_addr + dec_info->cb_offset + i * dec_info->cbcr_stride, 1, dec_info->actual_frame_width, fp);
            }
            break;
        case BM_JPU_IMAGE_FORMAT_YUV420P:
            for (unsigned int i = 0; i < dec_info->actual_frame_height / 2; i++) {
                fwrite(virt_addr + dec_info->cb_offset + i * dec_info->cbcr_stride, 1, dec_info->actual_frame_width / 2, fp);
            }
            for (unsigned int i = 0; i < dec_info->actual_frame_height / 2; i++) {
                fwrite(virt_addr + dec_info->cr_offset + i * dec_info->cbcr_stride, 1, dec_info->actual_frame_width / 2, fp);
            }
            break;
        case BM_JPU_IMAGE_FORMAT_NV16:
        case BM_JPU_IMAGE_FORMAT_NV61:
            for (unsigned int i = 0; i < dec_info->actual_frame_height; i++) {
                fwrite(virt_addr + dec_info->cb_offset + i * dec_info->cbcr_stride, 1, dec_info->actual_frame_width, fp);
            }
            break;
        case BM_JPU_IMAGE_FORMAT_YUV422P:
            for (unsigned int i = 0; i < dec_info->actual_frame_height; i++) {
                fwrite(virt_addr + dec_info->cb_offset + i * dec_info->cbcr_stride, 1, dec_info->actual_frame_width / 2, fp);
            }
            for (unsigned int i = 0; i < dec_info->actual_frame_height; i++) {
                fwrite(virt_addr + dec_info->cr_offset + i * dec_info->cbcr_stride, 1, dec_info->actual_frame_width / 2, fp);
            }
            break;
        case BM_JPU_IMAGE_FORMAT_YUV444P:
        case BM_JPU_IMAGE_FORMAT_RGB:
            for (unsigned int i = 0; i < dec_info->actual_frame_height; i++) {
                fwrite(virt_addr + dec_info->cb_offset + i * dec_info->cbcr_stride, 1, dec_info->actual_frame_width, fp);
            }
            for (unsigned int i = 0; i < dec_info->actual_frame_height; i++) {
                fwrite(virt_addr + dec_info->cr_offset + i * dec_info->cbcr_stride, 1, dec_info->actual_frame_width, fp);
            }
            break;
        case BM_JPU_IMAGE_FORMAT_GRAY:
            break;
        default:
            fprintf(stderr, "unsupported image format: %d\n", dec_info->image_format);
            return -1;
    }

    return 0;
}

BmJpuDecReturnCodes start_decode(BmJpuJPEGDecoder *jpeg_decoder, uint8_t *bs_buffer, size_t bs_size, FILE *fp_out, const uint8_t *ref_md5, int dump_crop, int timeout, int timeout_count)
{
    BmJpuDecReturnCodes ret = BM_JPU_DEC_RETURN_CODE_OK;
    BmJpuJPEGDecInfo dec_info;
    unsigned long long phys_addr = 0;
    uint8_t *virt_addr = NULL;
    size_t size = 0;
    struct timeval tv_start, tv_end;

    /* perform the actual jpeg decoding */
    gettimeofday(&tv_start, NULL);
    ret = bm_jpu_jpeg_dec_decode(jpeg_decoder, bs_buffer, bs_size, timeout, timeout_count);
    if (ret != 0) {
        fprintf(stderr, "decode failed!\n");
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }
    gettimeofday(&tv_end, NULL);
    printf("decode time is %ld us\n", (tv_end.tv_sec - tv_start.tv_sec) * 1000 * 1000 + (tv_end.tv_usec - tv_start.tv_usec));

    /* get output info */
    bm_jpu_jpeg_dec_get_info(jpeg_decoder, &dec_info);

    printf("aligned frame size: %u x %u\n"
           "pixel actual frame size: %u x %u\n"
           "pixel Y/Cb/Cr stride: %u/%u/%u\n"
           "pixel Y/Cb/Cr size: %u/%u/%u\n"
           "pixel Y/Cb/Cr offset: %u/%u/%u\n"
           "image format: %d\n",
           dec_info.aligned_frame_width, dec_info.aligned_frame_height,
           dec_info.actual_frame_width, dec_info.actual_frame_height,
           dec_info.y_stride, dec_info.cbcr_stride, dec_info.cbcr_stride,
           dec_info.y_size, dec_info.cbcr_size, dec_info.cbcr_size,
           dec_info.y_offset, dec_info.cb_offset, dec_info.cr_offset,
           dec_info.image_format);

    if (dec_info.framebuffer == NULL) {
        fprintf(stderr, "no framebuffer returned!\n");
        return BM_JPU_DEC_RETURN_CODE_INVALID_FRAMEBUFFER;
    }

    /* mmap DMA buffer to host */
    size = dec_info.framebuffer->dma_buffer->size;
    printf("decoded output frame size: %lu bytes\n", size);

    phys_addr = bm_mem_get_device_addr(*(dec_info.framebuffer->dma_buffer));
    // phys_addr = dec_info.framebuffer->dma_buffer->u.device.device_addr;
    printf("phys addr: %#llx, size: %u\n", phys_addr, dec_info.framebuffer->dma_buffer->size);

#ifdef BOARD_FPGA
    // int chn_fd = bm_jpu_dec_get_channel_fd(jpeg_decoder->decoder->channel_id);
    // virt_addr = bmjpeg_ioctl_mmap(chn_fd, phys_addr, size);
    virt_addr = bmjpeg_devm_map(phys_addr, size);
#else
    unsigned long long vaddr = 0;
    bm_handle_t bm_handle;

    bm_handle = bm_jpu_dec_get_bm_handle(jpeg_decoder->device_index);
    bm_mem_mmap_device_mem(bm_handle, dec_info.framebuffer->dma_buffer, &vaddr);
    virt_addr = (uint8_t *)vaddr;
#endif
    printf("virt addr: %p\n", virt_addr);

    if (dump_crop == 1) {
        save_yuv(fp_out, &dec_info, virt_addr);
    } else {
        fwrite(virt_addr, 1, size, fp_out);
    }

    if (ref_md5 != NULL) {
        uint8_t *out_md5 = NULL;
        int i;

        out_md5 = (uint8_t *)malloc(sizeof(uint8_t)*MD5_DIGEST_LENGTH);
        MD5(virt_addr, size, out_md5);

        /* convert md5 value to md5 string */
        char md5_str[MD5_STRING_LENGTH + 1];
        for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
            snprintf(md5_str + i*2, 3, "%02x", out_md5 + i);
        }
        fprintf(stdout, "output file md5: %s\n", md5_str);

        if (memcmp(out_md5, ref_md5, MD5_DIGEST_LENGTH) != 0) {
            fprintf(stderr, "md5 dismatch\n");
        }

        free(out_md5);
    }

#ifdef BOARD_FPGA
    // bmjpeg_ioctl_munmap(virt_addr, size);
    bmjpeg_devm_unmap(virt_addr, size);
#else
    bm_mem_unmap_device_mem(bm_handle, virt_addr, size);
#endif

    bm_jpu_jpeg_dec_frame_finished(jpeg_decoder, dec_info.framebuffer);

    return ret;
}
