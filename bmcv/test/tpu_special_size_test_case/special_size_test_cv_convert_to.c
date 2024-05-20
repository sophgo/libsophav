#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
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

int test_seed = -1;

typedef struct {
    int n;
    int c;
    int w;
    int h;
} image_shape_t;

#define ATHENA2_MAX_W (2048)
#define ATHENA2_MAX_H (2048)
#define MIN_W (16)
#define MIN_H (16)

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

#ifdef __linux__
#define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)
#else
#define ALIGN(x, a) __ALIGN_MASK(x, (int)(a)-1)
#endif

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef enum { MIN_TEST = 0, MAX_TEST, RAND_TEST, MAX_RAND_MODE } rand_mode_e;

// static int gen_test_size(
//     int chipid, int *image_w, int *image_h, int *image_n, int *image_c, int gen_mode) {
//     switch (gen_mode) {
//         case (MIN_TEST): {
//             *image_w = MIN_W;
//             *image_h = MIN_H;
//             *image_n = rand() % 1 + 1;
//             *image_c = 3;
//             break;
//         }
//         case (MAX_TEST): {
//             *image_w = ATHENA2_MAX_W;
//             *image_h = ATHENA2_MAX_H;
//             *image_n = rand() % 1 + 1;
//             *image_c = 3;
//             break;
//         }
//         case (RAND_TEST): {
//             *image_w = rand() % ATHENA2_MAX_W;
//             *image_h = rand() % ATHENA2_MAX_H;
//             *image_w = (*image_w < MIN_W) ? (MIN_W) : (*image_w);
//             *image_h = (*image_h < MIN_H) ? (MIN_H) : (*image_h);
//             *image_n = rand() % 1 + 1;
//             *image_c = 3;
//             break;
//         }
//         default: {
//             printf("gen mode error \n");
//             exit(-1);
//         }
//     }

//     return 0;
// }

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

int32_t convert_to_ref_float( float            *src,
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
        for(int c_idx = 0; c_idx < image_channel; c_idx++){
            for(int y = 0; y < image_h; y++){
                for(int x = 0; x < image_w; x++){
                    int check_idx = (n_idx * image_channel + c_idx) * image_w * image_h +
                                     y * image_w + x;
                    int src_check_idx = (n_idx * image_channel + c_idx) *
                                         image_w_stride * image_h +
                                         y * image_w_stride + x;
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

int32_t convert_to_ref_int8( signed char            *src,
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
        for(int c_idx = 0; c_idx < image_channel; c_idx++){
            for(int y = 0; y < image_h; y++){
                for(int x = 0; x < image_w; x++){
                    int check_idx = (n_idx * image_channel + c_idx) * image_w * image_h +
                                     y * image_w + x;
                    int src_check_idx = (n_idx * image_channel + c_idx) *
                                         image_w_stride * image_h +
                                         y * image_w_stride + x;
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

static int32_t convert_to_test_rand_int8(bm_handle_t       handle,
                                        int               input_data_type,
                                        int               output_data_type,
                                        image_shape_t     image_shape,
                                        convert_to_arg_t *convert_to_arg,
                                        int               convert_format,
                                        int               if_set_stride,
                                        long int          *cpu_time,
                                        long int          *tpu_time) {
    int image_num      = image_shape.n;
    int image_channel  = image_shape.c;
    int image_w        = image_shape.w;
    int image_w_stride = 0;
    int image_h        = image_shape.h;
    int image_len      = 0;
    bm_image_format_ext format = FORMAT_RGB_PLANAR;
    if(image_channel == 3){
        format = FORMAT_RGB_PLANAR;
    } else {
        format = FORMAT_GRAY;
    }
    struct timeval t1, t2, t3, t4;
    if (if_set_stride) {
        image_w_stride = ALIGN(image_w, 64);
        image_len = image_num * image_channel * image_w_stride * image_h;
    } else {
        image_w_stride = image_w;
        image_len = image_num * image_channel * image_w * image_h;
    }

    signed char *input = (signed char *)malloc(image_len * sizeof(signed char));
    signed char *bmcv_res = (signed char *)malloc(image_len * sizeof(signed char));
    signed char *ref_res = (signed char *)malloc(image_len * sizeof(signed char));

    int ret = -1;
    memset(input, 0x0, image_len);
    memset(bmcv_res, 0x0, image_len);
    memset(ref_res, 0x0, image_len);

    if(if_set_stride){
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

    if(if_set_stride){
        for(int img_idx = 0; img_idx < input_num; img_idx++){
            int set_w_stride = image_w_stride * sizeof(signed char);
            bm_image_create(handle,
                            image_h,
                            image_w,
                            format,
                            input_data_format_ext,
                            &input_images[img_idx],
                            &set_w_stride);
        }
    } else {
        for(int img_idx = 0; img_idx < input_num; img_idx++){
            bm_image_create(handle,
                            image_h,
                            image_w,
                            format,
                            input_data_format_ext,
                            &input_images[img_idx],
                            NULL);
        }
    }

    int is_contiguous = 1;
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
        bm_image_create(handle,
                        image_h,
                        image_w,
                        format,
                        output_data_format_ext,
                        &output_images[img_idx],
                        NULL);
    }

    bm_image_alloc_contiguous_mem(output_num, output_images, BMCV_HEAP0_ID);

    //TODO: using time
    #ifdef __linux__
        gettimeofday(&t1, NULL);
        bmcv_image_convert_to(handle, input_num, convert_to_attr, input_images, output_images);
        gettimeofday(&t2, NULL);
        printf("Convert_to test_rand_int8 TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
        *tpu_time += TIME_COST_US(t1, t2);
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
    convert_to_ref_int8(input,
                        ref_res,
                        convert_to_arg,
                        image_num,
                        image_channel,
                        image_h,
                        image_w_stride,
                        image_w,
                        convert_format);
    gettimeofday(&t4, NULL);
    printf("Convert_to test_rand_int8 CPU using time = %ld(us)\n", TIME_COST_US(t3, t4));
    *cpu_time += TIME_COST_US(t3, t4);
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
    if(is_contiguous) {
        bm_image_free_contiguous_mem(input_num, input_images);
    }
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
                                          convert_to_arg_t *convert_to_arg,
                                          int               convert_format,
                                          int               if_set_stride,
                                          long int          *cpu_time,
                                          long int          *tpu_time) {
    int image_num      = image_shape.n;
    int image_channel  = image_shape.c;
    int image_w        = image_shape.w;
    int image_w_stride = 0;
    int image_h        = image_shape.h;
    int image_len      = 0;
    bm_image_format_ext format = FORMAT_RGB_PLANAR;
    if(image_channel == 3){
        format = FORMAT_RGB_PLANAR;
    } else {
        format = FORMAT_GRAY;
    }
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

    if(if_set_stride){
        for(int img_idx = 0; img_idx < input_num; img_idx++){
            int set_w_stride = image_w_stride * sizeof(float);
            bm_image_create(handle,
                            image_h,
                            image_w,
                            format,
                            input_data_format_ext,
                            &input_images[img_idx],
                            &set_w_stride);
        }
    } else {
        for(int img_idx = 0; img_idx < input_num; img_idx++){
            bm_image_create(handle,
                            image_h,
                            image_w,
                            format,
                            input_data_format_ext,
                            &input_images[img_idx],
                            NULL);
        }
    }

    int is_contiguous = 1;
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
        bm_image_create(handle,
                        image_h,
                        image_w,
                        format,
                        output_data_format_ext,
                        &output_images[img_idx],
                        NULL);
    }

    bm_image_alloc_contiguous_mem(output_num, output_images, BMCV_HEAP0_ID);

    //TODO: using time
    #ifdef __linux__
    gettimeofday(&t1, NULL);
    bmcv_image_convert_to(handle, input_num, convert_to_attr, input_images, output_images);
    gettimeofday(&t2, NULL);
    printf("Convert_to test_rand_float TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);
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
    convert_to_ref_float(input,
                         ref_res,
                         convert_to_arg,
                         image_num,
                         image_channel,
                         image_h,
                         image_w_stride,
                         image_w,
                         convert_format);
    gettimeofday(&t4, NULL);
    printf("Convert_to test_rand_float CPU using time = %ld(us)\n", TIME_COST_US(t3, t4));
    *cpu_time += TIME_COST_US(t3, t4);
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
    if(is_contiguous) {
        bm_image_free_contiguous_mem(input_num, input_images);
    }
    bm_image_free_contiguous_mem(output_num, output_images);
    for(int i = 0; i > input_num; i++){
        bm_image_destroy(&input_images[i]);
    }
    for(int i = 0; i < output_num; i++){
            bm_image_destroy(&output_images[i]);
    }
    return 0;
}

int main(int32_t argc, char **argv){
    printf("log will be saved in special_size_test_cv_convert_to.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_convert_to.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot get handle\n");
        return -1;
    }
    int image_n = 1;
    int image_c;
    int image_h_num_int8[10] = {16, 16, 100, 400, 800, 720, 1080, 1440, 2160, 4096};
    int image_w_num_int8[10] = {16, 16, 100, 400, 800, 1280, 1920, 2560, 3840, 4096};
    int image_h_num_fp32[9] = {16, 100, 400, 800, 720, 1080, 1440, 2160, 4096};
    int image_w_num_fp32[9] = {16, 100, 400, 800, 1280, 1920, 2560, 3840, 4096};
    int format_num[3] = {8, 9, 14};
    for (int i = 0;i < 3; i++) {
        int format = format_num[i];
        printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^format = %d^^^^^^^^^^^^^^^^^^^^^^^^^\n", format);
        if(format == 14) {
            image_c = 1;
        } else {
            image_c = 3;
        }
        for(int j = 0;j < 10; j++){
            int image_h = image_h_num_int8[j];
            int image_w = image_w_num_int8[j];
            image_shape_t image_shape;
            image_shape.n = image_n;
            image_shape.c = image_c;
            image_shape.h = image_h;
            image_shape.w = image_w;
            convert_to_arg_t convert_to_arg[image_c], same_convert_to_arg[image_c];
            memset(convert_to_arg, 0x0, sizeof(convert_to_arg));
            memset(same_convert_to_arg, 0x0, sizeof(same_convert_to_arg));
            float same_alpha = 1.0f;  //0~1.9
            float same_beta = 0.5f;  //-1~0.9
            int convert_format = CONVERT_1N_TO_1N;
            for(int k = 0; k < image_c; k++){
                convert_to_arg[k].alpha = 1.0f;
                convert_to_arg[k].beta = 0.5f;
                same_convert_to_arg[k].alpha = same_alpha;
                same_convert_to_arg[k].beta = same_beta;
            }
            int if_set_stride = 0;
            printf("int8--img_w: %d, img_h: %d, img_c: %d, img_n: %d, if_set_stride: %d\n",
            image_shape.w, image_shape.h, image_shape.c, image_shape.n, if_set_stride);
            long int cputime = 0;
            long int tputime = 0;
            long int *cpu_time = &cputime;
            long int *tpu_time = &tputime;
            for(int loop = 0; loop < 3; loop++) {
                int ret = convert_to_test_rand_int8(handle,
                                         INT8_C3,
                                         INT8_C3,
                                         image_shape,
                                         convert_to_arg,
                                         convert_format,
                                         0,
                                         cpu_time,
                                         tpu_time);
                if (ret) {
                    printf("------Test convert_to Failed!------\n");
                    return -1;
                }
            }
            printf("------average CPU time = %ldus------\n", cputime / 3);
            printf("------average TPU time = %ldus------\n", tputime / 3);
        }
        for(int f = 0;f < 9; f++){
            int image_h = image_h_num_fp32[f];
            int image_w = image_w_num_fp32[f];
            image_shape_t image_shape;
            image_shape.n = image_n;
            image_shape.c = image_c;
            image_shape.h = image_h;
            image_shape.w = image_w;
            convert_to_arg_t convert_to_arg[image_c], same_convert_to_arg[image_c];
            memset(convert_to_arg, 0x0, sizeof(convert_to_arg));
            memset(same_convert_to_arg, 0x0, sizeof(same_convert_to_arg));
            float same_alpha = 1.0f;
            float same_beta = 0.5f;
            int convert_format = CONVERT_1N_TO_1N;
            for(int k = 0; k < image_c; k++){
                convert_to_arg[k].alpha = 1.0f;
                convert_to_arg[k].beta = 0.5f;
                same_convert_to_arg[k].alpha = same_alpha;
                same_convert_to_arg[k].beta = same_beta;
            }
            int if_set_stride = 0;
            printf("FP32--img_w: %d, img_h: %d, img_c: %d, img_n: %d, if_set_stride: %d\n",
            image_shape.w, image_shape.h, image_shape.c, image_shape.n, if_set_stride);
            long int cputime_fp32 = 0;
            long int tputime_fp32 = 0;
            long int *cpu_time = &cputime_fp32;
            long int *tpu_time = &tputime_fp32;
            for(int loop = 0; loop < 3; loop++) {
                int ret = convert_to_test_rand_float(handle,
                                                    FLOAT32_C3,
                                                    FLOAT32_C3,
                                                    image_shape,
                                                    same_convert_to_arg,
                                                    convert_format,
                                                    0,
                                                    cpu_time,
                                                    tpu_time);
                if (ret) {
                    printf("------Test convert_to Failed!------\n");
                    return -1;
                }
            }
            printf("------average CPU time = %ldus------\n", cputime_fp32 / 3);
            printf("------average TPU time = %ldus------\n", tputime_fp32 / 3);
        }
    }
    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}