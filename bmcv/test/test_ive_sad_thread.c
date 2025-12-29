#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0
#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

extern void bm_ive_read_bin(bm_image src, const char *input_name);
extern void bm_ive_write_bin(bm_image dst, const char *output_name);
extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);

typedef struct ive_sad_ctx_{
    int loop;
    int i;
} ive_sad_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
bm_image_format_ext fmt = FORMAT_GRAY;
bmcv_ive_sad_mode sadMode = BM_IVE_SAD_MODE_MB_8X8;
bmcv_ive_sad_out_ctrl sadOutCtrl = BM_IVE_SAD_OUT_BOTH;
int datatype = 0;
int u16_thr = 0x800;
int u8_min_val = 2;
int u8_max_val = 30;
char *src1_name = "./data/00_352x288_y.yuv", *src2_name = "./data/bin_352x288_y.yuv";
char *dstSad_name = "sad_res.yuv", *dstThr_name = "sad_thr_res.yuv";
char *refSad_name = "./data/result/sample_Sad_sad_mode1_out0.bin";
char *refThr_name = "./data/result/sample_Sad_thr_mode1_out0.bin";
bm_handle_t handle = NULL;

void SadCtrl_to_str(bmcv_ive_sad_attr sad_ctrl, char* mode_res, char* sad_out_res){
    switch(sad_ctrl.en_mode){
        case BM_IVE_SAD_MODE_MB_4X4:
            strcpy(mode_res, "BM_IVE_SAD_MODE_MB_4X4");
            break;
        case BM_IVE_SAD_MODE_MB_8X8:
            strcpy(mode_res, "BM_IVE_SAD_MODE_MB_8X8");
            break;
        case BM_IVE_SAD_MODE_MB_16X16:
            strcpy(mode_res, "BM_IVE_SAD_MODE_MB_16X16");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }

    switch(sad_ctrl.en_out_ctrl){
        case BM_IVE_SAD_OUT_BOTH:
            strcpy(sad_out_res, "BM_IVE_SAD_OUT_BOTH");
            break;
        case BM_IVE_SAD_OUT_SAD:
            strcpy(sad_out_res, "BM_IVE_SAD_OUT_SAD");
            break;
        case BM_IVE_SAD_OUT_THRESH:
            strcpy(sad_out_res, "BM_IVE_SAD_OUT_THRESH");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

int cmp_u8(unsigned char* ref, unsigned char* got, int h, int w, int dsize, bool isDmaHalf){
    int stride = align_up(width, 16);
    int sad_stride = (isDmaHalf) ? stride / 2 : stride;
    int n = 0;
    for(int i = 0; i < h; i++, ref += stride * dsize, got += sad_stride * dsize){
        n = memcmp(ref, got, w * dsize);
        if(n != 0){
            for(int j = 0; j < w * dsize; j++){
                if(ref[j] != got[j]){
                    printf("cmp failed, i is %d, ref = %d, got = %d \n", i * h + j, ref[j], got[j]);
                    return -1;
                }
            }
        }
    }
    return n;
}

const int cmp_sad(char* ref_name, unsigned char* got,
                  int sad_width, int sad_height, int data_size, bool ifDmahalf){
    int len = width * height * data_size;
    int n = -1;
    FILE *ref_fp = fopen(ref_name, "rb");
    if(ref_fp == NULL){
        printf("%s : No such file \n", ref_name);
        return -1;
    }

    unsigned char* ref = malloc(len);
    fread((void *)ref, 1, len, ref_fp);
    n = cmp_u8(ref, got, sad_height, sad_width, data_size, ifDmahalf);
    free(ref);
    fclose(ref_fp);
    return n;
}

const int sad_write_img(char* file_name, unsigned char *data,
                        int sad_width, int sad_height, int dsize,
                        int stride){
    unsigned char* tmp = malloc(sad_width * dsize);
    FILE *fp = fopen(file_name, "wb");
    for(int h = 0; h < sad_height; h++, data += stride){
        memcpy(tmp, data, sad_width * dsize);
        fwrite((void *)tmp, 1, sad_width * dsize, fp);
    }
    fclose(fp);
    free(tmp);
    return 0;
}

static void * ive_sad(void* arg){
    bm_status_t ret;
    ive_sad_ctx ctx = *(ive_sad_ctx*)arg;
    int dst_width = 0, dst_height = 0;
    bm_image src[2];
    bm_image dstSad, dstSadThr;
    int stride[4];
    int dstSad_stride[4];
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

    bmcv_ive_sad_attr sadAttr;
    bmcv_ive_sad_thresh_attr sadthreshAttr;
    sadAttr.en_mode = sadMode;
    sadAttr.en_out_ctrl = sadOutCtrl;
    sadthreshAttr.u16_thr = u16_thr;
    sadthreshAttr.u8_min_val = u8_min_val;
    sadthreshAttr.u8_max_val = u8_max_val;

    bm_image_data_format_ext dst_sad_dtype = DATA_TYPE_EXT_1N_BYTE;
    bool flag = false;

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);

    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src[0], stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src[1], stride);

    if(sadOutCtrl == BM_IVE_SAD_OUT_BOTH) {
        if (datatype == 0){
            dst_sad_dtype = DATA_TYPE_EXT_U16;
            bm_ive_image_calc_stride(handle, height, width, fmt, dst_sad_dtype, dstSad_stride);
            bm_image_create(handle, height, width, fmt, dst_sad_dtype, &dstSad, dstSad_stride);
            bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dstSadThr, stride);
        } else {
            bm_ive_image_calc_stride(handle, height, width, fmt, dst_sad_dtype, dstSad_stride);
            bm_image_create(handle, height, width, fmt, dst_sad_dtype, &dstSad, dstSad_stride);
            bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dstSadThr, stride);
        }
    }

    int data_size = (dst_sad_dtype == DATA_TYPE_EXT_U16) ? sizeof(unsigned short) : sizeof(unsigned char);
    switch(sadMode){
        case BM_IVE_SAD_MODE_MB_4X4:
            dst_width = width / 4;
            dst_height = height / 4;
            break;
        case BM_IVE_SAD_MODE_MB_8X8:
            dst_width = width  / 8;
            dst_height = height / 8;
            break;
        case BM_IVE_SAD_MODE_MB_16X16:
            dst_width = width  / 16;
            dst_height = height / 16;
            break;
        default:
            printf("not support mode type %d, return\n", sadOutCtrl);
            break;
    }

    ret = bm_image_alloc_dev_mem(src[0], BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src1 bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(src[1], BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src2 bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dstSad, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("dstSad bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dstSadThr, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("dstSadThr bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        exit(-1);
    }
    if (dstSad.data_type == DATA_TYPE_EXT_1N_BYTE){
        flag = true;
    }

    bm_ive_read_bin(src[0], src1_name);
    bm_ive_read_bin(src[1], src2_name);

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_sad(handle, src, &dstSad, &dstSadThr, &sadAttr, &sadthreshAttr);
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
            printf("bmcv_ive_sad failed. ret = %d\n", ret);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        unsigned char* sad_res = (unsigned char*)malloc(width * height * data_size);
        unsigned char* thr_res = (unsigned char*)malloc(width * height);
        memset(sad_res, 0, width * height * data_size);
        memset(thr_res, 0, width * height);

        ret = bm_image_copy_device_to_host(dstSad, (void **)&sad_res);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_image_copy_device_to_host(dstSadThr, (void **)&thr_res);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            exit(-1);
        }
        int cmp = cmp_sad(refSad_name, sad_res, dst_width, dst_height, data_size, flag);
        if(cmp != 0){
            printf("sad_res compare failed. cmp = %d\n", cmp);
            exit(-1);
        }

        printf("[bmcv ive sad] SAD cmp succed \n");

        cmp = cmp_sad(refThr_name, thr_res, dst_width, dst_height, sizeof(unsigned char), false);
        if(cmp != 0){
            printf("thr_res compare failed. cmp = %d\n", cmp);
            exit(-1);
        }
        printf("[bmcv ive sad] Thr cmp succed \n");

        if(bWrite){
            int sad_stride = (flag) ? dstSad_stride[0] / 2 : dstSad_stride[0];
            sad_write_img(dstSad_name, sad_res, dst_width, dst_height, data_size, sad_stride);
            sad_write_img(dstThr_name, thr_res, dst_width, dst_height, sizeof(unsigned char), stride[0]);
        }
        free(sad_res);
        free(thr_res);
    }

    bm_image_destroy(&src[0]);
    bm_image_destroy(&src[1]);
    bm_image_destroy(&dstSad);
    bm_image_destroy(&dstSadThr);

    char fmt_str[100], modeStr[100], outCtrlStr[100];
    format_to_str(src[1].image_format, fmt_str);
    SadCtrl_to_str(sadAttr, modeStr, outCtrlStr);

    printf("bmcv_ive_sad: src_format %s, size %d x %d -> %d x %d \n", fmt_str, width, height, dst_width, dst_height);
    printf("bmcv_ive_sad: sad_mode %s, sad_out_ctrl %s , sad_u16Thr %d , sad_min_value %d , sad_max_value %d \n",
                modeStr, outCtrlStr, u16_thr, u8_min_val, u8_max_val);
    printf("idx:%d, bmcv_ive_sad: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
     printf("----- [bmcv ive sad] test successful -----\n");
    return 0;
}

int main(int argc, char **argv){
    if (argc >= 13) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        sadMode = atoi(argv[3]);
        sadOutCtrl = atoi(argv[4]);
        datatype = atoi(argv[5]);
        u16_thr = atoi(argv[6]);
        u8_min_val = atoi(argv[7]);
        u8_max_val = atoi(argv[8]);
        src1_name = argv[9];
        src2_name = argv[10];
        refSad_name = argv[11];
        refThr_name = argv[12];
        if(argc > 13) dev_id = atoi(argv[13]);
        if(argc > 14) test_threads_num = atoi(argv[14]);
        if(argc > 15) test_loop_times  = atoi(argv[15]);
        if(argc > 16) bWrite = atoi(argv[16]);
        if(argc > 17) dstSad_name = argv[17];
        if(argc > 18) dstThr_name = argv[18];
    }
    if (argc == 2)
        test_threads_num  = atoi(argv[1]);
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 11) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height sad_mode sad_outCtrl_mode sadThr sadMinValue sadMaxValue src1_name src2_name refSad_name refThr_name dev_id thread_num loop_num bWrite dstSad_name dstSadThr_name\n \
        %s 352 288 1 0 0 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode1_out0.bin ive_data/result/sample_Sad_thr_mode1_out0.bin 0 1 1 0\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive sad] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST ive sad] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_sad_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_sad, (void *)(ctx + i))) {
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