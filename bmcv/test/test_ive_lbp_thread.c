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

typedef struct ive_lbp_ctx_{
    int loop;
    int i;
} ive_lbp_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
bm_image_format_ext fmt = FORMAT_GRAY;
bmcv_lbp_cmp_mode lbpCmpMode = BM_IVE_LBP_CMP_MODE_NORMAL;
char *src_name = "./data/00_352x288_y.yuv", *dst_name = "ive_lbp_result";
char *golden_sample_name = "./data/result/sample_LBP_Normal.yuv";
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

void lbpCmpMode_to_str(bmcv_lbp_cmp_mode enMode, char* res){
    switch(enMode){
        case BM_IVE_LBP_CMP_MODE_NORMAL:
            strcpy(res, "BM_IVE_LBP_CMP_MODE_NORMAL");
            break;
        case BM_IVE_LBP_CMP_MODE_ABS:
            strcpy(res, "BM_IVE_LBP_CMP_MODE_ABS");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

static void * ive_lbp(void* arg){
    bm_status_t ret;
    ive_lbp_ctx ctx = *(ive_lbp_ctx*)arg;
    bm_image src, dst;
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

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);

    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("dst bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    bm_ive_read_bin(src, src_name);

    bmcv_ive_lbp_ctrl_attr lbp_ctrl;
    lbp_ctrl.en_mode = lbpCmpMode;
    lbp_ctrl.un8_bit_thr.s8_val = (lbpCmpMode == BM_IVE_LBP_CMP_MODE_ABS ? 35 : 41);

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_lbp(handle, src, dst, lbp_ctrl);
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
    }
    if(ret != BM_SUCCESS){
        printf("idx is %d ,bmcv_image_ive_lbp failed . \n", ctx.i);
        exit(-1);
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        unsigned char* ive_lbp_res = malloc(width * height * sizeof(unsigned char));
        memset(ive_lbp_res, 0, width * height * sizeof(unsigned char));

        ret = bm_image_copy_device_to_host(dst, (void **)&ive_lbp_res);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            free(ive_lbp_res);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }

        int cmp = cmp_u8(golden_sample_name, ive_lbp_res, width * height);
        if(cmp != 0){
            printf("bmcv ive lbp test faile \n");

            free(ive_lbp_res);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }
        printf("bmcv ive lbp test successful \n");

        if(bWrite){
            FILE *ive_result_fp = fopen(dst_name, "wb");
            fwrite((void *)ive_lbp_res, 1, width * height, ive_result_fp);
            fclose(ive_result_fp);
        }
        free(ive_lbp_res);
    }

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    char fmt_str[100], mode_str[100];
    format_to_str(src.image_format, fmt_str);
    lbpCmpMode_to_str(lbpCmpMode, mode_str);

    printf("bmcv_ive_lbp: format %s, lbpCmpMopde is %s ,size %d x %d \n",
            fmt_str, mode_str, width, height);
    printf("idx:%d, bmcv_ive_lbp: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    return 0;
}

int main(int argc, char **argv){
    if (argc == 11) {
        dst_name = argv[10];
    }
    if (argc >= 10) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        lbpCmpMode = atoi(argv[3]);
        src_name = argv[4];
        golden_sample_name = argv[5];
        dev_id = atoi(argv[6]);
        test_threads_num = atoi(argv[7]);
        test_loop_times  = atoi(argv[8]);
        bWrite = atoi(argv[9]);
    }
    if (argc == 2)
        test_threads_num  = atoi(argv[1]);
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 9) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height lbpCmpMode src_name golden_sample_name dev_id thread_num loop_num bWrite dst_name\n \
        %s 352 288 1 ive_data/00_352x288_y.yuv ive_data/result/sample_LBP_Abs.yuv 0 1 1 0\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST convert] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST convert] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_lbp_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_lbp, (void *)(ctx + i))) {
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