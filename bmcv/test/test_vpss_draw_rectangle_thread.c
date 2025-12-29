#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "md5.h"
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
int src_h = 1080, src_w = 1920, dev_id = 0;
bm_image_format_ext src_fmt = FORMAT_RGB_PACKED;
char *src_name = "/opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin", *dst_name = "out/draw_rect_1920x1080_rgb.bin";
bmcv_rect_t rect[9] = {{.start_x = 0, .start_y = 0, .crop_w = 500, .crop_h = 300},
                        {.start_x = 640, .start_y = 0, .crop_w = 500, .crop_h = 300},
                        {.start_x = 1280, .start_y = 0, .crop_w = 500, .crop_h = 300},
                        {.start_x = 0, .start_y = 360, .crop_w = 500, .crop_h = 300},
                        {.start_x = 640, .start_y = 360, .crop_w = 500, .crop_h = 300},
                        {.start_x = 1280, .start_y = 360, .crop_w = 500, .crop_h = 300},
                        {.start_x = 0, .start_y = 720, .crop_w = 500, .crop_h = 300},
                        {.start_x = 640, .start_y = 720, .crop_w = 500, .crop_h = 300},
                        {.start_x = 1280, .start_y = 720, .crop_w = 500, .crop_h = 300}};
int rect_num = 9;
bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
bm_handle_t handle = NULL;
char *md5 = "6848abc0d1ba1e378aae2e0f38dcd10c";

static void * draw_rect(void* arg) {
    bm_status_t ret;
    convert_ctx ctx = *(convert_ctx*)arg;
    bm_image src;
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

    ret = bm_image_alloc_dev_mem(src,BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    bm_read_bin(src,src_name);

    for(i = 0;i < loop_time; i++){
        gettimeofday(&tv_start, NULL);

        bmcv_image_draw_rectangle(handle, src, rect_num, rect, 10, 255, 0, 0);

        gettimeofday(&tv_end, NULL);
        if(i == 0){
            if(md5 == NULL)
                bm_write_bin(src, dst_name);
            else{
                int image_byte_size[4] = {0};
                bm_image_get_byte_size(src, image_byte_size);
                int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
                unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
                void* out_ptr[4] = {(void*)output_ptr,
                                    (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                                    (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                                    (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
                bm_image_copy_device_to_host(src, (void **)out_ptr);
                if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
                    bm_write_bin(src, "error_cmp.bin");
                    bm_image_destroy(&src);
                    exit(-1);
                }
                free(output_ptr);
            }
        }
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

    bm_image_destroy(&src);

    char src_fmt_str[100],algorithm_str[100];
    format_to_str(src.image_format, src_fmt_str);
    algorithm_to_str(algorithm, algorithm_str);


    printf("idx:%d, %d*%d->%d*%d, %s->%s,%s\n",ctx.i,src_w,src_h,src_w,src_h,src_fmt_str,src_fmt_str,algorithm_str);
    printf("idx:%d, bmcv_image_draw_rectangle:loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

static void print_help(char **argv){
    printf("please follow this order:\n \
        %s src_w src_h src_fmt src_name start_x start_y rect_w rect_h dst_name dev_id thread_num loop_num md5\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
};

int main(int argc, char **argv) {
    if (argc >= 14) {
        md5 = argv[13];
    } else if(argc > 3){
        md5 = NULL;
    }
    if (argc >= 13) {
        test_threads_num = atoi(argv[11]);
        test_loop_times  = atoi(argv[12]);
    }
    if (argc >= 11) {
        src_w = atoi(argv[1]);
        src_h = atoi(argv[2]);
        src_fmt = (bm_image_format_ext)atoi(argv[3]);
        src_name = argv[4];
        rect[0].start_x = atoi(argv[5]);
        rect[0].start_y = atoi(argv[6]);
        rect[0].crop_w = atoi(argv[7]);
        rect[0].crop_h = atoi(argv[8]);
        dst_name = argv[9];
        dev_id = atoi(argv[10]);
        rect_num = 1;
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
    } else if (argc > 3 && argc < 11) {
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
                &pid[i], NULL, draw_rect, (void *)(ctx + i))) {
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

