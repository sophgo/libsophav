
#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "bmcv_internal.h"
typedef struct {
    int loop;
    int if_use_img;
    int data_type;
    int format;
    int width;
    int height;
    int in_width_stride;
    int out_width_stride;
    char* src_name;
    char* dst_name;
    bm_handle_t handle;
} cv_width_align_thread_arg_t;

static int parameters_check(int height, int width, int format, int type)
{
    int error = 0;
    if (height > 4096 || width > 4096){
        printf("Unsupported size : size_max = 4096 x 4096 \n");
        error = -1;
    }
    if (format > 14) {
        printf("Unsupported image format!\n");
        error = -1;
    }
    if(type != 1 && type != 0){
        printf("parameters error : type = 1 / 0, 1 is uint8, 0 is float \n");
        error = -1;
    }
    return error;
}

static void get_image_stride(int format, int type, int *src_stride, int *dst_stride, int in_width_stride_, int out_width_stride_) {
    int data_size = 1;
    if (type == 0)
        data_size = 4;
    int in_width_stride = in_width_stride_ * data_size;
    int out_width_stride = out_width_stride_ * data_size;
    switch (format) {
        case FORMAT_YUV420P:
        case FORMAT_YUV422P:
            src_stride[0] = in_width_stride;
            src_stride[1] = in_width_stride/2;
            src_stride[2] = in_width_stride/2;
            dst_stride[0] = out_width_stride;
            dst_stride[1] = out_width_stride/2;
            dst_stride[2] = out_width_stride/2;
            break;
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
            src_stride[0] = in_width_stride * 3;
            dst_stride[0] = out_width_stride * 3;
            break;
        default:
            for (int i = 0; i < 4; i++) {
                src_stride[i] = in_width_stride;
                dst_stride[i] = out_width_stride;
            }
            break;
    }
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

static void fill_input_f32(float* input, int img_size)
{
    int i;

    for (i = 0; i < img_size; ++i) {
        input[i] = (float)rand() / (float)(RAND_MAX) * 255.0f;
    }
}

static void fill_input_u8(unsigned char* input, int img_size)
{
    int i;

    for (i = 0; i < img_size; ++i) {
        input[i] = rand() % 256;
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

static int tpu_width_align(void* input, void* output, int format, int height,
                        int width, int in_width_stride, int out_width_stride, int dtype, bm_handle_t handle)
{
    bm_status_t ret = BM_SUCCESS;
    int type = dtype;   // float: 0; uint: 1
    bm_image input_img, output_img;
    int src_stride[4] = {0};
    int dst_stride[4] = {0};
    get_image_stride(format, type, src_stride, dst_stride, in_width_stride, out_width_stride);
    // src_stride[0] = in_width_stride;
    // dst_stride[0] = out_width_stride;
    // src_stride[1] = in_width_stride/2;
    // dst_stride[1] = out_width_stride/2;
    // src_stride[2] = in_width_stride/2;
    // dst_stride[2] = out_width_stride/2;

    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, (bm_image_data_format_ext)type, &input_img, src_stride);
    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, (bm_image_data_format_ext)type, &output_img, dst_stride);
    ret = bm_image_alloc_dev_mem(input_img, 2);
    ret = bm_image_alloc_dev_mem(output_img, 2);

    int image_byte_size[4] = {0};
    bm_image_get_byte_size(input_img, image_byte_size);
    void* input_addr[4] = {(void *)input,
                            (void *)((unsigned char*)input + image_byte_size[0]),
                            (void *)((unsigned char*)input + image_byte_size[0] + image_byte_size[1]),
                            (void *)((unsigned char*)input + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
    void* out_addr[4] = {(void *)output,
                            (void *)((unsigned char*)output + image_byte_size[0]),
                            (void *)((unsigned char*)output + image_byte_size[0] + image_byte_size[1]),
                            (void *)((unsigned char*)output + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

    ret = bm_image_copy_host_to_device(input_img, (void **)input_addr);
    ret = bmcv_width_align(handle, input_img, output_img);
    ret = bm_image_copy_device_to_host(output_img, (void**)out_addr);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot copyy d2h\n");
        return -1;
    }

    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);
    return ret;
}

static int test_width_align_random(int if_use_img, int format, int data_type, int width, int height, int in_width_stride, int out_width_stride, char* src_name, char* dst_name, bm_handle_t handle) {
    int ret = 0;
    int width_ = ALIGN(width, 2);
    int height_ = ALIGN(height, 2);
    int in_width_stride_ = ALIGN(in_width_stride, 2);
    int out_width_stride_ = ALIGN(out_width_stride, 2);

    int raw_img_size = get_image_size(format, width_, height_);
    int input_img_size = get_image_size(format, in_width_stride_, height_);
    int output_img_size = get_image_size(format, out_width_stride_, height_);
    if (data_type == 0) {
        float* raw_img = (float*)malloc(raw_img_size * sizeof(float));
        float* input = (float*)malloc(input_img_size * sizeof(float));
        float* tpu_out = (float*)malloc(output_img_size * sizeof(float));

        if (if_use_img) {
            printf("Real img only support DTYPE_U8 \n");
            return -1;
        } else {
            printf("test random data ! \n");
            fill_input_f32(raw_img, raw_img_size);
        }
        float* raw_offset;
        float* src_offset;
        for (int i = 0; i < height_; i++) {
            raw_offset = raw_img + i * width_;
            src_offset = input + i * in_width_stride_;
            memcpy(src_offset, raw_offset, width_);
        }
        printf("width = %d, height = %d, format = %d, type = %d\n", width_, height_, format, data_type);

        ret = tpu_width_align((void*)input, (void*)tpu_out, format, height_, width_, in_width_stride_, out_width_stride_, data_type, handle);
        if (ret) {
            printf("the fp32 tpu_width_align is failed!\n");
            free(input);
            free(tpu_out);
            return -1;
        }
        free(input);
        free(tpu_out);
    } else if (data_type == 1) {
        unsigned char* raw_img = (unsigned char*)malloc(raw_img_size * sizeof(unsigned char));
        unsigned char* input = (unsigned char*) malloc(input_img_size * sizeof(unsigned char));
        unsigned char* tpu_out = (unsigned char*) malloc(output_img_size * sizeof(unsigned char));

        if (if_use_img) {
            printf("test real img \n");
            readBin(src_name, raw_img, raw_img_size);
        } else {
            printf("test random data ! \n");
            fill_input_u8(raw_img, raw_img_size);
        }
        unsigned char* raw_offset;
        unsigned char* src_offset;
        for (int i = 0; i < height_; i++) {
            raw_offset = raw_img + i * width_;
            src_offset = input + i * in_width_stride_;
            memcpy(src_offset, raw_offset, width_);
        }
        printf("width = %d, height = %d, format = %d, type = %d\n", width_, height_, format, data_type);
        ret = tpu_width_align((void*)input, (void*)tpu_out, format, height_, width_, in_width_stride_, out_width_stride_, data_type, handle);
        if (ret) {
            printf("the fp32 tpu_width_align is failed!\n");
            free(input);
            free(tpu_out);
            return -1;
        }
        if(if_use_img){
            printf("output img by tpu\n");
            writeBin(dst_name, tpu_out, output_img_size);
        }
        free(input);
        free(tpu_out);
    }

    return ret;
}

void* test_width_align(void* args) {
    cv_width_align_thread_arg_t* cv_width_align_thread_arg = (cv_width_align_thread_arg_t*)args;
    int loop = cv_width_align_thread_arg->loop;
    int if_use_img = cv_width_align_thread_arg->if_use_img;
    int data_type = cv_width_align_thread_arg->data_type;
    int format = cv_width_align_thread_arg->format;
    int width = cv_width_align_thread_arg->width;
    int height = cv_width_align_thread_arg->height;
    int in_width_stride = cv_width_align_thread_arg->in_width_stride;
    int out_width_stride = cv_width_align_thread_arg->out_width_stride;
    char *src_name = cv_width_align_thread_arg->src_name;
    char *dst_name = cv_width_align_thread_arg->dst_name;
    bm_handle_t handle = cv_width_align_thread_arg->handle;
    for (int i = 0; i < loop; i++) {
        int ret = test_width_align_random(if_use_img, format, data_type, width, height, in_width_stride, out_width_stride, src_name, dst_name, handle);
        if (ret) {
            printf("------Test width_align Failed!------\n");
            exit(-1);
        }
        printf("------Test width_align Successed!------\n");
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
    int height = 1 + rand() % 4096;
    int width = 1 + rand() % 4096;
    int in_width_stride = width + rand() % 10;
    int out_width_stride = width + rand() % 10;
    int format_num[] = {0,1,2,8,9,10,11,12,13,14};
    int size = sizeof(format_num) / sizeof(format_num[0]);
    int rand_num = rand() % size;
    int format = format_num[rand_num];
    int data_type = 0;  //0: float; 1: uint;
    char* src_name = NULL;
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
        printf("%s thread_num loop if_use_img data_type format width height input_width_stride output_width_stride src_name dst_name(when use_real_img = 1,need to set src_name and dst_name) \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 1\n", args[0]);
        printf("%s 1 1 0 1 1 8 8 12 12 \n", args[0]);
        printf("%s 1 1 1 0 0 8 8 12 12 res/1920x1080_rgb out/out_add_weighted.bin \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) if_use_img = atoi(args[3]);
    if (argc > 4) data_type = atoi(args[4]);
    if (argc > 5) format = atoi(args[5]);
    if (argc > 6) width = atoi(args[6]);
    if (argc > 7) height = atoi(args[7]);
    if (argc > 8) in_width_stride = atoi(args[8]);
    if (argc > 9) out_width_stride = atoi(args[9]);
    if (argc > 10) src_name = args[10];
    if (argc > 11) dst_name = args[11];

    check = parameters_check(height, width, format, data_type);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    printf("seed = %d\n", seed);

    pthread_t pid[thread_num];
    cv_width_align_thread_arg_t cv_width_align_thread_arg[thread_num];
    for (i = 0; i < thread_num; i++) {
        cv_width_align_thread_arg[i].loop = loop;
        cv_width_align_thread_arg[i].if_use_img = if_use_img;
        cv_width_align_thread_arg[i].height = height;
        cv_width_align_thread_arg[i].width = width;
        cv_width_align_thread_arg[i].in_width_stride = in_width_stride;
        cv_width_align_thread_arg[i].out_width_stride = out_width_stride;
        cv_width_align_thread_arg[i].format = format;
        cv_width_align_thread_arg[i].data_type = data_type;
        cv_width_align_thread_arg[i].src_name = src_name;
        cv_width_align_thread_arg[i].dst_name = dst_name;
        cv_width_align_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_width_align, &cv_width_align_thread_arg[i]) != 0) {
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
