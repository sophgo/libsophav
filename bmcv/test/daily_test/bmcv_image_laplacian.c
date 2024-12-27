#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

int main()
{
    int height = 1080;
    int width = 1920;
    unsigned int ksize = 3;
    bm_image_format_ext fmt = FORMAT_GRAY; /* 14 */
    int ret = 0;
    char* src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char* dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_laplacian.bin";

    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    unsigned char* input = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    unsigned char* tpu_out = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    unsigned char* cpu_out = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    int len;

    len = width * height;
    readBin(src_name, input, len);
    memset(tpu_out, 0, len * sizeof(unsigned char));


    bm_image_data_format_ext data_type = DATA_TYPE_EXT_1N_BYTE;
    bm_image input_img, output_img;

    ret = bm_image_create(handle, height, width, fmt, data_type, &input_img, NULL);
    ret = bm_image_create(handle, height, width, fmt, data_type, &output_img, NULL);
    ret = bm_image_alloc_dev_mem(input_img, 2);
    ret = bm_image_alloc_dev_mem(output_img, 2);

    ret = bm_image_copy_host_to_device(input_img, (void **)(&input));

    ret = bmcv_image_laplacian(handle, input_img, output_img, ksize);

    ret = bm_image_copy_device_to_host(output_img, (void **)(&tpu_out));

    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);

    writeBin(dst_name, tpu_out, len);
    free(input);
    free(tpu_out);
    free(cpu_out);

    bm_dev_free(handle);
    return ret;
}