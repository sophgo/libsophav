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

struct frame_size {
    int height;
    int width;
};

typedef struct {
    int loop;
    int if_use_img;
    int height;
    int width;
    int format;
    float alpha;
    float beta;
    float gamma;
    char* src1_name;
    char* src2_name;
    char* dst_name;
    bm_handle_t handle;
} cv_add_weighted_thread_arg_t;

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
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

static void fill_img(unsigned char* input, int img_size)
{
    int i;

    for (i = 0; i < img_size; ++i) {
        input[i] = rand() % 256;
    }
}

static int cpu_add_weighted(unsigned char* input1, unsigned char* input2, unsigned char* output,
                            int img_size,  float alpha, float beta, float gamma)
{
    double roundNum = 0;
    int i;

    for (i = 0; i < img_size; i++) {
        roundNum = round(input1[i] * alpha + input2[i] * beta + gamma);
        if (roundNum > UINT8_MAX) {
            roundNum = UINT8_MAX; /*If exceed the max value, result is UINT8_MAX*/
        } else if (roundNum < 0) {
            roundNum = 0; /*If less than the min value, result is 0*/
        }
        output[i] = (uint8_t)roundNum;
    }
    return 0;
}

static int tpu_add_weighted(unsigned char* input1, unsigned char* input2,
                            unsigned char* output, int height, int width,
                            int format, float alpha, float beta, float gamma, bm_handle_t handle, long int *tpu_time)
{
    bm_image input1_img;
    bm_image input2_img;
    bm_image output_img;
    struct timeval t1, t2;
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

     if (format == FORMAT_YUV420P) {
        unsigned char* in1_ptr[3] = {input1, input1 + width * height, input1 + width * height * 5 / 4};
        unsigned char* in2_ptr[3] = {input2, input2 + width * height, input2 + width * height * 5 / 4};
        ret = bm_image_copy_host_to_device(input1_img, (void **)in1_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image copy input1_img h2d failed. ret = %d\n", ret);
            return -1;
        }
        ret = bm_image_copy_host_to_device(input2_img, (void **)in2_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image copy input2_img h2d failed. ret = %d\n", ret);
            return -1;
        }
        gettimeofday(&t1, NULL);
        ret = bmcv_image_add_weighted(handle, input1_img, alpha, input2_img, beta, gamma, output_img);
        if (ret != BM_SUCCESS) {
            printf("bm_image add_weighted failed. ret = %d\n", ret);
            return -1;
        }
        gettimeofday(&t2, NULL);
        printf("add_weighted TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
        *tpu_time += TIME_COST_US(t1, t2);
        unsigned char* out_ptr[3] = {output, output + height * width, output + width * height * 5 / 4};
        ret = bm_image_copy_device_to_host(output_img, (void **)out_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image copy output_img d2h failed. ret = %d\n", ret);
            return -1;
        }
    } else if (format == FORMAT_YUV422P) {
        unsigned char* in1_ptr[3] = {input1, input1 + width * height, input1 + width * height * 3 / 2};
        unsigned char* in2_ptr[3] = {input2, input2 + width * height, input2 + width * height * 3 / 2};
        ret = bm_image_copy_host_to_device(input1_img, (void **)in1_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image copy input1_img h2d failed. ret = %d\n", ret);
            return -1;
        }
        ret = bm_image_copy_host_to_device(input2_img, (void **)in2_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image copy input2_img h2d failed. ret = %d\n", ret);
            return -1;
        }
        gettimeofday(&t1, NULL);
        ret = bmcv_image_add_weighted(handle, input1_img, alpha, input2_img, beta, gamma, output_img);
        if (ret != BM_SUCCESS) {
            printf("bm_image add_weighted failed. ret = %d\n", ret);
            return -1;
        }
        gettimeofday(&t2, NULL);
        printf("add_weighted TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
        *tpu_time += TIME_COST_US(t1, t2);
        unsigned char* out_ptr[3] = {output, output + height * width, output + width * height * 3 / 2};
        ret = bm_image_copy_device_to_host(output_img, (void **)out_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image copy output_img d2h failed. ret = %d\n", ret);
            return -1;
        }
    } else {
        unsigned char* in1_ptr[3] = {input1, input1 + height * width, input1 + 2 * height * width};
        unsigned char* in2_ptr[3] = {input2, input2 + height * width, input2 + 2 * height * width};
        ret = bm_image_copy_host_to_device(input1_img, (void **)in1_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image copy input1_img h2d failed. ret = %d\n", ret);
            return -1;
        }
        ret = bm_image_copy_host_to_device(input2_img, (void **)in2_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image copy input2_img h2d failed. ret = %d\n", ret);
            return -1;
        }
        gettimeofday(&t1, NULL);
        ret = bmcv_image_add_weighted(handle, input1_img, alpha, input2_img, beta, gamma, output_img);
        if (ret != BM_SUCCESS) {
            printf("bm_image add_weighted failed. ret = %d\n", ret);
            return -1;
        }
        gettimeofday(&t2, NULL);
        printf("add_weighted TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
        *tpu_time += TIME_COST_US(t1, t2);
        unsigned char* out_ptr[3] = {output, output + height * width, output + 2 * height * width};
        ret = bm_image_copy_device_to_host(output_img, (void **)out_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image copy output_img d2h failed. ret = %d\n", ret);
            return -1;
        }
    }

    bm_image_destroy(&input1_img);
    bm_image_destroy(&input2_img);
    bm_image_destroy(&output_img);

    return 0;
}

static int cmp_result(unsigned char* got, unsigned char* exp, int len)
{
    for (int i = 0; i < len; ++i) {
        if (got[i] != exp[i]) {
            printf("cmp error: idx=%d  exp=%d  got=%d\n", i, exp[i], got[i]);
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



static int test_add_weighted_random(int if_use_img, void* frame, int format, float alpha, float beta, float gamma,
                                    char* src1_name, char* src2_name, char* dst_name, bm_handle_t handle, long int *cpu_time, long int *tpu_time)
{
    int img_size = 0;
    struct frame_size* frame_random = (struct frame_size*)frame;
    int width = frame_random->width;
    int height = frame_random->height;
    int ret;
    struct timeval t1, t2;

    img_size = get_image_size(format, width, height);

    unsigned char *input1_data = (unsigned char *)malloc(width * height * 3);
    unsigned char *input2_data = (unsigned char *)malloc(width * height * 3);
    unsigned char *output_tpu = (unsigned char *)malloc(width * height * 3);
    unsigned char *output_cpu = (unsigned char *)malloc(width * height * 3);

    if (if_use_img) {
        printf("test real img \n");
        readBin(src1_name, input1_data, img_size);
        readBin(src2_name, input2_data, img_size);
    } else {
        fill_img(input1_data, img_size);
        fill_img(input2_data, img_size);
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
    *cpu_time += TIME_COST_US(t1, t2);
    ret = tpu_add_weighted(input1_data, input2_data, output_tpu, height, width, format, alpha, beta, gamma, handle, tpu_time);
    if (ret) {
        printf("tpu_add_weighted failed!\n");
        return ret;
    }

    ret = cmp_result(output_tpu, output_cpu, img_size);
    if (ret) {
        printf("add_weighted cpu & tpu cmp failed!\n");
        return ret;
    }
    if(if_use_img){
        printf("output img by tpu\n");
        writeBin(dst_name, output_tpu, img_size);
    }

    free(input1_data);
    free(input2_data);
    free(output_tpu);
    free(output_cpu);

    return ret;
}

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_add_weight.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_add_weight.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    int if_use_img = 0;
    struct frame_size frame;
    int special_height[6] = {1, 720, 1080, 1440, 2160, 4096};
    int special_width[6] = {1, 1280, 1920, 2560, 3840, 4096};
    int format_num[10] = {0,1,2,8,9,10,11,12,13,14};
    int format;
    float alpha = 0.5;
    float beta = 1.0f - alpha;
    float gamma = 50.0;
    char* src1_name = NULL;
    char* src2_name = NULL;
    char* dst_name = NULL;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }
    for(int i = 0; i < 10; i++) {
        format = format_num[i];
        for(int j = 0; j < 6; j++) {
            frame.width = special_width[j];
            frame.height = special_height[j];
            printf("width: %d , height: %d, format: %d\n", frame.width, frame.height, format);
            long int cputime = 0;
            long int tputime = 0;
            long int *cpu_time = &cputime;
            long int *tpu_time = &tputime;
            for(int loop = 0; loop < 3; loop++) {
                int ret = test_add_weighted_random(if_use_img, &frame, format, alpha, beta, gamma,
                                                   src1_name, src2_name, dst_name, handle, cpu_time, tpu_time);
                if (ret) {
                    printf("------Test add_weighted Failed!------\n");
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
