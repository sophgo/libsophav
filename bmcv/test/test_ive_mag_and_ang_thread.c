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

typedef struct ive_magAndAng_ctx_{
    int loop;
    int i;
} ive_magAndAng_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
int templateSize = 0; // 0: 3x3 1:5x5
unsigned short thrValue = 0;
bm_image_format_ext fmt = FORMAT_GRAY;
bmcv_ive_mag_and_ang_outctrl enMode = BM_IVE_MAG_AND_ANG_OUT_ALL;
char *src_name = "./data/00_352x288_y.yuv";
char *dst_magName = "ive_magAndAng_magResult.yuv", *dst_angName = "ive_magAndAng_angResult.yuv";
char *golden_Magsample_name = "./data/result/sample_MagAndAng_MagAndAng3x3_Mag.yuv";
char *golden_Angsample_name = "./data/result/sample_MagAndAng_MagAndAng3x3_Ang.yuv";
bm_handle_t handle = NULL;

/* 5 by 5*/
signed char arr5by5[25] = { -1, -2, 0,  2,  1, -4, -8, 0,  8,  4, -6, -12, 0,
                   12, 6,  -4, -8, 0, 8,  4,  -1, -2, 0, 2, 1 };
/* 3 by 3*/
signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                   2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };

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

void magAndAngMode_to_str(bmcv_ive_mag_and_ang_outctrl enMode, char* res){
    switch(enMode){
        case BM_IVE_MAG_AND_ANG_OUT_MAG:
            strcpy(res, "BM_IVE_MAG_AND_ANG_OUT_MAG");
            break;
        case BM_IVE_MAG_AND_ANG_OUT_ALL:
            strcpy(res, "BM_IVE_MAG_AND_ANG_OUT_ALL");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

static void * ive_magAndAng(void* arg){
    bm_status_t ret;
    ive_magAndAng_ctx ctx = *(ive_magAndAng_ctx*)arg;
    bm_image src;
    bm_image dst_mag, dst_ang;
    int stride[4];
    int magStride[4];
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
    bmcv_ive_mag_and_ang_ctrl magAndAng_attr;
    memset(&magAndAng_attr, 0, sizeof(bmcv_ive_mag_and_ang_ctrl));
    magAndAng_attr.en_out_ctrl = enMode;
    magAndAng_attr.u16_thr = thrValue;
    if(templateSize == 0)
        memcpy(magAndAng_attr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));
    else
        memcpy(magAndAng_attr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char));

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_U16, magStride);

    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &dst_mag, magStride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dst_mag, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("dst_mag bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    bm_ive_read_bin(src, src_name);

    if(enMode == BM_IVE_MAG_AND_ANG_OUT_ALL){
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst_ang, stride);
        ret = bm_image_alloc_dev_mem(dst_ang, BMCV_HEAP1_ID);
        if (ret != BM_SUCCESS) {
            printf("dst_mag bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
    }

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_mag_and_ang(handle, &src, &dst_mag, &dst_ang, magAndAng_attr);
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
            printf("bmcv_ive_magAndAng failed , ret is %d \n", ret);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        unsigned short* magOut_res = malloc(width * height * sizeof(unsigned short));
        unsigned char*  angOut_res = malloc(width * height * sizeof(unsigned char));
        memset(magOut_res, 0, width * height * sizeof(unsigned short));
        memset(angOut_res, 0, width * height * sizeof(unsigned char));

        ret = bm_image_copy_device_to_host(dst_mag, (void **)&magOut_res);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            free(magOut_res);
            free(angOut_res);
            exit(-1);
        }

        int cmp = cmp_u8(golden_Magsample_name, (unsigned char*)magOut_res,
                    width * height * sizeof(unsigned short));
        if(cmp != 0){
            printf("bmcv ive magAndAng test failed \n");
            free(magOut_res);
            free(angOut_res);
            exit(-1);
        }
        printf("[bmcv ive magAndAng] mag cmp succed \n");

        if(enMode == BM_IVE_MAG_AND_ANG_OUT_ALL){
            ret = bm_image_copy_device_to_host(dst_ang, (void **)&angOut_res);
            if(ret != BM_SUCCESS){
                printf("bm_image_copy_device_to_host failed. cmp = %d\n", cmp);
                free(magOut_res);
                free(angOut_res);
                exit(-1);
            }

            cmp = cmp_u8(golden_Angsample_name, angOut_res, width * height);
            if(cmp != 0){
                printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
                free(magOut_res);
                free(angOut_res);
                exit(-1);
            }
            printf("[bmcv ive magAndAng] ang cmp succed \n");
        }

        if(bWrite){
            FILE *mag_fp = fopen(dst_magName, "wb");
            fwrite((void *)magOut_res, 1, width * height, mag_fp);
            fclose(mag_fp);

            if(enMode == BM_IVE_MAG_AND_ANG_OUT_ALL){
                FILE *ang_fp = fopen(dst_angName, "wb");
                fwrite((void *)angOut_res, 1, width * height, ang_fp);
                fclose(ang_fp);
            }
        }
        free(magOut_res);
        free(angOut_res);
    }

    bm_image_destroy(&src);
    bm_image_destroy(&dst_mag);
    if(enMode == BM_IVE_MAG_AND_ANG_OUT_ALL) bm_image_destroy(&dst_ang);

    char fmt_str[100], thr_str[50], modeStr[100];
    format_to_str(src.image_format, fmt_str);
    if(templateSize == 0)
        memcpy(thr_str, "3x3", 4);
    else
        memcpy(thr_str, "5x5", 4);
    magAndAngMode_to_str(magAndAng_attr.en_out_ctrl, modeStr);

    printf("bmcv_ive_magAndAng: format %s, MagAndAng_OutMode is %s , templateSize is %s, thrValue is %d size %d x %d \n",
            fmt_str, modeStr, thr_str, thrValue, width, height);
    printf("idx:%d, bmcv_ive_magAndAng: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive magAndAng test successful \n");

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 8 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        src_name = argv[3];
        thrValue = atoi(argv[4]);
        templateSize = atoi(argv[5]);
        enMode = atoi(argv[6]);
        golden_Magsample_name = argv[7];
        if(argc > 8) golden_Angsample_name = argv[8];
        if(argc > 9) dev_id = atoi(argv[9]);
        if(argc > 10) test_threads_num  = atoi(argv[10]);
        if(argc > 11) test_loop_times  = atoi(argv[11]);
        if(argc > 12) bWrite = atoi(argv[12]);
        if(argc > 13) dst_magName = argv[13];
        if(argc > 14) dst_angName = argv[14];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 7) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height src_name thrValue templateSize(0:3x3 1:5x5) magAndAng_mode goldenMagsample_name goldenAngSample_name dev_id thread_num loop_num bWrite dstMag_name dsiAng_name\n \
        %s 640 480 ive_data/sky_640x480.yuv 0 0 1 ive_data/result/sample_tile_MagAndAng_MagAndAng3x3_Mag.yuv ive_data/result/sample_tile_MagAndAng_MagAndAng3x3_Ang.yuv 0 1 6 0\n", argv[0], argv[0]);
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

    ive_magAndAng_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_magAndAng, (void *)(ctx + i))) {
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