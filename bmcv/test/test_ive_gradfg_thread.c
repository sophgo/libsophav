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

typedef struct ive_gradFg_ctx_{
    int loop;
    int i;
} ive_gradFg_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
bm_image_format_ext fmt = FORMAT_GRAY;
bmcv_ive_gradfg_mode gradfg_mode = GRAD_FG_MODE_USE_CUR_GRAD;
char *src_name = "./data/00_352x288_y.yuv";
char *dst_name = "./grad_fg_res.yuv";
char *ref_name = "./data/result/sample_GradFg_USE_CUR_GRAD.out";
bm_handle_t handle = NULL;

/* 3 by 3*/
signed char arr3by3[25] = { 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0, -2, 0,
                2, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0 };

void gradFgMode_to_str(bmcv_ive_gradfg_mode enMode, char* res){
    switch(enMode){
        case GRAD_FG_MODE_USE_CUR_GRAD:
            strcpy(res, "GRAD_FG_MODE_USE_CUR_GRAD");
            break;
        case GRAD_FG_MODE_FIND_MIN_GRAD:
            strcpy(res, "GRAD_FG_MODE_FIND_MIN_GRAD");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

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

static void * ive_gradfg(void* arg){
    bm_status_t ret;
    ive_gradFg_ctx ctx = *(ive_gradFg_ctx*)arg;
    bm_image stBgdiffFg;
    bm_image stCurGrad, stBgGrad, stGragFg;
    int u8Stride[4], u16Stride[4];

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

    // config setting

    // normGrad config
    bmcv_ive_normgrad_ctrl normGradAttr;
    normGradAttr.en_mode = BM_IVE_NORM_GRAD_OUT_COMBINE;
    normGradAttr.u8_norm = 8;
    memcpy(&normGradAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

    bmcv_ive_gradfg_attr gradFgAttr;
    gradFgAttr.en_mode = gradfg_mode;
    gradFgAttr.u16_edw_factor = 1000;
    gradFgAttr.u8_crl_coef_thr = 80;
    gradFgAttr.u8_mag_crl_thr = 4;
    gradFgAttr.u8_min_mag_diff = 2;
    gradFgAttr.u8_noise_val = 1;
    gradFgAttr.u8_edw_dark = 1;

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, u8Stride);
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_U16, u16Stride);

    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stBgdiffFg, u8Stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &stCurGrad, u16Stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &stBgGrad, u16Stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stGragFg, u8Stride);

    ret = bm_image_alloc_dev_mem(stBgdiffFg, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("stBgdiffFg bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        bm_image_destroy(&stBgdiffFg);
        bm_image_destroy(&stCurGrad);
        bm_image_destroy(&stBgGrad);
        bm_image_destroy(&stGragFg);
        exit(-1);
    }
    bm_ive_read_bin(stBgdiffFg, src_name);

    ret = bm_image_alloc_dev_mem(stCurGrad, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("stCurGrad bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        bm_image_destroy(&stBgdiffFg);
        bm_image_destroy(&stCurGrad);
        bm_image_destroy(&stBgGrad);
        bm_image_destroy(&stGragFg);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(stBgGrad, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("stBgGrad bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        bm_image_destroy(&stBgdiffFg);
        bm_image_destroy(&stCurGrad);
        bm_image_destroy(&stBgGrad);
        bm_image_destroy(&stGragFg);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(stGragFg, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("stGragFg bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        bm_image_destroy(&stBgdiffFg);
        bm_image_destroy(&stCurGrad);
        bm_image_destroy(&stBgGrad);
        bm_image_destroy(&stGragFg);
        exit(-1);
    }

    // Create Fake Data.
    ret = bmcv_ive_norm_grad(handle, &stBgdiffFg, NULL, NULL, &stCurGrad, normGradAttr);
    if(ret != BM_SUCCESS){
        printf("Create stCurGrad failed. ret = %d\n", ret);
        bm_image_destroy(&stBgdiffFg);
        bm_image_destroy(&stCurGrad);
        bm_image_destroy(&stBgGrad);
        bm_image_destroy(&stGragFg);
        exit(-1);
    }

    normGradAttr.u8_norm = 2;
    ret = bmcv_ive_norm_grad(handle, &stBgdiffFg, NULL, NULL, &stBgGrad, normGradAttr);
    if(ret != BM_SUCCESS){
        printf("Create stBgGrad failed. ret = %d\n", ret);
        bm_image_destroy(&stBgdiffFg);
        bm_image_destroy(&stCurGrad);
        bm_image_destroy(&stBgGrad);
        bm_image_destroy(&stGragFg);
        exit(-1);
    }

    for (i = 0; i < loop_time; i++) {
        // Run ive gradFg
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_gradfg(handle, stBgdiffFg, stCurGrad, stBgGrad, stGragFg, gradFgAttr);
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
            printf("bmcv_ive_gradfg failed. ret = %d\n", ret);
            bm_image_destroy(&stBgdiffFg);
            bm_image_destroy(&stCurGrad);
            bm_image_destroy(&stBgGrad);
            bm_image_destroy(&stGragFg);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        unsigned char *gradFg_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
        memset(gradFg_res, 0, width * height * sizeof(unsigned char));

        ret = bm_image_copy_device_to_host(stGragFg, (void**)&gradFg_res);
        if(ret != BM_SUCCESS){
            printf("stGradFg bm_image_copy_device_to_host failed. ret = %d\n", ret);
            free(gradFg_res);
            bm_image_destroy(&stBgdiffFg);
            bm_image_destroy(&stCurGrad);
            bm_image_destroy(&stBgGrad);
            bm_image_destroy(&stGragFg);
            exit(-1);
        }

        int cmp = cmp_u8(ref_name, gradFg_res, width * height * sizeof(unsigned char));
        if(cmp != 0){
            printf("gradFg_res compare failed. cmp = %d\n", cmp);
            free(gradFg_res);
            bm_image_destroy(&stBgdiffFg);
            bm_image_destroy(&stCurGrad);
            bm_image_destroy(&stBgGrad);
            bm_image_destroy(&stGragFg);
            exit(-1);
        }
        printf("gradFg_res compare succed.\n");

        if(bWrite){
            FILE *ive_fp = fopen(dst_name, "wb");
            fwrite((void *)gradFg_res, 1, width * height, ive_fp);
            fclose(ive_fp);
        }
        free(gradFg_res);
    }

    bm_image_destroy(&stBgdiffFg);
    bm_image_destroy(&stCurGrad);
    bm_image_destroy(&stBgGrad);
    bm_image_destroy(&stGragFg);

    char fmt_str[100], modeStr[100];
    format_to_str(stBgdiffFg.image_format, fmt_str);
    gradFgMode_to_str(gradfg_mode, modeStr);

    printf("bmcv_ive_gradFg: format %s, gradFgMode is %s , size %d x %d \n", fmt_str, modeStr, width, height);
    printf("idx:%d, bmcv_ive_gradFg: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive gradFg test successful \n");

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 6 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        gradfg_mode = atoi(argv[3]);
        src_name = argv[4];
        ref_name = argv[5];
        if(argc > 6) dev_id = atoi(argv[6]);
        if(argc > 7) test_threads_num  = atoi(argv[7]);
        if(argc > 8) test_loop_times  = atoi(argv[8]);
        if(argc > 9) bWrite = atoi(argv[9]);
        if(argc > 10) dst_name = argv[10];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 5) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height gradfg_mode src_name ref_name dev_id thread_num loop_num bWrite dst_name\n \
        %s 352 288 0 ive_data/00_352x288_y.yuv ive_data/result/sample_GradFg_USE_CUR_GRAD.yuv\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST gradFg] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST gradFg] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_gradFg_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_gradfg, (void *)(ctx + i))) {
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