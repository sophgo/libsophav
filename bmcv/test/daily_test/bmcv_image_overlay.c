#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stb_image.h"

#include "bmcv_api_ext_c.h"

void readBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size){
        printf("read_Bin: file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("write_Bin: file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}


int main(){
    bm_image_format_ext image_fmt = FORMAT_RGB_PACKED;
    bm_image_format_ext overlay_fmt = FORMAT_ARGB_PACKED;

    int img_w = 1920, img_h = 1080;
    int overlay_w = 300, overlay_h = 300;
    int overlay_num = 1;
    int x = 500, y = 500;
    char *image_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin";
    char *overlay_name = "/home/linaro/A2test/bmcv/test/res/300x300_argb8888_dog.rgb";
    char *output_image = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_overlay.bin";
    bm_handle_t handle = NULL;
    bm_status_t ret = bm_dev_request(&handle, 0);

    // config setting
    bmcv_rect_t overlay_info;
    memset(&overlay_info, 0, sizeof(bmcv_rect_t));

    overlay_info.start_x = x;
    overlay_info.start_y = y;
    overlay_info.crop_h = overlay_h;
    overlay_info.crop_w = overlay_w;

    unsigned char* img = malloc(img_h * img_w * 3);
    unsigned char* output_tpu = malloc(img_h * img_w * 3);
    readBin(image_name, img, img_h * img_w * 3);
    memset(output_tpu, 0, img_h * img_w * 3);

    // create bm image struct & alloc dev mem
    bm_image image;
    bm_image_create(handle, img_h, img_w, image_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);
    ret = bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
    unsigned char *in1_ptr[1] = {img};
    bm_image_copy_host_to_device(image, (void **)(in1_ptr));

    unsigned char* overlay_ptr = malloc(overlay_w * overlay_h * 4);
    readBin(overlay_name, overlay_ptr, overlay_w * overlay_h * 4);

    bm_image overlay_image[overlay_num];
    bm_image_create(handle, overlay_h, overlay_w, overlay_fmt, DATA_TYPE_EXT_1N_BYTE, overlay_image, NULL);
    ret = bm_image_alloc_dev_mem(overlay_image[0], BMCV_HEAP1_ID);
    unsigned char* in_overlay[4] = {overlay_ptr, overlay_ptr + overlay_h * overlay_w, overlay_ptr + 2 * overlay_w * overlay_h, overlay_ptr + overlay_w * overlay_h * 3};
    bm_image_copy_host_to_device(overlay_image[0], (void **)(in_overlay));

    ret = bmcv_image_overlay(handle, image, overlay_num, &overlay_info, overlay_image);

    unsigned char *out_ptr[3] = {output_tpu, output_tpu + img_h * img_w, output_tpu + 2 * img_h * img_w};
    bm_image_copy_device_to_host(image, (void **)out_ptr);

    writeBin(output_image, output_tpu, img_h * img_w * 3);

    free(img);
    free(output_tpu);
    bm_dev_free(handle);
    return ret;
}
