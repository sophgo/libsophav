#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <time.h>
#include <pthread.h>

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)

typedef struct {
    int loop;
    int if_use_img;
    int height;
    int width;
    int format;
    int data_type;
    float alpha;
    float beta;
    float gamma;
    char* src1_name;
    char* src2_name;
    char* dst_name;
    bm_handle_t handle;
} cv_add_weighted_thread_arg_t;

struct frame_size {
    int height;
    int width;
};

extern int cpu_add_weighted(unsigned char* input1, unsigned char* input2, unsigned char* output,
                            int img_size,  float alpha, float beta, float gamma);

int cpu_add_weighted_float(float* input1, float* input2, float* output,
                            int img_size,  float alpha, float beta, float gamma)
{
    // double roundNum = 0;
    int i;

    for (i = 0; i < img_size; i++) {
        output[i] = input1[i] * alpha + input2[i] * beta + gamma;
    }
    return 0;
}

static void readBin_uint(const char* path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

static void readBin_float(const char* path, float* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, sizeof(float), size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

static void writeBin_uint(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

static void writeBin_float(const char * path, float* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, sizeof(float), size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

static void fill_img_uint(unsigned char* input, int img_size)
{
    int i;

    for (i = 0; i < img_size; ++i) {
        input[i] = rand() % 256;
    }
}

static void fill_img_float(float* input, int img_size)
{
    int i;

    for (i = 0; i < img_size; ++i) {
        input[i] = (float)rand() / (float)(RAND_MAX) * 255.0f;
    }
}

static int tpu_add_weighted_uint(unsigned char* input1, unsigned char* input2,
                            unsigned char* output, int height, int width,
                            int format, float alpha, float beta, float gamma, bm_handle_t handle)
{
    bm_image input1_img;
    bm_image input2_img;
    bm_image output_img;
    // struct timeval t1, t2;
    bm_status_t ret = BM_SUCCESS;

    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input1_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input2_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    ret = bm_image_alloc_dev_mem(input1_img, 2);
    if (ret != BM_SUCCESS) {
        printf("bm_image alloc input1_img failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_image_alloc_dev_mem(input2_img, 2);
    if (ret != BM_SUCCESS) {
        printf("bm_image alloc input1_img failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_image_alloc_dev_mem(output_img, 2);
    if (ret != BM_SUCCESS) {
        printf("bm_image alloc input1_img failed. ret = %d\n", ret);
        return -1;
    }
    int image_byte_size[4] = {0};
    bm_image_get_byte_size(input1_img, image_byte_size);
    void* input_addr1[4] = {(void *)input1,
                            (void *)((unsigned char*)input1 + image_byte_size[0]),
                            (void *)((unsigned char*)input1 + image_byte_size[0] + image_byte_size[1]),
                            (void *)((unsigned char*)input1 + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
    void* input_addr2[4] = {(void *)input2,
                            (void *)((unsigned char*)input2 + image_byte_size[0]),
                            (void *)((unsigned char*)input2 + image_byte_size[0] + image_byte_size[1]),
                            (void *)((unsigned char*)input2 + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
    void* out_addr[4] = {(void *)output,
                            (void *)((unsigned char*)output + image_byte_size[0]),
                            (void *)((unsigned char*)output + image_byte_size[0] + image_byte_size[1]),
                            (void *)((unsigned char*)output + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

     bm_image_copy_host_to_device(input1_img, (void **)input_addr1);
     bm_image_copy_host_to_device(input2_img, (void **)input_addr2);

     ret = bmcv_image_add_weighted(handle, input1_img, alpha, input2_img, beta, gamma, output_img);

     bm_image_copy_device_to_host(output_img, (void **)out_addr);

    bm_image_destroy(&input1_img);
    bm_image_destroy(&input2_img);
    bm_image_destroy(&output_img);

    return 0;
}

static int tpu_add_weighted_float(float* input1, float* input2,
                            float* output, int height, int width,
                            int format, float alpha, float beta, float gamma, bm_handle_t handle)
{
    bm_image input1_img;
    bm_image input2_img;
    bm_image output_img;
    // struct timeval t1, t2;
    bm_status_t ret = BM_SUCCESS;

    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_FLOAT32, &input1_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_FLOAT32, &input2_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_FLOAT32, &output_img, NULL);
    ret = bm_image_alloc_dev_mem(input1_img, 2);
    if (ret != BM_SUCCESS) {
        printf("bm_image alloc input1_img failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_image_alloc_dev_mem(input2_img, 2);
    if (ret != BM_SUCCESS) {
        printf("bm_image alloc input1_img failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_image_alloc_dev_mem(output_img, 2);
    if (ret != BM_SUCCESS) {
        printf("bm_image alloc input1_img failed. ret = %d\n", ret);
        return -1;
    }
    int image_byte_size[4] = {0};
    bm_image_get_byte_size(input1_img, image_byte_size);
    void* input_addr1[4] = {(void *)input1,
                            (void *)((float*)input1 + image_byte_size[0]/sizeof(float)),
                            (void *)((float*)input1 + (image_byte_size[0] + image_byte_size[1])/sizeof(float)),
                            (void *)((float*)input1 + (image_byte_size[0] + image_byte_size[1] + image_byte_size[2])/sizeof(float))};
    void* input_addr2[4] = {(void *)input2,
                            (void *)((float*)input2 + image_byte_size[0]/sizeof(float)),
                            (void *)((float*)input2 + (image_byte_size[0] + image_byte_size[1])/sizeof(float)),
                            (void *)((float*)input2 + (image_byte_size[0] + image_byte_size[1] + image_byte_size[2])/sizeof(float))};
    void* out_addr[4] = {(void *)output,
                            (void *)((float*)output + image_byte_size[0]/sizeof(float)),
                            (void *)((float*)output + (image_byte_size[0] + image_byte_size[1])/sizeof(float)),
                            (void *)((float*)output + (image_byte_size[0] + image_byte_size[1] + image_byte_size[2])/sizeof(float))};

     bm_image_copy_host_to_device(input1_img, (void **)input_addr1);
     bm_image_copy_host_to_device(input2_img, (void **)input_addr2);

     ret = bmcv_image_add_weighted(handle, input1_img, alpha, input2_img, beta, gamma, output_img);

     bm_image_copy_device_to_host(output_img, (void **)out_addr);

    bm_image_destroy(&input1_img);
    bm_image_destroy(&input2_img);
    bm_image_destroy(&output_img);

    return 0;
}

static int cmp_result_uint(unsigned char* got, unsigned char* exp, int len)
{
    for (int i = 0; i < len; ++i) {
        if (abs(got[i] - exp[i]) > 1) {
            printf("uint cmp error: idx=%d  exp=%d  got=%d\n", i, exp[i], got[i]);
            return -1;
        }
    }
    return 0;
}

static int cmp_result_float(float* got, float* exp, int len)
{
    for (int i = 0; i < len; ++i) {
        if (abs(got[i] - exp[i]) > 1.0) {
            printf("float cmp error: idx=%d  exp=%f  got=%f\n", i, exp[i], got[i]);
            return -1;
        }
    }
    return 0;
}

static int get_image_size(int format, int width, int height)
{
    int size = 0;
    switch (format) {
        case FORMAT_YUV420P:
            size = width * height + (height * width / 4) * 2;
            break;
        case FORMAT_YUV422P:
            size = width * height + (height * width / 2) * 2;
            break;
        case FORMAT_YUV444P:
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE:
            size = width * height * 3;
            break;
        case FORMAT_NV12:
        case FORMAT_NV21:
            size = width * height + width * height / 2;
            break;
        case FORMAT_NV16:
        case FORMAT_NV61:
        case FORMAT_NV24:
            size = width * height * 2;
            break;
        case FORMAT_GRAY:
            size = width * height;
            break;
        default:
            printf("image format error \n");
            break;
    }
    return size;
}

static int parameters_check(int height, int width, float alpha, float beta)
{
    int error = 0;
    if (height > 4096 || width > 4096){
        printf("Unsupported size : size_max = 4096 x 4096 \n");
        error = -1;
    }
    // if((alpha + beta) != 1){
    //     printf("parameters error : alpha + beta != 1 \n");
    //     error = -1;
    // }
    return error;
}

static int test_add_weighted_random(int if_use_img, void* frame, int format, int data_type,
                                    float alpha, float beta, float gamma,
                                    char* src1_name, char* src2_name, char* dst_name, bm_handle_t handle)
{
    int img_size = 0;
    struct frame_size* frame_random = (struct frame_size*)frame;
    int width = ALIGN(frame_random->width, 2);
    int height = ALIGN(frame_random->height, 2);
    int ret = 0;
    struct timeval t1, t2;

    printf("if_use_img: %d, width: %d, height: %d, format: %d, data_type: %d, alpha: %f, beta: %f, gamma: %f\n", if_use_img,
            width, height, format, data_type, alpha, beta, gamma);
    img_size = get_image_size(format, width, height);
    printf("img_size is %d \n", img_size);

    if (data_type == 1) {
        unsigned char *input1_data = (unsigned char *)malloc(img_size);
        unsigned char *input2_data = (unsigned char *)malloc(img_size);
        unsigned char *output_tpu = (unsigned char *)malloc(img_size);
        unsigned char *output_cpu = (unsigned char *)malloc(img_size);

        if (if_use_img) {
            printf("test real img \n");
            readBin_uint(src1_name, input1_data, img_size);
            readBin_uint(src2_name, input2_data, img_size);
        } else {
            printf("test random data !\n");
            fill_img_uint(input1_data, img_size);
            fill_img_uint(input2_data, img_size);
        }
        memset(output_tpu, 0, img_size * sizeof(unsigned char));
        memset(output_cpu, 0, img_size * sizeof(unsigned char));

        gettimeofday(&t1, NULL);
        ret = cpu_add_weighted(input1_data, input2_data, output_cpu, img_size, alpha, beta, gamma);
        if (ret) {
            printf("cpu_add_weighted failed!\n");
            return ret;
        }
        gettimeofday(&t2, NULL);
        printf("add_weighted CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

        ret = tpu_add_weighted_uint(input1_data, input2_data, output_tpu, height, width, format, alpha, beta, gamma, handle);
        if (ret) {
            printf("tpu_add_weighted failed!\n");
            return ret;
        }

        ret = cmp_result_uint(output_tpu, output_cpu, img_size);
        if (ret) {
            printf("add_weighted cpu & tpu cmp failed!\n");
            return ret;
        }
        if(if_use_img){
            printf("output img by tpu\n");
            writeBin_uint(dst_name, output_tpu, img_size);
        }
        free(input1_data);
        free(input2_data);
        free(output_tpu);
        free(output_cpu);
    } else if (data_type == 0) {
        float *input1_data = (float *)malloc(img_size * sizeof(float));
        float *input2_data = (float *)malloc(img_size * sizeof(float));
        float *output_tpu = (float *)malloc(img_size * sizeof(float));
        float *output_cpu = (float *)malloc(img_size * sizeof(float));

        if (if_use_img) {
            printf("test real img \n");
            readBin_float(src1_name, input1_data, img_size);
            readBin_float(src2_name, input2_data, img_size);
        } else {
            printf("test random data !\n");
            fill_img_float(input1_data, img_size);
            fill_img_float(input2_data, img_size);
        }
        memset(output_tpu, 0, img_size * sizeof(float));
        memset(output_cpu, 0, img_size * sizeof(float));

        gettimeofday(&t1, NULL);
        ret = cpu_add_weighted_float(input1_data, input2_data, output_cpu, img_size, alpha, beta, gamma);
        if (ret) {
            printf("cpu_add_weighted failed!\n");
            return ret;
        }
        gettimeofday(&t2, NULL);
        printf("add_weighted CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

        ret = tpu_add_weighted_float(input1_data, input2_data, output_tpu, height, width, format, alpha, beta, gamma, handle);
        if (ret) {
            printf("tpu_add_weighted failed!\n");
            return ret;
        }

        ret = cmp_result_float(output_tpu, output_cpu, img_size);
        if (ret) {
            printf("add_weighted cpu & tpu cmp failed!\n");
            return ret;
        }
        if(if_use_img){
            printf("output img by tpu\n");
            writeBin_float(dst_name, output_tpu, img_size);
        }
        free(input1_data);
        free(input2_data);
        free(output_tpu);
        free(output_cpu);
    }

    return ret;
}

void* test_add_weighted(void* args) {
    cv_add_weighted_thread_arg_t* cv_add_weighted_thread_arg = (cv_add_weighted_thread_arg_t*)args;
    struct frame_size frame;
    int loop = cv_add_weighted_thread_arg->loop;
    int if_use_img = cv_add_weighted_thread_arg->if_use_img;
    frame.height = cv_add_weighted_thread_arg->height;
    frame.width = cv_add_weighted_thread_arg->width;
    int format = cv_add_weighted_thread_arg->format;
    int data_type = cv_add_weighted_thread_arg->data_type;
    float alpha = cv_add_weighted_thread_arg->alpha;
    float beta = cv_add_weighted_thread_arg->beta;
    float gamma = cv_add_weighted_thread_arg->gamma;
    char* src1_name = cv_add_weighted_thread_arg->src1_name;
    char* src2_name = cv_add_weighted_thread_arg->src2_name;
    char* dst_name = cv_add_weighted_thread_arg->dst_name;
    bm_handle_t handle = cv_add_weighted_thread_arg->handle;
    for (int i = 0; i < loop; i++) {
        int ret = test_add_weighted_random(if_use_img, &frame, format, data_type, alpha, beta, gamma, src1_name, src2_name, dst_name, handle);
        if (ret) {
            printf("------Test add_weighted Failed!------\n");
            exit(-1);
        }
        printf("------Test add_weighted Successed!------\n");
    }
    return (void*)0;
}

int main(int argc, char* args[])
{
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int thread_num = 1;
    int loop = 1;
    int if_use_img = 0;
    struct frame_size frame;
    frame.height = 1 + rand() % 4096;
    frame.width = 1 + rand() % 4096;
    int format_num[] = {0,1,2,8,9,10,11,12,13,14};
    int size = sizeof(format_num) / sizeof(format_num[0]);
    int rand_num = rand() % size;
    int format = format_num[rand_num];
    int data_type = 0;  //0: float; 1: uint;
    float alpha = roundf((float)rand() / RAND_MAX * 10)/ 10.0;
    float beta = 1.0f - alpha;
    float gamma = ((float)rand()/RAND_MAX) * 255.0f;
    char* src1_name = NULL;
    char* src2_name = NULL;
    char* dst_name = NULL;
    int ret = 0;
    int i;
    int check = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop if_use_img width height format data_type alpha beta gamma src1_name src2_name dst_name(when use_real_img = 1,need to set src_name and dst_name) \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 1\n", args[0]);
        printf("%s 1 1 0 8 8 12 0 0 0.5 0.5 10 \n", args[0]);
        printf("%s 1 1 1 1920 1080 8 0 0.5 0.5 10 res/1920x1080_rgb.bin res/1920x1080_rgb out/out_add_weighted.bin \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) if_use_img = atoi(args[3]);
    if (argc > 4) frame.width = atoi(args[4]);
    if (argc > 5) frame.height = atoi(args[5]);
    if (argc > 6) format = atoi(args[6]);
    if (argc > 7) data_type = atoi(args[7]);
    if (argc > 8) alpha = atof(args[8]);
    if (argc > 9) beta = atof(args[9]);
    if (argc > 10) gamma = atof(args[10]);
    if (argc > 11) src1_name = args[11];
    if (argc > 12) src2_name = args[12];
    if (argc > 13) dst_name = args[13];

    check = parameters_check(frame.height, frame.width, alpha, beta);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    printf("seed = %d\n", seed);
    printf("width = %d, height = %d, format = %d\n", frame.width, frame.height, format);
    // test for multi-thread
    pthread_t pid[thread_num];
    cv_add_weighted_thread_arg_t cv_add_weighted_thread_arg[thread_num];
    for (i = 0; i < thread_num; i++) {
        cv_add_weighted_thread_arg[i].loop = loop;
        cv_add_weighted_thread_arg[i].if_use_img = if_use_img;
        cv_add_weighted_thread_arg[i].height = frame.height;
        cv_add_weighted_thread_arg[i].width = frame.width;
        cv_add_weighted_thread_arg[i].format = format;
        cv_add_weighted_thread_arg[i].data_type = data_type;
        cv_add_weighted_thread_arg[i].alpha = alpha;
        cv_add_weighted_thread_arg[i].beta = beta;
        cv_add_weighted_thread_arg[i].gamma = gamma;
        cv_add_weighted_thread_arg[i].src1_name = src1_name;
        cv_add_weighted_thread_arg[i].src2_name = src2_name;
        cv_add_weighted_thread_arg[i].dst_name = dst_name;
        cv_add_weighted_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_add_weighted, &cv_add_weighted_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return ret;
}
