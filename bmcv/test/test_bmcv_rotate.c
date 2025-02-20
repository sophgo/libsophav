#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include "bmcv_api_ext_c.h"
#include <math.h>
#include <float.h>
#include "bmlib_runtime.h"

#include <assert.h>
#include <unistd.h>

#define BM1688 0x1686a200
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))
#define TIME_COST_MS(t1, t2) (TIME_COST_US(t1, t2) / 1000.0)
extern void bm_read_bin(bm_image src, const char *input_name);
#define THREAD_NUM 128
int thread_stat_fps[THREAD_NUM] = {0};
double thread_stat_lastts[THREAD_NUM] = {0};

typedef unsigned long long u64;
typedef struct sg_api_cv_rotate {
  int channel;
  int rotation_angle;
  u64 input_addr[3];
  u64 output_addr[3];
  int width;
  int height;
} sg_api_cv_rotate_t;

typedef struct {
    int thread_idx;
    int loop_num;
    int use_real_img;
    int width;
    int height;
    int format;
    int rotation_angle;
    char* input_path;
    char* output_path;
    bm_handle_t handle;
} cv_rotate_thread_arg_t;

static int parameters_check(int height, int width)
{
    if (height > 8192 || width > 8192){
        printf("Unsupported size : size_max = 8192 x 8192 \n");
        return -1;
    }
    if (height < 8 || width < 8) {
        printf("Unsupported size : size_min = 8 x 8 \n");
        return -1;
    }
    return 0;
}

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

static void write_bin_nv21(const char *output_path, unsigned char *output_data, int img_size) {
    FILE *fp_dst = fopen(output_path, "wb");
    if (fp_dst == NULL)
    {
        printf("Can not open file! %s\n", output_path);
        return;
    }
    fwrite(output_data, sizeof(unsigned char), img_size, fp_dst);
    fclose(fp_dst);
}

static void fill(
        unsigned char* input,
        int channel,
        int width,
        int height) {
    for (int i = 0; i < channel; i++) {
        for (int j = 0; j < height; j++) {
            for(int k = 0; k < width; k++) {
                unsigned char num =  rand() % 256;
                input[i * width * height + j * width + k] = num;
            }
        }
    }
}

static void fillNV21(unsigned char* nv21Buffer, int width, int height) {
    // filling Y component
    for (int j = 0; j < height; j++) {
        for (int k = 0; k < width; k++) {
            unsigned char yValue = rand() % 256;
            nv21Buffer[j * width + k] = yValue;
        }
    }
    // filling VU component
    int vuIndex = width * height; // VU starting index
    for (int j = 0; j < height / 2; j++) {
        for (int k = 0; k < width / 2; k++) {
            unsigned char uValue = rand() % 256;
            unsigned char vValue = rand() % 256;

            // VU stored interleaved -> skip 2 pos each time
            nv21Buffer[vuIndex++] = vValue;
            nv21Buffer[vuIndex++] = uValue;
        }
    }
}

static int rotate_cpu(
        unsigned char* input,
        unsigned char* output,
        int width,
        int height,
        int format,
        int rotation_angle,
        bm_image input_img,
        bm_handle_t handle) {
    switch (format) {
        case FORMAT_GRAY:
            switch (rotation_angle) {
                case 90:
                    for (int y = 0; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            int newIndex = x * height + (height - y - 1);
                            int oldIndex = y * width + x;
                            output[newIndex] = input[oldIndex];
                        }
                    }
                    break;
                case 180:
                    for (int y = 0; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            int newIndex = (height - y - 1) * width + (width - x - 1);
                            int oldIndex = y * width + x;
                            output[newIndex] = input[oldIndex];
                        }
                    }
                    break;
                case 270:
                    for (int y = 0; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            int newIndex = height * (width - x - 1) + y;
                            int oldIndex = x + y * width;
                            output[newIndex] = input[oldIndex];
                        }
                    }
                    break;
            }

            break;
        case FORMAT_YUV444P:
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE:
            switch (rotation_angle) {
                case 90:
                    for(int channel = 0; channel < 3; channel++) {
                        for (int y = 0; y < height; ++y) {
                            for (int x = 0; x < width; ++x) {
                                int newX = height - y - 1;
                                int newY = x;
                                output[channel * width * height + newY * height + newX] = input[channel * width * height + y * width + x];
                            }
                        }
                    }
                    break;
                case 180:
                    for(int channel = 0; channel < 3; channel++) {
                        for (int y = 0; y < height; ++y) {
                            for (int x = 0; x < width; ++x) {
                                int newX = width - x - 1;
                                int newY = height - y - 1;
                                output[channel * width * height + newY * width + newX] = input[channel * width * height + y * width + x];
                            }
                        }
                    }
                    break;
                case 270:
                    for(int channel = 0; channel < 3; channel++) {
                        for (int y = 0; y < height; ++y) {
                            for (int x = 0; x < width; ++x) {
                                int newX = y;
                                int newY = width - x - 1;
                                output[channel * height * width + newY * height + newX] = input[channel * width * height + y * width + x];
                            }
                        }
                    }
                    break;
            }
            break;
        default:
            break;
    }
    return 0;
}

static int rotate_tpu(
        int thread_idx,
        unsigned char* input,
        unsigned char* output,
        int width,
        int height,
        int format,
        int rotation_angle,
        bm_handle_t handle,
        bm_image nv12_input_img) {
    bm_status_t ret;
    struct timeval t1, t2;
    bm_image input_img, output_img;

    if(rotation_angle == 180) {
        bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
        bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    } else {
        bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
        bm_image_create(handle, width, height, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    }

    bm_image_alloc_dev_mem(input_img, 2);
    bm_image_alloc_dev_mem(output_img, 2);
    if (format == FORMAT_GRAY) {
        unsigned char *input_addr[1] = {input};
        bm_image_copy_host_to_device(input_img, (void **)(input_addr));
    } else if (format == FORMAT_YUV444P ||
               format == FORMAT_RGB_PLANAR ||
               format == FORMAT_BGR_PLANAR ||
               format == FORMAT_RGBP_SEPARATE ||
               format == FORMAT_BGRP_SEPARATE) {
        unsigned char *input_addr[3] = {input, input + height * width, input + 2 * height * width};
        bm_image_copy_host_to_device(input_img, (void **)(input_addr));
    }
    gettimeofday(&t1, NULL);
    if(format == FORMAT_NV12 || format == FORMAT_NV21) {
        ret = bmcv_image_rotate_trans(handle, nv12_input_img, output_img, rotation_angle);
    } else {
    ret = bmcv_image_rotate_trans(handle, input_img, output_img, rotation_angle);
    }
    gettimeofday(&t2, NULL);
    double duration = TIME_COST_MS(t1, t2);
    thread_stat_fps[thread_idx] += 1;
    printf("For thread %d, using time before duration = %f, duration = %f\n", thread_idx, thread_stat_lastts[thread_idx], duration);
    thread_stat_lastts[thread_idx] += duration;
    if (thread_stat_lastts[thread_idx] > 1000) {
        printf("FPS for thread %d is: %d\n", thread_idx, thread_stat_fps[thread_idx]);
        thread_stat_fps[thread_idx] = 0;
        thread_stat_lastts[thread_idx] = 0;
    }

    if (ret != BM_SUCCESS) {
        printf("bmcv_image_rotate error!");
        bm_image_destroy(&input_img);
        bm_image_destroy(&output_img);
        bm_dev_free(handle);
        return -1;
    }

    if (format == FORMAT_GRAY) {
        unsigned char *output_addr[1] = {output};
        bm_image_copy_device_to_host(output_img, (void **)output_addr);
    } else if (format == FORMAT_NV12 || format == FORMAT_NV21) {
        unsigned char *output_addr[2] = {output, output + height * width};
        bm_image_copy_device_to_host(output_img, (void **)output_addr);
    } else {
        unsigned char *output_addr[3] = {output, output + height * width, output + 2 * height * width};
        bm_image_copy_device_to_host(output_img, (void **)output_addr);
    }

    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);
    return 0;
}

static int cmp_rotate(
    unsigned char *got,
    unsigned char *exp,
    int len,
    int format) {
    if (format == FORMAT_NV12 || format == FORMAT_NV21) {
        for (int i = 0; i < len; i++) {
            if (abs(got[i] - exp[i]) > 100) {
                printf("cmp error: idx=%d  exp=%d  got=%d\n", i, exp[i], got[i]);
                return -1;
            }
        }
    } else {
        for (int i = 0; i < len; i++) {
            if (got[i] != exp[i]) {
                printf("cmp error: idx=%d  exp=%d  got=%d\n", i, exp[i], got[i]);
                return -1;
            }
        }

    }
    return 0;
}

static int test_rotate_random(
        int thread_idx,
        int width,
        int height,
        int format,
        int rotation_angle,
        int use_real_img,
        char *input_path,
        char *output_path,
        bm_handle_t handle) {
    printf("width: %d , height: %d, format = %d, rotation_angle: %d\n", width, height, format, rotation_angle);
    int ret;
    struct timeval t1, t2;
    unsigned char* input_data;
    unsigned char* output_cpu;
    unsigned char* output_tpu;
    bm_image input_img;
    if(format == FORMAT_GRAY){
        input_data = (unsigned char*)malloc(width * height * sizeof(unsigned char));
        output_cpu = (unsigned char*)malloc(width * height * sizeof(unsigned char));
        output_tpu = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    } else if (format == FORMAT_NV12 ||
               format == FORMAT_NV21) {
        input_data = (unsigned char*)malloc(width * height * 3 / 2 * sizeof(unsigned char));
        output_cpu = (unsigned char*)malloc(width * height * 3 / 2 * sizeof(unsigned char));
        output_tpu = (unsigned char*)malloc(width * height * 3 / 2 * sizeof(unsigned char));
    } else {
        input_data = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
        output_cpu = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
        output_tpu = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
    }

    if(use_real_img == 1){
        if(format == FORMAT_GRAY){
            read_bin(input_path, input_data, width, height, 1);
        } else if(format == FORMAT_YUV444P ||
                  format == FORMAT_RGB_PLANAR  ||
                  format == FORMAT_BGR_PLANAR  ||
                  format == FORMAT_RGBP_SEPARATE ||
                  format == FORMAT_BGRP_SEPARATE) {
            read_bin(input_path, input_data, width, height, 3);
        } else {
            bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
            bm_image_alloc_dev_mem(input_img, BMCV_HEAP_ANY);
            bm_read_bin(input_img,input_path);
        }
    } else {
        if(format == FORMAT_GRAY){
            fill(input_data, 1, width, height);
        } else if (format == FORMAT_YUV444P  ||
                   format == FORMAT_RGB_PLANAR  ||
                   format == FORMAT_BGR_PLANAR  ||
                   format == FORMAT_RGBP_SEPARATE ||
                   format == FORMAT_BGRP_SEPARATE) {
            fill(input_data, 3, width, height);
        } else if (format == FORMAT_NV12 ||
                   format == FORMAT_NV21) {
            fillNV21(input_data, width, height);
        } else {
            printf("not support input format random test!\n");
            free(input_data);
            free(output_cpu);
            free(output_tpu);
            bm_image_destroy(&input_img);
            return -1;
        }
    }
    gettimeofday(&t1, NULL);
    ret = rotate_cpu(input_data, output_cpu, width, height, format, rotation_angle, input_img, handle);
    gettimeofday(&t2, NULL);
    printf("Rotate CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    if(ret != 0){
        free(input_data);
        free(output_cpu);
        free(output_tpu);
        bm_image_destroy(&input_img);
        return ret;
    }

    ret = rotate_tpu(thread_idx, input_data, output_tpu, width, height, format, rotation_angle, handle, input_img);
    if(ret != 0){
        free(input_data);
        free(output_cpu);
        free(output_tpu);
        bm_image_destroy(&input_img);
        return ret;
    }

    if(format == FORMAT_GRAY){
        ret = cmp_rotate(output_tpu, output_cpu, width * height, format);
    } else {
        ret = cmp_rotate(output_tpu, output_cpu, width * height * 3, format);
    }
    if (ret == 0) {
        printf("Compare TPU result with CPU result successfully!\n");
        if (use_real_img == 1) {
            if(format == FORMAT_GRAY){
                write_bin(output_path, output_tpu, width, height, 1);
            } else {
                write_bin(output_path, output_tpu, width, height, 3);
            }
        }
    } else {
        printf("cpu and tpu failed to compare \n");
    }
    if (format == FORMAT_NV12 ||
        format == FORMAT_NV21) {
        write_bin_nv21(output_path, output_tpu, width * height * 3 / 2);
    } else if (format == FORMAT_GRAY) {
        write_bin(output_path, output_tpu, width, height, 1);
    } else {
        write_bin(output_path, output_tpu, width, height, 3);
    }

    free(input_data);
    free(output_cpu);
    free(output_tpu);

    return ret;
}

void* test_rotate(void* args) {
    cv_rotate_thread_arg_t* cv_rotate_thread_arg = (cv_rotate_thread_arg_t*)args;
    int thread_idx = cv_rotate_thread_arg->thread_idx;
    int loop_num = cv_rotate_thread_arg->loop_num;
    int use_real_img = cv_rotate_thread_arg->use_real_img;
    int width = cv_rotate_thread_arg->width;
    int height = cv_rotate_thread_arg->height;
    int format = cv_rotate_thread_arg->format;
    int rotation_angle = cv_rotate_thread_arg->rotation_angle;
    char* input_path = cv_rotate_thread_arg->input_path;
    char* output_path = cv_rotate_thread_arg->output_path;
    bm_handle_t handle = cv_rotate_thread_arg->handle;
    for (int i = 0; i < loop_num; i++) {
        if(loop_num > 1) {
            width = 8 + rand() % 8185;
            height = 8 + rand() % 8185;
            int format_num[] = {FORMAT_RGB_PLANAR, FORMAT_BGR_PLANAR, FORMAT_RGBP_SEPARATE, FORMAT_BGRP_SEPARATE, FORMAT_GRAY};
            int rand_format_num = rand() % 5;
            format = format_num[rand_format_num];
            int rotation_angle_num[] = {90, 180, 270};
            int rand_angle_num = rand() % 3;
            rotation_angle = rotation_angle_num[rand_angle_num];
        }
        if (0 != test_rotate_random(thread_idx, width, height, format, rotation_angle, use_real_img, input_path, output_path, handle)){
            printf("------TEST CV_ROTATE FAILED------\n");
            exit(-1);
        }
        printf("------TEST CV_ROTATE PASSED!------\n");
    }
    return NULL;
}

int main(int argc, char* args[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    unsigned int seed = tp.tv_nsec;
    srand(seed);
    int thread_num = 1;
    int loop = 1;
    int use_real_img = 0;
    int width = 8 + rand() % 8185;
    int height = 8 + rand() % 8185;
    int format_num[] = {FORMAT_RGB_PLANAR, FORMAT_BGR_PLANAR, FORMAT_RGBP_SEPARATE, FORMAT_BGRP_SEPARATE, FORMAT_GRAY};
    int rand_format_num = rand() % 5;
    int format = format_num[rand_format_num];
    int rotation_angle_num[] = {90, 180, 270};
    int rand_angle_num = rand() % 3;
    int rotation_angle = rotation_angle_num[rand_angle_num];
    int check = 0;
    char *input_path = NULL;
    char *output_path = NULL;
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage:\n");
        printf("%s thread_num loop use_real_img width height format rotation_angle input_path output_path(when use_real_img = 1,need to set input_path and output_path) \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 1 0 512 512 8 90\n", args[0]);
        printf("%s 1 1 1 1920 1080 8 90 res/1920x1080_rgbp.bin out/output_image_rotate_90.bin \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) use_real_img = atoi(args[3]);
    if (argc > 4) width = atoi(args[4]);
    if (argc > 5) height = atoi(args[5]);
    if (argc > 6) format = atoi(args[6]);
    if (argc > 7) rotation_angle = atoi(args[7]);
    if (argc > 8) input_path = args[8];
    if (argc > 9) output_path = args[9];
    check = parameters_check(height, width);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    // test for multi-thread
    pthread_t pid[thread_num];
    cv_rotate_thread_arg_t cv_rotate_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_rotate_thread_arg[i].thread_idx = i;
        cv_rotate_thread_arg[i].loop_num = loop;
        cv_rotate_thread_arg[i].use_real_img = use_real_img;
        cv_rotate_thread_arg[i].width = width;
        cv_rotate_thread_arg[i].height = height;
        cv_rotate_thread_arg[i].format = format;
        cv_rotate_thread_arg[i].rotation_angle = rotation_angle;
        cv_rotate_thread_arg[i].input_path = input_path;
        cv_rotate_thread_arg[i].output_path = output_path;
        cv_rotate_thread_arg[i].handle = handle;
        if (pthread_create(pid + i, NULL, test_rotate, cv_rotate_thread_arg + i) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        int ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return ret;
}
