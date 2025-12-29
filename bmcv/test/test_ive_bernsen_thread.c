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

typedef struct ive_bersen_ctx_{
    int loop;
    int i;
} ive_bersen_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
bmcv_ive_bernsen_mode mode = BM_IVE_BERNSEN_NORMAL;
int winsize = 5;
int height = 288, width = 352;
bm_image_format_ext fmt = FORMAT_GRAY;
char *src_name = "ive_data/00_352x288_y.yuv";
char *ref_name = "ive_data/result/sample_Bernsen_5x5.yuv";
bm_handle_t handle = NULL;

void bernsenMode_to_str(bmcv_ive_bernsen_mode enMode, char* res){
    switch(enMode){
        case BM_IVE_BERNSEN_NORMAL:
            strcpy(res, "BM_IVE_BERNSEN_NORMAL");
            break;
        case BM_IVE_BERNSEN_THRESH:
            strcpy(res, "BM_IVE_BERNSEN_THRESH");
            break;
        case BM_IVE_BERNSEN_PAPER:
            strcpy(res, "BM_IVE_BERNSEN_PAPER");
            break;
        default:
            printf("Not found such mode \n");
            break;
    }
}

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

static void * ive_bersen(void* arg) {
    bm_status_t ret;
    ive_bersen_ctx ctx = *(ive_bersen_ctx*)arg;
    bm_image src, dst;
    int stride[4];
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

    // config setting
    bmcv_ive_bernsen_attr attr;
    memset(&attr, 0, sizeof(bmcv_ive_bernsen_attr));

    attr.en_mode = mode;
    attr.u8_thr = 128;
    attr.u8_contrast_threshold = 15;
    attr.u8_win_size = winsize;

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);

    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        exit(-1);
    }

    // read image data from input files
    bm_ive_read_bin(src, src_name);

    for(i = 0; i < loop_time; i++)
    {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_bernsen(handle, src, dst, attr);
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
            printf("bmcv_ive_bernsen is failed \n");
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = width * height * fps_actual/1024/1024;

    if(ctx.i == 0){
        unsigned char *ive_add_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
        memset(ive_add_res, 0, width * height * sizeof(unsigned char));

        ret = bm_image_copy_device_to_host(dst, (void **)&ive_add_res);
        if(ret != BM_SUCCESS){
            printf("dst bm_image_copy_device_to_host failed, ret is %d \n", ret);
            exit(-1);
        }

        int cmp = cmp_u8(ref_name, ive_add_res, width * height * sizeof(unsigned char));
        if(cmp != 0){
            printf("[bmcv ive bernsen] cmp failed, cmp = %d \n", cmp);
            exit(-1);
        }

        printf("[bmcv ive bernsen] cmp successful, cmp = %d \n", cmp);

        if(bWrite) bm_ive_write_bin(dst, "bernsen_res.bin");
    }

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    char algorithm_str[100] = "ive_bernsen";
    char fmt_str[100], mode_str[100];
    format_to_str(src.image_format, fmt_str);
    bernsenMode_to_str(attr.en_mode, mode_str);

    printf("idx:%d, %s, mode %s, fmt %s, size %d*%d\n",ctx.i, algorithm_str, mode_str, fmt_str, width, height);
    printf("idx:%d, bmcv_ive_bernsen: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

int main(int argc, char **argv) {
    if(argc >= 7){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        mode = atoi(argv[3]);
        winsize = atoi(argv[4]);
        src_name = argv[5];
        ref_name = argv[6];
        if(argc > 7) dev_id = atoi(argv[7]);
        if(argc > 8) test_threads_num = atoi(argv[8]);
        if(argc > 9) test_loop_times  = atoi(argv[9]);
        if(argc > 10) bWrite = atoi(argv[10]);
        // if(argc > 14) dst_name = argv[14];
    }
    if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    }
    if ((argc > 3 && argc < 6) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height mode winsize src_name ref_name dev_id thread_num loop_num bWrite\n \
        %s 352 288 0 5 ive_data/00_352x288_y.yuv ive_data/result/sample_Bernsen_5x5.yuv 0 1 1 0\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive bernsen] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > MAX_THREAD_NUM || test_threads_num < 1) {
        printf("[TEST ive bernsen] thread nums should be 1~%d\n", MAX_THREAD_NUM);
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_bersen_ctx ctx[MAX_THREAD_NUM];
    #ifdef __linux__
        pthread_t pid[MAX_THREAD_NUM];
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_bersen, (void *)(ctx + i))) {
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