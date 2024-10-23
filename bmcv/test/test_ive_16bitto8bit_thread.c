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

typedef struct ive_16bitto8bit_ctx_{
    int loop;
    int i;
} ive_16bitto8bit_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
bmcv_ive_16bit_to_8bit_mode mode = BM_IVE_S16_TO_S8;
unsigned char u8Numerator = 41;
unsigned short u16Denominator = 18508;
signed char s8Bias = 0;
int height = 288, width = 352;
bm_image_data_format_ext srcDtype = DATA_TYPE_EXT_S16;
bm_image_data_format_ext dstDtype = DATA_TYPE_EXT_1N_BYTE_SIGNED;
char *src_name = "ive_data/00_704x576.s16";
char *ref_name = "ive_data/result/sample_16BitTo8Bit_S16ToS8.yuv";
bm_handle_t handle = NULL;

void mode16bitTo8bit_to_str(bmcv_ive_16bit_to_8bit_mode enMode, char* res){
    switch(enMode){
        case BM_IVE_S16_TO_S8:
            strcpy(res, "BM_IVE_S16_TO_S8");
            break;
        case BM_IVE_S16_TO_U8_ABS:
            strcpy(res, "BM_IVE_S16_TO_U8_ABS");
            break;
        case BM_IVE_S16_TO_U8_BIAS:
            strcpy(res, "BM_IVE_S16_TO_U8_BIAS");
            break;
        case BM_IVE_U16_TO_U8:
            strcpy(res, "BM_IVE_U16_TO_U8");
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

static void * ive_16bitTo8bit(void* arg) {
    bm_status_t ret;
    ive_16bitto8bit_ctx ctx = *(ive_16bitto8bit_ctx*)arg;
    bm_image src, dst;
    int srcStride[4], dstStride[4];
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
    bmcv_ive_16bit_to_8bit_attr attr;
    attr.mode = mode;
    attr.u16_denominator = u16Denominator;
    attr.u8_numerator = u8Numerator;
    attr.s8_bias = s8Bias;

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, FORMAT_GRAY, srcDtype, srcStride);
    bm_ive_image_calc_stride(handle, height, width, FORMAT_GRAY, dstDtype, dstStride);

    bm_image_create(handle, height, width, FORMAT_GRAY, srcDtype, &src, srcStride);
    bm_image_create(handle, height, width, FORMAT_GRAY, dstDtype, &dst, dstStride);

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
        ret = bmcv_ive_16bit_to_8bit(handle, src, dst, attr);
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
            printf("bmcv_ive_16bitto8bit is failed \n");
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = width * height * fps_actual/1024/1024;

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

        int cmp = cmp_u8(ref_name, output_ptr, byte_size);
        if(cmp != 0){
            printf("[bmcv ive 16bitTo8bit] cmp failed, cmp = %d \n", cmp);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            free(output_ptr);
            exit(-1);
        }

        printf("[bmcv ive 16bitTo8bit] cmp successful, cmp = %d \n", cmp);
        free(output_ptr);

        if(bWrite) bm_ive_write_bin(dst, "16bitTo8bit_res.bin");

    }

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    char algorithm_str[100] = "ive_16bitTo8bit";
    char fmt_str[100], mode_str[100];
    format_to_str(src.image_format, fmt_str);
    mode16bitTo8bit_to_str(attr.mode, mode_str);

    printf("idx:%d, %s, mode %s, fmt %s, size %d*%d\n",ctx.i, algorithm_str, mode_str, fmt_str, width, height);
    printf("idx:%d, ive_16bitTo8bit: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

int main(int argc, char **argv) {
    if(argc >= 11){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        mode = atoi(argv[3]);
        u8Numerator = atoi(argv[4]);
        u16Denominator = atoi(argv[5]);
        s8Bias = atoi(argv[6]);
        srcDtype = atoi(argv[7]);
        dstDtype = atoi(argv[8]);
        src_name = argv[9];
        ref_name = argv[10];
        if(argc > 11) dev_id = atoi(argv[11]);
        if(argc > 12) test_threads_num = atoi(argv[12]);
        if(argc > 13) test_loop_times  = atoi(argv[13]);
        if(argc > 14) bWrite = atoi(argv[14]);
    }
    if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    }
    if ((argc > 3 && argc < 10) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height mode u8Numerator u16Denominator s8Bias srcDtype dstDtype src_name ref_name dev_id thread_num loop_num bWrite\n \
        %s 352 288 0 41 18508 0 6 2 ive_data/00_704x576.s16 ive_data/result/sample_16BitTo8Bit_S16ToS8.yuv 0 1 1 0\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive 16bitTo8bit] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > MAX_THREAD_NUM || test_threads_num < 1) {
        printf("[TEST ive 16bitTo8bit] thread nums should be 1~%d\n", MAX_THREAD_NUM);
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_16bitto8bit_ctx ctx[MAX_THREAD_NUM];
    #ifdef __linux__
        pthread_t pid[MAX_THREAD_NUM];
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_16bitTo8bit, (void *)(ctx + i))) {
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