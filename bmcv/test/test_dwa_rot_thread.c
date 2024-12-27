#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0
#define IMG_MAX_HEIGHT 4096
#define IMG_MAX_WIDTH  4096
#define IMG_MIN_HEIGHT 32
#define IMG_MIN_WIDTH  32
#define DWA_ALIGN 32
#define ALIGN(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

extern void bm_read_bin(bm_image src, const char *input_name);
extern void bm_write_bin(bm_image dst, const char *output_name);
extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

typedef struct convert_ctx_{
    int loop;
    int i;
}convert_ctx;

int rand_mode = 0;
int loop_mode = 0;
int rand_input_width = 0, rand_input_height = 0;
int test_loop_times  = 1;
int test_threads_num = 1;
int src_h = 1080, src_w = 1920, dev_id = 0, dst_h = 1080, dst_w = 1920;
bm_image_format_ext fmt = FORMAT_YUV420P;
char *src_name = "/opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin", *dst_name = "dst.bin";
bm_handle_t handle = NULL;
bmcv_rot_mode rot_mode = BMCV_ROTATION_0;
char *md5 = "ad2ec6fe09dea2b4c7163b62c0fad5ce";

int get_image_data_size(int width, int height, bm_image_format_ext format) {
    int size;
    switch (format) {
        case FORMAT_YUV420P:
        case FORMAT_NV12:
        case FORMAT_NV21:
            size = width * height * 3 / 2;
            break;
        case FORMAT_YUV422P:
        case FORMAT_YUV422_YUYV:
        case FORMAT_YUV422_YVYU:
        case FORMAT_YUV422_UYVY:
        case FORMAT_YUV422_VYUY:
        case FORMAT_NV16:
        case FORMAT_NV61:
            size = width * height * 2;
            break;
        case FORMAT_YUV444P:
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
            size = width * height * 3;
            break;
        case FORMAT_GRAY:
            size = width * height;
            break;
        default:
            printf("Unknown format! \n");
            size = width * height * 3;  // Default case, adjust as needed
            break;
    }
    return size;
}

static void * dwa_rot(void* arg) {
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

    bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);

    if (rot_mode == BMCV_ROTATION_90 || rot_mode == BMCV_ROTATION_270) {
        dst_w = src_h;
        dst_h = src_w;
    } else {
        dst_w = src_w;
        dst_h = src_h;
    }
    bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

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

    int random_size = get_image_data_size(src_w, src_h, fmt);
    unsigned char *random_input_data = (unsigned char *)malloc(random_size);
    if (!random_input_data) {
        printf("Memory Allocation Failed \n");
        ret = BM_ERR_FAILURE;
        free(random_input_data);
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        exit(-1);
    }
    if (rand_mode == 0) {
        bm_read_bin(src,src_name);
    } else {
        for (int i = 0; i < random_size; i++) {
            random_input_data[i] = rand() % 256;
        }
        int random_image_byte_size[4] = {0};
        bm_image_get_byte_size(src, random_image_byte_size);

        void *random_in_ptr[4] = {(void *)random_input_data,
                            (void *)((char *)random_input_data + random_image_byte_size[0]),
                            (void *)((char *)random_input_data + random_image_byte_size[0] + random_image_byte_size[1]),
                            (void *)((char *)random_input_data + random_image_byte_size[0] + random_image_byte_size[1] + random_image_byte_size[2])};
        bm_image_copy_host_to_device(src, (void **)random_in_ptr);
    }


    for(i = 0;i < loop_time; i++){
        gettimeofday(&tv_start, NULL);

        bmcv_dwa_rot(handle, src, dst, rot_mode);

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

     // Only compare and save output data when in user config mode
    if (rand_mode == 0)
    {
        if(ctx.i == 0){
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
                if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size)!=0) {
                    bm_write_bin(dst, "error_cmp.bin");
                    exit(-1);
                }
                free(output_ptr);
            }
        }
    } else {
        free(random_input_data);
    }
    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    char algorithm_str[100] = "bmcv_dwa_rot";
    char fmt_str[100];
    format_to_str(src.image_format, fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s, %s\n",ctx.i,src_w,src_h,dst_w,dst_h,fmt_str, algorithm_str);
    printf("idx:%d, bmcv_dwa_rot:loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

static void print_help(char **argv){
    printf("please follow this order:\n \
        %s src_w src_h src_fmt src_name dst_w dst_h dst_name rot_mode thread_num loop_num md5 /*For user input test*/\n \
        %s rand_mode(1) thread_num loop_num         /*For random size test*/\n \
        %s loop_mode(1) rand_mode(1) width height   /*For size loop test*/\n", argv[0], argv[0], argv[0]);
};

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
        src_w = atoi(argv[1]);
        src_h = atoi(argv[2]);
        fmt = (bm_image_format_ext)atoi(argv[3]);
        src_name = argv[4];
        dst_w = atoi(argv[5]);
        dst_h = atoi(argv[6]);
        dst_name = argv[7];
        rot_mode = (bmcv_rot_mode)(atoi(argv[8]));
    }
    if (argc == 2) {
        if (atoi(argv[1]) < 0){
            print_help(argv);
            exit(-1);
        } else
            test_threads_num  = atoi(argv[1]);
    } else if (argc == 3) {
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    } else if (argc == 4) {
        rand_mode = atoi(argv[1]);
        test_threads_num = atoi(argv[2]);
        test_loop_times  = atoi(argv[3]);
    } else if (argc == 5) {
        rand_mode = atoi(argv[1]);
        loop_mode = atoi(argv[2]);
        rand_input_width = atoi(argv[3]);
        rand_input_height = atoi(argv[4]);
        for (src_w = rand_input_width; src_w <= IMG_MAX_WIDTH; src_w += 32) {
            for (src_h = rand_input_height; src_h <= IMG_MAX_HEIGHT; src_h += 32) {
                rot_mode = BMCV_ROTATION_0;
                fmt = FORMAT_RGB_PLANAR;
                dst_w = src_w;
                dst_w = src_h;
                printf("Size loop mode for rot: width = %d, height = %d, rot_mode = %d, fmt = %d\n", src_w, src_h, rot_mode, fmt);
                test_threads_num = 1;
                test_loop_times  = 1;
                int ret = (int)bm_dev_request(&handle, dev_id);
                if (ret != 0) {
                    printf("Create bm handle failed. ret = %d\n", ret);
                    exit(-1);
                }
                convert_ctx ctx[test_threads_num];
#ifdef __linux__
                pthread_t* pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
                for (int i = 0; i < test_threads_num; i++) {
                    ctx[i].i = i;
                    ctx[i].loop = test_loop_times;
                    if (pthread_create(&pid[i], NULL, dwa_rot, (void *)(ctx + i))) {
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
#endif
                if (src_w == 4096 && src_h == 4096) {
                    printf("--------ALL ROT SIZE LOOP TEST OVER---------\n");
                    return 0;
                }
            }
            rand_input_height = 32;
        }
    } else if (argc == 1 || (argc > 5 && argc < 9)) {
        printf("command input error\n");
        print_help(argv);
        exit(-1);
    }
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    if(argc == 4 && rand_mode == 1){
        srand(time(NULL));
        src_h = rand() % (IMG_MAX_HEIGHT - IMG_MIN_HEIGHT + 1) + IMG_MIN_HEIGHT;
        src_w = rand() % (IMG_MAX_WIDTH - IMG_MIN_WIDTH + 1) + IMG_MIN_WIDTH;
        src_h = ALIGN(src_h, DWA_ALIGN);
        src_w = ALIGN(src_w, DWA_ALIGN);
        int rand_rot = rand() % 3;
        switch (rand_rot) {
            case 0:
                rot_mode = BMCV_ROTATION_0;
                break;
            case 1:
                rot_mode = BMCV_ROTATION_90;
                break;
            case 2:
                rot_mode = BMCV_ROTATION_270;
                break;
            default:
                printf("Error: Invalid random index.\n");
                return -1;
        }
        int rand_fmt = rand() % 4;
        switch (rand_fmt) {
            case 0:
                fmt = FORMAT_GRAY;
                break;
            case 1:
                fmt = FORMAT_RGB_PLANAR;
                break;
            case 2:
                fmt = FORMAT_YUV420P;
                break;
            case 3:
                fmt = FORMAT_YUV444P;
                break;
            default:
                printf("Error: Invalid random index.\n");
                return -1;
        }
        dst_name = "dwa_rot_output_rand.yuv";
        printf("Random mode: width = %d, height = %d, rot_mode = %d, img_fmt = %d\n", src_w, src_h, rot_mode, fmt);
    }
    convert_ctx ctx[test_threads_num];
#ifdef __linux__
    pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
    for (int i = 0; i < test_threads_num; i++) {
        ctx[i].i = i;
        ctx[i].loop = test_loop_times;
        if (pthread_create(&pid[i], NULL, dwa_rot, (void *)(ctx + i))) {
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
