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

typedef struct ive_canny_ctx_{
    int loop;
    int i;
} ive_canny_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int thrSize = 0; //0 -> 3x3; 1 -> 5x5
int height = 288, width = 352;
bm_image_format_ext fmt = FORMAT_GRAY;
char *src_name = "./ive_data/00_352x288_y.yuv";
char *dstEdge_name = "ive_edge_res.yuv";
char *goldenEdge_name = "./ive_data/result/sample_CannyEdge_3x3.yuv";
bm_handle_t handle = NULL;

signed char arr5by5[25] = { -1, -2, 0, 2, 1, -4, -8, 0, 8, 4, -6, -12, 0,
                   12, 6, -4, -8, 0, 8, 4, -1, -2, 0, 2, 1 };
signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                   2, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0 };

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

static void * ive_canny(void* arg){
    bm_status_t ret;
    ive_canny_ctx ctx = *(ive_canny_ctx*)arg;
    bm_image src;
    bm_device_mem_t canny_stmem;
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
    int stmem_len = width * height * 4 * (sizeof(unsigned short) + sizeof(unsigned char));

    unsigned char *edge_res = malloc(width * height * sizeof(unsigned char));
    memset(edge_res, 0, width * height * sizeof(unsigned char));

    bmcv_ive_canny_hys_edge_ctrl cannyHysEdgeAttr;
    memset(&cannyHysEdgeAttr, 0, sizeof(bmcv_ive_canny_hys_edge_ctrl));
    cannyHysEdgeAttr.u16_low_thr = (thrSize == 0) ? 42 : 108;
    cannyHysEdgeAttr.u16_high_thr = 3 * cannyHysEdgeAttr.u16_low_thr;
    (thrSize == 0) ? memcpy(cannyHysEdgeAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char)) :
                     memcpy(cannyHysEdgeAttr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char));

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);

        free(edge_res);
        bm_image_destroy(&src);
        bm_free_device(handle, cannyHysEdgeAttr.st_mem);
        exit(-1);
    }

    ret = bm_malloc_device_byte(handle, &canny_stmem, stmem_len);
    if (ret != BM_SUCCESS) {
        printf("cannyHysEdgeAttr_stmem bm_malloc_device_byte failed. ret = %d\n", ret);

        free(edge_res);
        bm_image_destroy(&src);
        bm_free_device(handle, cannyHysEdgeAttr.st_mem);
        exit(-1);
    }

    bm_ive_read_bin(src, src_name);
    cannyHysEdgeAttr.st_mem = canny_stmem;

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_canny(handle, src, bm_mem_from_system((void *)edge_res), cannyHysEdgeAttr);
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
            printf("bmcv_ive_canny failed. ret = %d\n", ret);

            free(edge_res);
            bm_image_destroy(&src);
            bm_free_device(handle, cannyHysEdgeAttr.st_mem);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        int cmp = cmp_u8(goldenEdge_name, (unsigned char*)edge_res, width * height);
        if(cmp != 0){
            printf("[bmcv ive canny] dstEdge cmp failed \n");

            free(edge_res);
            bm_image_destroy(&src);
            bm_free_device(handle, cannyHysEdgeAttr.st_mem);
            exit(-1);
        }

        printf("[bmcv ive canny] dstEdge cmp successful \n");

        if(bWrite){
            FILE *edge_fp = fopen(dstEdge_name, "wb");
            fwrite((void *)edge_res, 1, width * height, edge_fp);
            fclose(edge_fp);
        }
    }

    free(edge_res);
    bm_image_destroy(&src);
    bm_free_device(handle, cannyHysEdgeAttr.st_mem);

    char fmt_str[100], thrSize_str[100];
    format_to_str(src.image_format, fmt_str);
    if(thrSize == 0)
        memcpy(thrSize_str, "3x3", sizeof(100));
    else
        memcpy(thrSize_str, "5x5", sizeof(100));

    printf("bmcv_ive_canny: format %s, thrSize is %s ,size %d x %d \n",
            fmt_str, thrSize_str, width, height);
    printf("idx:%d, bmcv_ive_canny: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive canny test successful \n");

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 6 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        src_name = argv[3];
        thrSize = atoi(argv[4]);
        goldenEdge_name = argv[5];
        if(argc > 6) dev_id = atoi(argv[6]);
        if(argc > 7) test_threads_num  = atoi(argv[7]);
        if(argc > 8) test_loop_times  = atoi(argv[8]);
        if(argc > 9) bWrite = atoi(argv[9]);
        if(argc > 10) dstEdge_name = argv[10];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 5) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height src_name thrSize(0:3x3 1:5x5) goldenEdge_name dev_id thread_num loop_num bWrite dstEdgeName\n \
        %s 352 288 ive_data/00_352x288_y.yuv 0 ive_data/result/sample_CannyEdge_3x3.yuv\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive canny] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST ive canny] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_canny_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_canny, (void *)(ctx + i))) {
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