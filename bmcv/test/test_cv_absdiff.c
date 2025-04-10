#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <pthread.h>

#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#include "time.h"
#endif

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop_num;
    int use_real_img;
    int format;
    int height;
    int width;
    char *src1_name;
    char *src2_name;
    char *dst_name;
    bm_handle_t handle;
} cv_absdiff_thread_arg_t;

extern int absdiff_cmodel(unsigned char* input1,
                          unsigned char* input2,
                          unsigned char* output,
                          int img_size);

void readBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

static void fill(unsigned char* input, int img_size){
    for (int i = 0; i < img_size; i++) {
        input[i] = rand() % 256;
    }
}

static int get_image_size(int format, int width, int height){
    int size = 0;
    switch (format){
        case FORMAT_YUV420P:
        case FORMAT_NV12:
        case FORMAT_NV21:
            size = width * height + (height * width / 4) * 2;
            break;
        case FORMAT_YUV422P:
        case FORMAT_NV16:
        case FORMAT_NV61:
            size = width * height + (height * width / 2) * 2;
            break;
        case FORMAT_NV24:
        case FORMAT_YUV444P:
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE:
            size = width * height * 3;
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

static int parameters_check(int height, int width)
{
    if (height > 4096 || width > 4096){
        printf("Unsupported size : size_max = 4096 x 4096 \n");
        return -1;
    }
    return 0;
}

static int cmp( unsigned char* got, unsigned char* exp, int len) {
    for(int i = 0; i < len; i++){
        if(got[i] != exp[i]){
            printf("cmp error: idx=%d  exp=%d  got=%d\n", i, exp[i], got[i]);
            return -1;
        }
    }
    return 0;
}



static int absdiff_tpu(
        unsigned char* input1,
        unsigned char* input2,
        unsigned char* output,
        int height,
        int width,
        int format,
        bm_handle_t handle) {
    bm_image input1_img;
    bm_image input2_img;
    bm_image output_img;

    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input1_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input2_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input1_img, 2);
    bm_image_alloc_dev_mem(input2_img, 2);
    bm_image_alloc_dev_mem(output_img, 2);
    if (format == FORMAT_YUV420P) {
        unsigned char *in1_ptr[3] = {input1, input1 + width * height, input1 + width * height * 5 / 4};
        unsigned char *in2_ptr[3] = {input2, input2 + width * height, input2 + width * height * 5 / 4};
        bm_image_copy_host_to_device(input1_img, (void **)(in1_ptr));
        bm_image_copy_host_to_device(input2_img, (void **)(in2_ptr));
    } else if (format == FORMAT_YUV422P) {
        unsigned char *in1_ptr[3] = {input1, input1 + width * height, input1 + width * height * 3 / 2};
        unsigned char *in2_ptr[3] = {input2, input2 + width * height, input2 + width * height * 3 / 2};
        bm_image_copy_host_to_device(input1_img, (void **)(in1_ptr));
        bm_image_copy_host_to_device(input2_img, (void **)(in2_ptr));
    } else if (format == FORMAT_NV12 ||
               format == FORMAT_NV21 ||
               format == FORMAT_NV16 ||
               format == FORMAT_NV61 ||
               format == FORMAT_NV24) {
        unsigned char *in1_ptr[2] = {input1, input1 + width * height};
        unsigned char *in2_ptr[2] = {input2, input2 + width * height};
        bm_image_copy_host_to_device(input1_img, (void **)(in1_ptr));
        bm_image_copy_host_to_device(input2_img, (void **)(in2_ptr));
    } else {
        unsigned char *in1_ptr[3] = {input1, input1 + height * width, input1 + 2 * height * width};
        unsigned char *in2_ptr[3] = {input2, input2 + width * height, input2 + 2 * height * width};
        bm_image_copy_host_to_device(input1_img, (void **)(in1_ptr));
        bm_image_copy_host_to_device(input2_img, (void **)(in2_ptr));
    }
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    bm_status_t ret = bmcv_image_absdiff(handle, input1_img, input2_img, output_img);
    if (BM_SUCCESS != ret) {
        bm_image_destroy(&input1_img);
        bm_image_destroy(&input2_img);
        bm_image_destroy(&output_img);
        return -1;
    }
    gettimeofday(&t2, NULL);
    int using_time = ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec);
    printf("Absdiff TPU using time: %d(us)\n", using_time);
    if (format == FORMAT_YUV420P) {
        unsigned char *out_ptr[3] = {output, output + width * height, output + width * height * 5 / 4};
        bm_image_copy_device_to_host(output_img, (void **)out_ptr);
    } else if (format == FORMAT_YUV422P) {
        unsigned char *out_ptr[3] = {output, output + width * height, output + width * height * 3 / 2};
        bm_image_copy_device_to_host(output_img, (void **)out_ptr);
    } else if (format == FORMAT_NV12 ||
               format == FORMAT_NV21 ||
               format == FORMAT_NV16 ||
               format == FORMAT_NV61 ||
               format == FORMAT_NV24) {
        unsigned char *out_ptr[2] = {output, output + width * height};
        bm_image_copy_device_to_host(output_img, (void **)out_ptr);
    } else {
        unsigned char *out_ptr[3] = {output, output + height * width, output + 2 * height * width};
        bm_image_copy_device_to_host(output_img, (void **)out_ptr);
    }
    bm_image_destroy(&input1_img);
    bm_image_destroy(&input2_img);
    bm_image_destroy(&output_img);
    return 0;
}

static int test_absdiff_random( int if_use_img,
                                int format,
                                int height,
                                int width,
                                char *src1_name,
                                char *src2_name,
                                char *dst_name,
                                bm_handle_t handle) {
    struct timeval t1, t2;
    printf("width: %d , height: %d, format: %d\n", width, height, format);
    int img_size = 0;
    img_size = get_image_size(format, width, height);
    printf("img_size is %d \n", img_size);
    unsigned char* input1_data = malloc(width * height * 3);
    unsigned char* input2_data = malloc(width * height * 3);
    unsigned char* output_tpu = malloc(width * height * 3);
    unsigned char* output_cmodel = malloc(width * height * 3);
    if(if_use_img){
        printf("test real img \n");
        readBin(src1_name, input1_data, img_size);
        readBin(src2_name, input2_data, img_size);
    }else{
        printf("test random data !\n");
        fill(input1_data, img_size);
        fill(input2_data, img_size);
    }
    memset(output_tpu, 0, width * height * 3);
    memset(output_cmodel, 0, width * height * 3);
    gettimeofday(&t1, NULL);
    absdiff_cmodel(input1_data,
                   input2_data,
                   output_cmodel,
                   img_size);
    gettimeofday(&t2, NULL);
    printf("Absdiff CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    int ret = absdiff_tpu(input1_data,
                input2_data,
                output_tpu,
                height, width,
                format,
                handle);
    if (ret != 0) {
        free(input1_data);
        free(input2_data);
        free(output_tpu);
        free(output_cmodel);
        return -1;
    }
    ret = cmp(output_tpu, output_cmodel, img_size);
    if(if_use_img){
        printf("output img by tpu\n");
        writeBin(dst_name, output_tpu, img_size);
    }
    free(input1_data);
    free(input2_data);
    free(output_tpu);
    free(output_cmodel);
    return ret;
}

void* test_thread_absdiff(void* args) {
    cv_absdiff_thread_arg_t* cv_absdiff_thread_arg = (cv_absdiff_thread_arg_t*)args;
    int loop_num = cv_absdiff_thread_arg->loop_num;
    int use_real_img = cv_absdiff_thread_arg->use_real_img;
    int format = cv_absdiff_thread_arg->format;
    int height = cv_absdiff_thread_arg->height;
    int width = cv_absdiff_thread_arg->width;
    char *src1_name = cv_absdiff_thread_arg->src1_name;
    char *src2_name = cv_absdiff_thread_arg->src2_name;
    char *dst_name = cv_absdiff_thread_arg->dst_name;
    bm_handle_t handle = cv_absdiff_thread_arg->handle;

    for (int i = 0; i < loop_num; i++) {
        // if (loop_num > 1) {
        //     height = 1 + rand() % 4096;
        //     width = 1 + rand() % 4096;
        //     format = rand() % 15;
        // }
        if(0 != test_absdiff_random(use_real_img, format, height, width, src1_name, src2_name, dst_name, handle)){
            printf("------TEST ABSDIFF FAILED!------\n");
            exit(-1);
        }
        printf("------TEST ABSDIFF SUCCESSED!------\n");
    }
    return (void*)0;
}

int main(int argc, char* args[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int loop = 1;
    int use_real_img = 0;
    int height = 1 + rand() % 4096;
    int width = 1 + rand() % 4096;
    int format = rand() % 15;
    int thread_num = 1;
    char *src1_name = NULL;
    char *src2_name = NULL;
    char *dst_name = NULL;
    int check = 0;
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    printf("seed = %d\n", seed);
    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop use_real_img format height width src1_name src2_name dst_name(when use_real_img = 1,need to set src_name and dst_name) \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 100\n", args[0]);
        printf("%s 2 1 0 12 8 8 \n", args[0]);
        printf("%s 1 1 1 12 1024 1024 input1.bin input2.bin asbdiff_output.bin \n", args[0]);
        return 0;
    }
    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop =  atoi(args[2]);
    if (argc > 3) use_real_img = atoi(args[3]);
    if (argc > 4) format = atoi(args[4]);
    if (argc > 5) height  = atoi(args[5]);
    if (argc > 6) width = atoi(args[6]);
    if (argc > 7) src1_name = args[7];
    if (argc > 8) src2_name = args[8];
    if (argc > 9) dst_name = args[9];

    check = parameters_check(height, width);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }
    // test for multi-thread
    pthread_t pid[thread_num];
    cv_absdiff_thread_arg_t cv_absdiff_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_absdiff_thread_arg[i].loop_num = loop;
        cv_absdiff_thread_arg[i].use_real_img = use_real_img;
        cv_absdiff_thread_arg[i].format = format;
        cv_absdiff_thread_arg[i].height = height;
        cv_absdiff_thread_arg[i].width = width;
        cv_absdiff_thread_arg[i].src1_name = src1_name;
        cv_absdiff_thread_arg[i].src2_name = src2_name;
        cv_absdiff_thread_arg[i].dst_name = dst_name;
        cv_absdiff_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_thread_absdiff, &cv_absdiff_thread_arg[i]) != 0) {
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