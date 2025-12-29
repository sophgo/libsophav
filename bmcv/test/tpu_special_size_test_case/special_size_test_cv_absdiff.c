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

static int cmp( unsigned char* got, unsigned char* exp, int len) {
    for(int i = 0; i < len; i++){
        if(got[i] != exp[i]){
            printf("cmp error: idx=%d  exp=%d  got=%d\n", i, exp[i], got[i]);
            return -1;
        }
    }
    return 0;
}

static int absdiff_cmodel(unsigned char* input1,
                          unsigned char* input2,
                          unsigned char* output,
                          int img_size){
    for(int i = 0; i < img_size; i++)
        output[i] = abs(input1[i] - input2[i]);
    return 0;
}

static int absdiff_tpu(
        unsigned char* input1,
        unsigned char* input2,
        unsigned char* output,
        int height,
        int width,
        int format,
        bm_handle_t handle,
        long int *tpu_time) {
    bm_image input1_img;
    bm_image input2_img;
    bm_image output_img;

    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input1_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input2_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input1_img, 2);
    bm_image_alloc_dev_mem(input2_img, 2);
    bm_image_alloc_dev_mem(output_img, 2);
    if(format == 0){
        unsigned char *in1_ptr[3] = {input1, input1 + width * height, input1 + width * height * 5 / 4};
        unsigned char *in2_ptr[3] = {input2, input2 + width * height, input2 + width * height * 5 / 4};
        bm_image_copy_host_to_device(input1_img, (void **)(in1_ptr));
        bm_image_copy_host_to_device(input2_img, (void **)(in2_ptr));
    } else if(format == 1) {
        unsigned char *in1_ptr[3] = {input1, input1 + width * height, input1 + width * height * 3 / 2};
        unsigned char *in2_ptr[3] = {input2, input2 + width * height, input2 + width * height * 3 / 2};
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
    long int using_time = ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec);
    printf("Absdiff TPU using time: %ld(us)\n", using_time);
    *tpu_time += using_time;
    if(format == 0){
        unsigned char *out_ptr[3] = {output, output + width * height, output + width * height * 5 / 4};
        bm_image_copy_device_to_host(output_img, (void **)out_ptr);
    } else if (format == 1){
        unsigned char *out_ptr[3] = {output, output + width * height, output + width * height * 3 / 2};
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
                                bm_handle_t handle,
                                long int *cpu_time,
                                long int *tpu_time) {
    struct timeval t1, t2;
    int img_size = 0;
    img_size = get_image_size(format, width, height);
    //printf("img_size is %d \n", img_size);
    unsigned char* input1_data = malloc(width * height * 3);
    unsigned char* input2_data = malloc(width * height * 3);
    unsigned char* output_tpu = malloc(width * height * 3);
    unsigned char* output_cmodel = malloc(width * height * 3);
    if(if_use_img){
        printf("test real img \n");
        readBin(src1_name, input1_data, img_size);
        readBin(src2_name, input2_data, img_size);
    }else{
        //printf("test random data !\n");
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
    *cpu_time += TIME_COST_US(t1, t2);
    int ret = absdiff_tpu(input1_data,
                input2_data,
                output_tpu,
                height, width,
                format,
                handle,
                tpu_time);
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

int main(int argc, char* args[]) {
    printf("log will be saved in special_size_test_cv_absdiff.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_absdiff.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    int use_real_img = 0;
    int special_height[6] = {1, 720, 1080, 1440, 2160, 4096};
    int special_width[6] = {1, 1280, 1920, 2560, 3840, 4096};
    int format_max = 14;
    int height, width, format;
    char *src1_name = NULL;
    char *src2_name = NULL;
    char *dst_name = NULL;

    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }
    for(format = 0; format <= format_max; format++) {
        for(int i = 0; i < 6; i++) {
            width = special_width[i];
            height = special_height[i];
            printf("width: %d , height: %d, format: %d\n", width, height, format);
            long int cputime = 0;
            long int tputime = 0;
            long int *cpu_time = &cputime;
            long int *tpu_time = &tputime;
            for(int loop = 0; loop < 3; loop++) {
                if(0 != test_absdiff_random(use_real_img, format, height, width, src1_name, src2_name, dst_name, handle, cpu_time, tpu_time)) {
                    printf("------TEST ABSDIFF FAILED!------\n");
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
    return 0;
}