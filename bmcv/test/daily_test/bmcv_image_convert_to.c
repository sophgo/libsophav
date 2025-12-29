#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"

typedef enum {
    UINT8_C1 = 0,
    UINT8_C3,
    INT8_C1,
    INT8_C3,
    FLOAT32_C1,
    FLOAT32_C3,
    MAX_COLOR_TYPE
} cv_color_e;

typedef enum {
    CONVERT_1N_TO_1N = 0
} convert_storage_mode_e;

typedef struct {
    int idx;
    int trials;
    bm_handle_t handle;
} convert_to_thread_arg_t;

typedef struct {
    float alpha;
    float beta;
} convert_to_arg_t;


typedef struct {
    int n;
    int c;
    int w;
    int h;
} image_shape_t;

#define BM1688_MAX_W (2048)
#define BM1688_MAX_H (2048)
#define MIN_W (16)
#define MIN_H (16)

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

#ifdef __linux__
#define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)
#else
#define ALIGN(x, a) __ALIGN_MASK(x, (int)(a)-1)
#endif

typedef enum { MIN_TEST = 0, MAX_TEST, RAND_TEST, MAX_RAND_MODE } rand_mode_e;

static int writeBin(const char* path, void* output_data, int size)
{
    int len = 0;
    FILE* fp_dst = fopen(path, "wb+");

    if (fp_dst == NULL) {
        perror("Error opening file\n");
        return -1;
    }

    len = fwrite((void*)output_data, 1, size, fp_dst);
    if (len < size) {
        printf("file size = %d is less than required bytes = %d\n", len, size);
        return -1;
    }

    fclose(fp_dst);
    return 0;
}

static bm_image_data_format_ext convert_to_type_translation(int cv_color, int data_type, int if_input) {
    bm_image_data_format_ext image_format = DATA_TYPE_EXT_1N_BYTE;
    if (((UINT8_C3 == cv_color) || (UINT8_C1 == cv_color)) &&
            (CONVERT_1N_TO_1N == data_type)) {
        image_format = DATA_TYPE_EXT_1N_BYTE;
    } else if ((FLOAT32_C1 == cv_color) || (FLOAT32_C3 == cv_color)) {
        image_format = DATA_TYPE_EXT_FLOAT32;
    } else if (((INT8_C3 == cv_color) || (INT8_C1 == cv_color)) &&
            (CONVERT_1N_TO_1N == data_type)) {
        image_format = DATA_TYPE_EXT_1N_BYTE_SIGNED;
    }

    return image_format;
}


int main(){
    int dev_id = 0;
    bm_handle_t handle = NULL;

    int image_n = 1;
    int image_c = 3;
    int image_w = 320;
    int image_h = 320;
    image_shape_t image_shape;

    convert_to_arg_t convert_to_arg[image_c], same_convert_to_arg[image_c];
    int convert_format = CONVERT_1N_TO_1N;

    bm_status_t ret = bm_dev_request(&handle, dev_id);

    image_shape.n = image_n;
    image_shape.c = image_c;
    image_shape.h = image_h;
    image_shape.w = image_w;
    memset(convert_to_arg, 0x0, sizeof(convert_to_arg));
    memset(same_convert_to_arg, 0x0, sizeof(same_convert_to_arg));
    float same_alpha = ((float)(rand() % 20)) / (float)10;
    float same_beta = ((float)(rand() % 20)) / (float)10 - 1;
    for(int i = 0; i < image_c; i++){
        convert_to_arg[i].alpha = ((float)(rand() % 20)) / (float)10;
        convert_to_arg[i].beta = ((float)(rand() % 20)) / (float)10 - 1;
        same_convert_to_arg[i].alpha = same_alpha;
        same_convert_to_arg[i].beta = same_beta;
    }

    int input_data_type = INT8_C3;
    int output_data_type = INT8_C3;

    int image_num      = image_shape.n;
    int image_channel  = image_shape.c;
    // int image_w        = image_shape.w;
    int image_w_stride = 0;
    // int image_h        = image_shape.h;
    int image_len      = 0;

    image_w_stride = image_w;
    image_len = image_num * image_channel * image_w * image_h;

    signed char *input = malloc(image_len * sizeof(signed char));
    signed char *bmcv_res = malloc(image_len * sizeof(signed char));

    memset(input, 0x0, image_len);
    memset(bmcv_res, 0x0, image_len);

    for(int i = 0; i < image_len; i++){
            input[i] = rand() % 255;
    }

    bm_image_data_format_ext input_data_format_ext, output_data_format_ext;
    bmcv_convert_to_attr     convert_to_attr;
    convert_to_attr.alpha_0 = convert_to_arg[0].alpha;
    convert_to_attr.beta_0  = convert_to_arg[0].beta;
    convert_to_attr.alpha_1 = convert_to_arg[1].alpha;
    convert_to_attr.beta_1  = convert_to_arg[1].beta;
    convert_to_attr.alpha_2 = convert_to_arg[2].alpha;
    convert_to_attr.beta_2  = convert_to_arg[2].beta;

    input_data_format_ext = convert_to_type_translation(input_data_type, convert_format, 1);
    output_data_format_ext = convert_to_type_translation(output_data_type, convert_format, 0);

    bm_image input_images[image_num];
    bm_image output_images[32];

    int input_num = image_num;
    int output_num = image_num;

    for(int img_idx = 0; img_idx < input_num; img_idx++){
        bm_image_create(handle,
                        image_h,
                        image_w,
                        FORMAT_BGR_PLANAR,
                        input_data_format_ext,
                        &input_images[img_idx],
                        NULL);
    }

    for(int img_idx = 0; img_idx < input_num; img_idx++){
        int img_offset = image_w_stride * image_h * image_channel;
        signed char *input_img_data = input + img_offset * img_idx;
        bm_image_copy_host_to_device(input_images[img_idx], (void **)&input_img_data);
    }

    for(int img_idx = 0; img_idx < output_num; img_idx++){
        bm_image_create(handle,
                        image_h,
                        image_w,
                        FORMAT_BGR_PLANAR,
                        output_data_format_ext,
                        &output_images[img_idx],
                        NULL);
    }

    bm_image_alloc_contiguous_mem(output_num, output_images, BMCV_HEAP0_ID);

    bmcv_image_convert_to(handle, input_num, convert_to_attr, input_images, output_images);

    char filename[256];

    for(int img_idx = 0; img_idx < output_num; img_idx++){
        int img_offset = image_w * image_h * image_channel;
        signed char *res_img_data = bmcv_res + img_offset * img_idx;
        bm_image_copy_device_to_host(output_images[img_idx], (void **)&res_img_data);
        snprintf(filename, sizeof(filename), "/home/linaro/A2test/bmcv/test/res/out/daily_test_convert_to_dst_%d.bin", img_idx);
        writeBin(filename, bmcv_res, image_h * image_w);
    }

    writeBin("/home/linaro/A2test/bmcv/test/res/out/daily_test_convert_to_src.bin", input, image_h * image_w);


    free(input);
    free(bmcv_res);

    for(int i = 0; i < input_num; i++){
        bm_image_destroy(&input_images[i]);
    }
    for(int i = 0; i < output_num; i++){
            bm_image_destroy(&output_images[i]);
    }
    bm_dev_free(handle);

    return ret;
}