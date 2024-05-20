#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "bmcv_api_ext_c.h"

extern void bm_read_bin(bm_image src, const char *input_name);
extern void bm_write_bin(bm_image dst, const char *output_name);
extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

typedef struct overlay_ctx_{
    int loop;
    int i;
} overlay_ctx;

int dev_id = 0;
int test_loop_times  = 1;
int test_threads_num = 1;
bm_image_format_ext image_fmt = FORMAT_RGB_PACKED;
bm_image_format_ext overlay_fmt = FORMAT_ARGB1555_PACKED;
int img_w = 1920, img_h = 1080;
int overlay_w = 80, overlay_h = 60;
int overlay_num = 1;
int x = 200, y = 200;
bool bWrite = false;
char *image_name = "./res/car_rgb888.rgb";
char *overlay_name = "./res/dog_s_80x60_pngto1555.bin";
char *md5 = "a858701cb01131f1356f80c76e7813c0";    // argb1555
bm_handle_t handle = NULL;

static void * vpss_overlay(void* arg){
    bm_status_t ret;
    overlay_ctx ctx = *(overlay_ctx*)arg;
    bm_image image;
    bm_image overlay_image[overlay_num];

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
    bmcv_rect_t overlay_info;
    memset(&overlay_info, 0, sizeof(bmcv_rect_t));

    overlay_info.start_x = x;
    overlay_info.start_y = y;
    overlay_info.crop_h = overlay_h;
    overlay_info.crop_w = overlay_w;

    // create bm image struct & alloc dev mem
    bm_image_create(handle, img_h, img_w, image_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);

    ret = bm_image_alloc_dev_mem(image, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("image bm_image_alloc_dev_mem failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_read_bin(image, image_name);

    for(int idx = 0; idx < overlay_num; idx++){
        bm_image_create(handle, overlay_h, overlay_w, overlay_fmt,
                               DATA_TYPE_EXT_1N_BYTE, &overlay_image[idx], NULL);

        ret = bm_image_alloc_dev_mem(overlay_image[idx], BMCV_HEAP_ANY);
        if(ret != BM_SUCCESS){
            printf("image bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_read_bin(overlay_image[idx], overlay_name);
    }

    for(i = 0;i < loop_time; i++){
        gettimeofday(&tv_start, NULL);
        ret = bmcv_image_overlay(handle, image, overlay_num, &overlay_info, overlay_image);
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
            printf("bmcv_image_overlay failed. ret = %d\n", ret);
            exit(-1);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = img_w * img_h * fps_actual/1024/1024;

    if(md5 == NULL){
        bm_write_bin(image, "./bmcv_image_overlay_res.bin");
    } else {
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(image, image_byte_size);
        int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_device_to_host(image, (void **)out_ptr);

        if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0){
            bm_write_bin(image, "error_cmp.bin");
            bm_image_destroy(&image);
            exit(-1);
        }
        free(output_ptr);
    }

    if(bWrite) bm_write_bin(image, "./bmcv_image_overlay_res.bin");
    // bm_write_bin(image, "./bmcv_image_overlay_argb4444_res.bin");

    char img_fmt_str[100], overlay_fmt_str[100];
    format_to_str(image.image_format, img_fmt_str);
    format_to_str(overlay_image[0].image_format, overlay_fmt_str);

    printf("idx:%d, bmcv_image_overlay: image fmt %s size %d x %d, overlay_num %d, x %d, y %d, overlay_img fmt %s size %d x %d \n",
        ctx.i, img_fmt_str, img_w, img_h, overlay_num, x, y, overlay_fmt_str, overlay_w, overlay_h);
    printf("idx:%d, bmcv_image_overlay: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

int main(int argc, char **argv){
    if(argc >= 11){
        img_w = atoi(argv[1]);
        img_h = atoi(argv[2]);
        image_fmt = atoi(argv[3]);
        overlay_w = atoi(argv[4]);
        overlay_h = atoi(argv[5]);
        overlay_fmt = atoi(argv[6]);
        x = atoi(argv[7]);
        y = atoi(argv[8]);
        image_name = argv[9];
        overlay_name = argv[10];
        md5 = argv[11];
        if(argc > 12) dev_id = atoi(argv[12]);
        if(argc > 13) bWrite = argv[13];
    }
    if (argc == 2)
        test_threads_num  = atoi(argv[1]);
    else if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if (argc > 3 && argc < 10) {
        printf("usage: %s img_w img_h image_fmt overlay_num overlay_w overlay_h overlay_fmt x y image_name overlay_name dev_id bWrite )\n", argv[0]);
        printf("example:\n");
        printf("%s 1920 1080 10 1 300 300 18 20 20 ./res/1920x1080_rgb.bin ./res/300x300_argb8888_dog.rgb 0 fasle\n", argv[0]);
        return -1;
    }

    bm_status_t ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    overlay_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, vpss_overlay, (void *)(ctx + i))) {
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