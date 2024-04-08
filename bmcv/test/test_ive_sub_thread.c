#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0
#define MAX_THREAD_NUM 20

extern void bm_ive_read_bin(bm_image src, const char *input_name);
extern void bm_ive_write_bin(bm_image dst, const char *output_name);
extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);

typedef struct ive_sub_ctx_{
    int loop;
    int i;
} ive_sub_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
bmcv_ive_sub_mode sub_mode = IVE_SUB_ABS; /* IVE_SUB_MODE_ABS */
bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
char *src1_name = "./ive_data/00_352x288_y.yuv", *src2_name = "./ive_data/01_352x288_y.yuv";
char *dst_name = "ive_sample_Sub.yuv";
char *ref_name = "./ive_data/result/sample_Sub_Abs.yuv";
bm_handle_t handle = NULL;

const int cmp_u8(char* file_name, unsigned char* got, int len){
    FILE *ref_fp = fopen(file_name, "rb");
    if(ref_fp == NULL){
        printf("%s : No such file \n", file_name);
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

static void * ive_sub(void* arg) {
    bm_status_t ret;
    ive_sub_ctx ctx = *(ive_sub_ctx*)arg;
    bmcv_ive_sub_attr sub_attr;
    bm_image src1, src2, dst;
    int src_stride[4];
    int dst_stride[4];
    unsigned int i = 0, loop_time = 0;
    unsigned long long time_single, time_total = 0, time_avg = 0;
    unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
#if SLEEP_ON
    int fps = 60;
    int sleep_time = 1000000 / fps;
#endif

    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval timediff;

    loop_time = ctx.loop;

    // set ive sub params
    memset(&sub_attr, 0, sizeof(bmcv_ive_sub_attr));
    sub_attr.en_mode = sub_mode;

    // calc ive image stride
    bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
    bm_ive_image_calc_stride(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, dst_stride);

    // create bm image struct
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src1, src_stride);
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src2, src_stride);
    bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

    // alloc bm image memory
    ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }

    // read image data from input files
    bm_ive_read_bin(src1, src1_name);
    bm_ive_read_bin(src2, src2_name);

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);

        ret = bmcv_ive_sub(handle, src1, src2, dst, sub_attr);

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
            printf("[bmcv_ive_sub] is failed \n");
            exit(-1);
        }
    }
    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = width * height * fps_actual/1024/1024;

    if(ctx.i == 0){
        unsigned char* ive_sub_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
        memset(ive_sub_res, 0, width * height * sizeof(unsigned char));

        ret = bm_image_copy_device_to_host(dst, (void **)&ive_sub_res);
        if(ret != BM_SUCCESS){
            printf("dst bm_image_copy_device_to_host failed, ret is %d \n", ret);
            exit(-1);
        }

        int cmp = cmp_u8(ref_name, ive_sub_res, width * height * sizeof(unsigned char));
        if(cmp != 0){
            printf("[bmcv ive sub] cmp failed, cmp = %d \n", cmp);
            exit(-1);
        }
        printf("[bmcv ive sub] cmp successful \n");

        if(bWrite){
            FILE *fp = fopen(dst_name, "wb");
            fwrite((void *)ive_sub_res, 1, width * height * sizeof(unsigned char), fp);
            fclose(fp);
        }
        free(ive_sub_res);

    }

    bm_image_destroy(&src1);
    bm_image_destroy(&src2);
    bm_image_destroy(&dst);

    char algorithm_str[100] = "ive_sub";
    char src_fmt_str[100],dst_fmt_str[100];
    format_to_str(src1.image_format, src_fmt_str);
    format_to_str(dst.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s,%s\n",ctx.i,width,height,width,height,src_fmt_str,dst_fmt_str,algorithm_str);
    printf("idx:%d, bm_ive_sub: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

int main(int argc, char **argv) {
    if(argc >= 9){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        sub_mode = atoi(argv[3]);
        src_fmt = (bm_image_format_ext)atoi(argv[4]);
        dst_fmt = (bm_image_format_ext)atoi(argv[5]);
        src1_name = argv[6];
        src2_name = argv[7];
        ref_name = argv[8];
        if(argc > 9) dev_id = atoi(argv[9]);
        if(argc > 10) test_threads_num = atoi(argv[10]);
        if(argc > 11) test_loop_times  = atoi(argv[11]);
        if(argc > 12) bWrite = atoi(argv[12]);
        if(argc > 13) dst_name = argv[13];
    }
    if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    }
    if (argc > 3 && argc < 8) {
        printf("command input error, please follow this order:\n \
        %s width height sub_mode src_fmt dst_fmt src1_name src2_name dev_id thread_num loop_num bWrite dst_name\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive sub] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > MAX_THREAD_NUM || test_threads_num < 1) {
        printf("[TEST ive sub] thread nums should be 1~%d\n", MAX_THREAD_NUM);
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_sub_ctx ctx[MAX_THREAD_NUM];
    #ifdef __linux__
        pthread_t pid[MAX_THREAD_NUM];
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_sub, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        }
        for (int i = 0; i < test_threads_num; i++) {
            ret = pthread_join(pid[i], NULL);
            if (ret != 0) {
                perror("Thread join failed");
                exit(-1);
            }
        }
        bm_dev_free(handle);
        printf("--------ALL THREADS TEST OVER---------\n");
    #endif

    return 0;
}