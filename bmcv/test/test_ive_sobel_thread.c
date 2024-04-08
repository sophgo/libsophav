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

typedef struct ive_sobel_ctx_{
    int loop;
    int i;
} ive_sobel_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
int thrSize = 0; // 0 -> 3x3  1-> 5x5
bmcv_ive_sobel_out_mode enMode = BM_IVE_SOBEL_OUT_MODE_BOTH;
bm_image_format_ext fmt = FORMAT_GRAY;
char *src_name = "./data/00_352x288_y.yuv";
char *sobel_hName = "iveSobelH_res.yuv", *sobel_vName = "iveSobelV_res.yuv";
char *golden_hDstName = "./data/result/sample_Sobel_Hor3x3.yuv", *golden_vDstName = "./data/result/sample_Sobel_Ver3x3.yuv";
bm_handle_t handle = NULL;

/* 5 by 5*/
signed char arr5by5[25] = { -1, -2, 0,  2,  1, -4, -8, 0,  8,  4, -6, -12, 0,
                  12, 6,  -4, -8, 0, 8,  4,  -1, -2, 0, 2, 1 };
/* 3 by 3*/
signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                  2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };

const int cmp_s16(char* ref_name, signed short* got, int len){
    FILE *ref_fp = fopen(ref_name, "rb");
    if(ref_fp == NULL){
        printf("%s : No such file \n", ref_name);
        return -1;
    }

    signed short* ref = malloc(len * sizeof(signed short));
    fread((void *)ref, sizeof(signed short), len, ref_fp);
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

void sobelMode_to_str(bmcv_ive_sobel_out_mode enMode, char* res){
    switch(enMode){
        case BM_IVE_SOBEL_OUT_MODE_BOTH:
            strcpy(res, "BM_IVE_SOBEL_OUT_MODE_BOTH");
            break;
        case BM_IVE_SOBEL_OUT_MODE_HOR:
            strcpy(res, "BM_IVE_SOBEL_OUT_MODE_HOR");
            break;
        case BM_IVE_SOBEL_OUT_MODE_VER:
            strcpy(res, "BM_IVE_SOBEL_OUT_MODE_VER");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

static void * ive_sobel(void* arg){
    bm_status_t ret;
    ive_sobel_ctx ctx = *(ive_sobel_ctx*)arg;
    bm_image src;
    bm_image dst_H, dst_V;
    int src_stride[4];
    int dst_stride[4];
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
    bmcv_ive_sobel_ctrl sobelAtt;
    sobelAtt.sobel_mode = enMode;
    (thrSize == 0) ? (memcpy(sobelAtt.as8_mask, arr3by3, 5 * 5 * sizeof(signed char))) :
                     (memcpy(sobelAtt.as8_mask, arr5by5, 5 * 5 * sizeof(signed char)));

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    bm_ive_read_bin(src, src_name);

    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_S16, dst_stride);
    if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_HOR){
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_S16, &dst_H, dst_stride);
        ret = bm_image_alloc_dev_mem(dst_H, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("dst_H bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
    }

    if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_VER){
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_S16, &dst_V, dst_stride);
        ret = bm_image_alloc_dev_mem(dst_V, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("dst_V bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
    }

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_sobel(handle, &src, &dst_H, &dst_V, sobelAtt);
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
            printf("bmcv_ive_sobel failed, ret is %d \n", ret);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        signed short* iveSobel_h = malloc(width * height * sizeof(signed short));
        signed short* iveSobel_v = malloc(width * height * sizeof(signed short));
        memset(iveSobel_h, 0, width * height * sizeof(signed short));
        memset(iveSobel_v, 0, width * height * sizeof(signed short));

        if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_HOR){
            ret = bm_image_copy_device_to_host(dst_H, (void**)&iveSobel_h);
            if(ret != BM_SUCCESS){
                printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
                free(iveSobel_h);
                free(iveSobel_v);
                exit(-1);
            }

            int cmp = cmp_s16(golden_hDstName, iveSobel_h, width * height);
            if(cmp != 0){
                printf("bmcv ive sobelResult_h cmp failed \n");
                free(iveSobel_h);
                free(iveSobel_v);
                exit(-1);
            }
            printf("[bmcv ive sobel] sobelResult_h cmp succed \n");
        }

        if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_VER){
            ret = bm_image_copy_device_to_host(dst_V, (void**)&iveSobel_v);
            if(ret != BM_SUCCESS){
                printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
                free(iveSobel_h);
                free(iveSobel_v);
                exit(-1);
            }

            int cmp = cmp_s16(golden_vDstName, iveSobel_v, width * height);
            if(cmp != 0){
                printf("bmcv ive sobelResult_v cmp failed \n");
                free(iveSobel_h);
                free(iveSobel_v);
                exit(-1);
            }
            printf("[bmcv ive sobel] sobelResult_v cmp succed \n");
        }

        if(bWrite){
            if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_HOR){
                FILE *sobelH_fp = fopen(sobel_hName, "wb");
                fwrite((void *)iveSobel_h, sizeof(signed short), width * height, sobelH_fp);
                fclose(sobelH_fp);
            }

            if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_VER){
                FILE *sobelV_fp = fopen(sobel_vName, "wb");
                fwrite((void *)iveSobel_v, sizeof(signed short), width * height, sobelV_fp);
                fclose(sobelV_fp);
            }
        }

        free(iveSobel_h);
        free(iveSobel_v);
    }


    bm_image_destroy(&src);
    if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_HOR)
        bm_image_destroy(&dst_H);

    if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_VER)
        bm_image_destroy(&dst_V);

    char fmt_str[100], thr_str[50], modeStr[100];
    format_to_str(src.image_format, fmt_str);
    (thrSize == 0) ? memcpy(thr_str, "3x3", 50) : memcpy(thr_str, "5x5", 50);

    sobelMode_to_str(sobelAtt.sobel_mode, modeStr);

    printf("bmcv_ive_sobel: format %s, sobelOutMode is %s , thrSize is %s, size %d x %d \n",
            fmt_str, modeStr, thr_str, width, height);
    printf("idx:%d, bmcv_ive_sobel: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive sobel test successful \n");
    return 0;
}

int main(int argc, char **argv){
    if(argc >= 7 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        src_name = argv[3];
        thrSize = atoi(argv[4]);
        enMode = atoi(argv[5]);
        golden_hDstName = argv[6];
        if(argc > 7) golden_vDstName = argv[7];
        if(argc > 8) dev_id = atoi(argv[8]);
        if(argc > 9) test_threads_num  = atoi(argv[9]);
        if(argc > 10) test_loop_times  = atoi(argv[10]);
        if(argc > 11) bWrite = atoi(argv[11]);
        if(argc > 12) sobel_hName = argv[12];
        if(argc > 13) sobel_vName = argv[13];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if (argc > 3 && argc < 6) {
        printf("command input error, please follow this order:\n \
        %s width height src_name sobelSize(0:3x3 1:5x5) sobel_out_mode golden_hDstName golden_vDstName dev_id thread_num loop_num bWrite sobel_hName sobel_vName\n \
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

    ive_sobel_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_sobel, (void *)(ctx + i))) {
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