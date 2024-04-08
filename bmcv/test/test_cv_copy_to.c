#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>  // For pthread functions (Linux)
#include "bmcv_api_ext_c.h"

#ifdef __linux__
  #include <unistd.h>
  #include <pthread.h>
  #include <sys/time.h>
#else
  #include <windows.h>
  #include "time.h"
#endif
#include "bmcv_internal.h"

#define MAX_SIZE (2048)
#define MIN_SIZE (32)
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int trials;
} copy_to_thread_arg_t;

static int parameters_check(int test_loop_times, int test_threads_num)
{
    int error = 0;
    if (test_loop_times > 1500 || test_loop_times < 1) {
        printf("[TEST COPY_TO] loop times should be 1~1500 \n");
        error = -1;
    }
    if (test_threads_num > 4 || test_threads_num < 1) {
        printf("[TEST COPY_TO] threads num should be 1~4 \n");
        error = -1;
    }

    return error;
}

void gen_test_size(int *in_w, int *in_h, int *out_w, int *out_h, int *start_x, int *start_y, int gen_mode) {
    int out_w_tmp, out_h_tmp, in_w_tmp, in_h_tmp;

    switch (gen_mode) {
        case MAX_SIZE_MODE:
            out_w_tmp = MAX_SIZE;
            out_h_tmp = MAX_SIZE;
            break;
        case MIN_SIZE_MODE:
            out_w_tmp = MIN_SIZE;
            out_h_tmp = MIN_SIZE;
            break;
        case RAND_SIZE_MODE:
            out_w_tmp = MIN_SIZE + rand() % (MAX_SIZE - MIN_SIZE);
            out_h_tmp = MIN_SIZE + rand() % (MAX_SIZE - MIN_SIZE);
            break;
        default:
            out_w_tmp = MIN_SIZE + rand() % (MAX_SIZE - MIN_SIZE);
            out_h_tmp = MIN_SIZE + rand() % (MAX_SIZE - MIN_SIZE);
            break;
    }

    *out_w = out_w_tmp;
    *out_h = out_h_tmp;

    in_w_tmp = (*out_w != MIN_SIZE) ? (MIN_SIZE + rand() % (*out_w - MIN_SIZE)) : (MIN_SIZE);
    in_h_tmp = (*out_h != MIN_SIZE) ? (MIN_SIZE + rand() % (*out_h - MIN_SIZE)) : (MIN_SIZE);

    *in_w = in_w_tmp;
    *in_h = in_h_tmp;

    *start_x = (*out_w != in_w_tmp) ? (rand() % (*out_w - in_w_tmp)) : (0);
    *start_y = (*out_h != in_h_tmp) ? (rand() % (*out_h - in_h_tmp)) : (0);
}

static int packedToplanar_float(float * packed, int channel, int width, int height){
    float * planar = (float *)malloc(channel * width * height * sizeof(float));
    float * p_img = packed;
    float * p_r = planar;
    float * p_g = planar + height * width;
    float * p_b = planar + height * width * 2;
    for(int i = 0; i < height * width; i++){
        * p_r++ = * p_img++;
        * p_g++ = * p_img++;
        * p_b++ = * p_img++;
    }
    memcpy(packed, planar, sizeof(float) * channel * width * height);
    free(planar);
    return 0;
}

typedef enum { COPY_TO_GRAY = 0, COPY_TO_BGR, COPY_TO_RGB } padding_corlor_e;
typedef enum { PLANNER = 0, PACKED } padding_format_e;

static int copy_to_ref_v2_float(float * src,
                                float * dst,
                                int channel,
                                int in_h,
                                int in_w,
                                int out_h,
                                int out_w,
                                int start_x,
                                int start_y,
                                int ispacked){
    if(ispacked){
        packedToplanar_float(src, channel, in_w, in_h);
    }
    for(int c = 0; c < channel; c++){
        for(int i = 0; i < in_h; i++){
            for(int j = 0; j < in_w; j++){
                int index = c * out_w * out_h + (start_y + i) * out_w + start_x + j;
                    dst[index] = src[(c * in_h * in_w) + i * in_w + j];
            }
        }
    }
    return 0;
}

static bool res_ref_comp_float(float *res_1n_planner, float *ref_1n_planner, int size) {
    for (int idx = 0; idx < size; idx++) {
        if (res_1n_planner[idx] != ref_1n_planner[idx]) {
            printf("[COMP ERROR]: idx: %d, res: %d, ref: %d\n", idx, (int)res_1n_planner[idx], (int)ref_1n_planner[idx]);
            return false;
        }
    }
    return true;
}

static bool res_ref_comp_v2_float(float * res_1n_planner,
                         float * ref_1n_planner,
                         int n,
                         int c,
                         int in_h,
                         int in_w,
                         int out_h,
                         int out_w,
                         int start_x,
                         int start_y) {
    int h_len = out_w;
    int c_len = out_w * out_h;
    int n_len = c_len * c;
    for (int n_idx = 0; n_idx < n; n_idx++) {
        for (int c_idx = 0; c_idx < c; c_idx++) {
            for (int h_idx = 0; h_idx < in_h; h_idx++) {
                for (int w_idx = 0; w_idx < in_w; w_idx++) {
                    int idx = n_idx * n_len + c_idx * c_len +
                              (h_idx + start_y) * h_len + w_idx + start_x;
                    if (res_1n_planner[idx] != ref_1n_planner[idx]) {
                        printf("[COMP ERROR]: idx: %d, res: %d, ref: %d, n_idx: %d, c_idx: %d, h_idx: %d, w_idx: %d\n",
                                                                            idx, (int)res_1n_planner[idx], (int)ref_1n_planner[idx],
                                                                            n_idx, c_idx, h_idx, w_idx);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

static int copy_to_test_rand_fp32(bm_handle_t handle,
                             int         image_format,
                             int         data_type,
                             int         in_h,
                             int         in_w,
                             int         out_h,
                             int         out_w,
                             int         start_x,
                             int         start_y) {
    int channel   = 3;
    int if_packed = false;
    switch (image_format) {
        case FORMAT_RGB_PLANAR: {
            channel = 3;
            break;
        }
        case FORMAT_RGB_PACKED: {
            channel   = 3;
            if_packed = true;
            break;
        }
        case FORMAT_BGR_PLANAR: {
            channel = 3;
            break;
        }
        case FORMAT_BGR_PACKED: {
            channel   = 3;
            if_packed = true;
            break;
        }
        case FORMAT_GRAY: {
            channel = 1;
            break;
        }
        default: {
            channel = 3;
            break;
        }
    }
    int image_n = 1;

    float* src_data = (float *)malloc(image_n * channel * in_w * in_h * sizeof(float));
    float* res_data = (float *)malloc(image_n * channel * out_w * out_h * sizeof(float));
    float* ref_data  = (float *)malloc(image_n * channel * out_w * out_h * sizeof(float));

    for (int i = 0; i < image_n * channel * in_w * in_h; i++) {
        src_data[i] = rand() % 255;
    }
    // calculate res
    bmcv_copy_to_atrr_t copy_to_attr;
    copy_to_attr.start_x    = start_x;
    copy_to_attr.start_y    = start_y;
    copy_to_attr.padding_r  = 0;
    copy_to_attr.padding_g  = 0;
    copy_to_attr.padding_b  = 0;
    copy_to_attr.if_padding = 0;
    bm_image input, output;
    bm_image_create(handle,
                    in_h,
                    in_w,
                    (bm_image_format_ext)image_format,
                    (bm_image_data_format_ext)data_type,
                    &input, NULL);
    bm_image_alloc_dev_mem(input, BMCV_HEAP_ANY);
    bm_image_copy_host_to_device(input, (void **)&src_data);
    bm_image_create(handle,
                    out_h,
                    out_w,
                    (bm_image_format_ext)image_format,
                    (bm_image_data_format_ext)data_type,
                    &output, NULL);
    bm_image_alloc_dev_mem(output, BMCV_HEAP_ANY);
    bm_status_t ret = BM_SUCCESS;
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    ret = bmcv_image_copy_to(handle, copy_to_attr, input, output);
    gettimeofday(&t2, NULL);
    printf("copy to using time: %ld(us)\n", TIME_COST_US(t1, t2));

    if (BM_SUCCESS != ret) {
        printf("bmcv_copy_to error 1 !!!\n");
        bm_image_destroy(&input);
        bm_image_destroy(&output);
        free(src_data);
        free(res_data);
        free(ref_data);
        return -1;
    }
    bm_image_copy_device_to_host(output, (void **)&res_data);

    for (int i = 0; i < image_n; i++) {
        copy_to_ref_v2_float( src_data + i * channel * in_w * in_h,
                           ref_data + i * channel * out_w * out_h,
                           channel,
                           in_h,
                           in_w,
                           out_h,
                           out_w,
                           start_x,
                           start_y,
                           if_packed);
    }
    if (if_packed) {
        packedToplanar_float(res_data, channel, out_w, out_h);
    }

    // float* res_no_padding_ptr
    if (!copy_to_attr.if_padding) {
        if (false == res_ref_comp_v2_float(res_data,
                                     ref_data,
                                     image_n,
                                     channel,
                                     in_h,
                                     in_w,
                                     out_h,
                                     out_w,
                                     start_x,
                                     start_y)) {
            printf("bmcv_copy_to error 2 !!!\n");
            bm_image_destroy(&input);
            bm_image_destroy(&output);
            free(src_data);
            free(res_data);
            free(ref_data);
            return -1;
        }
    } else {
        if (false == res_ref_comp_float(res_data,
                                     ref_data,
                                     image_n * channel * out_h * out_w)) {
            printf("bmcv_copy_to error 3 !!!\n");
            bm_image_destroy(&input);
            bm_image_destroy(&output);
            free(src_data);
            free(res_data);
            free(ref_data);
            return -1;
        }
    }
    bm_image_destroy(&input);
    bm_image_destroy(&output);
    free(src_data);
    free(res_data);
    free(ref_data);
    printf("test copy_to compare passed !!!\n");
    return ret;
}

static int test_copy_to_thread() {
    int test_loop_times = 1;
    printf("[TEST COPY_TO] test starts... LOOP times will be %d\n", test_loop_times);
    int         dev_id = 0;
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    for (int loop_idx = 0; loop_idx < test_loop_times; loop_idx++) {
        printf("------[TEST COPY_TO] LOOP %d\n",loop_idx );
        unsigned int seed = (unsigned)time(NULL);
        printf("seed: %d\n", seed);
        srand(seed);
        int in_w    = 400;
        int in_h    = 400;
        int out_w   = 800;
        int out_h   = 800;
        int start_x = 200;
        int start_y = 200;

        printf("---------COPY_TO CLASSICAL SIZE TEST----------" );

        ret = copy_to_test_rand_fp32(handle,
                                FORMAT_BGR_PACKED,
                                DATA_TYPE_EXT_FLOAT32,
                                in_h,
                                in_w,
                                out_h,
                                out_w,
                                start_x,
                                start_y);
        if(ret) {
            printf("copy_to failed \n");
            goto exit;
        }

        printf("[TEST COPY_TO] fp32 bgr planner: "
                    "1n_fp32->1n_fp32 starts\n");
        ret = copy_to_test_rand_fp32(handle,
                                FORMAT_BGR_PLANAR,
                                DATA_TYPE_EXT_FLOAT32,
                                in_h,
                                in_w,
                                out_h,
                                out_w,
                                start_x,
                                start_y);
        if(ret) {
            printf("copy_to failed \n");
            goto exit;
        }

        printf("----------COPY_TO CORNER TEST---------\n");
        int loop_num = 2;
        int in_w_, in_h_, out_w_, out_h_, start_x_, start_y_;
        for (int loop_idx = 0; loop_idx < loop_num; loop_idx++) {
            for (int rand_mode = 0; rand_mode < MAX_RAND_MODE; rand_mode++) {
                gen_test_size(
                    &in_w_, &in_h_, &out_w_, &out_h_, &start_x_, &start_y_, rand_mode);

                    printf("rand mode : %d, in_w : %d, in_h : %d, out_w : %d, out_h : %d, start_x : %d, start_y : %d\n",
                                rand_mode, in_w, in_h, out_w, out_h, start_x, start_y);
                    printf("[TEST COPY_TO] fp32 bgr packed: "
                            "1n_fp32->1n_fp32 starts\n");
                    ret = copy_to_test_rand_fp32(handle,
                                            FORMAT_BGR_PACKED,
                                            DATA_TYPE_EXT_FLOAT32,
                                            in_h_,
                                            in_w_,
                                            out_h_,
                                            out_w_,
                                            start_x_,
                                            start_y_);
                    if(ret) {
                        printf("copy_to failed \n");
                        goto exit;
                    }

                    printf("[TEST COPY_TO] fp32 bgr planner: "
                                "1n_fp32->1n_fp32 starts\n");
                    copy_to_test_rand_fp32(handle,
                                            FORMAT_BGR_PLANAR,
                                            DATA_TYPE_EXT_FLOAT32,
                                            in_h_,
                                            in_w_,
                                            out_h_,
                                            out_w_,
                                            start_x_,
                                            start_y_);
                    if(ret) {
                        printf("copy_to failed \n");
                        goto exit;
                    }
            }
        }
        printf("----------TEST COPY_TO OVER---------\n");
    }
    printf("----------[TEST COPY_TO] ALL TEST PASSED!---------\n");

    exit:
        bm_dev_free(handle);
        return ret;
}

int main(int argc, char **argv) {
    int test_loop_times = 1;
    int test_threads_num = 1;
    int check = 0;
    int ret = 0;

    if (argc == 2 && atoi(argv[1]) == -1) {
        printf("usage:\n");
        printf("%s loop_num multi_thread_num\n", argv[0]);
        printf("example:\n");
        printf("%s \n", argv[0]);
        printf("%s 1\n", argv[0]);
        printf("%s 2 1\n", argv[0]);
        return 0;
    }

    if (argc > 1) test_loop_times = atoi(argv[1]);
    if (argc > 2) test_threads_num = atoi(argv[2]);
    check = parameters_check(test_loop_times, test_threads_num);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    ret = test_copy_to_thread();
    if(ret) {
        printf("test_copy_to_thread failed \n");
        exit(-1);
    }

    return ret;
}
