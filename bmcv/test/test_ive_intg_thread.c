#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0

extern void bm_ive_read_bin(bm_image src, const char *input_name);
extern void bm_ive_write_bin(bm_image dst, const char *output_name);
extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);

typedef struct ive_intg_ctx_{
    int loop;
    int i;
} ive_intg_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int data_type = 0; // 0 u32 ; 1 u64
int height = 288, width = 352;
bm_image_format_ext src_fmt = FORMAT_GRAY;
ive_integ_out_ctrl_e integ_mode = 0;
char* src_name = "./data/00_352x288_y.yuv", *dst_name = "./hist_result.bin", *golden_sample_name = "./data/result/sample_Integ_Combine.yuv";
bm_handle_t handle = NULL;

void integCtrl_to_str(ive_integ_out_ctrl_e intg_ctrl, char* res){
    switch(intg_ctrl){
        case IVE_INTEG_MODE_COMBINE:
            strcpy(res, "IVE_INTEG_MODE_COMBINE");
            break;
        case IVE_INTEG_MODE_SUM:
            strcpy(res, "IVE_INTEG_MODE_SUM");
            break;
        case IVE_INTEG_MODE_SQSUM:
            strcpy(res, "IVE_INTEG_MODE_SQSUM");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

const int cmp_u32(char* ref_name, unsigned int* got, int len){
    FILE *ref_fp = fopen(ref_name, "rb");
    if(ref_fp == NULL){
        printf("%s : No such file \n", ref_name);
        return -1;
    }

    unsigned int* ref = malloc(len * sizeof(unsigned int));
    fread((void *)ref, sizeof(unsigned int), len, ref_fp);
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

const int cmp_u64(char* ref_name, unsigned long long* got, int len){
    FILE *ref_fp = fopen(ref_name, "rb");
    if(ref_fp == NULL){
        printf("%s : No such file \n", ref_name);
        return -1;
    }

    unsigned long long* ref = malloc(len * sizeof(unsigned long long));
    fread((void *)ref, sizeof(unsigned long long), len, ref_fp);
    fclose(ref_fp);

    for(int i = 0; i < len; i++){
        if(got[i] != ref[i]){
            printf("cmp error: idx=%d  ref=%lld  got=%lld\n", i, ref[i], got[i]);
            free(ref);
            return -1;
        }
    }
    free(ref);
    return 0;
}

static void * ive_intg(void* arg){
    bm_status_t ret;
    ive_intg_ctx ctx = *(ive_intg_ctx*)arg;
    bmcv_ive_integ_ctrl_s integ_attr;
    bm_image src;
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
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    bm_ive_read_bin(src, src_name);


    int data_size = (data_type == 0) ? sizeof(unsigned int) : sizeof(unsigned long long);
    ret = bm_malloc_device_byte(handle, &dst, height * width * data_size);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed. ret = %d\n", ret);
        exit(-1);
    }
    integ_attr.en_out_ctrl = integ_mode;

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_integ(handle, src, dst, integ_attr);
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
            printf("bmcv_image_ive_integ failed, ret is %d \n", ret);
            exit(-1);
        }
    }
    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        if(data_type == 0){
            unsigned int *dst_intg_u32 =  malloc(width * height * data_size);
            ret = bm_memcpy_d2s(handle, dst_intg_u32, dst);
            if(ret != BM_SUCCESS){
                printf("bm_memcpy_d2s failed. ret = %d\n", ret);
                free(dst_intg_u32);
                exit(-1);
            }
            int cmp = cmp_u32(golden_sample_name, dst_intg_u32, width * height);
            if(cmp != 0){
                printf("test ive intg u32 failed \n");
                free(dst_intg_u32);
                exit(-1);
            }
            printf("test ive intg u32 succed \n");

            if(bWrite){
                FILE *intg_result_fp = fopen(dst_name, "wb");
                fwrite((void *)dst_intg_u32, data_size, width * height, intg_result_fp);
                fclose(intg_result_fp);
            }
            free(dst_intg_u32);
        } else {
            unsigned long long *dst_intg_u64 = malloc(width * height * data_size);
            ret = bm_memcpy_d2s(handle, dst_intg_u64, dst);
            if(ret != BM_SUCCESS){
                printf("bm_memcpy_d2s failed. ret = %d\n", ret);
                free(dst_intg_u64);
                exit(-1);
            }

            int cmp = cmp_u64(golden_sample_name, dst_intg_u64, width * height);
            if(cmp != 0){
                printf("test ive intg u64 failed \n");
                free(dst_intg_u64);
                exit(-1);
            }
            printf("test ive intg u64 succed \n");

            if(bWrite){
                FILE *intg_result_fp = fopen(dst_name, "wb");
                fwrite((void *)cmp_u64, data_size, width * height, intg_result_fp);
                fclose(intg_result_fp);
            }
            free(dst_intg_u64);
        }
    }

    bm_image_destroy(&src);
    bm_free_device(handle, dst);

    char src_fmt_str[100], integ_mode_str[100], data_str[100];
    if(data_type == 0)
        strcpy(data_str, "u32");
    else
        strcpy(data_str, "u64");
    format_to_str(src.image_format, src_fmt_str);
    integCtrl_to_str(integ_mode, integ_mode_str);
    printf("bmcv_ive_intg: src_format %s, size %d x %d , data_type %s ,integ_mode %s \n",
            src_fmt_str, width, height, data_str, integ_mode_str);
    printf("idx:%d, bmcv_ive_intg: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("test ive intg test successful \n");
    return 0;
}

int main(int argc, char **argv){
    if (argc == 11) {
        test_threads_num = atoi(argv[9]);
        test_loop_times  = atoi(argv[10]);
    }
    if (argc >= 9) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        integ_mode = atoi(argv[3]);
        data_type = atoi(argv[4]);
        src_name = argv[5];
        dst_name = argv[6];
        golden_sample_name = argv[7];
        dev_id = atoi(argv[8]);
    }
    if (argc == 2)
        test_threads_num  = atoi(argv[1]);
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 8) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height integ_mode data_type src_name dst_name golden_sample_name dev_id thread_num loop_num\n \
        %s 352 288 2 1 ive_data/00_352x288_y.yuv integ_sqsum.bin ive_data/result/sample_Integ_Sqsum.yuv 0 1 1\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST Intg] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST Intg] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_intg_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_intg, (void *)(ctx + i))) {
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