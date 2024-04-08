#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0
#define IMG_NUM 3

extern void bm_ive_read_bin(bm_image src, const char *input_name);
extern void bm_ive_write_bin(bm_image dst, const char *output_name);
extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);

typedef struct ive_resize_ctx_{
    int loop;
    int i;
} ive_resize_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
bmcv_resize_algorithm resize_mode = BMCV_INTER_AREA;
bm_image_format_ext format[IMG_NUM] = {FORMAT_RGBP_SEPARATE, FORMAT_GRAY, FORMAT_RGB_PLANAR};
int src_widht = 352, src_height = 288;
int dst_width[IMG_NUM] = {176, 400, 320};
int dst_height[IMG_NUM] = {144, 300, 240};

char *src_name[IMG_NUM] = {"./ive_data/campus_352x288.rgb", "./ive_data/campus_352x288.gray", "./ive_data/campus_352x288.rgb"};
char *dst_name[IMG_NUM] = {"ive_res1.yuv", "ive_res2.yuv", "ive_res3.yuv"};
char *ref_name[IMG_NUM] = {"./ive_data/result/sample_Resize_Area_rgb.rgb",
                   "./ive_data/result/sample_Resize_Area_gray.yuv",
                   "./ive_data/result/sample_Resize_Area_240p.rgb"};
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

void resizeMode_to_str(bmcv_resize_algorithm enMode, char* res){
    switch(enMode){
        case BMCV_INTER_LINEAR:
            strcpy(res, "BMCV_INTER_LINEAR");
            break;
        case BMCV_INTER_AREA:
            strcpy(res, "BMCV_INTER_LINEAR");
            break;
        default:
            printf("Not support such mode in ive \n");
            break;
    }
}

static void * ive_resize(void* arg){
    bm_status_t ret;
    ive_resize_ctx ctx = *(ive_resize_ctx*)arg;
    bm_image src[IMG_NUM], dst[IMG_NUM];
    int src_stride[IMG_NUM][4], dst_stride[IMG_NUM][4];
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
    for(int img_idx = 0; img_idx < IMG_NUM; img_idx++){
        bm_ive_image_calc_stride(handle, src_height, src_widht,
                  format[img_idx], DATA_TYPE_EXT_1N_BYTE, src_stride[img_idx]);
        bm_image_create(handle, src_height, src_widht, format[img_idx],
                  DATA_TYPE_EXT_1N_BYTE, &src[img_idx], src_stride[img_idx]);

        bm_ive_image_calc_stride(handle, dst_height[img_idx], dst_width[img_idx],
                  format[img_idx], DATA_TYPE_EXT_1N_BYTE, dst_stride[img_idx]);
        bm_image_create(handle, dst_height[img_idx], dst_width[img_idx], format[img_idx],
                  DATA_TYPE_EXT_1N_BYTE, &dst[img_idx], dst_stride[img_idx]);

        ret = bm_image_alloc_dev_mem(src[img_idx], BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src[%d] bm_image_alloc_dev_mem failed. ret = %d\n", img_idx, ret);
            for(int idx = 0; idx < img_idx; idx++){
                bm_image_destroy(&src[idx]);
                bm_image_destroy(&dst[idx]);
            }
            exit(-1);
        }

        ret = bm_image_alloc_dev_mem(dst[img_idx], BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("dst[%d] bm_image_alloc_dev_mem failed. ret = %d\n", img_idx, ret);
            for(int idx = 0; idx < img_idx; idx++){
                bm_image_destroy(&src[idx]);
                bm_image_destroy(&dst[idx]);
            }
            exit(-1);
        }

        bm_ive_read_bin(src[img_idx], src_name[img_idx]);
    }

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        for (int idx=0; idx<IMG_NUM; idx++){
            ret = bmcv_ive_resize(handle, src[idx], dst[idx], resize_mode);
        }
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
            printf("bmcv_ive_resize failed ,ret is %d \n", ret);
            for(int idx = 0; idx < IMG_NUM; idx++){
                bm_image_destroy(&src[idx]);
                bm_image_destroy(&dst[idx]);
            }
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;

    if(ctx.i == 0){
         for(int j = 0; j < IMG_NUM; j++){
            int image_byte_size[4] = {0};
            bm_image_get_byte_size(dst[j], image_byte_size);
            int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
            unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
            memset(output_ptr, 0, sizeof(byte_size));

            void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((char*)output_ptr + image_byte_size[0]),
                            (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                            (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

            ret = bm_image_copy_device_to_host(dst[j], (void **)out_ptr);
            if(ret != BM_SUCCESS){
                printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
                for(int idx = 0; idx < IMG_NUM; idx++){
                    bm_image_destroy(&src[idx]);
                    bm_image_destroy(&dst[idx]);
                }
                free(output_ptr);
                exit(-1);
            }

            int cmp = cmp_u8(ref_name[j], output_ptr, byte_size);
            if(cmp != 0){
                printf("[bmcv ive resize] [img_num %d] cmp failed \n", j);
                for(int idx = 0; idx < IMG_NUM; idx++){
                    bm_image_destroy(&src[idx]);
                    bm_image_destroy(&dst[idx]);
                }
                free(output_ptr);
                exit(-1);
            }
            printf("[bmcv ive resize] [img_num %d] cmp succeed \n", j);

            if(bWrite){
                FILE *ive_fp = fopen(dst_name[j], "wb");
                fwrite((void *)output_ptr, 1, byte_size, ive_fp);
                fclose(ive_fp);
            }
            free(output_ptr);
        }
    }

    for(int idx = 0; idx < IMG_NUM; idx++){
        bm_image_destroy(&src[idx]);
        bm_image_destroy(&dst[idx]);
    }

    char fmt_str[IMG_NUM][100], enMode_str[100];
    resizeMode_to_str(resize_mode, enMode_str);

    for(int img_idx = 0; img_idx < IMG_NUM; img_idx++){
        format_to_str(src[img_idx].image_format, fmt_str[img_idx]);
        printf("bmcv_ive_resize: fmt is %s, resize_mode is %s, size is %d x %d -->> %d x %d\n",
               fmt_str[img_idx], enMode_str, src_widht, src_height, dst_width[img_idx], dst_height[img_idx]);
    }
    printf("idx:%d, bmcv_ive_resize: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            ctx.i, loop_time, time_max, time_avg, fps_actual);
    printf("bmcv ive resize test successful \n");

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 5 ){
        resize_mode = atoi(argv[1]);
        ref_name[0] = argv[2];
        ref_name[1] = argv[3];
        ref_name[2] = argv[4];
        if(argc > 5) bWrite = atoi(argv[5]);
        if(argc > 6) dev_id = atoi(argv[6]);
        if(argc > 7) test_threads_num  = atoi(argv[7]);
        if(argc > 8) test_loop_times  = atoi(argv[8]);
    }

    if (argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if (argc > 3 && argc < 5) {
        printf("command input error, please follow this order:\n \
        %s resize_mode ref_name1 ref_name2 ref_name3 bWrite dev_id thread_num loop_num\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive resize] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > 20 || test_threads_num < 1) {
        printf("[TEST ive resize] thread nums should be 1~20\n");
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_resize_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_resize, (void *)(ctx + i))) {
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