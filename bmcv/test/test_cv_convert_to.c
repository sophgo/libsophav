#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "bmcv_api_ext_c.h"

typedef struct {
    int idx;
    int trials;
    bm_handle_t handle;
} convert_to_thread_arg_t;

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
    float alpha;
    float beta;
} convert_to_arg_t;

int test_seed = -1;

typedef struct {
    int n;
    int c;
    int w;
    int h;
} image_shape_t;

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

#ifdef __linux__
#define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)
#else
#define ALIGN(x, a) __ALIGN_MASK(x, (int)(a)-1)
#endif


int32_t convert_to_ref_packed_int8( signed char            *src,
                             signed char            *dst,
                             convert_to_arg_t *convert_to_arg,
                             int               image_num,
                             int               image_channel,
                             int               image_h,
                             int               image_w_stride,
                             int               image_w,
                             int               convert_format){
    int image_len = image_num * image_channel * image_w_stride * image_h;
    signed char *temp_old_buf = malloc(image_len * sizeof(signed char));
    memcpy(temp_old_buf, src, sizeof(signed char) * image_len);

    for(int n_idx = 0; n_idx < image_num; n_idx++){
        for(int y = 0; y < image_h; y++){
            for(int x = 0; x < image_w; x++){
                for(int c_idx = 0; c_idx < image_channel; c_idx++){
                    int check_idx = n_idx * image_channel * image_w * image_h +
                                     y * image_w * image_channel + x * image_channel + c_idx;
                    int src_check_idx = n_idx * image_channel * image_w_stride * image_h +
                                     y * image_w_stride * image_channel + x * image_channel + c_idx;
                    float temp = 0.0;
                    temp = (temp_old_buf[src_check_idx]) * convert_to_arg[c_idx].alpha +
                            convert_to_arg[c_idx].beta;
                    temp = (temp > 127) ? (127)
                                            : ((temp < -128) ? (-128) : (temp));
                    dst[check_idx] = (signed char) round (temp);
                }
            }
        }
    }

    free(temp_old_buf);
    return 0;
}


int32_t convert_to_ref_packed_float( float            *src,
                              float            *dst,
                              convert_to_arg_t *convert_to_arg,
                              int               image_num,
                              int               image_channel,
                              int               image_h,
                              int               image_w_stride,
                              int               image_w,
                              int               convert_format){
    int image_len = image_num * image_channel * image_w_stride * image_h;
    float *temp_old_buf = malloc(image_len * sizeof(float));
    memcpy(temp_old_buf, src, sizeof(float) * image_len);

    for(int n_idx = 0; n_idx < image_num; n_idx++){
        for(int y = 0; y < image_h; y++){
            for(int x = 0; x < image_w; x++){
                for(int c_idx = 0; c_idx < image_channel; c_idx++){
                    int check_idx = n_idx * image_channel * image_w * image_h +
                                     y * image_w * image_channel + x * image_channel + c_idx;
                    int src_check_idx = n_idx * image_channel * image_w_stride * image_h +
                                     y * image_w_stride * image_channel + x * image_channel + c_idx;
                    float temp = 0.0;
                    temp = (temp_old_buf[src_check_idx]) * convert_to_arg[c_idx].alpha +
                            convert_to_arg[c_idx].beta;
                    dst[check_idx] = (float) round (temp);
                }
            }
        }
    }

    free(temp_old_buf);
    return 0;
}


extern int32_t convert_to_ref_float( float            *src,
                              float            *dst,
                              convert_to_arg_t *convert_to_arg,
                              int               image_num,
                              int               image_channel,
                              int               image_h,
                              int               image_w_stride,
                              int               image_w,
                              int               convert_format);
extern int32_t convert_to_ref_int8( signed char            *src,
                             signed char            *dst,
                             convert_to_arg_t *convert_to_arg,
                             int               image_num,
                             int               image_channel,
                             int               image_h,
                             int               image_w_stride,
                             int               image_w,
                             int               convert_format);

#define BM1688_MAX_W (2048)
#define BM1688_MAX_H (2048)
#define MIN_W (16)
#define MIN_H (16)

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

#ifdef __linux__
// #define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)
#else
#define ALIGN(x, a) __ALIGN_MASK(x, (int)(a)-1)
#endif

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef enum { MIN_TEST = 0, MAX_TEST, RAND_TEST, my_MAX_RAND_MODE } my_rand_mode_e;

static int parameters_check(int test_loop_times, int test_threads_num)
{
    int error = 0;
    if (test_loop_times > 1500 || test_loop_times < 1) {
        printf("[TEST CONVERT TO] loop times should be 1~1500 \n");
        error = -1;
    }
    if (test_threads_num > 4 || test_threads_num < 1) {
        printf("[TEST CONVERT TO] threads num should be 1~4 \n");
        error = -1;
    }

    return error;
}

static int gen_test_size(
    int chipid, int *image_w, int *image_h, int *image_n, int *image_c, int gen_mode) {
    switch (gen_mode) {
        case (MIN_TEST): {
            *image_w = MIN_W;
            *image_h = MIN_H;
            *image_n = rand() % 1 + 1;
            *image_c = 3;
            break;
        }
        case (MAX_TEST): {
            *image_w = BM1688_MAX_W;
            *image_h = BM1688_MAX_H;
            *image_n = rand() % 1 + 1;
            *image_c = 3;
            break;
        }
        case (RAND_TEST): {
            *image_w = rand() % BM1688_MAX_W;
            *image_h = rand() % BM1688_MAX_H;
            *image_w = (*image_w < MIN_W) ? (MIN_W) : (*image_w);
            *image_h = (*image_h < MIN_H) ? (MIN_H) : (*image_h);
            *image_n = rand() % 1 + 1;
            *image_c = 3;
            break;
        }
        default: {
            printf("gen mode error \n");
            exit(-1);
        }
    }

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

int32_t bmcv_convert_to_cmp_float(float *p_exp,
                                  float *p_got,
                                  int image_num,
                                  int image_channel,
                                  int image_h,
                                  int image_w){
    for(int n_idx = 0; n_idx < image_num; n_idx++){
        for(int c_idx = 0; c_idx < image_channel; c_idx++){
            for(int y = 0; y < image_h; y++){
                for(int x = 0; x < image_w; x++){
                    int check_idx =
                        (n_idx * image_channel + c_idx) * image_w * image_h +
                        y * image_w + x;
                    float gop = p_got[check_idx];
                    float exp = p_exp[check_idx];
                    if(abs(gop - exp) > 1){
                        printf("n: %d , c: %d , h: %d , w: %d, got: %f, exp: %f \n",
                                n_idx, c_idx, y, x, gop, exp);
                        return -1;
                    }
                }
            }
        }

    }
    return 0;
}

int32_t bmcv_convert_to_cmp_int8(signed char *p_exp,
                                 signed char *p_got,
                                 int image_num,
                                 int image_channel,
                                 int image_h,
                                 int image_w){
    for(int n_idx = 0; n_idx < image_num; n_idx++){
        for(int c_idx = 0; c_idx < image_channel; c_idx++){
            for(int y = 0; y < image_h; y++){
                for(int x = 0; x < image_w; x++){
                    int check_idx =
                        (n_idx * image_channel + c_idx) * image_w * image_h +
                        y * image_w + x;
                    float gop = (float)p_got[check_idx];
                    float exp = (float)p_exp[check_idx];
                    if(abs(gop - exp) > 1){
                        printf("n: %d , c: %d , h: %d , w: %d, got: %f, exp: %f \n",
                                n_idx, c_idx, y, x, gop, exp);
                        return -1;
                    }
                }
            }
        }

    }
    return 0;
}

static int32_t convert_to_test_rand_int8(bm_handle_t       handle,
                                        int               input_data_type,
                                        int               output_data_type,
                                        image_shape_t     image_shape,
                                        int               img_format,
                                        convert_to_arg_t *convert_to_arg,
                                        int               convert_format,
                                        int               if_set_stride) {
    int image_num      = image_shape.n;
    int image_channel  = image_shape.c;
    int image_w        = image_shape.w;
    int image_w_stride = 0;
    int image_h        = image_shape.h;
    int image_len      = 0;
    struct timeval t1, t2, t3, t4;
    if (if_set_stride) {
        image_w_stride = ALIGN(image_w, 64);
        image_len = image_num * image_channel * image_w_stride * image_h;
    } else {
        image_w_stride = image_w;
        image_len = image_num * image_channel * image_w * image_h;
    }

    signed char *input = malloc(image_len * sizeof(signed char));
    signed char *bmcv_res = malloc(image_len * sizeof(signed char));
    signed char *ref_res = malloc(image_len * sizeof(signed char));

    int ret = -1;
    memset(input, 0x0, image_len);
    memset(bmcv_res, 0x0, image_len);
    memset(ref_res, 0x0, image_len);

    if(if_set_stride){
        if (img_format == FORMAT_BGR_PLANAR) {
            for(int n_idx = 0; n_idx < image_num; n_idx++){
                for(int c_idx = 0; c_idx < image_channel; c_idx++){
                    for(int h_idx = 0; h_idx < image_h; h_idx++){
                        for(int w_idx = 0; w_idx < image_w; w_idx++){
                            input[w_idx + h_idx * image_w_stride +
                                c_idx * image_h * image_w_stride +
                                n_idx * image_channel * image_h * image_w_stride] = rand() % 255;
                        }
                        // padding
                        signed char *tmp = &input[image_w + h_idx * image_w_stride +
                                            c_idx * image_h * image_w_stride +
                                            n_idx * image_channel * image_h * image_w_stride];
                        memset(tmp,
                            0,
                            sizeof(signed char) * (image_w_stride - image_w));
                    }
                }
            }
        } else {
            for(int n_idx = 0; n_idx < image_num; n_idx++){
                for(int h_idx = 0; h_idx < image_h; h_idx++){
                    for(int w_idx = 0; w_idx < image_w; w_idx++){
                        for(int c_idx = 0; c_idx < image_channel; c_idx++){
                            input[c_idx + h_idx * image_w_stride * 3 +
                                w_idx * 3 + n_idx * image_channel * image_h * image_w_stride] = rand() % 255;
                        }
                        // padding
                        signed char *tmp = &input[image_w * 3 + h_idx * image_w_stride * 3 +
                                            n_idx * image_channel * image_h * image_w_stride];
                        memset(tmp,
                            0,
                            sizeof(signed char) * (image_w_stride * 3 - image_w * 3));
                    }
                }
            }
        }
    } else {
        for(int i = 0; i < image_len; i++){
            input[i] = rand() % 255;
        }
    }

    bm_image_data_format_ext input_data_format_ext, output_data_format_ext;
    bmcv_convert_to_attr     convert_to_attr;
    convert_to_attr.alpha_0 = convert_to_arg[0].alpha;
    convert_to_attr.beta_0  = convert_to_arg[0].beta;
    convert_to_attr.alpha_1 = convert_to_arg[1].alpha;
    convert_to_attr.beta_1  = convert_to_arg[1].beta;
    convert_to_attr.alpha_2 = convert_to_arg[2].alpha;
    convert_to_attr.beta_2  = convert_to_arg[2].beta;

    input_data_format_ext =
        convert_to_type_translation(input_data_type, convert_format, 1);
    output_data_format_ext =
        convert_to_type_translation(output_data_type, convert_format, 0);

    bm_image input_images[image_num];
    bm_image output_images[32];

    int input_num = image_num;
    int output_num = image_num;

    if (img_format == FORMAT_BGR_PLANAR) {
        if(if_set_stride){
            for(int img_idx = 0; img_idx < input_num; img_idx++){
                int set_w_stride = image_w_stride * sizeof(signed char);
                bm_image_create(handle,
                                image_h,
                                image_w,
                                FORMAT_BGR_PLANAR,
                                input_data_format_ext,
                                &input_images[img_idx],
                                &set_w_stride);
            }
        } else {
            for(int img_idx = 0; img_idx < input_num; img_idx++){
                bm_image_create(handle,
                                image_h,
                                image_w,
                                FORMAT_BGR_PLANAR,
                                input_data_format_ext,
                                &input_images[img_idx],
                                NULL);
            }
        }
    } else {
        if(if_set_stride){
            for(int img_idx = 0; img_idx < input_num; img_idx++){
                int set_w_stride = image_w_stride * 3 * sizeof(signed char);
                bm_image_create(handle,
                                image_h,
                                image_w,
                                FORMAT_BGR_PACKED,
                                input_data_format_ext,
                                &input_images[img_idx],
                                &set_w_stride);
            }
        } else {
            for(int img_idx = 0; img_idx < input_num; img_idx++){
                bm_image_create(handle,
                                image_h,
                                image_w,
                                FORMAT_BGR_PACKED,
                                input_data_format_ext,
                                &input_images[img_idx],
                                NULL);
            }
        }
    }


    int is_contiguous = rand() % 2;
    if(is_contiguous){
        bm_image_alloc_contiguous_mem(input_num, input_images, BMCV_HEAP0_ID);
    } else {
        for(int i = input_num - 1; i >= 0; i--){
            bm_image_alloc_dev_mem(input_images[i], BMCV_HEAP0_ID);
        }
    }

    for(int img_idx = 0; img_idx < input_num; img_idx++){
        int img_offset = image_w_stride * image_h * image_channel;
        signed char *input_img_data = input + img_offset * img_idx;
        bm_image_copy_host_to_device(input_images[img_idx], (void **)&input_img_data);
    }

    for(int img_idx = 0; img_idx < output_num; img_idx++){
        if (img_format == FORMAT_BGR_PLANAR) {
            bm_image_create(handle,
                        image_h,
                        image_w,
                        FORMAT_BGR_PLANAR,
                        output_data_format_ext,
                        &output_images[img_idx],
                        NULL);
        } else {
            bm_image_create(handle,
                        image_h,
                        image_w,
                        FORMAT_BGR_PACKED,
                        output_data_format_ext,
                        &output_images[img_idx],
                        NULL);
        }

    }

    bm_image_alloc_contiguous_mem(output_num, output_images, BMCV_HEAP0_ID);

    //TODO: using time
    #ifdef __linux__
        gettimeofday(&t1, NULL);
        bmcv_image_convert_to(handle, input_num, convert_to_attr, input_images, output_images);
        gettimeofday(&t2, NULL);
        printf("Convert_to test_rand_int8 TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    #else
    //     struct timespec tp1, tp2;
    //     clock_gettime_win(0, &tp1);
    //     bmcv_image_convert_to(handle, input_num, convert_to_attr, input_images, output_images);
    //     clock_gettime_win(0, &tp2);
    //     cout << "Convert to using time: " << ((tp2.tv_sec - tp1.tv_sec) * 1000000 + (tp2.tv_nsec - tp1.tv_nsec)/1000) << "us" << endl;
    #endif

    for(int img_idx = 0; img_idx < output_num; img_idx++){
        int img_offset = image_w * image_h * image_channel;
        signed char *res_img_data = bmcv_res + img_offset * img_idx;
        bm_image_copy_device_to_host(output_images[img_idx], (void **)&res_img_data);
    }
    gettimeofday(&t3, NULL);

    if (input_images->image_format == FORMAT_BGR_PLANAR) {
        convert_to_ref_int8(input,
                        ref_res,
                        convert_to_arg,
                        image_num,
                        image_channel,
                        image_h,
                        image_w_stride,
                        image_w,
                        convert_format);
    } else {
        convert_to_ref_packed_int8(input,
                        ref_res,
                        convert_to_arg,
                        image_num,
                        image_channel,
                        image_h,
                        image_w_stride,
                        image_w,
                        convert_format);
    }

    gettimeofday(&t4, NULL);
    printf("Convert_to test_rand_int8 CPU using time = %ld(us)\n", TIME_COST_US(t3, t4));
    ret = bmcv_convert_to_cmp_int8(ref_res, bmcv_res, image_num, image_channel, image_h, image_w);
    if(ret < 0){
        free(input);
        free(bmcv_res);
        free(ref_res);
        bm_image_free_contiguous_mem(input_num, input_images);
        bm_image_free_contiguous_mem(output_num, output_images);

        for(int i = 0; i < input_num; i++){
            bm_image_destroy(&input_images[i]);
        }
        for(int i = 0; i < output_num; i++){
            bm_image_destroy(&output_images[i]);
        }
        exit(-1);
    }

    free(input);
    free(bmcv_res);
    free(ref_res);

    if(is_contiguous)
        bm_image_free_contiguous_mem(input_num, input_images);
    bm_image_free_contiguous_mem(output_num, output_images);
    for(int i = 0; i < input_num; i++){
        bm_image_destroy(&input_images[i]);
    }
    for(int i = 0; i < output_num; i++){
            bm_image_destroy(&output_images[i]);
    }
    return 0;
}

static int32_t convert_to_test_rand_float(bm_handle_t       handle,
                                          int               input_data_type,
                                          int               output_data_type,
                                          image_shape_t     image_shape,
                                          int               img_format,
                                          convert_to_arg_t *convert_to_arg,
                                          int               convert_format,
                                          int               if_set_stride) {
    int image_num      = image_shape.n;
    int image_channel  = image_shape.c;
    int image_w        = image_shape.w;
    int image_w_stride = 0;
    int image_h        = image_shape.h;
    int image_len      = 0;
    struct timeval t1, t2, t3, t4;
    if (if_set_stride) {
        image_w_stride = ALIGN(image_w, 64);
        image_len = image_num * image_channel * image_w_stride * image_h;
    } else {
        image_w_stride = image_w;
        image_len = image_num * image_channel * image_w * image_h;
    }

    float *input = malloc(image_len * sizeof(float));
    float *bmcv_res = malloc(image_len * sizeof(float));
    float *ref_res = malloc(image_len * sizeof(float));

    int ret = -1;
    memset(input, 0x0, image_len);
    memset(bmcv_res, 0x0, image_len);
    memset(ref_res, 0x0, image_len);

    if(if_set_stride){
        if (img_format == FORMAT_BGR_PLANAR) {
            for(int n_idx = 0; n_idx < image_num; n_idx++){
                for(int c_idx = 0; c_idx < image_channel; c_idx++){
                    for(int h_idx = 0; h_idx < image_h; h_idx++){
                        for(int w_idx = 0; w_idx < image_w; w_idx++){
                            input[w_idx + h_idx * image_w_stride +
                                c_idx * image_h * image_w_stride +
                                n_idx * image_channel * image_h * image_w_stride] = rand() % 255;
                        }
                        // padding
                        float *tmp = &input[image_w + h_idx * image_w_stride +
                                            c_idx * image_h * image_w_stride +
                                            n_idx * image_channel * image_h * image_w_stride];
                        memset(tmp,
                            0,
                            sizeof(float) * (image_w_stride - image_w));
                    }
                }
            }
        } else {
            for(int n_idx = 0; n_idx < image_num; n_idx++){
                for(int h_idx = 0; h_idx < image_h; h_idx++){
                    for(int w_idx = 0; w_idx < image_w; w_idx++){
                        for(int c_idx = 0; c_idx < image_channel; c_idx++){
                            input[c_idx + h_idx * image_w_stride * 3 +
                                w_idx * 3 + n_idx * image_channel * image_h * image_w_stride] = rand() % 255;
                        }
                        // padding
                        float *tmp = &input[image_w * 3 + h_idx * image_w_stride * 3 +
                                            n_idx * image_channel * image_h * image_w_stride];
                        memset(tmp,
                            0,
                            sizeof(float) * (image_w_stride * 3 - image_w * 3));
                    }
                }
            }
        }
    } else {
        for(int i = 0; i < image_len; i++){
            input[i] = rand() % 255;
        }
    }

    bm_image_data_format_ext input_data_format_ext, output_data_format_ext;
    bmcv_convert_to_attr     convert_to_attr;
    convert_to_attr.alpha_0 = convert_to_arg[0].alpha;
    convert_to_attr.beta_0  = convert_to_arg[0].beta;
    convert_to_attr.alpha_1 = convert_to_arg[1].alpha;
    convert_to_attr.beta_1  = convert_to_arg[1].beta;
    convert_to_attr.alpha_2 = convert_to_arg[2].alpha;
    convert_to_attr.beta_2  = convert_to_arg[2].beta;

    input_data_format_ext =
        convert_to_type_translation(input_data_type, convert_format, 1);
    output_data_format_ext =
        convert_to_type_translation(output_data_type, convert_format, 0);

    bm_image input_images[image_num];
    bm_image output_images[32];

    int input_num = image_num;
    int output_num = image_num;

if (img_format == FORMAT_BGR_PLANAR) {
        if(if_set_stride){
            for(int img_idx = 0; img_idx < input_num; img_idx++){
                int set_w_stride = image_w_stride * sizeof(float);
                bm_image_create(handle,
                                image_h,
                                image_w,
                                FORMAT_BGR_PLANAR,
                                input_data_format_ext,
                                &input_images[img_idx],
                                &set_w_stride);
            }
        } else {
            for(int img_idx = 0; img_idx < input_num; img_idx++){
                bm_image_create(handle,
                                image_h,
                                image_w,
                                FORMAT_BGR_PLANAR,
                                input_data_format_ext,
                                &input_images[img_idx],
                                NULL);
            }
        }
    } else {
        if(if_set_stride){
            for(int img_idx = 0; img_idx < input_num; img_idx++){
                int set_w_stride = image_w_stride * 3 * sizeof(float);
                bm_image_create(handle,
                                image_h,
                                image_w,
                                FORMAT_BGR_PACKED,
                                input_data_format_ext,
                                &input_images[img_idx],
                                &set_w_stride);
            }
        } else {
            for(int img_idx = 0; img_idx < input_num; img_idx++){
                bm_image_create(handle,
                                image_h,
                                image_w,
                                FORMAT_BGR_PACKED,
                                input_data_format_ext,
                                &input_images[img_idx],
                                NULL);
            }
        }
    }

    int is_contiguous = rand() % 2;
    if(is_contiguous){
        bm_image_alloc_contiguous_mem(input_num, input_images, BMCV_HEAP0_ID);
    } else {
        for(int i = input_num - 1; i >= 0; i--){
            bm_image_alloc_dev_mem(input_images[i], BMCV_HEAP0_ID);
        }
    }

    for(int img_idx = 0; img_idx < input_num; img_idx++){
        int img_offset = image_w_stride * image_h * image_channel;
        float *input_img_data = input + img_offset * img_idx;
        bm_image_copy_host_to_device(input_images[img_idx], (void **)&input_img_data);
    }



    for(int img_idx = 0; img_idx < output_num; img_idx++){
        if (img_format == FORMAT_BGR_PLANAR) {
            bm_image_create(handle,
                        image_h,
                        image_w,
                        FORMAT_BGR_PLANAR,
                        output_data_format_ext,
                        &output_images[img_idx],
                        NULL);
        } else {
            bm_image_create(handle,
                        image_h,
                        image_w,
                        FORMAT_BGR_PACKED,
                        output_data_format_ext,
                        &output_images[img_idx],
                        NULL);
        }

    }

    bm_image_alloc_contiguous_mem(output_num, output_images, BMCV_HEAP0_ID);

    //TODO: using time
    #ifdef __linux__
    gettimeofday(&t1, NULL);
    bmcv_image_convert_to(handle, input_num, convert_to_attr, input_images, output_images);
    gettimeofday(&t2, NULL);
    printf("Convert_to test_rand_float TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    #else
    //     struct timespec tp1, tp2;
    //     clock_gettime_win(0, &tp1);
        // bmcv_image_convert_to(handle, input_num, convert_to_attr, input_images, output_images);
    //     clock_gettime_win(0, &tp2);
    //     cout << "Convert to using time: " << ((tp2.tv_sec - tp1.tv_sec) * 1000000 + (tp2.tv_nsec - tp1.tv_nsec)/1000) << "us" << endl;
    #endif

    for(int img_idx = 0; img_idx < output_num; img_idx++){
        int img_offset = image_w * image_h * image_channel;
        float *res_img_data = bmcv_res + img_offset * img_idx;
        bm_image_copy_device_to_host(output_images[img_idx], (void **)&res_img_data);
    }
    gettimeofday(&t3, NULL);
    if (input_images->image_format == FORMAT_BGR_PLANAR) {
        convert_to_ref_float(input,
                        ref_res,
                        convert_to_arg,
                        image_num,
                        image_channel,
                        image_h,
                        image_w_stride,
                        image_w,
                        convert_format);
    } else {
        convert_to_ref_packed_float(input,
                        ref_res,
                        convert_to_arg,
                        image_num,
                        image_channel,
                        image_h,
                        image_w_stride,
                        image_w,
                        convert_format);
    }
    gettimeofday(&t4, NULL);
    printf("Convert_to test_rand_float CPU using time = %ld(us)\n", TIME_COST_US(t3, t4));
    ret = bmcv_convert_to_cmp_float(ref_res, bmcv_res, image_num, image_channel, image_h, image_w);
    if(ret < 0){
        free(input);
        free(bmcv_res);
        free(ref_res);
        bm_image_free_contiguous_mem(input_num, input_images);
        bm_image_free_contiguous_mem(output_num, output_images);

        for(int i = 0; i < input_num; i++){
            bm_image_destroy(&input_images[i]);
        }
        for(int i = 0; i < output_num; i++){
            bm_image_destroy(&output_images[i]);
        }
        exit(-1);
    }

    free(input);
    free(bmcv_res);
    free(ref_res);

    if(is_contiguous)
        bm_image_free_contiguous_mem(input_num, input_images);
    bm_image_free_contiguous_mem(output_num, output_images);
    for(int i = 0; i > input_num; i++){
        bm_image_destroy(&input_images[i]);
    }
    for(int i = 0; i < output_num; i++){
            bm_image_destroy(&output_images[i]);
    }
    return 0;
}

#ifdef __linux__
void *test_convert_to_thread(void *arg)
#else
DWORD WINAPI test_convert_to_thread(LPVOID arg)
#endif
{
    convert_to_thread_arg_t *resize_thread_arg = (convert_to_thread_arg_t *)arg;
    bm_handle_t handle = resize_thread_arg->handle;
    int test_loop_times = resize_thread_arg->trials;
    printf("------MULTI THREAD TEST STARTING----------thread idx is %d \n", resize_thread_arg->idx);

    printf("[TEST CONVERT TO] test starts... LOOP times will be is %d \n", test_loop_times);

    for(int loop_idx = 0; loop_idx < test_loop_times; loop_idx++){
        printf("------[TEST CONVERT TO] LOOP is %d \n", loop_idx);

        int image_n = 1;
        int image_c = 3;
        int image_w = 320;
        int image_h = 320;
        image_shape_t image_shape;

        #ifdef __linux__
            convert_to_arg_t convert_to_arg[image_c], same_convert_to_arg[image_c];
        #else
            // TODO: window
        #endif

        int convert_format = CONVERT_1N_TO_1N;
        int test_cnt = 0;
        int dev_id = 0;
        // TODO: get random seed
        //struct timespec tp;
        // #ifdef __linux__
        // clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tp);
        // #else
        // clock_gettime_win(0, &tp);
        // #endif
        unsigned int seed;
        if(test_seed == -1){
            seed = 0;//tp.tv_nsec;
        } else {
            seed = test_seed;
        }

        printf("random seed is %d \n", seed);
        srand(seed);
        bm_status_t ret = bm_dev_request(&handle, dev_id);
        if(ret != BM_SUCCESS){
            printf("Create bm handle failed. ret is %d \n", ret);
            exit(-1);
        }

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

        unsigned int chipid = 0x1686a200;
        bm_get_chipid(handle, &chipid);
        for (int i = 0; i < 2; i++) {
            int img_format = 0;
            if (i == 0) img_format = 0;
            if (i == 1) img_format = 9;
            switch(chipid){
                case 0x1686a200:
                    printf("---------CONVERT TO CLASSICAL SIZE TEST---------- \n");
                    convert_format = CONVERT_1N_TO_1N;
                    printf("CONVERT TO 1N int8 TO 1N int8 \n");
                    convert_to_test_rand_int8(handle,
                                            INT8_C3,
                                            INT8_C3,
                                            image_shape,
                                            img_format,
                                            convert_to_arg,
                                            convert_format,
                                            0);
                    printf("result of %d compare pass \n", test_cnt++);

                    printf("[WITH STRIDE] CONVERT TO 1N int8 TO 1N int8 \n");
                    convert_to_test_rand_int8(handle,
                                            INT8_C3,
                                            INT8_C3,
                                            image_shape,
                                            img_format,
                                            convert_to_arg,
                                            convert_format,
                                            1);
                    printf("result of %d compare pass \n", test_cnt++);

                    printf("[SAME PARA] CONVERT TO 1N int8 TO 1N int8 \n");
                    convert_to_test_rand_int8(handle,
                                            INT8_C3,
                                            INT8_C3,
                                            image_shape,
                                            img_format,
                                            same_convert_to_arg,
                                            convert_format,
                                            0);
                    printf("result of %d compare pass \n", test_cnt++);

                    printf("CONVERT TO 1N FP32 TO 1N FP32 \n");
                    convert_to_test_rand_float(handle,
                                            FLOAT32_C3,
                                            FLOAT32_C3,
                                            image_shape,
                                            img_format,
                                            convert_to_arg,
                                            convert_format,
                                            0);
                    printf("result of %d compare pass \n", test_cnt++);

                    printf("[WITH STRIDE] CONVERT TO 1N FP32 TO 1N FP32 \n");
                    convert_to_test_rand_float(handle,
                                               FLOAT32_C3,
                                               FLOAT32_C3,
                                               image_shape,
                                               img_format,
                                               convert_to_arg,
                                               convert_format,
                                               1);
                    printf("result of %d compare pass \n", test_cnt++);

                    printf("[SAME PARA] CONVERT TO 1N FP32 TO 1N FP32 \n");
                    convert_to_test_rand_float(handle,
                                            FLOAT32_C3,
                                            FLOAT32_C3,
                                            image_shape,
                                            img_format,
                                            same_convert_to_arg,
                                            convert_format,
                                            0);
                    printf("result of %d compare pass \n", test_cnt++);

                    printf("[SAME PARA] [WITH STRIDE] CONVERT TO 1N FP32 TO 1N FP32 \n");
                    convert_to_test_rand_float(handle,
                                               FLOAT32_C3,
                                               FLOAT32_C3,
                                               image_shape,
                                               img_format,
                                               same_convert_to_arg,
                                               convert_format,
                                               1);
                    printf("result of %d compare pass \n", test_cnt++);

                    printf("---------CONVERT TO CORNER TEST---------- \n");
                    int rand_loop_num = 2;
                    for(int rand_loop_idx = 0; rand_loop_idx < rand_loop_num; rand_loop_idx++){
                        for(int rand_mode = 0; rand_mode < my_MAX_RAND_MODE; rand_mode++){
                            gen_test_size(chipid,
                                        &image_shape.w,
                                        &image_shape.h,
                                        &image_shape.n,
                                        &image_shape.c,
                                        rand_mode);
                            printf("rand_mode: %d , img_w: %d ,img_h: %d , img_c: %d img_n: %d\n",
                                    rand_mode, image_shape.w, image_shape.h, image_shape.c, image_shape.n);
                            test_cnt = 0;
                            convert_format = CONVERT_1N_TO_1N;
                            printf("CONVERT TO 1N fp32 TO 1N fp32");
                            convert_to_test_rand_float(handle,
                                            FLOAT32_C3,
                                            FLOAT32_C3,
                                            image_shape,
                                            img_format,
                                            convert_to_arg,
                                            convert_format,
                                            0);
                            printf("result of %d compare pass \n", test_cnt++);

                            printf("[WITH STRIDE] CONVERT TO 1N int8 TO 1N int8 \n");
                            convert_to_test_rand_int8(handle,
                                               INT8_C3,
                                               INT8_C3,
                                               image_shape,
                                               img_format,
                                               convert_to_arg,
                                               convert_format,
                                               1);
                            printf("result of %d compare pass \n", test_cnt++);

                            printf("[WITH STRIDE] CONVERT TO 1N fp32 TO 1N fp32 \n");
                            convert_to_test_rand_float(handle,
                                               FLOAT32_C3,
                                               FLOAT32_C3,
                                               image_shape,
                                               img_format,
                                               convert_to_arg,
                                               convert_format,
                                               1);
                            printf("result of %d compare pass \n", test_cnt++);

                            printf("[SAME PARA] CONVERT TO 1N FP32 TO 1N FP32 \n");
                            convert_to_test_rand_float(handle,
                                            FLOAT32_C3,
                                            FLOAT32_C3,
                                            image_shape,
                                            img_format,
                                            same_convert_to_arg,
                                            convert_format,
                                            0);
                            printf("result of %d compare pass \n", test_cnt++);

                            printf("[SAME PARA] CONVERT TO 1N int8 TO 1N int8 \n");
                            convert_to_test_rand_int8(handle,
                                            INT8_C3,
                                            INT8_C3,
                                            image_shape,
                                            img_format,
                                            same_convert_to_arg,
                                            convert_format,
                                            0);
                            printf("result of %d compare pass \n", test_cnt++);

                            printf("[SAME PARA] [WITH STRIDE] CONVERT TO 1N FP32 TO 1N FP32 \n");
                            convert_to_test_rand_float(handle,
                                               FLOAT32_C3,
                                               FLOAT32_C3,
                                               image_shape,
                                               img_format,
                                               same_convert_to_arg,
                                               convert_format,
                                               1);
                            printf("result of %d compare pass \n", test_cnt++);
                        }
                    }

                    break;
                default:
                    printf("Chipid Is Not Support !\n");
                    break;
            }

        }

    }
    return NULL;
}

int main(int32_t argc, char **argv){
    int test_loop_times = 1;
    int test_threads_num = 1;
    int dev_id = -1;
    int check = 0;
    int dev_cnt = 1;

    if (argc == 2 && atoi(argv[1]) == -1) {
        printf("usage:\n");
        printf("%s loop_num multi_thread_num dev_id seed \n", argv[0]);
        printf("example:\n");
        printf("%s \n", argv[0]);
        printf("%s 1\n", argv[0]);
        printf("%s 2 1\n", argv[0]);
        printf("%s 1 2 -1 -1\n", argv[0]);
        return 0;
    }

    if (argc > 1) test_loop_times = atoi(argv[1]);
    if (argc > 2) test_threads_num = atoi(argv[2]);
    if (argc > 3) dev_id = atoi(argv[3]);
    if (argc > 4) test_seed = atoi(argv[4]);
    check = parameters_check(test_loop_times, test_threads_num);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    bm_dev_getcount(&dev_cnt);
    if(dev_id >= dev_cnt){
        printf("[TEST SOER] dev_id should less than device count, only detect %d devices \n", dev_cnt);
        exit(-1);
    }
    printf("device count = %d \n", dev_cnt);

    #ifdef __linux__
        bm_handle_t handle[dev_cnt];
    #else
        // TODO: WINDOWS
    #endif
    for(int i = 0; i < dev_cnt; i++){
        int id;
        if(dev_id != -1){
            dev_cnt = 1;
            id = dev_id;
        } else {
            id = i;
        }
        bm_status_t req = bm_dev_request(handle + i, id);
        if(req != BM_SUCCESS){
            printf("Create bm handle for dev %d failed \n", id);
            exit(-1);
        }
    }

    #ifdef __linux__
    pthread_t *pid = (pthread_t *)malloc(sizeof(pthread_t) * dev_cnt * test_threads_num);
    convert_to_thread_arg_t *convert_to_thread_arg = (convert_to_thread_arg_t *)
                        malloc(sizeof(convert_to_thread_arg_t) * dev_cnt * test_threads_num);
    for(int d = 0; d < dev_cnt; d++){
        for(int i = 0; i < test_threads_num; i++){
            int idx = d * test_threads_num + i;
            convert_to_thread_arg[idx].trials = test_loop_times;
            convert_to_thread_arg[idx].handle = handle[d];
            convert_to_thread_arg[idx].idx = i;
            if(pthread_create(&pid[idx],
                              NULL,
                              test_convert_to_thread,
                              &convert_to_thread_arg[idx])){
                free(pid);
                free(convert_to_thread_arg);
                printf("Create thread failed \n");
                exit(-1);
            }
        }
    }

    int ret = 0;
    for(int i = 0; i < dev_cnt; i++){
        ret = pthread_join(pid[i], NULL);
        if(ret != 0){
            free(pid);
            free(convert_to_thread_arg);
            printf("Thread join failed \n");
            exit(-1);
        }
    }
    for(int d = 0; d < dev_cnt; d++){
        bm_dev_free(handle[d]);
    }
    printf("------[TEST CONVERT TO] ALL TEST PASSED!------\n");
    free(pid);
    free(convert_to_thread_arg);
    #else
    // TODO: WINDOWS
    #endif
    return ret;
}
