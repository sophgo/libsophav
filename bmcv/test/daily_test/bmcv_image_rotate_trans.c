#include <stdio.h>
#include "bmcv_api_ext_c.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <float.h>

#define BM1688 0x1686a200
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

static void read_bin(const char *input_path, unsigned char *input_data, int width, int height, float channel) {
    FILE *fp_src = fopen(input_path, "rb");
    if (fp_src == NULL)
    {
        printf("Can not open file! %s\n", input_path);
        return;
    }
    if(fread(input_data, sizeof(unsigned char), width * height * channel, fp_src) != 0)
        printf("read image success\n");
    fclose(fp_src);
}

static void write_bin(const char *output_path, unsigned char *output_data, int width, int height, int channel) {
    FILE *fp_dst = fopen(output_path, "wb");
    if (fp_dst == NULL)
    {
        printf("Can not open file! %s\n", output_path);
        return;
    }
    fwrite(output_data, sizeof(unsigned char), width * height * channel, fp_dst);
    fclose(fp_dst);
}

int main() {
    int width = 1920;
    int height = 1080;
    int format = 14;         // FORMAT_GRAY
    int rotation_angle = 90;        // chosen from {90, 180, 270}
    char *input_path = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *output_path = "/home/linaro/A2test/bmcv/test/res/rotate_output.bin";
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    unsigned char* input_data = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
    unsigned char* output_tpu = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
    read_bin(input_path, input_data, width, height, 3);

    bm_image input_img, output_img;
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    bm_image_create(handle, width, height, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);

    bm_image_alloc_dev_mem(input_img, 2);
    bm_image_alloc_dev_mem(output_img, 2);

    unsigned char *input_addr[3] = {input_data, input_data + height * width, input_data + 2 * height * width};
    bm_image_copy_host_to_device(input_img, (void **)(input_addr));

    ret = bmcv_image_rotate_trans(handle, input_img, output_img, rotation_angle);
    if (ret != BM_SUCCESS) {
        printf("bmcv_image_rotate error!");
        bm_image_destroy(&input_img);
        bm_image_destroy(&output_img);
        bm_dev_free(handle);
        return -1;
    }

    unsigned char *output_addr[3] = {output_tpu, output_tpu + height * width, output_tpu + 2 * height * width};
    bm_image_copy_device_to_host(output_img, (void **)output_addr);

    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);

    write_bin(output_path, output_tpu, width, height, 1);

    free(input_data);
    free(output_tpu);
    bm_dev_free(handle);
    return ret;
}
