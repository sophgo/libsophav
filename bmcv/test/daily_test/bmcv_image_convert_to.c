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
    len = fwrite((void*)output_data, sizeof(unsigned char), size, fp_dst);
    if (len < size) {
        printf("file size = %d is less than required bytes = %d\n", len, size);
        return -1;
    }

    fclose(fp_dst);
    return 0;
}

uint8_t fp32_to_u8(float fp32) {
    // Parse the binary representation of the float as an integer using pointer casting
    union {
        float f;
        uint32_t i;
    } u;
    u.f = fp32;

    // Extract sign, exponent, and mantissa
    uint32_t sign = (u.i >> 31) & 0x1;       // Sign bit
    uint32_t exponent = (u.i >> 23) & 0xFF;  // Exponent part
    uint32_t fraction = u.i & 0x7FFFFF;      // Mantissa (fraction part)

    // If exponent is 0, it's a denormal number or zero - map directly to 0
    if (exponent == 0) {
        return 0;
    }

    // Calculate the actual float value
    float value = pow(-1, sign) * pow(2, exponent - 127) * (1 + fraction / pow(2, 23));

    // Map to u8 range (0-255)
    if (value < 0) value = 0;               // Clamp minimum value
    if (value > 255) value = 255;           // Clamp maximum value

    return (uint8_t)(value + 0.5f);         // Round and return
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

    int image_n = 3;
    int image_c = 3;
    int image_w = 1920;
    int image_h = 1080;

    convert_to_arg_t convert_to_arg[image_c];
    int convert_format = CONVERT_1N_TO_1N;

    bm_status_t ret = bm_dev_request(&handle, dev_id);

    memset(convert_to_arg, 0x0, sizeof(convert_to_arg));
    for(int i = 0; i < image_c; i++){
        convert_to_arg[i].alpha = ((float)(rand() % 20)) / (float)10;
        convert_to_arg[i].beta = ((float)(rand() % 20)) / (float)10 - 1;
    }

    int image_len      = 0;
    image_len = image_n * image_c * image_w * image_h;

    int input_data_type = UINT8_C3;
    unsigned char *input = malloc(image_len * sizeof(unsigned char));
    memset(input, 0x0, image_len);

    int output_data_type = FLOAT32_C3;
    float *bmcv_res = malloc(image_len * sizeof(float));
    for (size_t i = 0; i < image_len; ++i) {
        bmcv_res[i] = 0;
    }

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

    bm_image input_images[image_n];
    bm_image output_images[32];

    for(int img_idx = 0; img_idx < image_n; img_idx++){
        bm_image_create(handle,
                        image_h,
                        image_w,
                        FORMAT_RGB_PLANAR,
                        input_data_format_ext,
                        &input_images[img_idx],
                        NULL);
    }
    for(int img_idx = 0; img_idx < image_n; img_idx++){
        unsigned char *input_img_data = input + image_len / image_n * img_idx;
        bm_image_copy_host_to_device(input_images[img_idx], (void **)&input_img_data);
    }
    for(int img_idx = 0; img_idx < image_n; img_idx++){
        bm_image_create(handle,
                        image_h,
                        image_w,
                        FORMAT_RGB_PLANAR,
                        output_data_format_ext,
                        &output_images[img_idx],
                        NULL);
    }

    bm_image_alloc_contiguous_mem(image_n, output_images, BMCV_HEAP0_ID);

    bmcv_image_convert_to(handle, image_n, convert_to_attr, input_images, output_images);

    char filename[256];

    unsigned char *res_u8_data = (unsigned char*)malloc(image_len * sizeof(unsigned char));
    for(int img_idx = 0; img_idx < image_n; img_idx++){
        float *res_img_data = bmcv_res + image_len / image_n * img_idx;
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(output_images[img_idx], image_byte_size);
        void* out_addr[4] = {(void *)res_img_data,
                            (void *)((float*)res_img_data + image_byte_size[0]/sizeof(float)),
                            (void *)((float*)res_img_data + (image_byte_size[0] + image_byte_size[1])/sizeof(float)),
                            (void *)((float*)res_img_data + (image_byte_size[0] + image_byte_size[1] + image_byte_size[2])/sizeof(float))};

        bm_image_copy_device_to_host(output_images[img_idx], (void **)&out_addr);
        for (int j = 0; j < image_len / image_n; j++) {
            res_u8_data[j] = fp32_to_u8(res_img_data[j]);
        }
        snprintf(filename, sizeof(filename), "daily_test_convert_to_dst_%d.bin", img_idx);
        writeBin(filename, res_u8_data, image_len / image_n);
    }

    free(res_u8_data);
    free(input);
    free(bmcv_res);

    for(int i = 0; i < image_n; i++){
        bm_image_destroy(&input_images[i]);
    }
    for(int i = 0; i < image_n; i++){
            bm_image_destroy(&output_images[i]);
    }
    bm_dev_free(handle);

    return ret;
}
