#include <assert.h>
#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void readBin(const char* path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

static void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

int main()
{
    int height = 1080;
    int width = 1920;
    int ret = 0;
    char* src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char* dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_pyramid_down.bin";

    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    int ow = width / 2;
    int oh = height / 2;
    int channel = 1;
    unsigned char* input_data = (unsigned char*)malloc(width * height * channel * sizeof(unsigned char));
    unsigned char* output_tpu = (unsigned char*)malloc(ow * oh * channel * sizeof(unsigned char));
    unsigned char* output_cpu = (unsigned char*)malloc(ow * oh * channel * sizeof(unsigned char));
    readBin(src_name, input_data, width * height * channel);
    memset(output_tpu, 0, ow * oh * channel * sizeof(unsigned char));

    bm_image_format_ext fmt = FORMAT_GRAY;
    bm_image img_i;
    bm_image img_o;

    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &img_i, NULL);
    ret = bm_image_create(handle, oh, ow, fmt, DATA_TYPE_EXT_1N_BYTE, &img_o, NULL);

    ret = bm_image_alloc_dev_mem(img_i, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(img_o, BMCV_HEAP1_ID);

    ret = bm_image_copy_host_to_device(img_i, (void **)(&input_data));
    ret = bmcv_image_pyramid_down(handle, img_i, img_o);
    ret = bm_image_copy_device_to_host(img_o, (void **)(&output_tpu));

    bm_image_destroy(&img_i);
    bm_image_destroy(&img_o);

    writeBin(dst_name, output_tpu, ow * oh * channel);

    free(input_data);
    free(output_tpu);
    free(output_cpu);


    bm_dev_free(handle);
    return ret;
}