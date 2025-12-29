#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmcv_api_ext_c.h"

static int writeBin(const char* path, void* output_data, int size)
{
    int len = 0;
    FILE* fp_dst = fopen(path, "wb+");

    if (fp_dst == NULL) {
        perror("Error opening file\n");
        return -1;
    }

    len = fwrite((void*)output_data, 1, size, fp_dst);
    if (len < size) {
        printf("file size = %d is less than required bytes = %d\n", len, size);
        return -1;
    }

    fclose(fp_dst);
    return 0;
}
bm_status_t open_water(
    bm_handle_t           handle,
    char *                src_name,
    int                   src_size,
    bm_device_mem_t *     dst)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned char * src = (unsigned char *)malloc(sizeof(unsigned char) * src_size);
    ret = bm_malloc_device_byte(handle, dst, src_size);
    if(ret != BM_SUCCESS){
        printf("bm_malloc_device_byte fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
        goto fail;
    }

    FILE * fp_src = fopen(src_name, "rb");
    size_t read_size = fread((void *)src, src_size, 1, fp_src);
    printf("fread %ld byte\n", read_size);
    fclose(fp_src);
#ifdef _FPGA
    ret = bm_memcpy_s2d_fpga(handle, dst[0], (void*)src);
#else
    ret = bm_memcpy_s2d(handle, dst[0], (void*)src);
#endif
    if(ret != BM_SUCCESS){
        printf("bm_memcpy_s2d fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
    }
fail:
    free(src);
    return ret;
}


int main() {
    char *filename_src = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgbp.bin";
    char *filename_water = "/home/linaro/A2test/bmcv/test/res/800x600_yuv420.bin";
    char *filename_dst = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_watermark_superpose.bin";
    int in_width = 1920;
    int in_height = 1080;
    int water_width = 800;
    int water_height = 600;
    bm_image_format_ext src_format = FORMAT_RGB_PLANAR;
    bmcv_rect_t rects = {
        .start_x = 200,
        .start_y = 200,
        .crop_w = 800,
        .crop_h = 600};
    bmcv_color_t color = {
        .r = 0,
        .g = 0,
        .b = 0};

    bm_status_t ret = BM_SUCCESS;

    int src_size = in_width * in_height * 3;
    int water_size = water_width * water_height * 3;
    unsigned char *src_data = (unsigned char *)malloc(src_size);
    unsigned char *water_data = (unsigned char *)malloc(water_size);

    FILE *file;
    file = fopen(filename_water, "rb");
    fread(water_data, sizeof(unsigned char), water_size, file);
    fclose(file);

    file = fopen(filename_src, "rb");
    fread(src_data, sizeof(unsigned char), src_size, file);
    fclose(file);

    bm_handle_t handle = NULL;
    int dev_id = 0;
    bm_image src;
    bm_device_mem_t water;
    ret = bm_dev_request(&handle, dev_id);

    open_water(handle, filename_water, water_size, &water);

    bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);

    int src_image_byte_size[4] = {0};
    bm_image_get_byte_size(src, src_image_byte_size);

    void *src_in_ptr[4] = {(void *)src_data,
                        (void *)((char *)src_data + src_image_byte_size[0]),
                        (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                        (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};

    bm_image_copy_host_to_device(src, (void **)src_in_ptr);
    ret = bmcv_image_watermark_superpose(handle, &src, &water, 1, 0, water_width, &rects, color);
    bm_image_copy_device_to_host(src, (void **)src_in_ptr);

    writeBin(filename_dst, src_data, src_size);

    bm_image_destroy(&src);
    bm_dev_free(handle);

    free(src_data);
    free(water_data);

    return ret;
}
