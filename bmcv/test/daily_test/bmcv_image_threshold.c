#include <stdio.h>
#include "bmcv_api_ext_c.h"
#include "test_misc.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <float.h>

static void read_bin(const char *input_path, unsigned char *input_data, int width, int height)
{
    FILE *fp_src = fopen(input_path, "rb");
    if (fp_src == NULL)
    {
        printf("无法打开输出文件 %s\n", input_path);
        return;
    }
    if(fread(input_data, sizeof(char), width * height, fp_src) != 0)
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
    fwrite(output_data, sizeof(char), width * height, fp_dst);
    fclose(fp_dst);
}



int main() {
    int height = 1080;
    int width = 1920;
    int type = rand() % 5;
    char *input_path = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *output_path = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_threshold.bin";
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }
    int channel = 1;

    unsigned char threshold = 50;
    unsigned char max_value = 228;
    printf("type: %d\n", type);
    printf("threshold: %d , max_value: %d\n", threshold, max_value);
    printf("width: %d , height: %d , channel: %d \n", width, height, channel);

    unsigned char* input_data = (unsigned char*)malloc(width * height);
    unsigned char* output_tpu = (unsigned char*)malloc(width * height);
    read_bin(input_path, input_data, width, height);

    bm_image input_img;
    bm_image output_img;

    bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input_img, 1);
    bm_image_alloc_dev_mem(output_img, 1);

    unsigned char* in_ptr[1] = {input_data};
    bm_image_copy_host_to_device(input_img, (void **)in_ptr);
    bmcv_image_threshold(handle, input_img, output_img, threshold, max_value, (bm_thresh_type_t)type);
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
