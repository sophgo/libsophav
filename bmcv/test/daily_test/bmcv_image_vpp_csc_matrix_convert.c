#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmcv_api_ext_c.h"

int main() {
    char* filename_src = "/home/linaro/A2test/bmcv/test/res/1920x1080_yuv420.bin";
    char* filename_dst = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_vpp_csc_matrix_convert.bin";
    int in_width = 1920;
    int in_height = 1080;
    int out_width = 1920;
    int out_height = 1080;
    // int use_real_img = 1;
    bm_image_format_ext src_format = 0;
    bm_image_format_ext dst_format = 0;
    bmcv_rect_t crop_rect = {
        .start_x = 0,
        .start_y = 0,
        .crop_w = 200,
        .crop_h = 200};
    bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
    bm_image_data_format_ext data_format = 1;

    bm_status_t ret = BM_SUCCESS;

    int src_size = in_width * in_height * 3 / 2;
    int dst_size = out_width * out_height * 3 / 2;
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
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, out_height, out_width, dst_format, data_format, &dst, NULL);
    bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    int src_image_byte_size[4] = {0};
    bm_image_get_byte_size(src, src_image_byte_size);

    void *src_in_ptr[4] = {(void *)src_data,
                            (void *)((char *)src_data + src_image_byte_size[0]),
                            (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                            (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};

    bm_image_copy_host_to_device(src, (void **)src_in_ptr);

    ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, src, &dst, CSC_MAX_ENUM, NULL, algorithm, &crop_rect);

    int dst_image_byte_size[4] = {0};
    bm_image_get_byte_size(dst, dst_image_byte_size);

    void *dst_in_ptr[4] = {(void *)dst_data,
                            (void *)((char *)dst_data + dst_image_byte_size[0]),
                            (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1]),
                            (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1] + dst_image_byte_size[2])};

    bm_image_copy_device_to_host(dst, (void **)dst_in_ptr);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    bm_dev_free(handle);

    file = fopen(filename_dst, "wb");
    fwrite(dst_data, sizeof(unsigned char), dst_size, file);
    fclose(file);


    free(src_data);
    free(dst_data);

    return ret;
}
