#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>
#include <errno.h>

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
char *src_name = "1920x1080.yuv", *dst_name = "output_1920x1080_rot0.yuv", /**perf_name = "1920x1080_rot0.yuv",*/ *mesh_name = "example.mesh";
bm_handle_t handle = NULL;
char *md5 = "03fd51eb71461e1fcc64e072588b5754";

static void * ldc_gdc_load_mesh(void * arg)
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

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
    if(ret != BM_SUCCESS){
        printf("bm_image_alloc_dev_mem_src failed \n");
        goto fail;
    }

    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
    if(ret != BM_SUCCESS){
        printf("bm_image_alloc_dev_mem_dst failed \n");
        goto fail;
    }

    // read image data from input files
    bm_read_bin(src, src_name);

    // read mesh data
    if (!mesh_name) {
        printf("Please asign file name for LDC mesh.\n");
        goto fail;
    }
    bm_device_mem_t dmem;
    u32 mesh_1st_size = 0, mesh_2nd_size = 0;
    test_mesh_gen_get_size(width, height, &mesh_1st_size, &mesh_2nd_size);
    u32 mesh_size = mesh_1st_size + mesh_2nd_size;
    ret = bm_malloc_device_byte(handle, &dmem, mesh_size);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed: %s\n", strerror(errno));
        goto fail;
    }

    unsigned char *buffer = (unsigned char *)malloc(mesh_size);
    if (buffer == NULL) {
        printf("malloc buffer for mesh failed!\n");
        goto fail;
    }
    memset(buffer, 0, mesh_size);

    FILE *fp = fopen(mesh_name, "rb");
    if (!fp) {
        printf("open file:%s failed.\n", mesh_name);
        goto fail;
    }

    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);

    if (mesh_size != (unsigned int)fileSize) {
        printf("loadmesh file:(%s) size is not match.\n", mesh_name);
        fclose(fp);
        goto fail;
    }
    rewind(fp);

    fread(buffer, mesh_size, 1, fp);
    ret = bm_memcpy_s2d(handle, dmem, (void*)buffer);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d failed!\n");
        fclose(fp);
        free(buffer);
        goto fail;
    }
    fclose(fp);
    free(buffer);

    for (int i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_ldc_gdc_load_mesh(handle, src, dst, dmem);
        if(ret != BM_SUCCESS) {
            printf("test bmcv_gdc_ldc_load_mesh failed \n");
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
    bm_free_device(handle, dmem);

    if (ctx.i == 0) {
        if (md5 == NULL) {
            bm_write_bin(dst, dst_name);
        } else {
            int image_byte_size[4] = {0};
            bm_image_get_byte_size(dst, image_byte_size);
            int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
            unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
            void* out_ptr[4] = {(void*)output_ptr,
                                (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                                (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                                (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
            bm_image_copy_device_to_host(dst, (void **)out_ptr);
            if(md5_cmp(output_ptr, (unsigned char*)md5, byte_size) != 0) {
                bm_write_bin(dst, "error_cmp.bin");
                exit(-1);
            }
            free(output_ptr);
        }
    }

    char algorithm_str[100] = "bmcv_ldc_gdc_load_mesh";
    char src_fmt_str[100], dst_fmt_str[100];
    format_to_str(src.image_format, src_fmt_str);
    format_to_str(dst.image_format, dst_fmt_str);

    printf("idx:%d, %d*%d->%d*%d, %s->%s,%s\n", ctx.i, width, height, align_width, align_height, src_fmt_str, dst_fmt_str, algorithm_str);
    printf("idx:%d, bmcv_ldc_gdc_load_mesh: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
                                            ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

fail:
    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    return 0;
}

int main(int argc, char **argv) {
    if (argc >= 11) {
        md5 = argv[10];
    } else if(argc > 3) {
        md5 = NULL;
    }
    if (argc >= 10) {
        test_threads_num = atoi(argv[8]);
        test_loop_times  = atoi(argv[9]);
    }
    if (argc >= 8) {
        src_name = argv[1];
        dst_name = argv[2];
        width = atoi(argv[3]);
        height = atoi(argv[4]);
        src_fmt = (bm_image_format_ext)atoi(argv[5]);
        dst_fmt = (bm_image_format_ext)atoi(argv[6]);
        mesh_name = argv[7];
    }
    if (argc == 3){
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    }

    if(argc == 1 || (argc > 3 && argc < 8)){
        printf("command input error, please follow this order:\n \
        %s src_name dst_name width height src_fmt dst_fmt mesh_file_name thread_num loop_num md5\n \
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
                &pid[i], NULL, ldc_gdc_load_mesh, (void *)(ctx + i))) {
            perror("create thread failed\n");
            exit(-1);
        }
    }
    for (int i = 0; i < test_threads_num; i++) {
        bm_status_t ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            perror("Thread join failed.");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    printf("--------ALL THREADS TEST OVER---------\n");
#endif

    return 0;
}
