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

#define SATURATE(a, s, e) ((a) > (e) ? (e) : ((a) < (s) ? (s) : (a)))
#define IMAGE_CHN_NUM_MAX 3
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

static int line_num = 6;
static bmcv_point_t start[6] = {{0, 0}, {500, 0}, {500, 500}, {0, 500}, {0, 0}, {0, 500}};
static bmcv_point_t end[6] = {{500, 0}, {500, 500}, {0, 500}, {0, 0}, {500, 500}, {500, 0}};
static unsigned char color[3] = {255, 0, 0};
static int thickness = 4;

typedef struct {
    int loop;
    int height;
    int width;
    int format;
    char* src_path;
    char* dst_path;
    bm_handle_t handle;
} cv_dl_thread_arg_t;

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

static void swap(int* a, int* b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

static int draw_line_cpu(unsigned char* input, int height, int width, int line_num, int format,
                        const bmcv_point_t* start, const bmcv_point_t* end,
                        unsigned char color[3], int thickness)
{

    bmcv_point_t* sp = (bmcv_point_t*)malloc(line_num * sizeof(bmcv_point_t));
    bmcv_point_t* ep = (bmcv_point_t*)malloc(line_num * sizeof(bmcv_point_t));
    bmcv_color_t rgb = {color[0], color[1], color[2]};
    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
    int step[IMAGE_CHN_NUM_MAX];
    int ret = 0;
    int i;
    bmMat mat;
    struct timeval t1, t2;

    for (i = 0; i < line_num; ++i) {
        sp[i].x = SATURATE(start[i].x, 0, width - 1);
        sp[i].y = SATURATE(start[i].y, 0, height - 1);
        ep[i].x = SATURATE(end[i].x, 0, width - 1);
        ep[i].y = SATURATE(end[i].y, 0, height - 1);
        // because only support start_y < end_y
        if (sp[i].y > ep[i].y) {
            swap(&(sp[i].x), &(ep[i].x));
            swap(&(sp[i].y), &(ep[i].y));
        }
    }

    ret = get_image_offset(format, width, height, offset_list);
    if (ret != 0) {
        printf("get_image_offset failed!\n");
        goto exit;
    }

    in_ptr[0] = input;
    in_ptr[1] = input + offset_list[0];
    in_ptr[2] = input + offset_list[0] + offset_list[1];

    ret = get_image_default_step(format, width, step);
    if (ret != 0) {
        printf("get_image_default_step failed!\n");
        goto exit;
    }

    mat.width = width;
    mat.height = height;
    mat.format = (bm_image_format_ext)format;
    mat.step = step;
    mat.data = (void**)in_ptr;

    gettimeofday(&t1, NULL);
    for (i = 0; i < line_num; ++i) {
        draw_line(&mat, sp[i], ep[i], rgb, thickness);
    }
    gettimeofday(&t2, NULL);
    printf("Draw_lines CPU using time: %ld(us)\n", TIME_COST_US(t1, t2));

exit:
    free(sp);
    free(ep);
    return ret;
}

static int draw_line_tpu(bm_handle_t handle, unsigned char* input, int height, int width, int line_num,
                        int format, const bmcv_point_t* p1, const bmcv_point_t* p2,
                        unsigned char color[3], int thickness)
{
    bm_status_t ret = BM_SUCCESS;
    bm_image input_img;
    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
    bmcv_color_t rgb = {color[0], color[1], color[2]};
    struct timeval t1, t2;

    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, \
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
    bmcv_image_draw_lines(handle, input_img, p1, p2, line_num, rgb, thickness);
    gettimeofday(&t2, NULL);
    printf("Draw_lines TPU using time: %ld(us)\n", TIME_COST_US(t1, t2));

    ret = bm_image_copy_device_to_host(input_img, (void**)in_ptr);
    if (ret != BM_SUCCESS) {
        printf("bm_image_copy_device_to_host failed!\n");
        goto exit1;
    }

exit1:
    bm_image_destroy(&input_img);
exit0:
    bm_dev_free(handle);
    return ret;
}

static int test_draw_line_case(bm_handle_t handle, char* input, char* output, int height, int width, int format)
{
    unsigned char* data_cpu = (unsigned char*)malloc(width * height * IMAGE_CHN_NUM_MAX * sizeof(unsigned char));
    unsigned char* data_tpu = (unsigned char*)malloc(width * height * IMAGE_CHN_NUM_MAX * sizeof(unsigned char));
    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    int total_size = 0;
    int ret = 0;
    int i;

    if (input != NULL) {
        ret = readBin(input, data_cpu);
        if (ret != 0) {
            printf("readBin path = %s failed!\n", input);
            goto exit;
        }
    } else {
        fill_img(data_cpu, width, height);
    }
    memcpy(data_tpu, data_cpu, width * height * IMAGE_CHN_NUM_MAX);

    ret = draw_line_cpu(data_cpu, height, width, line_num, format, start, end, color, thickness);
    if (ret != 0) {
        printf("draw_line_cpu failed!\n");
        goto exit;
    }

    ret = draw_line_tpu(handle, data_tpu, height, width, line_num, format, start, end, color, thickness);
    if (ret != 0) {
        printf("draw_line_tpu failed!\n");
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

    if (output != NULL) {
        ret = writeBin(output, data_tpu, total_size);
        if (ret != 0) {
            printf("writeBin path = %s failed!\n", output);
            goto exit;
        }
    }

exit:
    free(data_cpu);
    free(data_tpu);
    return ret;
}

void* test_draw_line(void* args) {
    cv_dl_thread_arg_t* cv_dl_thread_arg = (cv_dl_thread_arg_t*)args;
    int loop = cv_dl_thread_arg->loop;
    int format = cv_dl_thread_arg->format;
    int height = cv_dl_thread_arg->height;
    int width = cv_dl_thread_arg->width;
    char* input_path = cv_dl_thread_arg->src_path;
    char* output_path = cv_dl_thread_arg->dst_path;
    bm_handle_t handle = cv_dl_thread_arg->handle;
    int i;
    int ret = 0;

    for (i = 0; i < loop; ++i) {
        ret = test_draw_line_case(handle, input_path, output_path, height, width, format);
        if (ret != 0) {
            printf("------Test Draw_lines Failed!------\n");
            exit(-1);
        }
        printf("------Test Draw_lines Successed!------\n");
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
    int width = 100 + rand() % 1900;
    int height = 100 + rand() % 2048;
    int format = rand() % 7;
    int loop = 1;
    int ret = 0;
    char* input_path = NULL;
    char* output_path = NULL;
    int i;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop height width format input_path output_path \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 1 1 512 512 0\n", args[0]);
        printf("%s 1 1 1080 1920 0 input.bin output.bin \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) height = atoi(args[3]);
    if (argc > 4) width = atoi(args[4]);
    if (argc > 5) format = atoi(args[5]);
    if (argc > 6) input_path = args[6];
    if (argc > 7) output_path = args[7];

    pthread_t pid[thread_num];
    cv_dl_thread_arg_t cv_dl_thread_arg[thread_num];
    for (i = 0; i < thread_num; ++i) {
        cv_dl_thread_arg[i].loop = loop;
        cv_dl_thread_arg[i].format = format;
        cv_dl_thread_arg[i].height = height;
        cv_dl_thread_arg[i].width = width;
        cv_dl_thread_arg[i].src_path = input_path;
        cv_dl_thread_arg[i].dst_path = output_path;
        cv_dl_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_draw_line, &cv_dl_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }

    for (i = 0; i < thread_num; ++i) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }

    bm_dev_free(handle);
    return ret;
}