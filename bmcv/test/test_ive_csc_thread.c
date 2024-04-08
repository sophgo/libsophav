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

typedef struct ive_csc_ctx_{
    int loop;
    int i;
} ive_csc_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
csc_type_t csc_type = CSC_MAX_ENUM;
bm_image_format_ext src_fmt = FORMAT_NV21; // FORMAT_NV21: 4; yuv444p : 2 FORMAT_RGB_PACKED: 10
bm_image_format_ext dst_fmt = FORMAT_RGB_PACKED;
char *src_name = "./ive_data/00_352x288_SP420.yuv";
char *dst_name = "ive_csc_res.yuv";
char *ref_dst_name = "./ive_data/result/sample_CSC_YUV2RGB.rgb";
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

void cscMode_to_str(csc_type_t csc_type, char* res){
    switch(csc_type){
        case CSC_YCbCr2RGB_BT601:
            strcpy(res, "CSC_YCbCr2RGB_BT601");
            break;

        case CSC_YPbPr2RGB_BT601:
            strcpy(res, "CSC_YPbPr2RGB_BT601");
            break;

        case CSC_RGB2YCbCr_BT601:
            strcpy(res, "CSC_RGB2YCbCr_BT601");
            break;

        case CSC_YCbCr2RGB_BT709:
            strcpy(res, "CSC_YCbCr2RGB_BT709");
            break;

        case CSC_RGB2YCbCr_BT709:
            strcpy(res, "CSC_RGB2YCbCr_BT709");
            break;

        case CSC_RGB2YPbPr_BT601:
            strcpy(res, "CSC_RGB2YPbPr_BT601");
            break;

        case CSC_YPbPr2RGB_BT709:
            strcpy(res, "CSC_YPbPr2RGB_BT709");
            break;

        case CSC_RGB2YPbPr_BT709:
            strcpy(res, "CSC_RGB2YPbPr_BT709");
            break;

        default:
            printf("Not found such mode \n");
            break;
    }
}

static void * ive_csc(void* arg){
    bm_status_t ret;
    ive_csc_ctx ctx = *(ive_csc_ctx*)arg;
    bm_image src, dst;
    int src_stride[4], dst_stride[4];
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
    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
    bm_ive_image_calc_stride(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, dst_stride);

    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        exit(-1);
    }

    bm_ive_read_bin(src, src_name);

    int image_byte_size[4] = {0};
    bm_image_get_byte_size(dst, image_byte_size);
    int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
    memset(output_ptr, 0, sizeof(byte_size));

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ive_csc(handle, src, dst, csc_type);
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
            printf("bmcv_ive_csc failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
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

        int cmp = cmp_u8(ref_dst_name, output_ptr, byte_size);
        if(cmp != 0){
            printf("[bmcv ive csc] csc_res cmp failed \n");
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            free(output_ptr);
            exit(-1);
        }

        if(bWrite){
            FILE *csc_fp = fopen(dst_name, "wb");
            fwrite((void *)output_ptr, 1, byte_size, csc_fp);
            fclose(csc_fp);
        }
    }

    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    free(output_ptr);

    char srcFmt_str[100], dstFmt_str[100], cscMode_str[100];
    format_to_str(src.image_format, srcFmt_str);
    format_to_str(dst.image_format, dstFmt_str);
    cscMode_to_str(csc_type, cscMode_str);

    printf("bmcv_ive_csc: %s -->> %s, csc_mode is %s , size %d x %d \n",
            srcFmt_str, dstFmt_str, cscMode_str, width, height);
    printf("idx:%d, bmcv_ive_csc: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive csc test successful \n");

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 8 ){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        src_fmt = atoi(argv[3]);
        csc_type = atoi(argv[4]);
        dst_fmt = atoi(argv[5]);
        src_name = argv[6];
        ref_dst_name = argv[7];
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
    } else if (argc > 3 && argc < 7) {
        printf("command input error, please follow this order:\n \
        %s width height src_fmt csc_type dst_fmt src_name goldenDst_name dev_id thread_num loop_num bWrite dst_name\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive csc] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST ive csc] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_csc_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_csc, (void *)(ctx + i))) {
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