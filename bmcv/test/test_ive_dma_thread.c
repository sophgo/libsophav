#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0
#define MAX_THREAD_NUM 20

extern void bm_read_bin(bm_image src, const char *input_name);
extern void bm_write_bin(bm_image dst, const char *output_name);

typedef struct ive_dma_ctx_{
    int loop;
    int i;
} ive_dma_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
bmcv_ive_dma_mode dma_mode = IVE_DMA_DIRECT_COPY; /* IVE_DMA_MODE_DIRECT_COPY */
unsigned long long val = 0;      /* used in memset mode */
unsigned char hor_seg_size = 0;  /* used in interval-copy mode */
unsigned char elem_size = 0;     /* used in interval-copy mode */
unsigned char ver_seg_rows = 0;  /* used in interval-copy mode */
bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
char *src_name = "./ive_data/00_352x288_y.yuv", *dst_name = "sample_DMA_Direct.bin";
char *ref_name = "./ive_data/result/sample_DMA_Direct.bin";
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

static void * ive_dma(void* arg) {
    bm_status_t ret;
    ive_dma_ctx ctx = *(ive_dma_ctx*)arg;
    bmcv_ive_interval_dma_attr dma_interval_attr;
    bm_image src, dst;
    int dst_h = 0, dst_w = 0;
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

    // create bm image struct
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    if (dma_mode == 0) {
        bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
    } else if (dma_mode == 1){
        memset(&dma_interval_attr, 0, sizeof(bmcv_ive_interval_dma_attr));
        dma_interval_attr.hor_seg_size = hor_seg_size;
        dma_interval_attr.elem_size = elem_size;
        dma_interval_attr.ver_seg_rows = ver_seg_rows;
        dst_h = height / ver_seg_rows;
        dst_w = (width / hor_seg_size) * elem_size;
        bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
    }

    // alloc bm image memory
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }

    // read image data from input files
    bm_read_bin(src, src_name);


    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        if (dma_mode == 0) {
            ret = bmcv_ive_dma(handle, src, dst, dma_mode, NULL);
        } else if (dma_mode == 1){
            ret = bmcv_ive_dma(handle, src, dst, dma_mode, &dma_interval_attr);
        }
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
            printf("bmcv_ive_dma failed \n");
            exit(-1);
        }
    }
    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = width * height * fps_actual/1024/1024;

    if(ctx.i == 0){
        unsigned char *ive_res = (unsigned char*) malloc (width * height * sizeof(unsigned char));
        memset(ive_res, 0, width * height * sizeof(unsigned char));
        ret = bm_image_copy_device_to_host(dst, (void**)&ive_res);
        if(ret != BM_SUCCESS){
            printf("dst bm_image_copy_device_to_host is failed \n");
            exit(-1);
        }

        int cmp = cmp_u8(ref_name, ive_res, width * height * sizeof(unsigned char));
        if(cmp != 0){
            printf("[bmcv ive map] cmp failed, cmp = %d \n", cmp);
            exit(-1);
        }
        printf("[bmcv ive map] cmp successful, cmp = %d \n", cmp);

        if(bWrite){
            FILE *fp = fopen(dst_name, "wb");
            fwrite((void *)ive_res, 1, width * height * sizeof(unsigned char), fp);
            fclose(fp);
        }
        free(ive_res);
    }

    bm_write_bin(dst, dst_name);
    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    bm_dev_free(handle);

    char algorithm_str[100] = "ive_dma";
    char src_fmt_str[100],dst_fmt_str[100];
    format_to_str(src.image_format, src_fmt_str);
    format_to_str(dst.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s,%s\n",ctx.i,width,height,width,height,src_fmt_str,dst_fmt_str,algorithm_str);
    printf("idx:%d, bm_ive_dma: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

int main(int argc, char **argv) {
    if(argc >= 11){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        dma_mode = atoi(argv[3]);
        hor_seg_size = atoi(argv[4]);
        elem_size = atoi(argv[5]);
        ver_seg_rows = atoi(argv[6]);
        src_fmt = (bm_image_format_ext)atoi(argv[7]);
        dst_fmt = (bm_image_format_ext)atoi(argv[8]);
        src_name = argv[9];
        ref_name = argv[10];
        if(argc > 11) dev_id = atoi(argv[11]);
        if(argc > 12) test_threads_num = atoi(argv[12]);
        if(argc > 13) test_loop_times  = atoi(argv[13]);
        if(argc > 14) bWrite = atoi(argv[14]);
        if(argc > 15) dst_name = argv[15];
    }
    if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    }
    if ((argc > 3 && argc < 10) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height dma_mode val hor_seg_size elem_size ver_seg_rows src_fmt dst_fmt src_name ref_name dev_id thread_num loop_num bWrite dst_name\n \
        %s 352 288 0 0 0 0 14 14 ive_data/00_352x288_y.yuv ive_data/result/sample_DMA_Direct.bin 0 1 5 0\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST convert] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > MAX_THREAD_NUM || test_threads_num < 1) {
        printf("[TEST convert] thread nums should be 1~%d\n", MAX_THREAD_NUM);
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_dma_ctx ctx[MAX_THREAD_NUM];
    #ifdef __linux__
        pthread_t pid[MAX_THREAD_NUM];
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_dma, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        }
        for (int i = 0; i < test_threads_num; i++) {
            ret = pthread_join(pid[i], NULL);
            if (ret != 0) {
                perror("Thread join failed\n");
                exit(-1);
            }
        }

        printf("--------ALL THREADS TEST OVER---------\n");
    #endif

    return 0;
}