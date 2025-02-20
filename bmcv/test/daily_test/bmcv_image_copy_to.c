#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "bmcv_api_ext_c.h"

typedef enum { COPY_TO_GRAY = 0, COPY_TO_BGR, COPY_TO_RGB } padding_corlor_e;
typedef enum { PLANNER = 0, PACKED } padding_format_e;

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


int main() {
    int         dev_id = 0;
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, dev_id);

    int in_w    = 400;
    int in_h    = 400;
    int out_w   = 800;
    int out_h   = 800;
    int start_x = 200;
    int start_y = 200;

    int image_format = FORMAT_RGB_PLANAR;
    int data_type = DATA_TYPE_EXT_FLOAT32;
    int channel   = 3;

    int image_n = 1;

    float* src_data = (float *)malloc(image_n * channel * in_w * in_h * sizeof(float));
    float* res_data = (float *)malloc(image_n * channel * out_w * out_h * sizeof(float));

    for (int i = 0; i < image_n * channel * in_w * in_h; i++) {
        src_data[i] = rand() % 255;
    }
    // calculate res
    bmcv_copy_to_atrr_t copy_to_attr;
    copy_to_attr.start_x    = start_x;
    copy_to_attr.start_y    = start_y;
    copy_to_attr.padding_r  = 0;
    copy_to_attr.padding_g  = 0;
    copy_to_attr.padding_b  = 0;
    copy_to_attr.if_padding = 1;
    bm_image input, output;
    bm_image_create(handle,
                    in_h,
                    in_w,
                    (bm_image_format_ext)image_format,
                    (bm_image_data_format_ext)data_type,
                    &input, NULL);
    bm_image_alloc_dev_mem(input, BMCV_HEAP1_ID);
    bm_image_copy_host_to_device(input, (void **)&src_data);
    bm_image_create(handle,
                    out_h,
                    out_w,
                    (bm_image_format_ext)image_format,
                    (bm_image_data_format_ext)data_type,
                    &output, NULL);
    bm_image_alloc_dev_mem(output, BMCV_HEAP1_ID);
    ret = bmcv_image_copy_to(handle, copy_to_attr, input, output);

    if (BM_SUCCESS != ret) {
        printf("bmcv_copy_to error 1 !!!\n");
        bm_image_destroy(&input);
        bm_image_destroy(&output);
        free(src_data);
        free(res_data);
        return -1;
    }
    bm_image_copy_device_to_host(output, (void **)&res_data);

    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_copy_to_dst.bin";
    writeBin(dst_name, res_data, out_w * out_h * 3);
    writeBin("/home/linaro/A2test/bmcv/test/res/out/daily_test_copy_to_src.bin", src_data, in_h * in_w * 3);

    bm_image_destroy(&input);
    bm_image_destroy(&output);
    free(src_data);
    free(res_data);

    return ret;
}