#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmcv_api_ext_c.h"


int main() {
    char *filename_src = "/home/linaro/A2test/bmcv/test/res/1920x1080_yuv420.bin";
    char *filename_dst = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_vpp_convert_padding.bin";
    int in_width = 1920;
    int in_height = 1080;
    int out_width = 1920;
    int out_height = 1080;
    bm_image_format_ext src_format = 0;     // FORMAT_YUV420P
    bm_image_format_ext dst_format = 0;
    bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;

    bmcv_rect_t crop_rect = {
        .start_x = 100,
        .start_y = 100,
        .crop_w = 500,
        .crop_h = 500
        };

    bmcv_padding_attr_t padding_rect = {
        .dst_crop_stx = 0,
        .dst_crop_sty = 0,
        .dst_crop_w = 1000,
        .dst_crop_h = 1000,
        .padding_r = 155,
        .padding_g = 20,
        .padding_b = 36,
        .if_memset = 1
        };

    bm_status_t ret = BM_SUCCESS;

    int src_size = in_height * in_width * 3 / 2;
    int dst_size = in_height * in_width * 3 / 2;
    unsigned char *src_data = (unsigned char *)malloc(src_size);
    unsigned char *dst_data = (unsigned char *)malloc(dst_size);

    FILE *file;
    file = fopen(filename_src, "rb");
    fread(src_data, sizeof(unsigned char), src_size, file);
    fclose(file);

    bm_handle_t handle = NULL;
    int dev_id = 0;
    bm_image src, dst;

    ret = bm_dev_request(&handle, dev_id);

    bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, out_height, out_width, dst_format, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
    bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    int src_image_byte_size[4] = {0};
    bm_image_get_byte_size(src, src_image_byte_size);
    void *src_in_ptr[4] = {(void *)src_data,
                        (void *)((char *)src_data + src_image_byte_size[0]),
                        (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                        (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};

    bm_image_copy_host_to_device(src, (void **)src_in_ptr);
    ret = bmcv_image_vpp_convert_padding(handle, 1, src, &dst, &padding_rect, &crop_rect, algorithm);

    int dst_image_byte_size[4] = {0};
    bm_image_get_byte_size(dst, dst_image_byte_size);
    void *dst_in_ptr[4] = {(void *)dst_data,
                        (void *)((char *)dst_data + dst_image_byte_size[0]),
                        (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1]),
                        (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1] + dst_image_byte_size[2])};

    bm_image_copy_device_to_host(dst, (void **)dst_in_ptr);

    FILE *fp_dst = fopen(filename_dst, "wb");
    if (fwrite((void *)dst_data, 1, dst_size, fp_dst) < (unsigned int)dst_size){
        printf("file size is less than %d required bytes\n", dst_size);
    };
    fclose(fp_dst);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    bm_dev_free(handle);

    free(src_data);
    free(dst_data);

    return ret;
}
