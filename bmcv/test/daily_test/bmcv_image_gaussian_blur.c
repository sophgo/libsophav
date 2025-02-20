#include <stdio.h>
#include "bmcv_api_ext_c.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <float.h>
#include <math.h>

static void read_bin(const char *input_path, unsigned char *input_data, int width, int height) {
    FILE *fp_src = fopen(input_path, "rb");
    if (fp_src == NULL) {
    printf("无法打开输出文件 %s\n", input_path);
    return;
    }
    if(fread(input_data, sizeof(char), width * height, fp_src) != 0) {
    printf("read image success\n");
    }
    fclose(fp_src);
}

static void write_bin(const char *output_path, unsigned char *output_data, int width, int height) {
    FILE *fp_dst = fopen(output_path, "wb");
    if (fp_dst == NULL) {
    printf("无法打开输出文件 %s\n", output_path);
    return;
    }
    fwrite(output_data, sizeof(char), width * height, fp_dst);
    fclose(fp_dst);
}




int main() {
    int width = 1920;
    int height = 1080;
    int format = FORMAT_GRAY;
    float sigmaX = (float)rand() / RAND_MAX * 5.0f;
    float sigmaY = (float)rand() / RAND_MAX * 5.0f;
    int ret = 0;
    char *input_path = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *output_path = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_gaussian_blur.bin";
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    int kw = 3, kh = 3;

    unsigned char *input_data = (unsigned char*)malloc(width * height);
    unsigned char *output_tpu = (unsigned char*)malloc(width * height);

    read_bin(input_path, input_data, width, height);

    bm_image img_i;
    bm_image img_o;

    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &img_i, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &img_o, NULL);
    bm_image_alloc_dev_mem(img_i, 2);
    bm_image_alloc_dev_mem(img_o, 2);

    unsigned char *input_addr[3] = {input_data, input_data + height * width, input_data + 2 * height * width};
    bm_image_copy_host_to_device(img_i, (void **)(input_addr));

    ret = bmcv_image_gaussian_blur(handle, img_i, img_o, kw, kh, sigmaX, sigmaY);
    unsigned char *output_addr[3] = {output_tpu, output_tpu + height * width, output_tpu + 2 * height * width};
    bm_image_copy_device_to_host(img_o, (void **)output_addr);

    bm_image_destroy(&img_i);
    bm_image_destroy(&img_o);


    write_bin(output_path, output_tpu, width, height);
    free(input_data);
    free(output_tpu);

    bm_dev_free(handle);
    return ret;
}