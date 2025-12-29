#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

extern void bm_read_bin(bm_image src, const char *input_name);
extern void bm_write_bin(bm_image dst, const char *output_name);
extern int  md5_cmp(unsigned char *got, unsigned char *exp, int size);

typedef struct rotate_ctx_ {
    int loop;
    int i;
} rotate_ctx;

int                 test_loop_times  = 1;
int                 test_threads_num = 1;
int                 rot_angle   = 90;
int                 src_w = 1920, src_h = 1088, dst_w = 1088, dst_h = 1920, dev_id = 0;
bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
//char               *src_name = "/opt/sophon/libsophon-current/bin/res/1920x1088_yuv420.bin", *dst_name = "dst.bin";
char               *src_name = NULL, *dst_name = NULL;
bm_handle_t         handle = NULL;
pthread_mutex_t     mutex;
pthread_cond_t      condition;
// char               *md5 = "4076d67100f06a6cb2170007858d289f";
char               *md5 = NULL;

static void *rotate(void *arg) {
    bm_status_t        ret;
    rotate_ctx         ctx = *(rotate_ctx *)arg;
    bm_image           src, dst;
    unsigned int       i = 0, loop_time        = 0;
    unsigned long long time_total = 0, time_avg = 0;
    unsigned long long fps_actual = 0, pixel_per_sec = 0;
#if SLEEP_ON
    int fps        = 60;
    int sleep_time = 1000000 / fps;
#endif

    struct timeval tv_start;
    struct timeval tv_end;
    loop_time = ctx.loop;

    if (rot_angle == 180) {
        dst_w = src_w;
        dst_h = src_h;
    }
    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }

    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
        exit(-1);
    }

    if (src_name == NULL) {
        int image_byte_size[4] = {0};
        for (int i = 0; i < src.image_private->plane_num; i++) {
            image_byte_size[i] = src.image_private->memory_layout[i].size;
        }
        int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        char* input_ptr = (char *)malloc(byte_size);

        for (int i = 0; i < byte_size; ++i) {
            input_ptr[i] = rand() % 128;
        }

        void* in_ptr[4] = {(void*)input_ptr,
                        (void*)((char*)input_ptr + image_byte_size[0]),
                        (void*)((char*)input_ptr + image_byte_size[0] + image_byte_size[1]),
                        (void*)((char*)input_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_host_to_device(src, (void **)in_ptr);
        free(input_ptr);
    } else {
        bm_read_bin(src, src_name);
    }

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);

        bmcv_image_rotate(handle, src, dst, rot_angle);

        gettimeofday(&tv_end, NULL);
        printf("add_weighted TPU using time = %ld(us)\n", TIME_COST_US(tv_start, tv_end));

        time_total += TIME_COST_US(tv_start, tv_end);
    }
    time_avg      = time_total / loop_time;
    fps_actual    = 1000000 / time_avg;
    pixel_per_sec = src_w * src_h * fps_actual / 1024 / 1024;

    if (md5 == NULL) {
        if (dst_name != NULL) {
            bm_write_bin(dst, dst_name);
        }
    } else {
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(dst, image_byte_size);
        int            byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char *output_ptr = (unsigned char *)malloc(byte_size);
        void          *out_ptr[4] = {(void *)output_ptr,
                                     (void *)((unsigned char *)output_ptr + image_byte_size[0]),
                                     (void *)((unsigned char *)output_ptr + image_byte_size[0] + image_byte_size[1]),
                                     (void *)((unsigned char *)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_device_to_host(dst, (void **)out_ptr);
        if (md5_cmp(output_ptr, (unsigned char *)md5, byte_size) != 0) {
            bm_write_bin(dst, "error_cmp.bin");
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }
        free(output_ptr);
    }
    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    char src_fmt_str[100], dst_fmt_str[100];
    format_to_str(src.image_format, src_fmt_str);
    format_to_str(dst.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s\n", ctx.i, src_w, src_h, dst_w, dst_h, src_fmt_str, dst_fmt_str);
    printf("idx:%d, bmcv_image_rotate:loop %d cycles, time_avg = %llu, fps %llu, %lluM pps\n",
           ctx.i, loop_time, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

static void print_help(char **argv) {
    printf("please follow this order:\n \
        %s src_w src_h format rot_angle src_name dst_name [dev_id] [thread_num] [loop_num] [md5]\n \
        %s thread_num loop_num\n",
           argv[0], argv[0]);
};

int main(int argc, char **argv) {
    if (argc >= 11) {
        md5 = argv[10];
    } else if (argc > 3) {
        md5 = NULL;
    }
    if (argc >= 10) {
        test_loop_times = atoi(argv[9]);
    }
    if (argc >= 9) {
        test_threads_num = atoi(argv[8]);
    }
    if (argc >= 8) {
        dev_id = atoi(argv[7]);
    }

    if (argc >= 7) {
        src_name  = argv[5];
        dst_name  = argv[6];
    }

    if (argc >= 5) {
        src_w     = atoi(argv[1]);
        src_h     = atoi(argv[2]);
        dst_w     = src_h;
        dst_h     = src_w;
        src_fmt   = (bm_image_format_ext)atoi(argv[3]);
        rot_angle = atoi(argv[4]);
        dst_fmt   = src_fmt;
    }

    if (argc == 2) {
        if (atoi(argv[1]) < 0) {
            print_help(argv);
            exit(-1);
        } else {
            test_threads_num = atoi(argv[1]);
        }
    } else if (argc == 3) {
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if (argc > 3 && argc < 5) {
        printf("command input error\n");
        print_help(argv);
        exit(-1);
    }

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);
    rotate_ctx ctx[test_threads_num];
#ifdef __linux__
    pthread_t *pid = (pthread_t *)malloc(sizeof(pthread_t) * test_threads_num);
    for (int i = 0; i < test_threads_num; i++) {
        ctx[i].i    = i;
        ctx[i].loop = test_loop_times;
        if (pthread_create(&pid[i], NULL, rotate, (void *)(ctx + i))) {
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
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condition);
    bm_dev_free(handle);
    printf("--------ALL THREADS TEST OVER---------\n");
    free(pid);
#endif

    return 0;
}
