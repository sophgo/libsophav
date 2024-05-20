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

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int trials;
} copy_to_thread_arg_t;

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
                             int         start_y,
                             long int    *cpu_time,
                             long int    *tpu_time) {
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
    struct timeval t1, t2, t3, t4;
    gettimeofday(&t1, NULL);
    ret = bmcv_image_copy_to(handle, copy_to_attr, input, output);
    gettimeofday(&t2, NULL);
    printf("copy to TPU using time: %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);
    if (BM_SUCCESS != ret) {
        printf("bmcv_copy_to error 1 !!!\n");
        bm_image_destroy(&input);
        bm_image_destroy(&output);
        return -1;
    }
    bm_image_copy_device_to_host(output, (void **)&res_data);

    for (int i = 0; i < image_n; i++) {
        gettimeofday(&t3, NULL);
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
        gettimeofday(&t4, NULL);
        printf("copy to CPU using time: %ld(us)\n", TIME_COST_US(t3, t4));
        *cpu_time += TIME_COST_US(t3, t4);
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
            return -1;
        }
    } else {
        if (false == res_ref_comp_float(res_data,
                                     ref_data,
                                     image_n * channel * out_h * out_w)) {
            printf("bmcv_copy_to error 3 !!!\n");
            bm_image_destroy(&input);
            bm_image_destroy(&output);
            return -1;
        }
    }
    bm_image_destroy(&input);
    bm_image_destroy(&output);
    free(src_data);
    free(res_data);
    free(ref_data);
    return ret;
}

int main(int argc, char **argv) {
    printf("log will be saved in special_size_test_cv_copy_to.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_copy_to.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int data_type = DATA_TYPE_EXT_FLOAT32;
    int format_num[5] = {8, 9, 10, 11, 14};
    int input_w_num[10] = {1, 1, 100, 400, 800, 1280, 1920, 2560, 3840, 4096};
    int input_h_num[10] = {1, 1, 100, 400, 800, 720, 1080, 1440, 2160, 4096};
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot get handle\n");
        return -1;
    }

    for(int i = 0; i < 5; i++){
        int format = format_num[i];
        int start_x = 0;
        int start_y = 0;
        int out_h = 4096;
        int out_w = 4096;
        for(int j = 0; j < 10; j++){
            int input_w = input_w_num[j];
            int input_h = input_h_num[j];
            printf("format: %d , input_w: %d, input_h: %d, out_h: 4096, out_w: 4096, start_x: 0, start_y:0\n", format, input_w, input_h);
            long int cputime = 0;
            long int tputime = 0;
            long int *cpu_time = &cputime;
            long int *tpu_time = &tputime;
            for(int loop = 0; loop < 3; loop++) {
                int check = copy_to_test_rand_fp32(handle,
                            format,
                            data_type,
                            input_h,
                            input_w,
                            out_h,
                            out_w,
                            start_x,
                            start_y,
                            cpu_time,
                            tpu_time);
                if (check) {
                    printf("------Test copy_to Failed!------\n");
                    return -1;
                }
            }
            printf("------average CPU time = %ldus------\n", cputime / 3);
            printf("------average TPU time = %ldus------\n", tputime / 3);
        }
    }

    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}
