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

typedef struct ive_ncc_ctx_{
    int loop;
    int i;
} ive_ncc_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int height = 288, width = 352;
int bWrite = 0;
bm_image_format_ext src_fmt = FORMAT_GRAY;
char* src1_name = "./data/00_352x288_y.yuv", *src2_name = "./data/01_352x288_y.yuv";
char* dst_name = "ncc_result.bin", *golden_sample_name = "./data/result/sample_NCC_Mem.bin";
bm_handle_t handle = NULL;

const int _cmp(char* ref_name, unsigned char* got, int len){
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

static void * ive_ncc(void* arg){
    bm_status_t ret;
    ive_ncc_ctx ctx = *(ive_ncc_ctx*)arg;
    bm_image src1, src2;
    bm_device_mem_t dst;
    int src_stride[4];
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
    bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);

    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src1, src_stride);
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src2, src_stride);
    ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("src1 bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("src2 bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    bm_ive_read_bin(src1, src1_name);
    bm_ive_read_bin(src2, src2_name);

    int data_len = sizeof(bmcv_ive_ncc_dst_mem_t);

    ret = bm_malloc_device_byte(handle, &dst, data_len);
    if (ret != BM_SUCCESS) {
        printf("dst bm_malloc_device_byte failed. ret = %d\n", ret);
        exit(-1);
    }

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_ncc(handle, src1, src2, dst);
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
            printf("bmcv_image_ive_ncc failed, ret is %d \n", ret);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        unsigned long long *ncc_result = malloc(data_len);

        ret = bm_memcpy_d2s(handle, ncc_result, dst);
        if(ret != BM_SUCCESS){
            printf("bm_memcpy_d2s failed. ret = %d\n", ret);
                free(ncc_result);
                exit(-1);
        }
        int cmp = _cmp(golden_sample_name, (unsigned char*)ncc_result, data_len);
        if(cmp != 0){
            printf("bmcv ive NCC test failed \n");
            free(ncc_result);
            exit(-1);
        }

        unsigned long long *numerator = ncc_result;
        unsigned long long *quadSum1 = ncc_result + 1;
        unsigned long long *quadSum2 = quadSum1 + 1;
        float fr = (float)((double)*numerator / (sqrt((double)*quadSum1) * sqrt((double)*quadSum2)));
        printf("bmcv ive NCC value is %f \n", fr);

        if(bWrite){
            FILE *ncc_result_fp = fopen(dst_name, "wb");
            fwrite((void *)ncc_result, 1, data_len, ncc_result_fp);
            fclose(ncc_result_fp);
        }

        free(ncc_result);
    }

    char src_fmt_str[100];
    format_to_str(src1.image_format, src_fmt_str);

    printf("bmcv_ive_ncc: src_format %s, size %d x %d \n", src_fmt_str, width, height);
    printf("idx:%d, bmcv_ive_ncc: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive NCC test successful \n");

    bm_image_destroy(&src1);
    bm_image_destroy(&src2);
    bm_free_device(handle, dst);
    return 0;
}

int main(int argc, char **argv){
    if (argc == 10) {
        test_threads_num = atoi(argv[8]);
        test_loop_times  = atoi(argv[9]);
    }
    if (argc >= 8) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        src1_name = argv[3];
        src2_name = argv[4];
        dst_name = argv[5];
        golden_sample_name = argv[6];
        dev_id = atoi(argv[7]);
    }
    if (argc == 2)
        test_threads_num  = atoi(argv[1]);
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if (argc > 3 && argc < 7) {
        printf("command input error, please follow this order:\n \
        %s width height src1_name src2_name dst_name golden_sample_name dev_id thread_num loop_num\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
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

    ive_ncc_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_ncc, (void *)(ctx + i))) {
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