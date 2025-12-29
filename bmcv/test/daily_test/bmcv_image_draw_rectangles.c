#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"

int main() {
    char* filename_src= "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char* filename_dst = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_draw_rectangles.bin";
    int in_width = 1920;
    int in_height = 1080;
    int rect_num = 1;
    int line_width = 10;
    bm_image_format_ext src_format = 8;
    bmcv_rect_t crop_rect = {
    .start_x = 100,
    .start_y = 100,
    .crop_w = 200,
    .crop_h = 200};
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;

    bm_status_t ret = BM_SUCCESS;

    int src_size = in_height * in_width * 3;
    unsigned char *input_data = (unsigned char *)malloc(src_size * sizeof(unsigned char));

    FILE *file;
    file = fopen(filename_src, "rb");
    fread(input_data, sizeof(unsigned char), src_size, file);
    fclose(file);

    bm_handle_t handle;
    int dev_id = 0;
    bm_image src;

    ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
    printf("Create bm handle failed. ret = %d\n", ret);
    return ret;
    }

    bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);

    void *src_in_ptr[3] = {(void *)input_data,
                        (void *)((char *)input_data + in_height * in_width),
                        (void *)((char *)input_data + 2 * in_height * in_width)};

    bm_image_copy_host_to_device(src, (void **)src_in_ptr);
    ret = bmcv_image_draw_rectangle(handle, src, rect_num, &crop_rect, line_width, r, g, b);
    bm_image_copy_device_to_host(src, (void **)src_in_ptr);

    bm_image_destroy(&src);
    bm_dev_free(handle);

    file = fopen(filename_dst, "wb");
    fwrite(input_data, sizeof(unsigned char), src_size, file);
    fclose(file);


    free(input_data);
    return ret;
}
