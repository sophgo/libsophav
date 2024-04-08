#include <stdio.h>
#include "bmcv_api_ext_c.h"
#include "test_misc.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <float.h>
#include <sys/time.h>
#include <pthread.h>

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop_num;
    int height;
    int width;
    int use_realimg;
    char* input_path;
    char* output_path;
    int type;
    bm_handle_t handle;
} cv_threshold_thread_arg_t;

static int parameters_check(int height, int width)
{
    if (height > 8192 || width > 8192){
        printf("Unsupported size : size_max = 8192 x 8192 \n");
        return -1;
    }
    return 0;
}

static void read_bin(const char *input_path, unsigned char *input_data, int width, int height)
{
    FILE *fp_src = fopen(input_path, "rb");
    if (fp_src == NULL)
    {
        printf("无法打开输出文件 %s\n", input_path);
        return;
    }
    if(fread(input_data, sizeof(char), width * height, fp_src) != 0)
        printf("read image success\n");
    fclose(fp_src);
}

static void write_bin(const char *output_path, unsigned char *output_data, int width, int height)
{
    FILE *fp_dst = fopen(output_path, "wb");
    if (fp_dst == NULL)
    {
        printf("无法打开输出文件 %s\n", output_path);
        return;
    }
    fwrite(output_data, sizeof(char), width * height, fp_dst);
    fclose(fp_dst);
}

static void fill(
        unsigned char* input,
        int channel,
        int width,
        int height) {
    for (int i = 0; i < channel; i++) {
        int num = 10;
        for (int j = 0; j < height; j++) {
            for(int k = 0; k < width; k++){
                input[i * width * height + j * width + k] = num % 128;
                num++;
            }
        }
    }
}

#if 0
static int threshold_opencv(
        unsigned char* input,
        unsigned char* output,
        int height,
        int width,
        unsigned char threshold,
        unsigned char max_val,
        int type,
        int channel) {
    int threshold_type;
    Mat mat_i;
    Mat mat_o;
    mat_i = Mat(height, width, CV_8UC3, input);
    mat_o = Mat(height, width, CV_8UC3, output);
    switch (type) {
        case 0:
            threshold_type = THRESH_BINARY;
            break;
        case 1:
            threshold_type = THRESH_BINARY_INV;
            break;
        case 2:
            threshold_type = THRESH_TRUNC;
            break;
        case 3:
            threshold_type = THRESH_TOZERO;
            break;
        case 4:
            threshold_type = THRESH_TOZERO_INV;
            break;
        default:
            printf("threshold type is illegal!\n");
            return -1;
    }
    cv::threshold(mat_i, mat_o, threshold, max_val, threshold_type);
    return 0;
}
#endif

static int threshold_cpu(
        unsigned char* input,
        unsigned char* output,
        int height,
        int width,
        unsigned char threshold,
        unsigned char max_val,
        int type) {
    switch (type) {
        case 0:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = max_val;
                } else {
                    output[i] = 0;
                }
            }
            break;
        case 1:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = 0;
                } else {
                    output[i] = max_val;
                }
            }
            break;
        case 2:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = threshold;
                } else {
                    output[i] = input[i];
                }
            }
            break;
        case 3:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = input[i];
                } else {
                    output[i] = 0;
                }
            }
            break;
        case 4:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = 0;
                } else {
                    output[i] = input[i];
                }
            }
            break;
        default:
            printf("threshold type is illegal!\n");
            return -1;
    }
    return 0;
}

static int threshold_tpu(
        unsigned char* input,
        unsigned char* output,
        int height,
        int width,
        unsigned char thresh,
        unsigned char max_val,
        int type,
        bm_handle_t handle) {

    bm_image input_img;
    bm_image output_img;
    struct timeval t1, t2;

    bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input_img, 1);
    bm_image_alloc_dev_mem(output_img, 1);

    unsigned char* in_ptr[1] = {input};
    bm_image_copy_host_to_device(input_img, (void **)in_ptr);
    gettimeofday(&t1, NULL);
    bmcv_image_threshold(handle, input_img, output_img, thresh, max_val, (bm_thresh_type_t)type);
    gettimeofday(&t2, NULL);
    printf("Threshold TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    unsigned char* out_ptr[1] = {output};
    bm_image_copy_device_to_host(output_img, (void **)out_ptr);
    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);
    return 0;
}

static int cmp(
    unsigned char *got,
    unsigned char *exp,
    int len) {
    for (int i = 0; i < len; i++) {
        if (got[i] != exp[i]) {
            printf("cmp error: idx=%d  exp=%d  got=%d\n", i, exp[i], got[i]);
            return -1;
        }
    }
    return 0;
}

static int test_threshold_random(
        int height,
        int width,
        int use_realimg,
        char *input_path,
        char *output_path,
        int model,
        bm_handle_t handle) {
    printf("===== test threshold =====\n");
    int type = model;
    int channel = 1;
    int ret;
    struct timeval t1, t2;

    unsigned char threshold = 50;
    unsigned char max_value = 228;
    printf("type: %d\n", type);
    printf("threshold: %d , max_value: %d\n", threshold, max_value);
    printf("width: %d , height: %d , channel: %d \n", width, height, channel);

    unsigned char* input_data = (unsigned char*)malloc(width * height);
    unsigned char* output_cpu = (unsigned char*)malloc(width * height);
    unsigned char* output_tpu = (unsigned char*)malloc(width * height);
    if(use_realimg == 1){
        read_bin(input_path, input_data, width, height);
    } else {
        fill(input_data, 1, width, height);
    }
    gettimeofday(&t1, NULL);
    ret = threshold_cpu(input_data, output_cpu, height, width, threshold, max_value, type);
    gettimeofday(&t2, NULL);
    printf("Threshold CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    if(ret != 0){
        free(input_data);
        free(output_cpu);
        free(output_tpu);
        return ret;
    }

    ret = threshold_tpu(input_data, output_tpu, height, width, threshold, max_value, type, handle);
    if(ret != 0){
        free(input_data);
        free(output_cpu);
        free(output_tpu);
        return ret;
    }

#if 0 // Use opencv for comparison to prove that the cpu side's own implementation of ref is correct.
    unsigned char *output_opencv = (unsigned char*)malloc(width * height * 3);
    ret = threshold_opencv(input_data, output_opencv, height, width,
                           threshold, max_value, type, channel);
    if(ret != 0){
        free(input_data);
        free(output_cpu);
        free(output_tpu);
        free(output_opencv);
        return ret;
    }
    // cmp opencv & cpu reference
    if (0 != cmp(output_cpu, output_opencv, width * height))
        printf("opencv and cpu failed to compare \n");
    else
        printf("Compare CPU result with OpenCV successfully!\n");
    free(output_opencv);
#endif
    ret = cmp(output_tpu, output_cpu, width * height);
    if (ret == 0) {
        printf("Compare TPU result with CPU result successfully!\n");
        if (use_realimg == 1) {
            write_bin(output_path, output_tpu, width, height);
        }
    } else {
        printf("cpu and tpu failed to compare \n");
    }
    free(input_data);
    free(output_cpu);
    free(output_tpu);
    return ret;
}

void* test_threshold(void* args) {
    cv_threshold_thread_arg_t* cv_threshold_thread_arg = (cv_threshold_thread_arg_t*)args;
    int loop_num = cv_threshold_thread_arg->loop_num;
    int use_realimg = cv_threshold_thread_arg->use_realimg;
    int height = cv_threshold_thread_arg->height;
    int width = cv_threshold_thread_arg->width;
    int type = cv_threshold_thread_arg->type;
    char* input_path = cv_threshold_thread_arg->input_path;
    char* output_path = cv_threshold_thread_arg->output_path;
    bm_handle_t handle = cv_threshold_thread_arg->handle;
    for (int i = 0; i < loop_num; i++) {
        if (0 != test_threshold_random(height, width, use_realimg, input_path, output_path, type, handle)){
            printf("------TEST CV_THRESHOLD FAILED------\n");
            exit(-1);
        }
        printf("------TEST CV_THRESHOLD PASSED!------\n");
    }
    return (void*)0;
}

int main(int argc, char* args[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    unsigned int seed = tp.tv_nsec;
    srand(seed);
    printf("seed = %d\n", seed);
    int use_realimg = 0;
    int loop = 1;
    int height = 2 + rand() % 8190;
    int width = 2 + rand() % 8190;
    int type = rand() % 5;
    int thread_num = 1;
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
        printf("%s thread_num loop use_real_img height width type input_path output_path(when use_real_img = 1,need to set input_path and output_path) \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 1 0 512 512 2 \n", args[0]);
        printf("%s 1 1 1 1920 1080 2 res/1920x1080_gray.bin out/out_threshold.bin \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) use_realimg = atoi(args[3]);
    if (argc > 4) width = atoi(args[4]);
    if (argc > 5) height = atoi(args[5]);
    if (argc > 6) type = atoi(args[6]);
    if (argc > 7) input_path = args[7];
    if (argc > 8) output_path = args[8];
    check = parameters_check(height, width);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    // test for multi-thread
    pthread_t pid[thread_num];
    cv_threshold_thread_arg_t cv_threshold_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_threshold_thread_arg[i].loop_num = loop;
        cv_threshold_thread_arg[i].height = height;
        cv_threshold_thread_arg[i].width = width;
        cv_threshold_thread_arg[i].use_realimg = use_realimg;
        cv_threshold_thread_arg[i].input_path = input_path;
        cv_threshold_thread_arg[i].output_path = output_path;
        cv_threshold_thread_arg[i].type = type;
        cv_threshold_thread_arg[i].handle = handle;
        if (pthread_create(pid + i, NULL, test_threshold, cv_threshold_thread_arg + i) != 0) {
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
