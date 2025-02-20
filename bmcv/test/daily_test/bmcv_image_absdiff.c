#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>

void readBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}


int main() {
    int height = 1080;
    int width = 1920;
    int format = 8;
    char *src1_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *src2_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_iamge_absdiff.bin";
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }
    int img_size = width * height * 3;
    unsigned char* input1_data = malloc(width * height * 3);
    unsigned char* input2_data = malloc(width * height * 3);
    unsigned char* output_tpu = malloc(width * height * 3);
    readBin(src1_name, input1_data, width * height);
    readBin(src2_name, input2_data, img_size);
    memset(output_tpu, 0, width * height * 3);
    bm_image input1_img;
    bm_image input2_img;
    bm_image output_img;

    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input1_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input2_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input1_img, 2);
    bm_image_alloc_dev_mem(input2_img, 2);
    bm_image_alloc_dev_mem(output_img, 2);
    unsigned char *in1_ptr[3] = {input1_data, input1_data + height * width, input1_data + 2 * height * width};
    unsigned char *in2_ptr[3] = {input2_data, input2_data + width * height, input2_data + 2 * height * width};
    bm_image_copy_host_to_device(input1_img, (void **)(in1_ptr));
    bm_image_copy_host_to_device(input2_img, (void **)(in2_ptr));

    ret = bmcv_image_absdiff(handle, input1_img, input2_img, output_img);
    if (BM_SUCCESS != ret) {
        bm_image_destroy(&input1_img);
        bm_image_destroy(&input2_img);
        bm_image_destroy(&output_img);
        return -1;
    }
    unsigned char *out_ptr[3] = {output_tpu, output_tpu + height * width, output_tpu + 2 * height * width};
    bm_image_copy_device_to_host(output_img, (void **)out_ptr);

    bm_image_destroy(&input1_img);
    bm_image_destroy(&input2_img);
    bm_image_destroy(&output_img);

    writeBin(dst_name, output_tpu, img_size);
    free(input1_data);
    free(input2_data);
    free(output_tpu);
    bm_dev_free(handle);
    return ret;
}
