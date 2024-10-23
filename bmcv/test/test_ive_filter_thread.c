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

typedef struct ive_filter_ctx_{
    int loop;
    int i;
} ive_filter_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int thrSize = 0; //0 -> 3x3; 1 -> 5x5
unsigned char u8Norm = 4;
int height = 288, width = 352;
bm_image_format_ext fmt = FORMAT_GRAY; // 14 4 6
char *src_name = "./data/00_352x288_y.yuv";
char *dst_name = "ive_filter_res.yuv";
char *goldenDst_name = "./data/result/sample_Filter_Y3x3.yuv";
bm_handle_t handle = NULL;

signed char arr5by5[25] = { 1, 2, 3, 2, 1, 2, 5, 6, 5, 2, 3, 6, 8,
                    6, 3, 2, 5, 6, 5, 2, 1, 2, 3, 2, 1 };
signed char arr3by3[25] = { 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 2, 4,
                    2, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0 };

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

static void * ive_filter(void* arg){
    bm_status_t ret;
    ive_filter_ctx ctx = *(ive_filter_ctx*)arg;
    bm_image src, dst;
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

    bmcv_ive_filter_ctrl filterAttr;
    (thrSize == 0) ? memcpy(filterAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char)) :
                     memcpy(filterAttr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char));
    filterAttr.u8_norm = u8Norm;

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        exit(-1);
    }

    bm_ive_read_bin(src, src_name);

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_filter(handle, src, dst, filterAttr);
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
            printf("bmcv_ive_filter failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    if(ctx.i == 0){
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(dst, image_byte_size);
        int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
        memset(output_ptr, 0, sizeof(byte_size));

        void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((char*)output_ptr + image_byte_size[0]),
                            (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                            (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

        ret = bm_image_copy_device_to_host(dst, (void **)out_ptr);
        if(ret != BM_SUCCESS){
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            free(output_ptr);
            exit(-1);
        }

        int cmp = cmp_u8(goldenDst_name, output_ptr, byte_size);
        if(cmp != 0){
            printf("[bmcv ive filter] filter_res cmp failed \n");
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            free(output_ptr);
            exit(-1);
        }
        printf("[bmcv ive filter] filter_res cmp succed \n");

        if(bWrite){
            FILE *filter_fp = fopen(dst_name, "wb");
            fwrite((void *)output_ptr, 1, width * height, filter_fp);
            fclose(filter_fp);
        }
        free(output_ptr);
    }

    bm_image_destroy(&src);
    bm_image_destroy(&dst);


    char fmt_str[100], thrSize_str[100];
    format_to_str(src.image_format, fmt_str);
    if(thrSize == 0)
        memcpy(thrSize_str, "3x3", sizeof(100));
    else
        memcpy(thrSize_str, "5x5", sizeof(100));

    printf("bmcv_ive_filter: format %s, thrSize is %s ,size %d x %d \n",
            fmt_str, thrSize_str, width, height);
    printf("idx:%d, bmcv_ive_filter: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive filter test successful \n");
    return 0;
}

int main(int argc, char **argv){
    if(argc >= 8 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        fmt = atoi(argv[3]);
        src_name = argv[4];
        thrSize = atoi(argv[5]);
        u8Norm = atoi(argv[6]);
        goldenDst_name = argv[7];
        if(argc > 8) dev_id = atoi(argv[8]);
        if(argc > 9) test_threads_num  = atoi(argv[9]);
        if(argc > 10) test_loop_times  = atoi(argv[10]);
        if(argc > 11) bWrite = atoi(argv[11]);
        if(argc > 12) dst_name = argv[12];
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if ((argc > 3 && argc < 7) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height fmt src_name thrSize(0:3x3 1:5x5) u8Norm goldenDst_name dev_id thread_num loop_num bWrite dst_name\n \
        %s 352 288 6 ive_data/00_352x288_SP422.yuv 1 7 ive_data/result/sample_Filter_422SP5x5.yuv 0 1 9\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive filter] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST ive filter] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_filter_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_filter, (void *)(ctx + i))) {
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