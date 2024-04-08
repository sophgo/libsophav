#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#define DTYPE_F32 4
#define DTYPE_U8 1
#define ALIGN(a, b) (((a) + (b) - 1) / (b) * (b))
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop;
    int if_use_img;
    int channel;
    int height;
    int width;
    int stride;
    int type;
    char* src_name;
    char* dst_name;
    bm_handle_t handle;
} cv_transpose_thread_arg_t;

static void fill_input_f32(float* input, int channel, int height, int width)
{
    int num = channel * height * width;
    int i;

    for (i = 0; i < num; i++) {
        input[i] = (float)(rand() % 256);
    }
}

static void fill_input_u8(unsigned char* input, int channel, int height, int width)
{
    int num = channel * height * width;
    int i;

    for (i = 0; i < num; i++) {
        input[i] = (unsigned char)(rand() % 256);
    }
}

static void readBin(const char* path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

static void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb+");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

static int parameters_check(int height, int width, int channel, int type)
{
    int error = 0;
    if (height > 4096 || width > 4096){
        printf("Unsupported size : size_max = 4096 x 4096 \n");
        error = -1;
    }
    if(channel != 1 && channel != 3){
        printf("parameters error : channel = 1/3 \n");
        error = -1;
    }
    if(type != 1 && type != 4){
        printf("parameters error : type = 1 / 4, 1 is uint8, 4 is float \n");
        error = -1;
    }
    return error;
}

static int cmp_result(void* tpu_out, void* cpu_out, int channel, int height, int width, int type)
{
    int i;

    if (type == DTYPE_F32) {
        float* tpu_out_tmp = (float*)tpu_out;
        float* cpu_out_tmp = (float*)cpu_out;

        for (i = 0; i < channel * height * width; ++i) {
            if (tpu_out_tmp[i] != cpu_out_tmp[i]) {
                printf("ERROR[fp32]: compare of %d mismatch tpu = %f, cpu = %f\n", \
                        i, tpu_out_tmp[i], cpu_out_tmp[i]);
                return -1;
            }
        }
    } else {
        unsigned char* tpu_out_tmp = (unsigned char*)tpu_out;
        unsigned char* cpu_out_tmp = (unsigned char*)cpu_out;

        for (i = 0; i < channel * height * width; ++i) {
            if (tpu_out_tmp[i] != cpu_out_tmp[i]) {
                printf("ERROR[uint8]: compare of %d mismatch tpu = %d cpu = %d\n", \
                        i, tpu_out_tmp[i], cpu_out_tmp[i]);
                return -1;
            }
        }
    }

    return 0;
}

static int tpu_transpose(void* input, void* output, int channel, int height,
                        int width, int stride, int dtype, bm_handle_t handle)
{

    bm_status_t ret = BM_SUCCESS;
    int format;
    int type = 0;
    int stride_byte;
    bm_image input_img, output_img;
    struct timeval t1, t2;

    if (channel != 1 && channel != 3 && dtype != DTYPE_F32 && dtype != DTYPE_U8) {
        printf("the param channel or type is error!\n");
        return -1;
    }
    if (input == NULL || output == NULL) {
        printf("the input or the output is null!\n");
        return -1;
    }

    format = (channel == 3) ? FORMAT_BGR_PLANAR : FORMAT_GRAY;

    if (dtype == DTYPE_F32) {
        type = DATA_TYPE_EXT_FLOAT32;
        stride_byte = stride * sizeof(float);
    } else if (dtype == DTYPE_U8) {
        type = DATA_TYPE_EXT_1N_BYTE;
        stride_byte = stride * sizeof(unsigned char);
    }

    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format,
                        (bm_image_data_format_ext)type, &input_img, &stride_byte);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot create input_bm_image\n");
        return -1;
    }

    ret = bm_image_alloc_dev_mem(input_img, 2);
    if (ret != BM_SUCCESS) {
        printf("Alloc bm dev input mem failed. ret = %d\n", ret);
        bm_image_destroy(&input_img);
        return -1;
    }

    ret = bm_image_copy_host_to_device(input_img, (void**)&input);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot cpoy h2d\n");
        return -1;
    }

    ret = bm_image_create(handle, width, height, (bm_image_format_ext)format,
                        (bm_image_data_format_ext)type, &output_img, NULL);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot create output_bm_image\n");
        return -1;
    }

    ret = bm_image_alloc_dev_mem(output_img, 2);
    if (ret != BM_SUCCESS) {
        printf("Alloc bm dev output mem failed. ret = %d\n", ret);
        bm_image_destroy(&input_img);
        bm_image_destroy(&output_img);
        return -1;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_image_transpose(handle, input_img, output_img);
    if (ret != BM_SUCCESS) {
        printf("bmcv_image_transpose error!");
        ret = BM_ERR_FAILURE;
    }
    gettimeofday(&t2, NULL);
    printf("Transpose TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = bm_image_copy_device_to_host(output_img, (void**)&output);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot cpoy d2h\n");
        return -1;
    }

    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);
    return ret;
}

static int cpu_transpose(void* input, void* output, int channel, int height,
                        int width, int stride, int dtype)
{
    int i, j, k;

    if (input == NULL || output == NULL) {
        printf("the input or the output is null!\n");
        return -1;
    }
    if (dtype == DTYPE_F32) {
        float* input_tmp = (float*)input;
        float* output_tmp = (float*)output;

        for (i = 0; i < channel; ++i) {
            for (j = 0; j < width; ++j) {
                for (k = 0; k < height; ++k) {
                    output_tmp[i * height * width + j * height + k] = \
                                                    input_tmp[i * height * stride + k * stride + j];
                }
            }
        }
    } else if (dtype == DTYPE_U8) {
        unsigned char* input_tmp = (unsigned char*)input;
        unsigned char* output_tmp = (unsigned char*)output;

        for (i = 0; i < channel; ++i) {
            for (j = 0; j < width; ++j) {
                for (k = 0; k < height; ++k) {
                    output_tmp[i * height * width + j * height + k] = \
                                                    input_tmp[i * height * stride + k * stride + j];
                }
            }
        }
    }

    return 0;
}

static int test_transpose(int if_use_img, int channel, int height, int width, int stride, int type,
                          char* src_name, char* dst_name, bm_handle_t handle)
{
    int ret = 0;
    struct timeval t1, t2;
    if (type == DTYPE_F32) {
        float* input = (float*) malloc(channel * height * stride * sizeof(float));
        float* tpu_out = (float*) malloc(channel * height * width * sizeof(float));
        float* cpu_out = (float*) malloc(channel * height * width * sizeof(float));

        if (if_use_img) {
            printf("Real img only support DTYPE_U8 \n");
        } else {
            printf("test random data ! \n");
            fill_input_f32(input, channel, height, stride);
        }
        printf("width = %d, height = %d, channel = %d, type = %d\n", width, height, channel, type);
        memset(tpu_out, 0.f, channel * height * width);
        memset(cpu_out, 0.f, channel * height * width);

        ret = tpu_transpose((void*)input, (void*)tpu_out, channel, height, width, stride, type, handle);
        if (ret) {
            printf("the fp32 tpu_transpose is failed!\n");
            free(input);
            free(tpu_out);
            free(cpu_out);
            return -1;
        }
        gettimeofday(&t1, NULL);
        ret = cpu_transpose((void*)input, (void*)cpu_out, channel, height, width, stride, type);
        if (ret) {
            printf("the fp32 cpu_transpose is failed!\n");
            free(input);
            free(tpu_out);
            free(cpu_out);
            return -1;
        }
        gettimeofday(&t2, NULL);
        printf("Transpose CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

        ret = cmp_result((void*)tpu_out, (void*)cpu_out, channel, height, width, type);
        if (ret) {
            printf("the compare fp32 tpu_cpu is failed!\n");
            free(input);
            free(tpu_out);
            free(cpu_out);
            return -1;
        }
        free(input);
        free(tpu_out);
        free(cpu_out);
    } else if (type == DTYPE_U8) {
        unsigned char* input = (unsigned char*) malloc(channel * height * stride * sizeof(unsigned char));
        unsigned char* tpu_out = (unsigned char*) malloc(channel * height * width * sizeof(unsigned char));
        unsigned char* cpu_out = (unsigned char*) malloc(channel * height * width * sizeof(unsigned char));

        if (if_use_img) {
            printf("test real img \n");
            readBin(src_name, input, channel * height * stride);
        } else {
            printf("test random data ! \n");
            fill_input_u8(input, channel, height, stride);
        }
        memset(tpu_out, 0, channel * height * width);
        memset(cpu_out, 0, channel * height * width);

        ret = tpu_transpose((void*)input, (void*)tpu_out, channel, height, width, stride, type, handle);
        if (ret) {
            printf("the fp32 tpu_transpose is failed!\n");
            free(input);
            free(tpu_out);
            free(cpu_out);
            return -1;
        }

        gettimeofday(&t1, NULL);
        ret = cpu_transpose((void*)input, (void*)cpu_out, channel, height, width, stride, type);
        if (ret) {
            printf("the fp32 cpu_transpose is failed!\n");
            free(input);
            free(tpu_out);
            free(cpu_out);
            return -1;
        }
        gettimeofday(&t2, NULL);
        printf("Transpose CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

        ret = cmp_result((void*)tpu_out, (void*)cpu_out, channel, height, width, type);
        if (ret) {
            printf("the compare fp32 tpu_cpu is failed!\n");
            free(input);
            free(tpu_out);
            free(cpu_out);
            return -1;
        }
        if(if_use_img){
            printf("output img by tpu\n");
            writeBin(dst_name, tpu_out, channel * height * width);
        }

        free(input);
        free(tpu_out);
        free(cpu_out);
    }

    return ret;
}

void* thread_test_transpose(void* args) {
    cv_transpose_thread_arg_t* cv_transpose_thread_arg = (cv_transpose_thread_arg_t*)args;
    int loop = cv_transpose_thread_arg->loop;
    int if_use_img = cv_transpose_thread_arg->if_use_img;
    int channel = cv_transpose_thread_arg->channel;
    int height = cv_transpose_thread_arg->height;
    int width = cv_transpose_thread_arg->width;
    int stride = cv_transpose_thread_arg->stride;
    int type = cv_transpose_thread_arg->type;
    char* src_name = cv_transpose_thread_arg->src_name;
    char* dst_name = cv_transpose_thread_arg->dst_name;
    bm_handle_t handle = cv_transpose_thread_arg->handle;
    for (int i = 0; i < loop; ++i) {
        int ret = test_transpose(if_use_img, channel, height, width, stride, type, src_name, dst_name, handle);
        if (ret) {
            printf("------Test Transpose Failed!------\n");
            exit(-1);
        }
        printf("------Test Transpose Successed!------\n");
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
    int if_use_img = 0;
    int loop = 1;
    int channel = 1;
    int height = 1 + rand() % 4096;
    int width = 1 + rand() % 4096;
    int stride = width;
    int type = 1;
    int ret = 0;
    int check = 0;
    char* src_name = NULL;
    char* dst_name = NULL;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot get handle\n");
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop use_real_img type channel height width src_name dst_name(when use_real_img = 1,need to set src_name and dst_name) \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2 1 0\n", args[0]);
        printf("%s 2 1 0 1 3 512 512 \n", args[0]);
        printf("%s 1 1 1 4 3 512 512 input.bin transpose_output.bin \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) if_use_img = atoi(args[3]);
    if (argc > 4) type = atoi(args[4]);
    if (argc > 5) channel = atoi(args[5]);
    if (argc > 6) height = atoi(args[6]);
    if (argc > 7) width = atoi(args[7]);
    if (argc > 8) src_name = args[8];
    if (argc > 9) dst_name = args[9];

    stride = ALIGN(width, 64); /*stride is align width.*/
    check = parameters_check(height, width, channel, type);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }
    printf("seed = %d\n", seed);
    // test for multi-thread
    pthread_t pid[thread_num];
    cv_transpose_thread_arg_t cv_transpose_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_transpose_thread_arg[i].loop = loop;
        cv_transpose_thread_arg[i].if_use_img = if_use_img;
        cv_transpose_thread_arg[i].channel = channel;
        cv_transpose_thread_arg[i].height = height;
        cv_transpose_thread_arg[i].width = width;
        cv_transpose_thread_arg[i].stride = stride;
        cv_transpose_thread_arg[i].type = type;
        cv_transpose_thread_arg[i].src_name = src_name;
        cv_transpose_thread_arg[i].dst_name = dst_name;
        cv_transpose_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, thread_test_transpose, &cv_transpose_thread_arg[i]) != 0) {
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