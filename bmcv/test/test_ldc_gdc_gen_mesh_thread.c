#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>
#include <inttypes.h>

#define SLEEP_ON 0
#define MAX_THREAD_NUM 20
#define LDC_ALIGN 64
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
extern void test_mesh_gen_get_size(u32 u32Width,
                                   u32 u32Height,
                                   u32 *mesh_1st_size,
                                   u32 *mesh_2nd_size);

typedef struct ldc_gdc_ctx_{
    int loop;
    int i;
} ldc_gdc_ctx;

int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int height = 1080, width = 1920;
bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
bm_image_data_format_ext input_dataType = DATA_TYPE_EXT_1N_BYTE, output_dataType = DATA_TYPE_EXT_1N_BYTE;
bmcv_gdc_attr stLDCAttr = {0};
char *src_name = "1920x1080.yuv", *dst_name = "output_1920x1080_rot0.yuv";
bm_handle_t handle = NULL;
char *md5 = "eabb55a370ece25767ffc09915b802b0";

static void * ldc_gdc_gen_mesh(void * arg)
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
    int align_height = (height + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    int align_width  = (width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);

    // calc image stride
    bm_ldc_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
    bm_ldc_image_calc_stride(handle, align_height, align_width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, dst_stride);

    // create bm image
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    bm_image_create(handle, align_height, align_width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if(ret != BM_SUCCESS){
        printf("bm_image_alloc_dev_mem_src failed \n");
        goto fail;
    }

    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
    if(ret != BM_SUCCESS){
        printf("bm_image_alloc_dev_mem_dst failed \n");
        goto fail;
    }

    // read image data from input files
    bm_read_bin(src, src_name);

    bm_device_mem_t dmem;
    u32 mesh_1st_size = 0, mesh_2nd_size = 0;
    test_mesh_gen_get_size(width, height, &mesh_1st_size, &mesh_2nd_size);
    u32 mesh_size = mesh_1st_size + mesh_2nd_size;
    ret = bm_malloc_device_byte(handle, &dmem, mesh_size);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto fail;
    }

    for (int i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ldc_gdc_gen_mesh(handle, src, dst, stLDCAttr, dmem);
        if(ret != BM_SUCCESS) {
            printf("test bm_ldc_gdc_gen_mesh failed \n");
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
    pixel_per_sec = align_width * align_height * fps_actual/1024/1024;

    if (ctx.i == 0) {
        if (md5 == NULL) {
            unsigned char *buffer = (unsigned char *)malloc(mesh_size);
            if (buffer == NULL) {
                printf("malloc buffer for mesh failed!\n");
                free(buffer);
                goto fail;
            }
            memset(buffer, 0, mesh_size);
            ret = bm_memcpy_d2s(handle, (void*)buffer, dmem);

            char mesh_name[128];
            snprintf(mesh_name, 128, "./test_mesh_%dx%d_%d_%d_%d_%d_%d_%d_%d.mesh"
                , width, height, stLDCAttr.bAspect, stLDCAttr.s32XRatio, stLDCAttr.s32YRatio
                , stLDCAttr.s32XYRatio, stLDCAttr.s32CenterXOffset, stLDCAttr.s32CenterYOffset, stLDCAttr.s32DistortionRatio);

            FILE *fp = fopen(mesh_name, "wb");
            if (!fp) {
                printf("open file[%s] failed.\n", mesh_name);
                free(buffer);
                goto fail;
            }
            if (fwrite((void *)(unsigned long int)buffer, mesh_size, 1, fp) != 1) {
                printf("fwrite mesh data error\n");
                free(buffer);
                goto fail;
            }
            fclose(fp);
            free(buffer);
        } else {
            unsigned char *buffer = (unsigned char *)malloc(mesh_size);
            if (buffer == NULL) {
                printf("malloc buffer for mesh failed!\n");
                free(buffer);
                goto fail;
            }
            memset(buffer, 0, mesh_size);
            ret = bm_memcpy_d2s(handle, (void*)buffer, dmem);

            if(md5_cmp(buffer, (unsigned char*)md5, mesh_size) != 0) {
                char mesh_name[128];
                snprintf(mesh_name, 128, "./error_test_mesh_%dx%d_%d_%d_%d_%d_%d_%d_%d.mesh"
                    , width, height, stLDCAttr.bAspect, stLDCAttr.s32XRatio, stLDCAttr.s32YRatio
                    , stLDCAttr.s32XYRatio, stLDCAttr.s32CenterXOffset, stLDCAttr.s32CenterYOffset, stLDCAttr.s32DistortionRatio);

                FILE *fp = fopen(mesh_name, "wb");
                if (!fp) {
                    printf("open file[%s] failed.\n", mesh_name);
                    free(buffer);
                    goto fail;
                }
                if (fwrite((void *)(unsigned long int)buffer, mesh_size, 1, fp) != 1) {
                    printf("fwrite mesh data error\n");
                    free(buffer);
                    goto fail;
                }
                fclose(fp);
                exit(-1);
            }
            free(buffer);
        }
    }

    char algorithm_str[100] = "bmcv_ldc_gdc_gen_mesh";
    char src_fmt_str[100], dst_fmt_str[100];
    format_to_str(src.image_format, src_fmt_str);
    format_to_str(dst.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s,%s\n", ctx.i, width, height, align_width, align_height, src_fmt_str, dst_fmt_str, algorithm_str);
    printf("idx:%d, bmcv_ldc_gdc_gen_mesh: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
            ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

fail:
    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    return 0;
}

int main(int argc, char **argv) {
    if (argc >= 17) {
        md5 = argv[16];
    } else if(argc > 3) {
        md5 = NULL;
    }
    if (argc >= 16) {
        test_threads_num = atoi(argv[14]);
        test_loop_times  = atoi(argv[15]);
    }
    if (argc >= 14) {
        src_name = argv[1];
        dst_name = argv[2];
        width = atoi(argv[3]);
        height = atoi(argv[4]);
        src_fmt = (bm_image_format_ext)atoi(argv[5]);
        dst_fmt = (bm_image_format_ext)atoi(argv[6]);
        stLDCAttr.bAspect = atoi(argv[7]);
        if (stLDCAttr.bAspect == 0) {
            stLDCAttr.s32XRatio = atoi(argv[8]);
            stLDCAttr.s32YRatio = atoi(argv[9]);
            stLDCAttr.s32XYRatio = atoi(argv[10]);
            stLDCAttr.s32XYRatio = 0;
        } else {
            stLDCAttr.s32XRatio = atoi(argv[8]);
            stLDCAttr.s32XRatio = 0;
            stLDCAttr.s32YRatio = atoi(argv[9]);
            stLDCAttr.s32YRatio = 0;
            stLDCAttr.s32XYRatio = atoi(argv[10]);
        }
        stLDCAttr.s32CenterXOffset = atoi(argv[11]);
        stLDCAttr.s32CenterYOffset = atoi(argv[12]);
        stLDCAttr.s32DistortionRatio = atoi(argv[13]);
        stLDCAttr.grid_info.size = 0;
        stLDCAttr.grid_info.u.system.system_addr = NULL;
    }
    if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    }

    if(argc == 1 || (argc > 3 && argc < 14)){
        printf("command input error, please follow this order:\n \
        %s src_name dst_name width height src_fmt dst_fmt bAspect s32XRatio s32YRatio s32XYRatio s32CenterXOffset s32CenterYOffset s32DistortionRatio thread_num loop_num md5\n \
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

    ldc_gdc_ctx ctx[MAX_THREAD_NUM];
#ifdef __linux__
    pthread_t pid[MAX_THREAD_NUM];
    for (int i = 0; i < test_threads_num; i++) {
        ctx[i].i = i;
        ctx[i].loop = test_loop_times;
        if (pthread_create(
                &pid[i], NULL, ldc_gdc_gen_mesh, (void *)(ctx + i))) {
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
