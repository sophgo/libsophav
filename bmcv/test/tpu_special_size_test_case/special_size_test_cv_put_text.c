#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef __linux__
#include <sys/time.h>
#include <time.h>
#else
#include <windows.h>
#include <time.h>
#endif

#define IMAGE_CHN_NUM_MAX 3
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

static bmcv_point_t org = {500, 500};
static int fontFace = 0;
static int fontScale = 5;
static const char text[30] = "Hello, world!";
static unsigned char color[3] = {255, 0, 0};
static int thickness = 2;

typedef struct {
    int loop;
    int height;
    int width;
    int format;
    char* src_path;
    char* dst_path;
    bm_handle_t handle;
} cv_pt_thread_arg_t;

static void fill_img(unsigned char* input, int width, int height)
{
    int i, j;

    for (i = 0; i < height * 3; i++) {
        for (j = 0; j < width; j++) {
            input[i * width + j] = rand() % 256;
        }
    }
}

static int get_image_offset(int format, int width, int height, int* offset_list)
{
    int ret = 0;

    switch (format) {
        case FORMAT_YUV420P:
            offset_list[0] = width * height;
            offset_list[1] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;
            offset_list[2] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;
            break;
        case FORMAT_YUV422P:
            offset_list[0] = width * height;
            offset_list[1] = ALIGN(width, 2) * height >> 1;
            offset_list[2] = ALIGN(width, 2) * height >> 1;
            break;
        case FORMAT_YUV444P:
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE:
            offset_list[0] = width * height;
            offset_list[1] = width * height;
            offset_list[2] = width * height;
            break;
        case FORMAT_NV12:
        case FORMAT_NV21:
            offset_list[0] = width * height;
            offset_list[1] = ALIGN(width, 2) * ALIGN(height, 2) >> 1;
            break;
        case FORMAT_NV16:
        case FORMAT_NV61:
        case FORMAT_NV24:
            offset_list[0] = width * height;
            offset_list[1] = ALIGN(width, 2) * height;
            break;
        case FORMAT_GRAY:
            offset_list[0] = width * height;
            break;
        default:
            printf("image format error!\n");
            ret = -1;
            break;
    }

    return ret;
}

static int get_image_default_step(int format, int width, int* step)
{
    int ret = 0;

    switch (format) {
        case FORMAT_GRAY:
            step[0] = width;
            break;
        case FORMAT_YUV420P:
        case FORMAT_YUV422P:
            step[0] = width;
            step[1] = ALIGN(width, 2) >> 1;
            step[2] = ALIGN(width, 2) >> 1;
            break;
        case FORMAT_YUV444P:
            step[0] = width;
            step[1] = width;
            step[2] = width;
            break;
        case FORMAT_NV12:
        case FORMAT_NV21:
        case FORMAT_NV16:
        case FORMAT_NV61:
            step[0] = width;
            step[1] = ALIGN(width, 2);
            break;
        default:
            printf("not support format!\n");
            ret = -1;
            break;
    }

    return ret;
}

static int cmp_result(unsigned char* got, unsigned char* exp, int len)
{
    int i;

    for (i = 0; i < len; ++i) {
        if (got[i] != exp[i]) {
            printf("cmp error: idx = %d, exp = %d, got = %d\n", i, exp[i], got[i]);
            return -1;
        }
    }

    return 0;
}

static int writeBin(const char* path, void* output_data, int size)
{
    int len = 0;
    FILE* fp_dst = fopen(path, "wb+");

    if (fp_dst == NULL) {
        perror("Error opening file\n");
        return -1;
    }

    len = fwrite((void*)output_data, 1, size, fp_dst);
    if (len < size) {
        printf("file size = %d is less than required bytes = %d\n", len, size);
        return -1;
    }

    fclose(fp_dst);
    return 0;
}

static int readBin(const char* path, void* input_data)
{
    int len;
    int size;
    FILE* fp_src = fopen(path, "rb");

    if (fp_src == NULL) {
        perror("Error opening file\n");
        return -1;
    }

    fseek(fp_src, 0, SEEK_END);
    size = ftell(fp_src);
    fseek(fp_src, 0, SEEK_SET);

    len = fread((void*)input_data, 1, size, fp_src);
    if (len < size) {
        printf("file size = %d is less than required bytes = %d\n", len, size);
        return -1;
    }

    fclose(fp_src);
    return 0;
}

static int put_text_cpu(unsigned char* input, int height, int width, const char* text,
                        bmcv_point_t org, int fontFace, float fontScale, int format,
                        unsigned char color[3], int thickness, long int* cpu_time)
{
    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
    bmcv_color_t rgb = {color[0], color[1], color[2]};
    int step[IMAGE_CHN_NUM_MAX];
    int ret = 0;
    struct timeval t1, t2;
    bmMat mat;

    ret = get_image_offset(format, width, height, offset_list);
    if (ret != 0) {
        printf("get_image_offset failed!\n");
        return ret;
    }

    in_ptr[0] = input;
    in_ptr[1] = input + offset_list[0];
    in_ptr[2] = input + offset_list[0] + offset_list[1];

    ret = get_image_default_step(format, width, step);
    if (ret != 0) {
        printf("get_image_default_step failed!\n");
        return ret;
    }

    mat.width = width;
    mat.height = height;
    mat.format = (bm_image_format_ext)format;
    mat.step = step;
    mat.data = (void**)in_ptr;

    gettimeofday(&t1, NULL);
    put_text(mat, text, org, fontFace, fontScale, rgb, thickness);
    gettimeofday(&t2, NULL);
    printf("Put_text CPU using time: %ld(us)\n", TIME_COST_US(t1, t2));
    *cpu_time += TIME_COST_US(t1, t2);

    return ret;
}

static bm_status_t put_text_tpu(unsigned char* input,  int height, int width, const char* text,
                        bmcv_point_t org, int fontFace, float fontScale, int format,
                        unsigned char color[3], int thickness, bm_handle_t handle, long int* tpu_time)
{
    // UNUSED(fontFace);
    bm_status_t ret = BM_SUCCESS;
    bm_image input_img;
    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
    bmcv_color_t rgb = {color[0], color[1], color[2]};
    struct timeval t1, t2;

    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format,
                        DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    if (ret != BM_SUCCESS) {
        printf("bm_image_create failed!\n");
        goto exit0;
    }

    ret = bm_image_alloc_dev_mem(input_img, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem failed!\n");
        goto exit1;
    }

    ret = (bm_status_t)get_image_offset(format, width, height, offset_list);
    if (ret != BM_SUCCESS) {
        printf("get_image_offset failed!\n");
        goto exit1;
    }

    in_ptr[0] = input;
    in_ptr[1] = input + offset_list[0];
    in_ptr[2] = input + offset_list[0] + offset_list[1];

    ret = bm_image_copy_host_to_device(input_img, (void**)in_ptr);
    if (ret != BM_SUCCESS) {
        printf("bm_image_copy_host_to_device failed!\n");
        goto exit1;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_image_put_text(handle, input_img, text, org, rgb, fontScale, thickness);
    if (ret != BM_SUCCESS) {
        printf("bmcv_image_put_text failed!\n");
        goto exit1;
    }
    gettimeofday(&t2, NULL);
    printf("Put_text TPU using time: %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);

    ret = bm_image_copy_device_to_host(input_img, (void**)in_ptr);
    if (ret != BM_SUCCESS) {
        printf("bm_image_copy_device_to_host failed!\n");
        goto exit1;
    }

exit1:
    bm_image_destroy(&input_img);
exit0:
    return ret;
}

static int test_put_text_case(bm_handle_t handle, const char* input_path, const char* output_path, \
                            int height, int width, int format, long int* cpu_time, long int* tpu_time)
{
    int ret = 0;
    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    int total_size = 0;
    int i;
    unsigned char* data_cpu = (unsigned char*)malloc(width * height * IMAGE_CHN_NUM_MAX * sizeof(unsigned char));
    unsigned char* data_tpu = (unsigned char*)malloc(width * height * IMAGE_CHN_NUM_MAX * sizeof(unsigned char));

    if (input_path == NULL) {
        fill_img(data_cpu, width, height);
    } else {
        ret = readBin(input_path, data_cpu);
        if (ret != 0) {
            printf("readBin path = %s failed!\n", input_path);
            goto exit;
        }
    }
    memcpy(data_tpu, data_cpu, width * height * IMAGE_CHN_NUM_MAX);

    ret = put_text_cpu(data_cpu, height, width, text, org, fontFace, fontScale, format, color, thickness, cpu_time);
    if (ret != 0) {
        printf("put_text_cpu failed!\n");
        goto exit;
    }

    ret = put_text_tpu(data_tpu, height, width, text, org, fontFace, fontScale, format, color, thickness, handle, tpu_time);
    if (ret != 0) {
        printf("put_text_tpu failed!\n");
        goto exit;
    }

    ret = get_image_offset(format, width, height, offset_list);
    if (ret != 0) {
        printf("get_image_offset failed!\n");
        goto exit;
    }

    for (i = 0; i < IMAGE_CHN_NUM_MAX; ++i) {
        total_size += offset_list[i];
    }

    ret = cmp_result(data_tpu, data_cpu, total_size);
    if (ret != 0) {
        printf("cmp_result failed!\n");
        goto exit;
    }

    if (output_path != NULL) {
        ret = writeBin(output_path, data_tpu, total_size);
        if (ret != 0) {
            printf("writeBin path = %s failed!\n", output_path);
            goto exit;
        }
    }

exit:
    free(data_cpu);
    free(data_tpu);
    return ret;
}

// void* test_put_text(void* args) {
//     cv_pt_thread_arg_t* cv_pt_thread_arg = (cv_pt_thread_arg_t*)args;
//     int loop = cv_pt_thread_arg->loop;
//     int format = cv_pt_thread_arg->format;
//     int height = cv_pt_thread_arg->height;
//     int width = cv_pt_thread_arg->width;
//     char* input_path = cv_pt_thread_arg->src_path;
//     char* output_path = cv_pt_thread_arg->dst_path;
//     bm_handle_t handle = cv_pt_thread_arg->handle;
//     int i;
//     int ret = 0;

//     for (i = 0; i < loop; ++i) {
//         ret = test_put_text_case(handle, input_path, output_path, height, width, format);
//         if (ret != 0) {
//             printf("------Test Put_text Failed!------\n");
//             exit(-1);
//         }
//         printf("------Test Put_text Successed!------\n");
//     }
//     return (void*)0;
// }

// int main(int argc, char* args[])
// {
//     struct timespec tp;
//     clock_gettime(0, &tp);
//     int seed = tp.tv_nsec;
//     srand(seed);

//     int thread_num = 1;
//     int width = 100 + rand() % 1900;
//     int height = 100 + rand() % 2048;
//     int format = rand() % 7;
//     int loop = 1;
//     int ret = 0;
//     char* input_path = NULL;
//     char* output_path = NULL;
//     int i;
//     bm_handle_t handle;
//     ret = bm_dev_request(&handle, 0);
//     if (ret != BM_SUCCESS) {
//         printf("bm_dev_request failed. ret = %d\n", ret);
//         return -1;
//     }

//     if (argc == 2 && atoi(args[1]) == -1) {
//         printf("usage: %d\n", argc);
//         printf("%s thread_num loop height width format input_path output_path \n", args[0]);
//         printf("example:\n");
//         printf("%s \n", args[0]);
//         printf("%s 2\n", args[0]);
//         printf("%s 1 1 512 512 0\n", args[0]);
//         printf("%s 1 1 1080 1920 0 input.bin output.bin \n", args[0]);
//         return 0;
//     }

//     if (argc > 1) thread_num = atoi(args[1]);
//     if (argc > 2) loop = atoi(args[2]);
//     if (argc > 3) height = atoi(args[3]);
//     if (argc > 4) width = atoi(args[4]);
//     if (argc > 5) format = atoi(args[5]);
//     if (argc > 6) input_path = args[6];
//     if (argc > 7) output_path = args[7];

//     pthread_t pid[thread_num];
//     cv_pt_thread_arg_t cv_pt_thread_arg[thread_num];
//     for (i = 0; i < thread_num; ++i) {
//         cv_pt_thread_arg[i].loop = loop;
//         cv_pt_thread_arg[i].format = format;
//         cv_pt_thread_arg[i].height = height;
//         cv_pt_thread_arg[i].width = width;
//         cv_pt_thread_arg[i].src_path = input_path;
//         cv_pt_thread_arg[i].dst_path = output_path;
//         cv_pt_thread_arg[i].handle = handle;
//         if (pthread_create(&pid[i], NULL, test_put_text, &cv_pt_thread_arg[i]) != 0) {
//             printf("create thread failed\n");
//             return -1;
//         }
//     }

//     for (i = 0; i < thread_num; ++i) {
//         ret = pthread_join(pid[i], NULL);
//         if (ret != 0) {
//             printf("Thread join failed\n");
//             exit(-1);
//         }
//     }

//     bm_dev_free(handle);
//     return ret;
// }

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_pt.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_pt.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return -1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);

    int height_num[10] = {100, 256, 512, 1024, 720, 1080, 1440, 2160, 4096, 8192};
    int width_num[10] = {100, 256, 512, 1024, 1280, 1920, 2560, 3840, 4096, 8192};
    int format = 0;
    int height, width;
    bm_handle_t handle;
    int loop = 0;
    int i;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    for (i = 0; i < 10; ++i) {
        height = height_num[i];
        width = width_num[i];
        printf("the input_hieght = %d, the input_width = %d, the format = %d\n", height, width, format);
        long int cputime = 0;
        long int tputime = 0;
        for(loop = 0; loop < 10; loop++) {
            ret = test_put_text_case(handle, NULL, NULL, height, width, format, &cputime, &tputime);
            if (ret) {
                printf("------TEST PUT_TEXT FAILED------\n");
                return -1;
            }
        }
        printf("-----------average CPU time = %ldus----------\n", cputime / 10);
        printf("-----------average TPU time = %ldus----------\n", tputime / 10);
    }

    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}