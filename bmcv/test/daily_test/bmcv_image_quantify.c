#include <stdio.h>
#include "bmcv_api_ext_c.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <float.h>

static void read_bin(const char *input_path, float *input_data, int width, int height)
{
    FILE *fp_src = fopen(input_path, "rb");
    if (fp_src == NULL)
    {
        printf("无法打开输出文件 %s\n", input_path);
        return;
    }
    if(fread(input_data, sizeof(float), width * height, fp_src) != 0)
        printf("read image success\n");
    fclose(fp_src);
}

static void write_bin(const char *output_path, unsigned char *output_data, int width, int height)
{
    FILE *fp_dst = fopen(output_path, "wb");
    if (fp_dst == NULL)
    {
        printf("无法打开输出文件 %s\n", output_path);
        return;
    }
    fwrite(output_data, sizeof(int), width * height, fp_dst);
    fclose(fp_dst);
}


int main() {
    int height = 1080;
    int width = 1920;
    char *input_path = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin";
    char *output_path = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_quantify.bin";

    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);

    float* input_data = (float*)malloc(width * height * 3 * sizeof(float));
    unsigned char* output_tpu = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
    read_bin(input_path, input_data, width, height);

    bm_image input_img;
    bm_image output_img;
    bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_RGB_PLANAR, DATA_TYPE_EXT_FLOAT32, &input_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input_img, 1);
    bm_image_alloc_dev_mem(output_img, 1);
    float* in_ptr[1] = {input_data};

    bm_image_copy_host_to_device(input_img, (void **)in_ptr);
    bmcv_image_quantify(handle, input_img, output_img);
    unsigned char* out_ptr[1] = {output_tpu};

    bm_image_copy_device_to_host(output_img, (void **)out_ptr);
    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);

    write_bin(output_path, output_tpu, width, height);
    free(input_data);
    free(output_tpu);
    bm_dev_free(handle);
    return ret;
}