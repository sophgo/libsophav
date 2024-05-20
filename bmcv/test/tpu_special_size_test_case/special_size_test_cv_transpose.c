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
                        int width, int stride, int dtype, bm_handle_t handle, long int *tpu_time)
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
    *tpu_time += TIME_COST_US(t1, t2);
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
                          char* src_name, char* dst_name, bm_handle_t handle, long int *cpu_time, long int *tpu_time)
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
            fill_input_f32(input, channel, height, stride);
        }
        printf("width = %d, height = %d, channel = %d, type = %d\n", width, height, channel, type);
        memset(tpu_out, 0.f, channel * height * width);
        memset(cpu_out, 0.f, channel * height * width);

        ret = tpu_transpose((void*)input, (void*)tpu_out, channel, height, width, stride, type, handle, tpu_time);
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
        *cpu_time += TIME_COST_US(t1, t2);
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
            fill_input_u8(input, channel, height, stride);
        }
        memset(tpu_out, 0, channel * height * width);
        memset(cpu_out, 0, channel * height * width);

        ret = tpu_transpose((void*)input, (void*)tpu_out, channel, height, width, stride, type, handle, tpu_time);
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
        *cpu_time += TIME_COST_US(t1, t2);
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

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_transpose.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_transpose.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int if_use_img = 0;
    int channel;
    int special_height[9] = {1, 100, 400, 800, 720, 1080, 1440, 2160, 4096};
    int special_width[9] = {1, 100, 400, 800, 1280, 1920, 2560, 3840, 4096};
    int format_num[] = {8,9,14};
    int type_num[] = {1, 4};
    char* src_name = NULL;
    char* dst_name = NULL;
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot get handle\n");
        return -1;
    }
    for(int t = 0; t < 2; t++){
        int type = type_num[t];
        for(int i = 0; i < 3; i++) {
            int format = format_num[i];
            if(format == 14) {
                channel = 1;
            } else {
                channel = 3;
            }
            for(int j = 0; j < 9; j++) {
                int width = special_width[j];
                int height = special_height[j];
                int stride = width;
                printf("type: %d, width: %d, height: %d, format: %d\n", type, width, height, format);
                long int cputime = 0;
                long int tputime = 0;
                long int *cpu_time = &cputime;
                long int *tpu_time = &tputime;
                for(int loop = 0; loop < 3; loop++) {
                    int ret = test_transpose(if_use_img, channel, height, width, stride, type, src_name, dst_name, handle, cpu_time, tpu_time);
                    if (ret) {
                        printf("------Test Transpose Failed!------\n");
                        return -1;
                    }
                }
                printf("------average CPU time = %ldus------\n", cputime / 3);
                printf("------average TPU time = %ldus------\n", tputime / 3);
            }
        }
    }
    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}