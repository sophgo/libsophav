#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static void readBin(const char* path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");

    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

static void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

int main()
{
    int height = 1080;
    int width = 1920;
    char *src1_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *src2_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_bitwise_or.bin";
    int format = 8;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    int img_size = height * width * 3;

    unsigned char* input1 = (unsigned char*)malloc(width * height * 3);
    unsigned char* input2 = (unsigned char*)malloc(width * height * 3);
    unsigned char* output = (unsigned char*)malloc(width * height * 3);

    readBin(src1_name, input1, width * height);
    readBin(src2_name, input2, img_size);

    memset(output, 0, img_size * sizeof(unsigned char));

    bm_image input1_img;
    bm_image input2_img;
    bm_image output_img;

    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input1_img, NULL);
    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input2_img, NULL);
    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);

    ret = bm_image_alloc_dev_mem(input1_img, 2);
    ret = bm_image_alloc_dev_mem(input2_img, 2);
    ret = bm_image_alloc_dev_mem(output_img, 2);
    unsigned char* in1_ptr[1] = {input1};
    unsigned char* in2_ptr[3] = {input2, input2 + height * width, input2 + 2 * height * width};
    ret = bm_image_copy_host_to_device(input1_img, (void **)in1_ptr);
    ret = bm_image_copy_host_to_device(input2_img, (void **)in2_ptr);

    ret = bmcv_image_bitwise_or(handle, input1_img, input2_img, output_img);

    unsigned char* out_ptr[3] = {output, output + height * width, output + 2 * height * width};
    ret = bm_image_copy_device_to_host(output_img, (void **)out_ptr);


    bm_image_destroy(&input1_img);
    bm_image_destroy(&input2_img);
    bm_image_destroy(&output_img);

    if (ret) {
        printf("tpu_bitwise failed!\n");
        return ret;
    }

    writeBin(dst_name, output, img_size);
    free(input1);
    free(input2);
    free(output);

    bm_dev_free(handle);
    return ret;
}