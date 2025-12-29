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

typedef struct ive_map_ctx_{
    int loop;
    int i;
} ive_map_ctx;

static unsigned char FixMap[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03,
    0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0F, 0x10, 0x11, 0x12, 0x14, 0x15, 0x17, 0x18,
    0x1A, 0x1B, 0x1D, 0x1E, 0x20, 0x21, 0x23, 0x24, 0x26, 0x27,
    0x29, 0x2A, 0x2C, 0x2D, 0x2F, 0x31, 0x32, 0x34, 0x35, 0x37,
    0x38, 0x3A, 0x3B, 0x3D, 0x3E, 0x40, 0x41, 0x43, 0x44, 0x45,
    0x47, 0x48, 0x4A, 0x4B, 0x4D, 0x4E, 0x50, 0x51, 0x52, 0x54,
    0x55, 0x56, 0x58, 0x59, 0x5A, 0x5B, 0x5D, 0x5E, 0x5F, 0x60,
    0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x69, 0x6A, 0x6B, 0x6D,
    0x6E, 0x70, 0x71, 0x73, 0x75, 0x76, 0x78, 0x7A, 0x7B, 0x7D,
    0x7E, 0x80, 0x81, 0x83, 0x84, 0x86, 0x87, 0x88, 0x89, 0x8B,
    0x8C, 0x8D, 0x8E, 0x90, 0x92, 0x94, 0x97, 0x9A, 0x9C, 0x9E,
    0xA1, 0xA3, 0xA5, 0xA6, 0xA7, 0xA9, 0xAA, 0xAB, 0xAC, 0xAC,
    0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB3, 0xB4, 0xB5, 0xB7, 0xB9,
    0xBB, 0xBD, 0xBF, 0xC1, 0xC4, 0xC7, 0xCC, 0xD1, 0xD5, 0xDA,
    0xDE, 0xE0, 0xE2, 0xE3, 0xE4, 0xE5, 0xE5, 0xE6, 0xE6, 0xE6,
    0xE6, 0xE6, 0xE7, 0xE7, 0xE7, 0xE8, 0xE8, 0xE9, 0xEA, 0xEC,
    0xED, 0xEE, 0xF0, 0xF2, 0xF4, 0xF5, 0xF7, 0xF8, 0xFA, 0xFB,
    0xFD, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int height = 288, width = 352;
bmcv_ive_map_mode map_mode = IVE_MAP_U8; /* IVE_MAP_MODE_U8 */
bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
char *src_name = "./ive_data/00_352x288_y.yuv", *dst_name = "./out/sample_Map.yuv";
char *ref_name = "./ive_data/result/sample_Map.yuv";
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

static void * ive_map(void* arg) {
    bm_status_t ret;
    ive_map_ctx ctx = *(ive_map_ctx*)arg;
    bm_image src, dst;
    bm_device_mem_t mapTable;
    int src_stride[4];
    int dst_stride[4];
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

    // calc ive image stride
    bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);

    if (map_mode == 0) { /* IVE_MAP_MODE_U8 */
        bm_ive_image_calc_stride(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, dst_stride);
    } else if (map_mode == 1) { /* IVE_MAP_MODE_S16 */
        bm_ive_image_calc_stride(handle, height, width, dst_fmt, DATA_TYPE_EXT_S16, dst_stride);
    } else if (map_mode == 2) { /* IVE_MAP_MODE_U16 */
        bm_ive_image_calc_stride(handle, height, width, dst_fmt, DATA_TYPE_EXT_U16, dst_stride);
    } else {
        printf("unknown map mode %d\n", map_mode);
        exit(-1);
    }

    // create bm image struct
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    if (map_mode == 0) {
        bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);
        printf("dst datatype = %d \n", dst.data_type);
    } else if (map_mode == 1) {
        bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_S16, &dst, dst_stride);
    } else if (map_mode == 2) {
        bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_U16, &dst, dst_stride);
    }

    // alloc bm image memory
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src failed. ret = %d\n", ret);
        exit(-1);
    }
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst failed. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_malloc_device_byte(handle, &mapTable, MAP_TABLE_SIZE);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst failed. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_memcpy_s2d(handle, mapTable, FixMap);
    if(ret != BM_SUCCESS){
        printf("bm_memcpy_s2d failed . ret = %d\n", ret);
        exit(-1);
    }

    // read image data from input files
    bm_ive_read_bin(src, src_name);

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);

        ret = bmcv_ive_map(handle, src, dst, mapTable);

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
            printf("bmcv_ive_map failed, ret is %d \n", ret);
            exit(-1);
        }
    }
    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = width * height * fps_actual/1024/1024;

    if(ctx.i == 0){
        if(map_mode == 0){
            unsigned char* ive_res = (unsigned char*) malloc (width * height * sizeof(unsigned char));
            memset(ive_res, 0, width * height * sizeof(unsigned char));

            ret = bm_image_copy_device_to_host(dst, (void**)&ive_res);
            if(ret != BM_SUCCESS){
                printf("dst bm_image_copy_device_to_host is failed \n");
                exit(-1);
            }

            int cmp = cmp_u8(ref_name, ive_res, width * height * sizeof(unsigned char));
            if(cmp != 0){
                printf("[bmcv ive map] cmp failed, cmp = %d \n", cmp);
            } else {
                printf("[bmcv ive map] cmp successful, cmp = %d \n", cmp);
            }

            if(bWrite){
                FILE *fp = fopen(dst_name, "wb");
                fwrite((void *)ive_res, 1, width * height * sizeof(unsigned char), fp);
                fclose(fp);
            }
            free(ive_res);

        } else if (map_mode == 1){
            signed short *ive_res_s16 = (signed short*) malloc(width * height * sizeof(signed short));
            memset(ive_res_s16, 0, width * height * sizeof(signed short));

            ret = bm_image_copy_device_to_host(dst, (void**)&ive_res_s16);
            if(ret != BM_SUCCESS){
                printf("dst bm_image_copy_device_to_host is failed \n");
                exit(-1);
            }

            int cmp = cmp_u8(ref_name, (unsigned char*)ive_res_s16, width * height * sizeof(signed short));
            if(cmp != 0){
                printf("[bmcv ive map] cmp failed, cmp = %d \n", cmp);
            } else {
                printf("[bmcv ive map] cmp successful, cmp = %d \n", cmp);
            }

            if(bWrite){
                FILE *fp = fopen(dst_name, "wb");
                fwrite((void *)ive_res_s16, sizeof(signed short), width * height, fp);
                fclose(fp);
            }
            free(ive_res_s16);
        } else if (map_mode == 2){
            unsigned short *ive_res_u16 = (unsigned short*) malloc(width * height * sizeof(unsigned short));
            memset(ive_res_u16, 0, width * height * sizeof(unsigned short));

            ret = bm_image_copy_device_to_host(dst, (void**)&ive_res_u16);
            if(ret != BM_SUCCESS){
                printf("dst bm_image_copy_device_to_host is failed \n");
                exit(-1);
            }

            int cmp = cmp_u8(ref_name, (unsigned char*)ive_res_u16, width * height * sizeof(unsigned short));
            if(cmp != 0){
                printf("[bmcv ive map] cmp failed, cmp = %d \n", cmp);
            } else {
                printf("[bmcv ive map] cmp successful, cmp = %d \n", cmp);
            }

            if(bWrite){
                FILE *fp = fopen(dst_name, "wb");
                fwrite((void *)ive_res_u16, sizeof(signed short), width * height, fp);
                fclose(fp);
            }
            free(ive_res_u16);

        } else {
            printf("unknown map mode %d\n", map_mode);
            exit(-1);
        }

    }

    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    bm_free_device(handle, mapTable);

    char algorithm_str[100] = "ive_map";
    char src_fmt_str[100],dst_fmt_str[100];
    format_to_str(src.image_format, src_fmt_str);
    format_to_str(dst.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s,%s\n",ctx.i,width,height,width,height,src_fmt_str,dst_fmt_str,algorithm_str);
    printf("idx:%d, bm_ive_map: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

int main(int argc, char **argv) {
    if(argc >= 8){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        map_mode = atoi(argv[3]);
        src_fmt = (bm_image_format_ext)atoi(argv[4]);
        dst_fmt = (bm_image_format_ext)atoi(argv[5]);
        src_name = argv[6];
        ref_name = argv[7];
        if(argc > 8) dev_id = atoi(argv[8]);
        if(argc > 9) test_threads_num = atoi(argv[9]);
        if(argc > 10) test_loop_times  = atoi(argv[10]);
        if(argc > 11) bWrite = atoi(argv[11]);
        if(argc > 12) dst_name = argv[12];
    }
    if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    }
    if ((argc > 3 && argc < 7) || (argc == 1)) {
        printf("please follow this order to input command:\n \
        %s width height map_mode src_fmt dst_fmt src_name ref_name dev_id thread_num loop_num bWrite dst_name\n \
        %s 352 288 0 14 14 ive_data/00_352x288_y.yuv ive_data/result/sample_Map.yuv 0 1 5 0\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST ive map] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > MAX_THREAD_NUM || test_threads_num < 1) {
        printf("[TEST ive map] thread nums should be 1~%d\n", MAX_THREAD_NUM);
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    ive_map_ctx ctx[MAX_THREAD_NUM];
    #ifdef __linux__
        pthread_t pid[MAX_THREAD_NUM];
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ive_map, (void *)(ctx + i))) {
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