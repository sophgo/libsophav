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

typedef struct ive_normgrad_ctx_{
    int loop;
    int i;
} ive_normgrad_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
int thrSize = 0; // 0 -> 3x3  1-> 5x5
bmcv_ive_normgrad_outmode enMode = BM_IVE_NORM_GRAD_OUT_HOR_AND_VER;
bm_image_format_ext fmt = FORMAT_GRAY;
char *src_name = "./data/00_352x288_y.yuv";
char *dst_hName = "ive_normGradH_res.yuv", *dst_vName = "ive_normGradV_res.yuv";
char *dst_hvName = "ive_normGradHV_res.yuv";
char *golden_hDstName = "./data/result/sample_NormGrad_Hor3x3.yuv";
char *golden_vDstName = "./data/result/sample_NormGrad_Ver3x3.yuv";
char *golden_hvDstName = "./data/result/sample_NormGrad_Combine3x3.yuv";
bm_handle_t handle = NULL;

/* 5 by 5*/
signed char arr5by5[25] = { -1, -2, 0,  2,  1, -4, -8, 0,  8,  4, -6, -12, 0,
                   12, 6,  -4, -8, 0, 8,  4,  -1, -2, 0, 2, 1 };
/* 3 by 3*/
signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                   2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };

const int cmp_s8(char* ref_name, signed char* got, int len){
    FILE *ref_fp = fopen(ref_name, "rb");
    if(ref_fp == NULL){
        printf("%s : No such file \n", ref_name);
        return -1;
    }

    signed char* ref = malloc(len);
    fread((void *)ref, sizeof(signed char), len, ref_fp);
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

const int cmp_u16(char* ref_name, unsigned short* got, int len){
    FILE *ref_fp = fopen(ref_name, "rb");
    unsigned short* ref = malloc(len * sizeof(unsigned short));
    fread((void *)ref, sizeof(unsigned short), len, ref_fp);
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

void normGradMode_to_str(bmcv_ive_normgrad_outmode enMode, char* res){
    switch(enMode){
        case BM_IVE_NORM_GRAD_OUT_HOR_AND_VER:
            strcpy(res, "BM_IVE_NORM_GRAD_OUT_HOR_AND_VER");
            break;
        case BM_IVE_NORM_GRAD_OUT_HOR:
            strcpy(res, "BM_IVE_NORM_GRAD_OUT_HOR");
            break;
        case BM_IVE_NORM_GRAD_OUT_VER:
            strcpy(res, "BM_IVE_NORM_GRAD_OUT_VER");
            break;
        case BM_IVE_NORM_GRAD_OUT_COMBINE:
            strcpy(res, "BM_IVE_NORM_GRAD_OUT_COMBINE");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

static void * ive_normgrad(void* arg){
    bm_status_t ret;
    ive_normgrad_ctx ctx = *(ive_normgrad_ctx*)arg;
    bm_image src;
    bm_image dst_H, dst_V, dst_conbine_HV;
    int src_stride[4];
    int dst_stride[4], dst_combine_stride[4];
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
    bmcv_ive_normgrad_ctrl normgrad_attr;
    normgrad_attr.u8_norm = 8;
    normgrad_attr.en_mode = enMode;
    (thrSize == 0) ? (memcpy(normgrad_attr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char))) :
                     (memcpy(normgrad_attr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char)));

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    bm_ive_read_bin(src, src_name);

    if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_HOR){
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, dst_stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, &dst_H, dst_stride);
        ret = bm_image_alloc_dev_mem(dst_H, BMCV_HEAP1_ID);
        if (ret != BM_SUCCESS) {
            printf("dst_H bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
    }

    if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_VER){
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, dst_stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, &dst_V, dst_stride);
        ret = bm_image_alloc_dev_mem(dst_V, BMCV_HEAP1_ID);
        if (ret != BM_SUCCESS) {
            printf("dst_V bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
    }

    if(enMode == BM_IVE_NORM_GRAD_OUT_COMBINE){
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_U16, dst_combine_stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &dst_conbine_HV, dst_combine_stride);
        ret = bm_image_alloc_dev_mem(dst_conbine_HV, BMCV_HEAP1_ID);
        if (ret != BM_SUCCESS) {
            printf("dst_conbine_HV bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
    }

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_norm_grad(handle, &src, &dst_H, &dst_V, &dst_conbine_HV, normgrad_attr);
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
            printf("bmcv_ive_normgrad failed. ret = %d\n", ret);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        signed char* ive_h_res = malloc(width * height * sizeof(signed char));
        signed char* ive_v_res = malloc(width * height * sizeof(signed char));
        unsigned short* ive_combine_res = malloc(width * height * sizeof(unsigned short));

        memset(ive_h_res, 0, width * height * sizeof(signed char));
        memset(ive_v_res, 0, width * height * sizeof(signed char));
        memset(ive_combine_res, 1, width * height * sizeof(unsigned short));

        if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_HOR){
            ret = bm_image_copy_device_to_host(dst_H, (void**)&ive_h_res);
            if(ret != BM_SUCCESS){
                printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
                free(ive_h_res);
                free(ive_v_res);
                free(ive_combine_res);
                exit(-1);
            }

            int cmp = cmp_s8(golden_hDstName, ive_h_res, width * height * sizeof(signed char));
            if(cmp != 0){
                printf("bmcv ive ive_h_res cmp failed \n");
                free(ive_h_res);
                free(ive_v_res);
                free(ive_combine_res);
                exit(-1);
            }
            printf("[bmcv ive normgrad] ive_h_res cmp succed \n");
        }

        if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_VER){
            ret = bm_image_copy_device_to_host(dst_V, (void**)&ive_v_res);
            if(ret != BM_SUCCESS){
                printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
                free(ive_h_res);
                free(ive_v_res);
                free(ive_combine_res);
                exit(-1);
            }

            int cmp = cmp_s8(golden_vDstName, ive_v_res, width * height * sizeof(signed char));
            if(cmp != 0){
                printf("bmcv ive ive_v_res cmp failed \n");
                free(ive_h_res);
                free(ive_v_res);
                free(ive_combine_res);
                exit(-1);
            }
            printf("[bmcv ive normgrad] ive_v_res cmp succed \n");
        }

        if(enMode == BM_IVE_NORM_GRAD_OUT_COMBINE){
            ret = bm_image_copy_device_to_host(dst_conbine_HV, (void**)&ive_combine_res);
            if(ret != BM_SUCCESS){
                printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
                free(ive_h_res);
                free(ive_v_res);
                free(ive_combine_res);
                exit(-1);
            }

            int cmp = cmp_u16(golden_hvDstName, ive_combine_res, width * height);
            if(cmp != 0){
                printf("bmcv ive ive_combine_res cmp failed \n");
                free(ive_h_res);
                free(ive_v_res);
                free(ive_combine_res);
                exit(-1);
            }
            printf("[bmcv ive normgrad] ive_combine_res cmp succed \n");
        }

        if(bWrite){
            if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_HOR){
                FILE *iveH_fp = fopen(dst_hName, "wb");
                fwrite((void *)ive_h_res, sizeof(signed char), width * height, iveH_fp);
                fclose(iveH_fp);
            }

            if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_VER){
                FILE *iveV_fp = fopen(dst_vName, "wb");
                fwrite((void *)ive_v_res, sizeof(signed char), width * height, iveV_fp);
                fclose(iveV_fp);
            }

            if(enMode == BM_IVE_NORM_GRAD_OUT_COMBINE){
                FILE *iveHV_fp = fopen(dst_hvName, "wb");
                fwrite((void *)ive_combine_res, sizeof(unsigned short), width * height, iveHV_fp);
                fclose(iveHV_fp);
            }
        }
        free(ive_h_res);
        free(ive_v_res);
        free(ive_combine_res);
    }

    bm_image_destroy(&src);
    if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_HOR)
        bm_image_destroy(&dst_H);
    if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_VER)
        bm_image_destroy(&dst_V);
    if(enMode == BM_IVE_NORM_GRAD_OUT_COMBINE) bm_image_destroy(&dst_conbine_HV);

    char fmt_str[100], thr_str[50], modeStr[100];
    format_to_str(src.image_format, fmt_str);
    (thrSize == 0) ? memcpy(thr_str, "3x3", 4) : memcpy(thr_str, "5x5", 4);

    normGradMode_to_str(normgrad_attr.en_mode, modeStr);

    printf("bmcv_ive_normgrad: format %s, normgradOutMode is %s , thrSize is %s, size %d x %d \n",
            fmt_str, modeStr, thr_str, width, height);
    printf("idx:%d, bmcv_ive_normgrad: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive normgrad test successful \n");

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
        if(argc > 8) golden_hvDstName = argv[8];
        if(argc > 9) dev_id = atoi(argv[9]);
        if(argc > 10) test_threads_num  = atoi(argv[10]);
        if(argc > 11) test_loop_times  = atoi(argv[11]);
        if(argc > 12) bWrite = atoi(argv[12]);
        if(argc > 13) dst_hName = argv[13];
        if(argc > 14) dst_vName = argv[14];
        if(argc > 15) dst_hvName = argv[15];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 6) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height src_name thrSize(0:3x3 1:5x5) normgardMode golden_hDstName golden_vDstName golden_hvDstName dev_id thread_num loop_num bWrite dst_hName dst_vName dst_hvName\n \
        %s 352 288 ive_data/00_352x288_y.yuv 1 2 null ive_data/result/sample_NormGrad_Ver5x5.yuv\n", argv[0], argv[0]);
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

    ive_normgrad_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_normgrad, (void *)(ctx + i))) {
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