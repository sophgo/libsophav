#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0

extern void bm_ive_read_bin(bm_image src, const char *input_name);
extern void bm_ive_write_bin(bm_image dst, const char *output_name);
extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);

typedef struct ive_gmm_ctx_{
    int loop;
    int i;
} ive_gmm_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 704;
bm_image_format_ext src_fmt = FORMAT_GRAY;
char *input_name = "./data/campus.u8c1.1_100.raw";
char *dstFg_name = "gmm_fg_res.yuv", *dstBg_name = "gmm_bg_res.yuv";
char *goldenFg_name = "./data/result/sample_tile_GMM_U8C1_fg_31.yuv", *goldenBg_name = "./data/result/sample_tile_GMM_U8C1_bg_31.yuv";
bm_handle_t handle = NULL;

const int cmp_u8(char* ref_name, unsigned char* got, int len){
    FILE *ref_fp = fopen(ref_name, "rb");
    if(ref_fp == NULL){
        printf("%s : No such file \n", ref_name);
        return -1;
    }

    unsigned char* ref = malloc(len);
    fread((void *)ref, 1, len, ref_fp);
    fclose(ref_fp);

    for(int i = 0; i < len; i++){
        if(got[i] != ref[i]){
            printf("cmp error: idx=%d  ref=%d  got=%d\n", i, ref[i], got[i]);
            free(ref);
            return -1;
        }
    }
    free(ref);
    return 0;
}

static void * ive_gmm(void* arg){
    bm_status_t ret;
    ive_gmm_ctx ctx = *(ive_gmm_ctx*)arg;
    bm_image src;
    bm_image dst_fg, dst_bg;
    bm_device_mem_t dst_model;
    int stride[4];
    unsigned int i = 0, loop_time = 0;
    unsigned long long time_single, time_total = 0, time_avg = 0;
    unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
#if SLEEP_ON
    int fps = 60;
    int sleep_time = 1000000 / fps;
#endif

    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval timediff;

    loop_time = ctx.loop;

    unsigned int u32FrameNumMax = 32;
    unsigned int u32FrmCnt = 0;

    bmcv_ive_gmm_ctrl gmmAttr;
    gmmAttr.u0q16_bg_ratio = 45875;
    gmmAttr.u0q16_init_weight = 3277;
    gmmAttr.u22q10_noise_var = 225 * 1024;
    gmmAttr.u22q10_max_var = 2000 * 1024;
    gmmAttr.u22q10_min_var = 200 * 1024;
    gmmAttr.u8q8_var_thr = (unsigned short)(256 * 6.25);
    gmmAttr.u8_model_num = 3;

    int model_len = width * height * gmmAttr.u8_model_num * 8;

    unsigned char* inputData = malloc(width * height * u32FrameNumMax * sizeof(unsigned char));
    FILE *input_fp = fopen(input_name, "rb");
    fread((void *)inputData, 1, width * height * u32FrameNumMax * sizeof(unsigned char), input_fp);
    fclose(input_fp);

    unsigned char* srcData = malloc(width * height * sizeof(unsigned char));
    unsigned char* ive_fg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char* ive_bg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char* model_data = malloc(model_len * sizeof(unsigned char));

    memset(srcData, 0, width * height * sizeof(unsigned char));
    memset(ive_fg_res, 0, width * height * sizeof(unsigned char));
    memset(ive_bg_res, 0, width * height * sizeof(unsigned char));
    memset(model_data, 0, model_len * sizeof(unsigned char));

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, stride);

    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_fg, stride);
    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_bg, stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        free(inputData);
        free(ive_fg_res);
        free(srcData);
        free(ive_bg_res);
        free(model_data);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dst_fg, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        free(inputData);
        free(ive_fg_res);
        free(srcData);
        free(ive_bg_res);
        free(model_data);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dst_bg, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        free(inputData);
        free(ive_fg_res);
        free(srcData);
        free(ive_bg_res);
        free(model_data);
        exit(-1);
    }

    ret = bm_malloc_device_byte(handle, &dst_model, model_len);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed. ret = %d\n", ret);
        free(inputData);
        free(ive_fg_res);
        free(srcData);
        free(ive_bg_res);
        free(model_data);
        exit(-1);
    }

    for (i = 0; i < loop_time; i++) {
        ret = bm_memcpy_s2d(handle, dst_model, model_data);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d failed. ret = %d\n", ret);
            free(inputData);
            free(ive_fg_res);
            free(srcData);
            free(ive_bg_res);
            free(model_data);
            exit(-1);
        }
        for(u32FrmCnt = 0; u32FrmCnt < u32FrameNumMax; u32FrmCnt++){
            if(width > 480 ){
                for(int j = 0; j < 288; j++){
                    memcpy(srcData + (j * width),
                           inputData + (u32FrmCnt * 352 * 288 + j * 352), 352);
                    memcpy(srcData + (j * width + 352),
                           inputData + (u32FrmCnt * 352 * 288 + j * 352), 352);
                }
            } else {
                for(int j = 0; j < 288; j++){
                    memcpy(srcData + j * stride[0],
                           inputData + u32FrmCnt * width * 288 + j * width, width);
                    int s = stride[0] - width;
                    memset(srcData + j * stride[0] + width, 0, s);
                }
            }

            ret = bm_image_copy_host_to_device(src, (void**)&srcData);
            if(ret != BM_SUCCESS){
                printf("bm_image copy src h2d failed. ret = %d \n", ret);
                free(inputData);
                free(ive_fg_res);
                free(srcData);
                free(ive_bg_res);
                free(model_data);
                exit(-1);
            }

            if(u32FrmCnt >= 500)
                gmmAttr.u0q16_learn_rate = 131;  //0.02
            else
                gmmAttr.u0q16_learn_rate = 65535 / (u32FrmCnt + 1);

            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_gmm(handle, src, dst_fg, dst_bg, dst_model, gmmAttr);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
#if SLEEP_ON
            if(time_single < sleep_time)
                usleep((sleep_time - time_single));
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
#endif
            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;
            if(ret != BM_SUCCESS){
                printf("bmcv_ive_gmm execution failed \n");
                free(inputData);
                free(ive_fg_res);
                free(srcData);
                free(ive_bg_res);
                free(model_data);
                exit(-1);
            }
        }
    }

    time_avg = time_total / (loop_time * u32FrameNumMax);
    fps_actual = 1000000 / (time_avg * u32FrameNumMax);
    if(ctx.i == 0){
        ret = bm_image_copy_device_to_host(dst_fg, (void **)&ive_fg_res);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            free(inputData);
            free(ive_fg_res);
            free(srcData);
            free(ive_bg_res);
            free(model_data);
            exit(-1);
        }

        ret = bm_image_copy_device_to_host(dst_bg, (void **)&ive_bg_res);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            free(inputData);
            free(ive_fg_res);
            free(srcData);
            free(ive_bg_res);
            free(model_data);
            exit(-1);
        }

        int cmp = cmp_u8(goldenFg_name, ive_fg_res, width * height);
        if(cmp != 0){
            printf("[bmcv ive gmm] dstFg cmp failed \n");
            free(inputData);
            free(ive_fg_res);
            free(srcData);
            free(ive_bg_res);
            free(model_data);
            exit(-1);
        }

        printf("[bmcv ive gmm] dstFg cmp succed \n");

        cmp = cmp_u8(goldenBg_name, ive_bg_res, width * height);
        if(cmp != 0){
            printf("[bmcv ive gmm] dstBg cmp failed \n");
            free(inputData);
            free(ive_fg_res);
            free(srcData);
            free(ive_bg_res);
            free(model_data);
            exit(-1);
        }
        printf("[bmcv ive gmm] dstBg cmp succed \n");

        if(bWrite){
            FILE *fg_fp = fopen(dstFg_name, "wb");
            fwrite((void *)ive_fg_res, 1, width * height, fg_fp);
            fclose(fg_fp);

            FILE *bg_fp = fopen(dstBg_name, "wb");
            fwrite((void *)ive_bg_res, 1, width * height, bg_fp);
            fclose(bg_fp);
        }
    }

    free(inputData);
    free(ive_fg_res);
    free(srcData);
    free(ive_bg_res);
    free(model_data);
    bm_image_destroy(&src);
    bm_image_destroy(&dst_fg);
    bm_image_destroy(&dst_bg);
    bm_free_device(handle, dst_model);

    char fmt_str[100];
    format_to_str(src.image_format, fmt_str);

    printf("bmcv_ive_gmm: format %s, u32FrameNumMax is %d, size %d x %d \n",
            fmt_str, u32FrameNumMax, width, height);
    printf("idx:%d, bmcv_ive_gmm: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive gmm test successful \n");

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 6 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        src_fmt = atoi(argv[3]);
        input_name = argv[4];
        goldenFg_name = argv[5];
        goldenBg_name = argv[6];
        if(argc > 7) dev_id = atoi(argv[7]);
        if(argc > 8) test_threads_num  = atoi(argv[8]);
        if(argc > 9) test_loop_times  = atoi(argv[9]);
        if(argc > 10) bWrite = atoi(argv[10]);
        if(argc > 11) dstFg_name = argv[11];
        if(argc > 12) dstBg_name = argv[12];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 5) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height src_fmt src_name goldenFg_name goldenBg_name dev_id thread_num loop_num bWrite dstFg_name dstFg_name\n \
        %s 352 288 14 ive_data/campus.u8c1.1_100.raw ive_data/result/sample_GMM_U8C1_fg_31.yuv ive_data/result/sample_GMM_U8C1_bg_31.yuv\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive gmm] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST ive gmm] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_gmm_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_gmm, (void *)(ctx + i))) {
                free(pid);
                perror("create thread failed\n");
                exit(-1);
            }
        }
        for (int i = 0; i < test_threads_num; i++) {
            ret = pthread_join(pid[i], NULL);
            if (ret != 0) {
                free(pid);
                perror("Thread join failed");
                exit(-1);
            }
        }
        bm_dev_free(handle);
        printf("--------ALL THREADS TEST OVER---------\n");
        free(pid);
    #endif
    return 0;
}