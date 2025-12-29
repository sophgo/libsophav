#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 2160, dev_id = 0;
bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_yuv420.bin", *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_vpp_stitch.bin";
bmcv_rect_t dst_rect0 = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
bmcv_rect_t dst_rect1 = {.start_x = 0, .start_y = 1080, .crop_w = 1920, .crop_h = 1080};
bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
bm_handle_t handle = NULL;

int main() {
    bm_status_t ret;
    bm_image src, dst;
    ret = bm_dev_request(&handle, dev_id);
    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

    ret = bm_image_alloc_dev_mem(src,BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst,BMCV_HEAP1_ID);

    int src_size = src_h * src_w * 3 / 2;
    int dst_size = src_h * src_w * 3 / 2;
    unsigned char *src_data = (unsigned char *)malloc(src_size);
    unsigned char *dst_data = (unsigned char *)malloc(dst_size);

    FILE *file;
    file = fopen(src_name, "rb");
    fread(src_data, sizeof(unsigned char), src_size, file);
    fclose(file);

    int src_image_byte_size[4] = {0};
    bm_image_get_byte_size(src, src_image_byte_size);
    void *src_in_ptr[4] = {(void *)src_data,
                        (void *)((char *)src_data + src_image_byte_size[0]),
                        (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                        (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};
    bm_image_copy_host_to_device(src, (void **)src_in_ptr);

    bmcv_rect_t rect = {.start_x = 0, .start_y = 0, .crop_w = src_w, .crop_h = src_h};
    bmcv_rect_t src_rect[2] = {rect, rect};
    bmcv_rect_t dst_rect[2] = {dst_rect0, dst_rect1};

    bm_image input[2] = {src, src};

    bmcv_image_vpp_stitch(handle, 2, input, dst, dst_rect, src_rect, BMCV_INTER_LINEAR);

    int dst_image_byte_size[4] = {0};
    bm_image_get_byte_size(dst, dst_image_byte_size);
    void *dst_in_ptr[4] = {(void *)dst_data,
                        (void *)((char *)dst_data + dst_image_byte_size[0]),
                        (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1]),
                        (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1] + dst_image_byte_size[2])};
    bm_image_copy_device_to_host(dst, (void **)dst_in_ptr);

    FILE *fp_dst = fopen(dst_name, "wb");
    if (fwrite((void *)dst_data, 1, dst_size, fp_dst) < (unsigned int)dst_size){
        printf("file size is less than %d required bytes\n", dst_size);
    };
    fclose(fp_dst);


    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    bm_dev_free(handle);

    return ret;
}

