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

typedef struct ive_ccl_ctx_{
    int loop;
    int i;
} ive_ccl_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 576, width = 720;
bm_image_format_ext fmt = FORMAT_GRAY;
bmcv_ive_ccl_mode enMode = BM_IVE_CCL_MODE_8C;
unsigned short u16InitAreaThr = 4;
unsigned short u16Step = 2;

char *src_name = "./ive_data/ccl_raw_1.raw";
char *ive_res_name = "ive_ccl_res.yuv";
char *ref_name = "./ive_data/result/sample_CCL_1.bin";
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

static void * ive_ccl(void* arg){
    bm_status_t ret;
    ive_ccl_ctx ctx = *(ive_ccl_ctx*)arg;
    bm_image src_dst_img;
    bm_device_mem_t pst_blob;
    bmcv_ive_ccl_attr ccl_attr;
    bmcv_ive_ccblob *ccblob = NULL;
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

    ccl_attr.en_mode = enMode;
    ccl_attr.u16_init_area_thr = u16InitAreaThr;
    ccl_attr.u16_step = u16Step;

    ccblob = (bmcv_ive_ccblob *)malloc(sizeof(bmcv_ive_ccblob));
    memset(ccblob, 0, sizeof(bmcv_ive_ccblob));

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src_dst_img, stride);

    ret = bm_image_alloc_dev_mem(src_dst_img, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        free(ccblob);
        bm_image_destroy(&src_dst_img);
        exit(-1);
    }

    ret = bm_malloc_device_byte(handle, &pst_blob, sizeof(bmcv_ive_ccblob));
    if(ret != BM_SUCCESS){
        printf("pst_blob bm_malloc_device_byte failed, ret = %d \n", ret);
        free(ccblob);
        bm_image_destroy(&src_dst_img);
        exit(-1);
    }

    for (i = 0; i < loop_time; i++) {
        bm_ive_read_bin(src_dst_img, src_name);

        ret = bm_memcpy_s2d(handle, pst_blob, (void *)ccblob);
        if(ret != BM_SUCCESS){
            printf("pst_blob bm_memcpy_s2d failed, ret = %d \n", ret);
            free(ccblob);
            bm_image_destroy(&src_dst_img);
            bm_free_device(handle, pst_blob);
            exit(-1);
        }

        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_ccl(handle, src_dst_img, pst_blob, ccl_attr);
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
            printf("bmcv_ive_ccl failed. ret = %d\n", ret);
            free(ccblob);
            bm_image_destroy(&src_dst_img);
            bm_free_device(handle, pst_blob);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
        ret = bm_memcpy_d2s(handle, (void*)ccblob, pst_blob);
        if(ret != BM_SUCCESS){
            printf("pst_blob bm_memcpy_d2s failed, ret = %d \n", ret);
            free(ccblob);
            bm_image_destroy(&src_dst_img);
            bm_free_device(handle, pst_blob);
            exit(-1);
        }
        int cmp = cmp_u8(ref_name, (unsigned char*)ccblob, sizeof(bmcv_ive_ccblob));
        if(cmp != 0){
            printf("[bmcv ive ccl] ccblob cmp failed \n");
        } else {
            printf("[bmcv ive ccl] ccblob cmp succed \n");
            printf("bmcv ive ccl test successful \n");
        }

        if(bWrite){
            FILE *fp = fopen(ive_res_name, "wb");
            fwrite((void *)ccblob, 1, sizeof(bmcv_ive_ccblob), fp);
            fclose(fp);
        }
    }

    char fmt_str[100];
    format_to_str(src_dst_img.image_format, fmt_str);

    printf("bmcv_ive_ccl: format %s,size %d x %d \n",
            fmt_str, width, height);
    printf("idx:%d, bmcv_ive_ccl: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);

    free(ccblob);
    bm_image_destroy(&src_dst_img);
    bm_free_device(handle, pst_blob);

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 8 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        enMode = atoi(argv[3]);
        u16InitAreaThr = atoi(argv[4]);
        u16Step = atoi(argv[5]);
        src_name = argv[6];
        ref_name = argv[7];
        if(argc > 8) dev_id = atoi(argv[8]);
        if(argc > 9) test_threads_num  = atoi(argv[9]);
        if(argc > 10) test_loop_times  = atoi(argv[10]);
        if(argc > 11) bWrite = atoi(argv[11]);
        if(argc > 12) ive_res_name = argv[12];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 7) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height enMode u16InitAreaThr u16Step src_name ref_name dev_id thread_num loop_num bWrite ive_res_name\n \
        %s 720 576 1 4 2 ive_data/ccl_raw_1.raw ive_data/result/sample_CCL_1.bin 0 1 1 0\n", argv[0], argv[0]);
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

    ive_ccl_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_ccl, (void *)(ctx + i))) {
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