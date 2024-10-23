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

typedef struct ive_bgmodel_ctx_{
    int loop;
    int i;
} ive_bgmodel_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
bm_image_format_ext fmt = FORMAT_GRAY;
char *input_name = "./ive_data/campus.u8c1.1_100.raw";
char *ref_name = "./ive_data/result/sample_BgModelSample2_BgMdl_100.bin";
char *bgModel_name = "bgmodel_bgmdl_output_100.bin";
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

bm_status_t init_match_bgmodel(bm_handle_t handle, bm_image* pstCurImg,
                    bm_image* pstBgModel, bm_image* pstFgFlag,
                    bm_image* pstDiffFg, bm_device_mem_t* pstStatData,
                    bmcv_ive_match_bgmodel_attr* matchBgmodel_attr,
                    int width, int height)
{
    bm_status_t ret = BM_SUCCESS;
    int u8Stride[4] = {0}, s16Stride[4] = {0}, bgModelStride[4] = {0};

    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, u8Stride);
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_S16, s16Stride);
    bm_ive_image_calc_stride(handle, height, width * sizeof(bmcv_ive_bg_model_pix),
                                        fmt, DATA_TYPE_EXT_1N_BYTE, bgModelStride);

    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, pstCurImg, u8Stride);
    if(ret != BM_SUCCESS){
        printf("pstCurImg bm_image_create failed , ret = %d \n", ret);
        return ret;
    }

    ret = bm_image_create(handle, height, width * sizeof(bmcv_ive_bg_model_pix),
                                fmt, DATA_TYPE_EXT_1N_BYTE, pstBgModel, bgModelStride);
    if(ret != BM_SUCCESS){
        printf("pstBgModel bm_image_create failed , ret = %d \n", ret);
        return ret;
    }

    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, pstFgFlag, u8Stride);
    if(ret != BM_SUCCESS){
        printf("pstFgFlag bm_image_create failed , ret = %d \n", ret);
        return ret;
    }

    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_S16, pstDiffFg, s16Stride);
    if(ret != BM_SUCCESS){
        printf("pstDiffFg bm_image_create failed , ret = %d \n", ret);
        return ret;
    }

    ret = bm_image_alloc_dev_mem(*pstCurImg, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("pstCurImg bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        return ret;
    }

    ret = bm_image_alloc_dev_mem(*pstFgFlag, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("pstFgFlag bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        return ret;
    }

    ret = bm_image_alloc_dev_mem(*pstDiffFg, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("pstDiffFg bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        return ret;
    }

    ret = bm_image_alloc_dev_mem(*pstBgModel, BMCV_HEAP1_ID);
    if(ret != BM_SUCCESS){
        printf("pstBgModel bm_malloc_device_byte. ret = %d\n", ret);
        return ret;
    }

    ret = bm_malloc_device_byte(handle, pstStatData, sizeof(bmcv_ive_bg_stat_data));
    if(ret != BM_SUCCESS){
        printf("pstStatData bm_malloc_device_byte. ret = %d\n", ret);
        return ret;
    }

    // init match_bgmodel config
    matchBgmodel_attr->u32_cur_frm_num = 0;
    matchBgmodel_attr->u32_pre_frm_num = 0;
    matchBgmodel_attr->u16_time_thr = 20;
    matchBgmodel_attr->u8_diff_thr_crl_coef = 0;
    matchBgmodel_attr->u8_diff_max_thr = 10;
    matchBgmodel_attr->u8_diff_min_thr = 10;
    matchBgmodel_attr->u8_diff_thr_inc = 0;
    matchBgmodel_attr->u8_fast_learn_rate = 4;
    matchBgmodel_attr->u8_det_chg_region = 1;

    return ret;
}

bm_status_t destory_match_bgmodel(bm_handle_t handle, bm_image* pstCurImg,
                    bm_image* pstBgModel, bm_image* pstFgFlag,
                    bm_image* pstDiffFg, bm_device_mem_t* pstStatData)
{
    bm_image_destroy(pstCurImg);
    bm_image_destroy(pstFgFlag);
    bm_image_destroy(pstDiffFg);
    bm_image_destroy(pstBgModel);
    bm_free_device(handle, *pstStatData);

    return BM_SUCCESS;
}

bm_status_t init_update_bgmodel(bm_handle_t handle, bm_image* pstBgImg, bm_image* pstChgSta,
                              bmcv_ive_update_bgmodel_attr* update_bgmodel_attr,
                              int width, int height)
{
    int bg_img_stride[4] = {0}, chg_sta_stride[4] = {0};
    bm_status_t ret = BM_SUCCESS;

    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, bg_img_stride);
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_U32, chg_sta_stride);

    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, pstBgImg, bg_img_stride);
    if(ret != BM_SUCCESS){
        printf("pstBgImg bm_image_create failed , ret = %d \n", ret);
        return ret;
    }

    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U32, pstChgSta, chg_sta_stride);
    if(ret != BM_SUCCESS){
        printf("pstChgSta bm_image_create failed , ret = %d \n", ret);
        return ret;
    }

    ret = bm_image_alloc_dev_mem(*pstBgImg, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("pstBgImg bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        return ret;
    }

    ret = bm_image_alloc_dev_mem(*pstChgSta, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("pstChgSta bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        return ret;
    }

    // init update_bgmodel config
    update_bgmodel_attr->u32_cur_frm_num = 0;
    update_bgmodel_attr->u32_pre_chk_time = 0;
    update_bgmodel_attr->u32_frm_chk_period = 30;
    update_bgmodel_attr->u32_init_min_time = 25;
    update_bgmodel_attr->u32_sty_bg_min_blend_time = 100;
    update_bgmodel_attr->u32_sty_bg_max_blend_time = 1500;
    update_bgmodel_attr->u32_dyn_bg_min_blend_time = 0;
    update_bgmodel_attr->u32_static_det_min_time = 80;
    update_bgmodel_attr->u16_fg_max_fade_time = 15;
    update_bgmodel_attr->u16_bg_max_fade_time = 60;
    update_bgmodel_attr->u8_sty_bg_acc_time_rate_thr = 80;
    update_bgmodel_attr->u8_chg_bg_acc_time_rate_thr = 60;
    update_bgmodel_attr->u8_dyn_bg_acc_time_thr = 0;
    update_bgmodel_attr->u8_dyn_bg_depth = 3;
    update_bgmodel_attr->u8_bg_eff_sta_rate_thr = 90;
    update_bgmodel_attr->u8_acce_bg_learn = 0;
    update_bgmodel_attr->u8_det_chg_region = 1;

    return ret;
}

bm_status_t destory_update_bgmodel(bm_image* pstBgImg, bm_image* pstChgSta)
{
    bm_image_destroy(pstBgImg);
    bm_image_destroy(pstChgSta);

    return BM_SUCCESS;
}

static void * ive_bgmodel(void* arg)
{
    bm_status_t ret;
    ive_bgmodel_ctx ctx = *(ive_bgmodel_ctx*)arg;
    int srcStride[4];
    unsigned int i = 0, loop_time = 0;
    unsigned long long match_time_single, match_time_total = 0, match_time_avg = 0;
    unsigned long long match_time_max = 0, match_time_min = 10000, match_fps_actual = 0;

    unsigned long long update_time_single, update_time_total = 0, update_time_avg = 0;
    unsigned long long update_time_max = 0, update_time_min = 10000, update_fps_actual = 0;
#if SLEEP_ON
    int fps = 60;
    int sleep_time = 1000000 / fps;
#endif

    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval timediff;

    loop_time = ctx.loop;
    int update_cnt = 0;

    int frameNum;
    int frameNumMax = 100;
    int frmCnt = 0;

    int buf_size = frameNumMax * width * height;
    unsigned char *input_data = (unsigned char*)malloc(buf_size * sizeof(unsigned char));
    unsigned char *src_data = (unsigned char*)malloc(buf_size * sizeof(unsigned char));
    unsigned char *bgmodel_res = (unsigned char*) malloc(width * sizeof(bmcv_ive_bg_model_pix) * height);

    memset(input_data, 0, buf_size * sizeof(unsigned char));
    memset(src_data, 0, buf_size * sizeof(unsigned char));
    memset(bgmodel_res, 0, width * sizeof(bmcv_ive_bg_model_pix) * height);

    FILE *input_fp = fopen(input_name, "rb");
    if(input_fp == NULL){
        printf("%s : No such file \n", ref_name);
        exit(-1);
    }
    fread((void *)input_data, 1, buf_size * sizeof(unsigned char), input_fp);
    fclose(input_fp);

    // create src image
    bm_image src, stFgFlag, stDiffFg, stBgModel;
    bm_device_mem_t stStatData;
    bmcv_ive_match_bgmodel_attr matchBgModelAttr;

    ret = init_match_bgmodel(handle, &src, &stBgModel, &stFgFlag,
                    &stDiffFg, &stStatData, &matchBgModelAttr, width, height);
    if(ret != BM_SUCCESS){
        printf("init_match_bgmodel failed \n");
        exit(-1);
    }

    ret = bm_image_get_stride(src, srcStride);
    if(ret != BM_SUCCESS){
        printf("bm_image_get_stride failed \n");
        exit(-1);
    }

    // create dst image
    bm_image stBgImg, stChgStaLife;
    bmcv_ive_update_bgmodel_attr updateBgModelAttr;
    ret = init_update_bgmodel(handle, &stBgImg, &stChgStaLife, &updateBgModelAttr, width, height);
    if(ret != BM_SUCCESS){
        printf("init_update_bgmodel failed \n");
        exit(-1);
    }

    unsigned char* fg_flag = malloc(width * height);
    unsigned char* bg_model =
           (unsigned char*)malloc(width * sizeof(bmcv_ive_bg_model_pix) * height);

    bmcv_ive_bg_stat_data *stat = malloc(sizeof(bmcv_ive_bg_stat_data));

    memset(fg_flag, 0, width * height);
    memset(bg_model, 0, width * sizeof(bmcv_ive_bg_model_pix) * height);

    for(i = 0; i < loop_time; i++){
        int updCnt = 5;
        int preUpdTime = 0;
        int preChkTime = 0;
        int frmUpdPeriod = 10;
        int frmChkPeriod = 30;

        ret = bm_image_copy_host_to_device(stFgFlag, (void**)&fg_flag);
        if(ret != BM_SUCCESS){
            printf("stFgFlag bm_image_copy_host_to_device. ret = %d\n", ret);
            exit(-1);
        }

       ret = bm_image_copy_host_to_device(stBgModel, (void**)&bg_model);
       if(ret != BM_SUCCESS){
           printf("pstBgModel bm_image_copy_host_to_device. ret = %d\n", ret);
           exit(-1);
       }

        // config setting
        matchBgModelAttr.u32_cur_frm_num = frmCnt;
        for(frmCnt = 0; frmCnt < frameNumMax; frmCnt++){
            frameNum = frmCnt + 1;
            if(width > 480){
                for(int j = 0; j < 288; j++){
                    memcpy(src_data + (j * width),
                           input_data + (frmCnt * 352 * 288 + j * 352), 352);
                    memcpy(src_data + (j * width + 352),
                           input_data + (frmCnt * 352 * 288 + j * 352), 352);
                }
            } else {
                for(int j = 0; j < 288; j++){
                    memcpy(src_data + j * srcStride[0],
                           input_data + frmCnt * width * 288 + j * width, width);
                    int s = srcStride[0] - width;
                    memset(src_data + j * srcStride[0] + width, 0, s);
                }
            }

            ret = bm_image_copy_host_to_device(src, (void**)&src_data);
            if(ret != BM_SUCCESS){
                printf("bm_image copy src h2d failed. ret = %d \n", ret);
                exit(-1);
            }

            matchBgModelAttr.u32_pre_frm_num = matchBgModelAttr.u32_cur_frm_num;
            matchBgModelAttr.u32_cur_frm_num = frameNum;
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_match_bgmodel(handle, src, stBgModel,
                            stFgFlag, stDiffFg, stStatData, matchBgModelAttr);
            gettimeofday(&tv_end, NULL);
            if(ret != BM_SUCCESS){
                printf("bmcv_ive_match_bgmodel failed. ret = %d \n", ret);
                exit(-1);
            }
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            match_time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

#if SLEEP_ON
            if(match_time_single < sleep_time)
                usleep((sleep_time - match_time_single));
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            match_time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
#endif
            if(match_time_single > match_time_max){match_time_max = match_time_single;}
            if(match_time_single < match_time_min){match_time_min = match_time_single;}
            match_time_total = match_time_total + match_time_single;

            ret = bm_memcpy_d2s(handle, (void*)stat, stStatData);
            if(ret != BM_SUCCESS){
                printf("stStatData bm_memcpy_d2s failed \n");
                exit(-1);
            }

            printf("BM_IVE_MatchBgModel u32UpdCnt %d, u32FrameNum %d, frm %d, ", updCnt, frameNum, frmCnt);
            printf("stat pix_num_u32 = %d, sum_lum_u32 = %d\n", stat->u32_pix_num, stat->u32_sum_lum);

            if(updCnt == 0 || frameNum >= preUpdTime + frmUpdPeriod){
                updCnt++;
                preUpdTime = frameNum;
                updateBgModelAttr.u32_cur_frm_num = frameNum;
                updateBgModelAttr.u32_pre_chk_time = preChkTime;
                updateBgModelAttr.u32_frm_chk_period = 0;
                if(frameNum >= preChkTime + frmChkPeriod){
                    updateBgModelAttr.u32_frm_chk_period = frmChkPeriod;
                    preChkTime = frameNum;
                }

                gettimeofday(&tv_start, NULL);
                ret = bmcv_ive_update_bgmodel(handle, &src, &stBgModel,
                            &stFgFlag, &stBgImg, &stChgStaLife, stStatData, updateBgModelAttr);
                gettimeofday(&tv_end, NULL);
                if(ret != BM_SUCCESS){
                    printf("bmcv_ive_update_bgmodel failed, ret = %d \n", ret);
                    exit(-1);
                }
                timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
                timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
                update_time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
#if SLEEP_ON
                if(update_time_single < sleep_time)
                    usleep((sleep_time - update_time_single));
                gettimeofday(&tv_end, NULL);
                timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
                timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
                update_time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
#endif
                if(update_time_single > update_time_max){update_time_max = update_time_single;}
                if(update_time_single < update_time_min){update_time_min = update_time_single;}
                update_time_total = update_time_total + update_time_single;
                update_cnt++;

                ret = bm_memcpy_d2s(handle, (void*)stat, stStatData);
                if(ret != BM_SUCCESS){
                    printf("stStatData bm_memcpy_d2s failed \n");
                    exit(-1);
                }
                printf("BM_IVE_UpdateBgModel frm %d ,", frmCnt);
                printf("stat pix_num_u32 = %d, sum_lum_u32 = %d\n", stat->u32_pix_num, stat->u32_sum_lum);
            }
        }

        ret = bm_image_copy_device_to_host(stBgModel, (void**)&bgmodel_res);
        if(ret != BM_SUCCESS){
            printf("stBgModel bm_memcpy_d2s failed \n");
            exit(-1);
        }

        int cmp = cmp_u8(ref_name, bgmodel_res,
                  width * sizeof(bmcv_ive_bg_model_pix) * height);
        if(cmp != 0){
            printf("[bmcv ive bgmodel] bgmodel_res cmp failed \n");
            exit(-1);
        }
        printf("[bmcv ive bgmodel] bgmodel_res cmp succed \n");
    }
    free(input_data);
    free(src_data);
    free(fg_flag);
    free(bg_model);
    free(stat);

    match_time_avg = match_time_total / (loop_time * frameNumMax);
    match_fps_actual = 1000000 / (match_time_avg * frameNumMax);

    update_time_avg = update_time_total / update_cnt;
    update_fps_actual = 1000000 / (update_time_avg * update_cnt);

    if(bWrite){
        FILE *fp = fopen(bgModel_name, "wb");
        fwrite((void *)bgmodel_res, 1, width * sizeof(bmcv_ive_bg_model_pix) * height, fp);
        fclose(fp);
    }

    free(bgmodel_res);
    destory_match_bgmodel(handle, &src, &stBgModel, &stFgFlag, &stDiffFg, &stStatData);
    destory_update_bgmodel(&stBgImg, &stChgStaLife);

    char fmt_str[100];
    format_to_str(src.image_format, fmt_str);

    printf("bmcv_ive_bgmodel: format %s,  frameNumMax is %d, size %d x %d \n",
            fmt_str, frameNumMax, width, height);

    printf("idx:%d, bmcv_ive_match_bgmodel: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, match_time_max, match_time_avg, match_fps_actual);

    printf("idx:%d, bmcv_ive_update_bgmodel: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, update_time_max, update_time_avg, update_fps_actual);
    printf("bmcv ive bgmodel test successful \n");

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 5 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        input_name = argv[3];
        ref_name = argv[4];
        if(argc > 5) dev_id = atoi(argv[5]);
        if(argc > 6) test_threads_num  = atoi(argv[6]);
        if(argc > 7) test_loop_times  = atoi(argv[7]);
        if(argc > 8) bWrite = atoi(argv[8]);
        if(argc > 9) bgModel_name = argv[9];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 5) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height src_name ref_name dev_id thread_num loop_num bWrite bgModel_name\n \
        %s 352 288 ive_data/campus.u8c1.1_100.raw ive_data/result/sample_BgModelSample2_BgMdl_100.bin 0 1 1\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive bgmodel] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST ive bgmodel] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_bgmodel_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_bgmodel, (void *)(ctx + i))) {
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
