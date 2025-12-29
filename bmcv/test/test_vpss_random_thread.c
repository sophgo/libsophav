#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define SLEEP_ON 0

typedef struct convert_ctx_{
    int loop;
    int i;
}convert_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080, dev_id = 0;
bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
bmcv_rect_t rect = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
bm_handle_t handle = NULL;
bm_image_format_ext in_fmt[19] = {FORMAT_YUV420P, FORMAT_YUV422P, FORMAT_YUV444P, FORMAT_NV12, FORMAT_NV21,
                            FORMAT_NV16, FORMAT_NV61, FORMAT_RGB_PLANAR, FORMAT_BGR_PLANAR, FORMAT_RGB_PACKED,
                            FORMAT_BGR_PACKED, FORMAT_RGBP_SEPARATE, FORMAT_BGRP_SEPARATE, FORMAT_GRAY, FORMAT_YUV422_YUYV,
                            FORMAT_YUV422_YVYU, FORMAT_YUV422_UYVY, FORMAT_YUV422_VYUY, FORMAT_COMPRESSED};
bm_image_format_ext out_fmt[19] = {FORMAT_YUV420P, FORMAT_YUV422P, FORMAT_YUV444P, FORMAT_NV12, FORMAT_NV21,
                            FORMAT_NV16, FORMAT_NV61, FORMAT_RGB_PLANAR, FORMAT_BGR_PLANAR, FORMAT_RGB_PACKED,
                            FORMAT_BGR_PACKED, FORMAT_RGBP_SEPARATE, FORMAT_BGRP_SEPARATE, FORMAT_GRAY, FORMAT_YUV422_YUYV,
                            FORMAT_YUV422_YVYU, FORMAT_YUV422_UYVY, FORMAT_YUV422_VYUY, FORMAT_HSV_PLANAR};

extern unsigned char is_full_image(bm_image_format_ext image_format);
extern unsigned char is_yuv420_image(bm_image_format_ext image_format);

static void * ramdom(void* arg) {
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
    if(src_fmt == FORMAT_COMPRESSED){
        int stride[4] = {src_h*src_w/16, src_h*src_w, src_h*src_w/2/16, src_h*src_w/2};
        bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    } else
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

    for(i = 0;i < loop_time; i++){
        gettimeofday(&tv_start, NULL);

        bmcv_image_vpp_csc_matrix_convert(handle, 1, src, &dst, CSC_MAX_ENUM, NULL, algorithm, &rect);

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
    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    char src_fmt_str[100],dst_fmt_str[100],algorithm_str[100];
    format_to_str(src.image_format, src_fmt_str);
    format_to_str(dst.image_format, dst_fmt_str);
    algorithm_to_str(algorithm, algorithm_str);


    printf("thread_idx:%d, %d*%d->%d*%d, %s->%s,%s\n",ctx.i,src_w,src_h,dst_w,dst_h,src_fmt_str,dst_fmt_str,algorithm_str);
    printf("thread_idx:%d, bmcv_image_vpp_csc_matrix_convert:loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

static bm_status_t core_ramdom(void) {
    bm_status_t ret = BM_SUCCESS;
    bm_image src, dst;
    unsigned int i = 0, loop_time = 0;
    int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080;
    bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
    bmcv_rect_t rect = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
    bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
    bm_image_data_format_ext out_data = DATA_TYPE_EXT_1N_BYTE;

    loop_time = test_loop_times;
    for(int core = 0; core < 10; core++){
        char command[100];
        if((test_threads_num >> 16) != 0 &&
            !((0x1 << core) & (test_threads_num >> 16)))
            continue;
        int number = (0x1 << core) | (0x2400);
#ifdef BM_PCIE_MODE
        sprintf(command, "echo 0x%x > /sys/module/bmsophon/parameters/work_mask", number);
#else
        sprintf(command, "echo 0x%x > /sys/module/soph_vpss/parameters/work_mask", number);
#endif
        system(command);
        for(i = 0;i < loop_time; i++){
            src_fmt = in_fmt[(rand()%19)];
            dst_fmt = out_fmt[(rand()%19)];
            src_h = 16 + (rand() % 8177);
            src_w = 16 + (rand() % 8177);
            dst_h = 16 + (rand() % 8177);
            dst_w = 16 + (rand() % 8177);
            if(src_fmt == 15)
                src_fmt = src_fmt - 1;
            if(src_fmt == 3 || src_fmt == 4)
                src_fmt = src_fmt - 2;
            if (!is_full_image(src_fmt))
                src_w = src_w & (~0x1);
            if (is_yuv420_image(src_fmt))
                src_h = src_h & (~0x1);
            if (!is_full_image(dst_fmt))
                dst_w = dst_w & (~0x1);
            if (is_yuv420_image(dst_fmt))
                dst_h = dst_h & (~0x1);
            if(dst_w > 128 * src_w) dst_w = src_w * 128 - 2;
            if(dst_h > 128 * src_h) dst_h = src_h * 128 - 2;
            if(src_w > 128 * dst_w) src_w = dst_w * 128 - 2;
            if(src_h > 128 * dst_h) src_h = dst_h * 128 - 2;
            rect.crop_w = src_w;
            rect.crop_h = src_h;
            out_data = (bm_image_data_format_ext)(rand()%5);
            algorithm = (bmcv_resize_algorithm)(rand()%3);
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

            ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, src, &dst, CSC_MAX_ENUM, NULL, algorithm, &rect);

            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            if(ret || (i % 100 == 0)){
                char src_fmt_str[100],dst_fmt_str[100],algorithm_str[100];
                format_to_str(src.image_format, src_fmt_str);
                format_to_str(dst.image_format, dst_fmt_str);
                algorithm_to_str(algorithm, algorithm_str);
                printf("coreidx:%d, loop_idx:%d/%d, %d*%d->%d*%d, %s->%s,%s,out_data(%d)\n",core,i,
                    loop_time,src_w,src_h,dst_w,dst_h,src_fmt_str,dst_fmt_str,algorithm_str,out_data);
            }
            if(ret){
                printf("bmcv_image_vpp_csc_matrix_convert. core = %d\n", core);
                goto fail;
            }
        }
        printf("coreidx:%d, loop:%d done\n",core,loop_time);
    }
fail:
#ifdef BM_PCIE_MODE
    system("echo 0xff > /sys/module/bmsophon/parameters/work_mask");
#else
    system("echo 0xff > /sys/module/soph_vpss/parameters/work_mask");
#endif
    return ret;
}


static void * ramdom_param(void* arg) {
    bm_status_t ret;
    convert_ctx ctx = *(convert_ctx*)arg;
    bm_image src, dst;
    unsigned int i = 0, loop_time = 0;
    unsigned long long time_single, time_total = 0, time_avg = 0;
    unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
    int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080;
    bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
    bmcv_rect_t rect = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
    bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
    bm_image_data_format_ext out_data = DATA_TYPE_EXT_1N_BYTE;
#if SLEEP_ON
    int fps = 60;
    int sleep_time = 1000000 / fps;
#endif

    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval timediff;

    loop_time = ctx.loop;
    for(i = 0;i < loop_time; i++){
        src_fmt = in_fmt[(rand()%19)];
        dst_fmt = out_fmt[(rand()%19)];
        if(src_fmt == 15){
            src_fmt = src_fmt - 1;
        }

        bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, dst_fmt, out_data, &dst, NULL);

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

        gettimeofday(&tv_start, NULL);

        ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, src, &dst, CSC_MAX_ENUM, NULL, algorithm, &rect);

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
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        if(ret || (i % 100 == 0)){
            char src_fmt_str[100],dst_fmt_str[100],algorithm_str[100];
            format_to_str(src.image_format, src_fmt_str);
            format_to_str(dst.image_format, dst_fmt_str);
            algorithm_to_str(algorithm, algorithm_str);
            printf("thread_idx:%d, %d*%d->%d*%d, %s->%s,%s,out_data(%d)\n",ctx.i,src_w,src_h,dst_w,dst_h,src_fmt_str,dst_fmt_str,algorithm_str,out_data);
            printf("thread_idx:%d, bmcv_image_vpp_csc_matrix_convert:loop_idx %d/%d, time = %llu\n",
                ctx.i, i, loop_time, time_single);
        }
        if(ret)
            exit(-1);
    }
    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = src_w * src_h * fps_actual/1024/1024;
    if(loop_time > 1)
        printf("thread_idx:%d, loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
            ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;
}

static void print_help(char **argv){
    printf("please follow this order:\n \
        %s src_w src_h src_fmt start_x start_y crop_w crop_h dst_w dst_h dst_fmt algorithm dev_id thread_num loop_num md5\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
};

int main(int argc, char **argv) {
    if (argc == 15) {
        test_threads_num = atoi(argv[13]);
        test_loop_times  = atoi(argv[14]);
    }
    if (argc >= 13) {
        src_w = atoi(argv[1]);
        src_h = atoi(argv[2]);
        src_fmt = (bm_image_format_ext)atoi(argv[3]);
        rect.start_x = atoi(argv[4]);
        rect.start_y = atoi(argv[5]);
        rect.crop_w = atoi(argv[6]);
        rect.crop_h = atoi(argv[7]);
        dst_w = atoi(argv[8]);
        dst_h = atoi(argv[9]);
        dst_fmt = (bm_image_format_ext)atoi(argv[10]);
        algorithm = (bmcv_resize_algorithm)(atoi(argv[11]));
        dev_id = atoi(argv[12]);
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
    } else if (argc > 3 && argc < 13) {
        printf("command input error\n");
        print_help(argv);
        exit(-1);
    }
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    srand(time(NULL));
#ifdef __linux__
    if((test_threads_num & 0xffff) == 0){
        if(core_ramdom())
            exit(-1);
        return 0;
    }
    convert_ctx ctx[test_threads_num];
    pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
    if(src_w < 0){
        int stride = (src_fmt >= 21 && src_fmt <= 24) ? 2 : 1;
        for(src_w = 16; src_w <= 8192; src_w += stride){
            rect.crop_w = src_w;
            for (int i = 0; i < test_threads_num; i++) {
                ctx[i].i = i;
                ctx[i].loop = test_loop_times;
                if (pthread_create(
                        &pid[i], NULL, ramdom, (void *)(ctx + i))) {
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
        }
    } else if(src_h < 0){
        for(src_h = 16; src_h <= 8192; src_h += 1){
            rect.crop_h = src_h;
            for (int i = 0; i < test_threads_num; i++) {
                ctx[i].i = i;
                ctx[i].loop = test_loop_times;
                if (pthread_create(
                        &pid[i], NULL, ramdom, (void *)(ctx + i))) {
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
        }
    } else if(argc >= 4 && atoi(argv[3]) < 0){
        for(int m = 0; m < 19; m += 1){
            src_fmt = in_fmt[m];
            for (int i = 0; i < test_threads_num; i++) {
                ctx[i].i = i;
                ctx[i].loop = test_loop_times;
                if (pthread_create(
                        &pid[i], NULL, ramdom, (void *)(ctx + i))) {
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
        }
    } else if(rect.crop_w < 0){
        // int stride = is_full_image(src_fmt) ? 1 : 2;
        for(rect.crop_w = 16; rect.crop_w <= src_w; rect.crop_w += 1){
            for (int i = 0; i < test_threads_num; i++) {
                ctx[i].i = i;
                ctx[i].loop = test_loop_times;
                if (pthread_create(
                        &pid[i], NULL, ramdom, (void *)(ctx + i))) {
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
        }
    } else if(rect.crop_h < 0){
        // int stride = is_yuv420_image(src_fmt) ? 2 : 1;
        for(rect.crop_h = 16; rect.crop_h <= src_h; rect.crop_h += 1){
            for (int i = 0; i < test_threads_num; i++) {
                ctx[i].i = i;
                ctx[i].loop = test_loop_times;
                if (pthread_create(
                        &pid[i], NULL, ramdom, (void *)(ctx + i))) {
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
        }
    } else if(dst_w < 0){
        int stride = (dst_fmt >= 21 && dst_fmt <= 24) ? 2 : 1;
        for(dst_w = 16; dst_w <= 8192; dst_w += stride){
            for (int i = 0; i < test_threads_num; i++) {
                ctx[i].i = i;
                ctx[i].loop = test_loop_times;
                if (pthread_create(
                        &pid[i], NULL, ramdom, (void *)(ctx + i))) {
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
        }
    } else if(dst_h < 0){
        // int stride = is_yuv420_image(src_fmt) ? 2 : 1;
        for(dst_h = 16; dst_h <= 8192; dst_h += 1){
            for (int i = 0; i < test_threads_num; i++) {
                ctx[i].i = i;
                ctx[i].loop = test_loop_times;
                if (pthread_create(
                        &pid[i], NULL, ramdom, (void *)(ctx + i))) {
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
        }
    } else if(argc >= 11 && atoi(argv[10]) < 0){
        for(int m = 0; m < 19; m += 1){
            dst_fmt = out_fmt[m];
            for (int i = 0; i < test_threads_num; i++) {
                ctx[i].i = i;
                ctx[i].loop = test_loop_times;
                if (pthread_create(
                        &pid[i], NULL, ramdom, (void *)(ctx + i))) {
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
        }
    } else if (argc >= 13){
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ramdom, (void *)(ctx + i))) {
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
    } else {
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, ramdom_param, (void *)(ctx + i))) {
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
    }
    bm_dev_free(handle);
    printf("--------ALL THREADS TEST OVER---------\n");
    free(pid);
#endif

    return 0;
}

