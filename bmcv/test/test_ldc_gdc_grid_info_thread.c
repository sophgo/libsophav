#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0
#define MAX_THREAD_NUM 20
#define LDC_ALIGN 64
#define ALIGN(x, a)      (((x) + ((a)-1)) & ~((a)-1))
typedef unsigned int         u32;

extern void bm_read_bin(bm_image src, const char *input_name);
extern void bm_write_bin(bm_image dst, const char *output_name);
extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);
extern bm_status_t bm_ldc_image_calc_stride(bm_handle_t handle,
                                            int img_h,
                                            int img_w,
                                            bm_image_format_ext image_format,
                                            bm_image_data_format_ext data_type,
                                            int *stride);

typedef struct ldc_gdc_ctx_{
    int loop;
    int i;
} ldc_gdc_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int src_h = 1080, src_w = 1920, dst_h = 0, dst_w = 0;
bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
bm_image_data_format_ext input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
bmcv_gdc_attr stLDCAttr = {0};
char *src_name = "1920x1080.yuv", *dst_name = "output_1920x1080_rot0.yuv", *grid_name = "grid_info_79_44_3476_80_45_1280x720.dat";
bm_handle_t handle = NULL;
char *md5;
pthread_mutex_t mutex;
pthread_cond_t condition;
int counter = 0;
bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
bmcv_rect_t rect = {.start_x = 50, .start_y = 50, .crop_w = 1000, .crop_h = 600};

static void wait_pthread(void){
    // 加锁，确保对counter的操作是互斥的
    pthread_mutex_lock(&mutex);
    counter++;
    if (counter == test_threads_num) {
        // 当所有线程都到达节点时，发送信号通知其他线程
        pthread_cond_broadcast(&condition);
        counter = 0;
    } else {
        // 等待，直到所有线程都到达节点
        pthread_cond_wait(&condition, &mutex);
    }
    // 解锁
    pthread_mutex_unlock(&mutex);
}

static void * ldc_gdc_grid_info(void * arg)
{
    bm_status_t ret = BM_SUCCESS;
    ldc_gdc_ctx ctx = *(ldc_gdc_ctx*)arg;
    bm_image src, dst;
    int src_stride[4];
    int dst_stride[4];
    unsigned int /*i = 0,*/ loop_time = 0;
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

    // align
    if (src_h == 768) {
        src_h = ALIGN(src_h, LDC_ALIGN);
        dst_w = src_w;
        dst_h = ALIGN(src_h, LDC_ALIGN);
    } else if (src_h == 720)
    {
        dst_w = src_w;
        dst_h = ALIGN(src_h, LDC_ALIGN);
    }

    // calc image stride
    bm_ldc_image_calc_stride(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
    bm_ldc_image_calc_stride(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, dst_stride);

    // create bm image
    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if(ret != BM_SUCCESS){
        printf("bm_image_alloc_dev_mem_src failed \n");
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
    }

    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
    if(ret != BM_SUCCESS){
        printf("bm_image_alloc_dev_mem_dst failed \n");
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
    }

    // read image data from input files
    bm_read_bin(src, src_name);

    char *buffer = (char *)malloc(stLDCAttr.grid_info.size);
    if (buffer == NULL) {
        printf("malloc buffer for grid_info failed!\n");
        goto fail;
    }
    memset(buffer, 0, stLDCAttr.grid_info.size);

    FILE *fp = fopen(grid_name, "rb");
    if (!fp) {
        printf("open file:%s failed.\n", grid_name);
        goto fail;
    }

    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);

    if (stLDCAttr.grid_info.size != (u32)fileSize) {
        printf("load grid_info file:(%s) size is not match.\n", grid_name);
        fclose(fp);
        goto fail;
    }
    rewind(fp);

    fread(buffer, 1, stLDCAttr.grid_info.size, fp);

    fclose(fp);
    stLDCAttr.grid_info.u.system.system_addr = (void *)buffer;

    for (int i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ldc_gdc(handle, src, dst, stLDCAttr);
        if(ret != BM_SUCCESS) {
            printf("test bmcv_ldc_gdc failed \n");
            goto fail;
        }
        gettimeofday(&tv_end, NULL);
        timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
        timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
        time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

#if SLEEP_ON
        if(time_single < sleep_time) usleep((sleep_time - time_single));
        gettimeofday(&tv_end, NULL);
        timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
        timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
        time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
#endif
        if(time_single > time_max) {time_max = time_single;}
        if(time_single < time_min) {time_min = time_single;}
        time_total = time_total + time_single;
    }
    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = dst_h * dst_w * fps_actual/1024/1024;

    // vpss crop dst for md5 cmp
    bm_image crop_dst;
    int new_w = 950, new_h = 550;
    bm_image_create(handle, new_h, new_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &crop_dst, NULL);
    ret = bm_image_alloc_dev_mem(crop_dst, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        bm_image_destroy(&crop_dst);
        goto fail;
    }
    bmcv_image_vpp_csc_matrix_convert(handle, 1, dst, &crop_dst, CSC_MAX_ENUM, NULL, algorithm, &rect);

    if (ctx.i == 0) {
        if (md5 == NULL) {
            bm_write_bin(crop_dst, dst_name);
        } else {
            int image_byte_size[4] = {0};
            bm_image_get_byte_size(crop_dst, image_byte_size);
            int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
            unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
            void* out_ptr[4] = {(void*)output_ptr,
                                (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                                (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                                (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
            bm_image_copy_device_to_host(crop_dst, (void **)out_ptr);
            if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size) != 0) {
                bm_write_bin(crop_dst, "error_cmp.bin");
                exit(-1);
            }
            free(output_ptr);
        }
    }
    bm_image_destroy(&crop_dst);

    char algorithm_str[100] = "bmcv_ldc_gdc_grid_info";
    char src_fmt_str[100], dst_fmt_str[100];
    format_to_str(src.image_format, src_fmt_str);
    format_to_str(dst.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s,%s\n", ctx.i, src_w, src_h, new_w, new_h, src_fmt_str, dst_fmt_str, algorithm_str);
    printf("idx:%d, bmcv_ldc_gdc_grid_info: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
            ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

fail:
    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    free(buffer);
    return 0;
}

static void * ldc_gdc_grid_info_list(void* arg) {
    printf("-----------1280x768.yuv-----------\n");
    src_h = 768, src_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/1280x768.yuv", dst_name = "ldc_data/out_grid_info.yuv", grid_name = "ldc_data/grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "9fe88b54ed51a09a9d245b805b346435";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_00.yuv-----------\n");
    src_h = 720, src_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_00.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_00.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "c547898b0af47720760c4ef40145a772";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_01.yuv-----------\n");
    src_h = 720, src_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_01.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_01.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "21aa3c60abfae892379320dc1d2832f5";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_02.yuv-----------\n");
    src_h = 720, src_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_02.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_02.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "47d94ccb6493f89c382a41414f8eab34";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_03.yuv-----------\n");
    src_h = 720, src_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_03.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_03.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "af593594ed89a2885c0466691d933ead";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_04.yuv-----------\n");
    src_h = 720, src_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_04.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_04.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "527892257c5ac542eab9a90b04e4b163";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_05.yuv-----------\n");
    src_h = 720, src_w = 1280, dst_h = ALIGN(720, LDC_ALIGN), dst_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_05.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_05.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "9b791e34b454b4c4697b5dbce222a16c";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_06.yuv-----------\n");
    src_h = 720, src_w = 1280, dst_h = ALIGN(720, LDC_ALIGN), dst_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_06.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_06.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "17b375341975935683a70260aaa869d5";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_07.yuv-----------\n");
    src_h = 720, src_w = 1280, dst_h = ALIGN(720, LDC_ALIGN), dst_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_07.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_07.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "f23db442d0a0a4bfe8cc5bde869524c0";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_08.yuv-----------\n");
    src_h = 720, src_w = 1280, dst_h = ALIGN(720, LDC_ALIGN), dst_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_08.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_08.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "7356263e1d53330625f099a19ffea954";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    printf("-----------left_09.yuv-----------\n");
    src_h = 720, src_w = 1280, dst_h = ALIGN(720, LDC_ALIGN), dst_w = 1280;
    src_fmt = FORMAT_NV21, dst_fmt = FORMAT_NV21;
    input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
    stLDCAttr.grid_info.size = 336080;
    src_name = "ldc_data/grid_info_in_nv21/left_09.yuv", dst_name = "ldc_data/grid_info_out_nv21/out_left_09.yuv", grid_name = "ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat";
    md5 = "7e4c9499dfc89149808210d1ec0ecf87";
    ldc_gdc_grid_info(arg);
    wait_pthread();

    return 0;
}

int main(int argc, char **argv) {
    if (argc >= 12) {
        md5 = argv[11];
    } else if(argc > 3) {
        md5 = NULL;
    }
    if (argc >= 11) {
        test_threads_num = atoi(argv[9]);
        test_loop_times  = atoi(argv[10]);
    }
    if (argc >= 9) {
        src_name = argv[1];
        dst_name = argv[2];
        src_w = atoi(argv[3]);
        src_h = atoi(argv[4]);
        src_fmt = (bm_image_format_ext)atoi(argv[5]);
        dst_fmt = (bm_image_format_ext)atoi(argv[6]);
        grid_name = argv[7];
        stLDCAttr.grid_info.size = atoi(argv[8]);
    }
    if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    }

    if(argc == 1 || (argc > 3 && argc < 8)){
        printf("command input error, please follow this order:\n \
        %s src_name dst_name width height src_fmt dst_fmt grid_file_name grid_size test_threads_num test_loop_times\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
        exit(-1);
    }
    if (test_loop_times > 15000 || test_loop_times < 1) {
        printf("[TEST convert] loop times should be 1~15000\n");
        exit(-1);
    }
    if (test_threads_num > MAX_THREAD_NUM || test_threads_num < 1) {
        printf("[TEST convert] thread nums should be 1~%d\n", MAX_THREAD_NUM);
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);
    ldc_gdc_ctx ctx[MAX_THREAD_NUM];
#ifdef __linux__
    pthread_t pid[MAX_THREAD_NUM];
    for (int i = 0; i < test_threads_num; i++) {
        ctx[i].i = i;
        ctx[i].loop = test_loop_times;
        if (argc > 3) {
            if (pthread_create(
                    &pid[i], NULL, ldc_gdc_grid_info, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        } else {
            if (pthread_create(
                    &pid[i], NULL, ldc_gdc_grid_info_list, (void *)(ctx + i))) {
                perror("create thread failed\n");
                exit(-1);
            }
        }
    }
    for (int i = 0; i < test_threads_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            perror("Thread join failed");
            exit(-1);
        }
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condition);
    bm_dev_free(handle);
    printf("--------ALL THREADS TEST OVER---------\n");
#endif

    return 0;
}