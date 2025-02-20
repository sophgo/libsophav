#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DTYPE_F32 4
#define DTYPE_U8 1
#define ALIGN(a, b) (((a) + (b) - 1) / (b) * (b))
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))



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
    FILE *fp_dst = fopen(path, "wb+");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}


int main()
{
    int channel = 1;
    int height = 1080;
    int width = 1920;
    int stride = width;
    int dtype = 1;
    int ret = 0;
    char* src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char* dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_transpose.bin";
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    unsigned char* input = (unsigned char*) malloc(channel * height * stride * sizeof(unsigned char));
    unsigned char* output = (unsigned char*) malloc(channel * height * width * sizeof(unsigned char));
    readBin(src_name, input, channel * height * stride);
    memset(output, 0, channel * height * width);

    int format;
    int type = 0;
    int stride_byte;
    bm_image input_img, output_img;

    format = (channel == 3) ? FORMAT_BGR_PLANAR : FORMAT_GRAY;

    if (dtype == DTYPE_F32) {
        type = DATA_TYPE_EXT_FLOAT32;
        stride_byte = stride * sizeof(float);
    } else if (dtype == DTYPE_U8) {
        type = DATA_TYPE_EXT_1N_BYTE;
        stride_byte = stride * sizeof(unsigned char);
    }

    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, (bm_image_data_format_ext)type, &input_img, &stride_byte);
    ret = bm_image_alloc_dev_mem(input_img, 2);
    ret = bm_image_copy_host_to_device(input_img, (void**)&input);
    ret = bm_image_create(handle, width, height, (bm_image_format_ext)format, (bm_image_data_format_ext)type, &output_img, NULL);

    ret = bm_image_alloc_dev_mem(output_img, 2);

    ret = bmcv_image_transpose(handle, input_img, output_img);
    ret = bm_image_copy_device_to_host(output_img, (void**)&output);

    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);

    writeBin(dst_name, output, channel * height * width);

    free(input);
    free(output);

    bm_dev_free(handle);
    return ret;
}