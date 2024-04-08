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

typedef struct ive_gmm2_ctx_{
    int loop;
    int i;
} ive_gmm2_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
bool pixel_ctrl = false;
gmm2_life_update_factor_mode
        life_update_enMode = LIFE_UPDATE_FACTOR_MODE_GLB;
int height = 288, width = 704;
bm_image_format_ext src_fmt = FORMAT_GRAY;
char *input_name = "./data/campus.u8c1.1_100.raw";
char *inputFactor_name = "./data/sample_GMM2_U8C1_PixelCtrl_Factor.raw";
char *dstFg_name = "gmm2_fg_res.yuv", *dstBg_name = "gmm2_bg_res.yuv";
char *dstPcMatch_name = "gmm2_pcMatch_res.yuv";

char *goldenFg_name = "./data/result/sample_tile_GMM2_U8C1_fg_31.yuv";
char *goldenBg_name = "./data/result/sample_tile_GMM2_U8C1_bg_31.yuv";
char *goldenPcMatch_name = "./data/result/sample_GMM2_U8C1_PixelCtrl_match_31.yuv";
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

void snsFactorMode_to_str(gmm2_sns_factor_mode enMode, char* res){
    switch(enMode){
        case SNS_FACTOR_MODE_GLB:
            strcpy(res, "BM_IVE_GMM2_SNS_FACTOR_MODE_GLB");
            break;
        case SNS_FACTOR_MODE_PIX:
            strcpy(res, "BM_IVE_GMM2_SNS_FACTOR_MODE_PIX");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

void lifeUpdateFactorMode_to_str(gmm2_life_update_factor_mode enMode, char* res)
{
    switch(enMode){
        case LIFE_UPDATE_FACTOR_MODE_GLB:
            strcpy(res, "BM_IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_GLB");
            break;
        case LIFE_UPDATE_FACTOR_MODE_PIX:
            strcpy(res, "BM_IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_PIX");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

static void * ive_gmm2(void* arg){
    bm_status_t ret;
    ive_gmm2_ctx ctx = *(ive_gmm2_ctx*)arg;
    bm_image src, src_factor;
    bm_image dst_fg, dst_bg, dst_model_match_model_info;
    bm_device_mem_t dst_model;
    int stride[4], factorStride[4];
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

    unsigned int u32FrameNumMax = 32;
    unsigned int u32FrmCnt = 0;
    unsigned int u32FrmNum = 0;

    bmcv_ive_gmm2_ctrl gmm2Attr;
    gmm2Attr.u16_var_rate = 1;
    gmm2Attr.u8_model_num = 3;
    gmm2Attr.u9q7_max_var = (16 * 16) << 7;
    gmm2Attr.u9q7_min_var = (8 * 8) << 7;
    gmm2Attr.u8_glb_sns_factor = 8;
    gmm2Attr.en_sns_factor_mode =
            (pixel_ctrl) ? SNS_FACTOR_MODE_PIX : SNS_FACTOR_MODE_GLB;
    gmm2Attr.u16_freq_thr = 12000;
    gmm2Attr.u16_freq_init_val = 20000;
    gmm2Attr.u16_freq_add_factor = 0xEF;
    gmm2Attr.u16_freq_redu_factor = 0xFF00;
    gmm2Attr.u16_life_thr = 5000;
    gmm2Attr.en_life_update_factor_mode = life_update_enMode;

    unsigned char* inputData = malloc(width * height * u32FrameNumMax * sizeof(unsigned char));
    FILE *input_fp = fopen(input_name, "rb");
    fread((void *)inputData, sizeof(unsigned char), width * height * u32FrameNumMax, input_fp);
    fclose(input_fp);

    unsigned char* srcData = malloc(width * height * sizeof(unsigned char));
    unsigned short* srcFactorData = malloc(width * height * sizeof(unsigned short));
    memset(srcData, 0, width * height * sizeof(unsigned char));
    memset(srcFactorData, 0, width * height * sizeof(unsigned short));

    if(pixel_ctrl){
        FILE *input_factor_fp = fopen(inputFactor_name, "rb");
        fread((void *)srcFactorData, sizeof(unsigned short), width * height, input_factor_fp);
        fclose(input_factor_fp);
    }

    int model_len = width * height * gmm2Attr.u8_model_num * 8;
    unsigned char* model_data = malloc(model_len * sizeof(unsigned char));
    memset(model_data, 0, model_len * sizeof(unsigned char));

    unsigned char* ive_fg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char* ive_bg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char* ive_pc_match_res = malloc(width * height * sizeof(unsigned char));

    memset(ive_fg_res, 0, width * height * sizeof(unsigned char));
    memset(ive_bg_res, 0, width * height * sizeof(unsigned char));
    memset(ive_pc_match_res, 0, width * height * sizeof(unsigned char));

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, stride);

    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        goto fail;
    }

    bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_U16, factorStride);
    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_U16, &src_factor, factorStride);
    ret = bm_image_alloc_dev_mem(src_factor, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        goto fail;
    }
    ret = bm_image_copy_host_to_device(src_factor, (void **)&srcFactorData);
    if (ret != BM_SUCCESS) {
        printf("bm_image_copy_host_to_device. ret = %d\n", ret);
        goto fail;
    }

    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_fg, stride);
    ret = bm_image_alloc_dev_mem(dst_fg, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        goto fail;
    }

    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_bg, stride);
    ret = bm_image_alloc_dev_mem(dst_bg, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        goto fail;
    }

    bm_image_create(handle, height, width, FORMAT_GRAY,
                         DATA_TYPE_EXT_1N_BYTE, &dst_model_match_model_info, stride);
    ret = bm_image_alloc_dev_mem(dst_model_match_model_info, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        goto fail;
    }

    ret = bm_malloc_device_byte(handle, &dst_model, model_len);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed. ret = %d\n", ret);
        goto fail;
    }

    for (i = 0; i < loop_time; i++) {
        ret = bm_memcpy_s2d(handle, dst_model, model_data);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d failed. ret = %d\n", ret);
            goto fail;
        }

        for(u32FrmCnt = 0; u32FrmCnt < u32FrameNumMax; u32FrmCnt++){
            if(width > 480){
                for(int i = 0; i < 288; i++){
                    memcpy(srcData + (i * width),
                           inputData + (u32FrmCnt * 352 * 288 + i * 352), 352);
                    memcpy(srcData + (i * width + 352),
                           inputData + (u32FrmCnt * 352 * 288 + i * 352), 352);

                }
            } else {
                for(int i = 0; i < 288; i++){
                    memcpy(srcData + i * stride[0],
                           inputData + u32FrmCnt * width * 288 + i * width, width);
                    int s = stride[0] - width;
                    memset(srcData + i * stride[0] + width, 0, s);
                }
            }

            ret = bm_image_copy_host_to_device(src, (void**)&srcData);
            if(ret != BM_SUCCESS){
                printf("bm_image copy src h2d failed. ret = %d \n", ret);
                goto fail;
            }

            u32FrmNum = u32FrmCnt + 1;
            if(gmm2Attr.u8_model_num == 1)
                gmm2Attr.u16_freq_redu_factor = (u32FrmNum >= 500) ? 0xFFA0 : 0xFC00;
            else
                gmm2Attr.u16_glb_life_update_factor =
                                       (u32FrmNum >= 500) ? 4 : 0xFFFF / u32FrmNum;

            if(pixel_ctrl && u32FrmNum > 16)
                gmm2Attr.en_life_update_factor_mode = LIFE_UPDATE_FACTOR_MODE_PIX;

            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_gmm2(handle, &src, &src_factor, &dst_fg,
                         &dst_bg, &dst_model_match_model_info, dst_model, gmm2Attr);
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
                printf("bmcv_ive_gmm2 execution failed \n");
                goto fail;
            }
        }
    }

    time_avg = time_total / (loop_time * u32FrameNumMax);
    fps_actual = 1000000 / (time_avg * u32FrameNumMax);

    if(ctx.i == 0){
        ret = bm_image_copy_device_to_host(dst_fg, (void **)&ive_fg_res);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            goto fail;
        }

        ret = bm_image_copy_device_to_host(dst_bg, (void **)&ive_bg_res);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            goto fail;
        }

        ret = bm_image_copy_device_to_host(dst_model_match_model_info,
                                           (void **)&ive_pc_match_res);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            goto fail;
        }

        int cmp = cmp_u8(goldenFg_name, ive_fg_res, width * height);
        if(cmp != 0){
            printf("[bmcv ive gmm2] dstFg cmp failed \n");
            goto fail;
        }
        printf("[bmcv ive gmm2] dstFg cmp succed \n");

        cmp = cmp_u8(goldenBg_name, ive_bg_res, width * height);
        if(cmp != 0){
            printf("[bmcv ive gmm2] dstBg cmp failed \n");
            goto fail;
        }
        printf("[bmcv ive gmm2] dstBg cmp succed \n");

        if(pixel_ctrl){
            cmp = cmp_u8(goldenPcMatch_name, ive_pc_match_res, width * height);
            if(cmp != 0){
                printf("[bmcv ive gmm2] pc_match cmp failed \n");
                goto fail;
            }
            printf("[bmcv ive gmm2] pc_match cmp succed \n");
        }

        if(bWrite){
            FILE *fg_fp = fopen(dstFg_name, "wb");
            fwrite((void *)ive_fg_res, 1, width * height, fg_fp);
            fclose(fg_fp);

            FILE *bg_fp = fopen(dstBg_name, "wb");
            fwrite((void *)ive_bg_res, 1, width * height, bg_fp);
            fclose(bg_fp);

            if(pixel_ctrl){
                FILE *pcMatch_fp = fopen(dstPcMatch_name, "wb");
                fwrite((void *)ive_pc_match_res, 1, width * height, pcMatch_fp);
                fclose(pcMatch_fp);
            }
        }
    }

    char fmt_str[100],sns_mode_str[100], life_update_str[100];
    format_to_str(src.image_format, fmt_str);
    snsFactorMode_to_str(gmm2Attr.en_sns_factor_mode, sns_mode_str);
    lifeUpdateFactorMode_to_str(gmm2Attr.en_life_update_factor_mode, life_update_str);

    printf("bmcv_ive_gmm2: format %s, u32FrameNumMax is %d, size %d x %d \n",
            fmt_str, u32FrameNumMax, width, height);
    printf("bmcv_ive_gmm2: snsFactorMode is %s , lifeUpdateFactorMode is %s \n",
            sns_mode_str, life_update_str);
    printf("idx:%d, bmcv_ive_gmm2: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive gmm2 test successful \n");

fail:
    free(inputData);
    free(srcData);
    free(srcFactorData);
    free(model_data);
    free(ive_fg_res);
    free(ive_bg_res);
    free(ive_pc_match_res);
    bm_image_destroy(&src);
    bm_image_destroy(&src_factor);
    bm_image_destroy(&dst_fg);
    bm_image_destroy(&dst_model_match_model_info);
    bm_free_device(handle, dst_model);

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 10 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        src_fmt = atoi(argv[3]);
        pixel_ctrl = atoi(argv[4]);
        life_update_enMode = atoi(argv[5]);
        input_name = argv[6];
        inputFactor_name = argv[7];
        goldenFg_name = argv[8];
        goldenBg_name = argv[9];
        if(argc > 10) goldenPcMatch_name = argv[10];
        if(argc > 11) dev_id = atoi(argv[11]);
        if(argc > 12) test_threads_num  = atoi(argv[12]);
        if(argc > 13) test_loop_times  = atoi(argv[13]);
        if(argc > 14) bWrite = atoi(argv[14]);
        if(argc > 15) dstFg_name = argv[15];
        if(argc > 16) dstBg_name = argv[16];
        if(argc > 17) dstPcMatch_name = argv[17];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if (argc > 3 && argc < 9) {
        printf("command input error, please follow this order:\n \
        %s width height src_fmt pixel_ctrl life_update_enMode src_name inputFactor_name goldenFg_name goldenBg_name goldenPcMatch_name dev_id thread_num loop_num bWrite dstFg_name dstFg_name\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive gmm2] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST ive gmm2] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_gmm2_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_gmm2, (void *)(ctx + i))) {
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