#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0

extern void bm_read_bin(bm_image src, const char *input_name);
extern void bm_write_bin(bm_image dst, const char *output_name);
extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

typedef struct convert_ctx_{
    int loop;
    int i;
}convert_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int src_h = 1080, src_w = 1920, dst_w = 2048, dst_h = 2048, dev_id = 0;
bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
char *src_name = "/opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin", *dst_name = "out/pad_2048x2048_yuv420.bin";
bmcv_rect_t rect = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
bmcv_padding_attr_t padding_attr = {.if_memset = 1, .padding_b = 0, .padding_g = 0, .padding_r = 255, .dst_crop_stx = 0, .dst_crop_sty = 0, .dst_crop_w = 1920, .dst_crop_h = 1080};
bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
bm_handle_t handle = NULL;
char *md5 = "edc78e4ad83709a51395cb7cc4ba3d19";

static void * pad(void* arg) {
    bm_status_t ret;
    convert_ctx ctx = *(convert_ctx*)arg;
    bm_image src, dst;
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

    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

    ret = bm_image_alloc_dev_mem(src,BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    ret = bm_image_alloc_dev_mem(dst,BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }
    bm_read_bin(src,src_name);

    for(i = 0;i < loop_time; i++){
        gettimeofday(&tv_start, NULL);

        bmcv_image_vpp_convert_padding(handle, 1, src, &dst, &padding_attr, &rect, algorithm);

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
    }
    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = src_w * src_h * fps_actual/1024/1024;

    if(md5 == NULL)
        bm_write_bin(dst, dst_name);
    else{
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(dst, image_byte_size);
        int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_device_to_host(dst, (void **)out_ptr);
        if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
            bm_write_bin(dst, "error_cmp.bin");
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }
        free(output_ptr);
    }
    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    char src_fmt_str[100],dst_fmt_str[100],algorithm_str[100];
    format_to_str(src.image_format, src_fmt_str);
    format_to_str(dst.image_format, dst_fmt_str);
    algorithm_to_str(algorithm, algorithm_str);


    printf("idx:%d, %d*%d->%d*%d, %s->%s,%s\n",ctx.i,src_w,src_h,dst_w,dst_h,src_fmt_str,dst_fmt_str,algorithm_str);
    printf("idx:%d, bmcv_image_vpp_convert_padding:loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

static void print_help(char **argv){
    printf("please follow this order:\n \
        %s src_w src_h src_fmt src_name pad_start_x pad_start_y pad_w pad_h dst_w dst_h dst_fmt dst_name algorithm dev_id thread_num loop_num md5\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
};

int main(int argc, char **argv) {
    if (argc >= 18) {
        md5 = argv[17];
    } else if(argc > 3){
        md5 = NULL;
    }
    if (argc >= 17) {
        test_threads_num = atoi(argv[15]);
        test_loop_times  = atoi(argv[16]);
    }
    if (argc >= 15) {
        src_w = atoi(argv[1]);
        src_h = atoi(argv[2]);
        src_fmt = (bm_image_format_ext)atoi(argv[3]);
        src_name = argv[4];
        padding_attr.dst_crop_stx = atoi(argv[5]);
        padding_attr.dst_crop_sty = atoi(argv[6]);
        padding_attr.dst_crop_w = atoi(argv[7]);
        padding_attr.dst_crop_h = atoi(argv[8]);
        rect.start_x = 0;
        rect.start_y = 0;
        rect.crop_w = src_w;
        rect.crop_h = src_h;
        dst_w = atoi(argv[9]);
        dst_h = atoi(argv[10]);
        dst_fmt = (bm_image_format_ext)atoi(argv[11]);
        dst_name = argv[12];
        algorithm = (bmcv_resize_algorithm)(atoi(argv[13]));
        dev_id = atoi(argv[14]);
    }
    if (argc == 2){
        if (atoi(argv[1]) < 0){
            print_help(argv);
            exit(-1);
        } else
            test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if (argc > 3 && argc < 15) {
        printf("command input error\n");
        print_help(argv);
        exit(-1);
    }
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    convert_ctx ctx[test_threads_num];
    #ifdef __linux__
    pthread_t *          pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
    for (int i = 0; i < test_threads_num; i++) {
        ctx[i].i = i;
        ctx[i].loop = test_loop_times;
        if (pthread_create(
                &pid[i], NULL, pad, (void *)(ctx + i))) {
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

