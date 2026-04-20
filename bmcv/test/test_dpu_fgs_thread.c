#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"
#include <time.h>

#define IMG_MAX_HEIGHT 1080
#define IMG_MAX_WIDTH  1920
#define IMG_MIN_HEIGHT 64
#define IMG_MIN_WIDTH  64
#define IMG_ALIGN      4
#define MD5_DIGEST_LENGTH 16
#define MD5_STRING_LENGTH (MD5_DIGEST_LENGTH * 2)
#define SLEEP_ON 0

#define ALIGN(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int thread_num = 1;
int loop_time = 1;
bool read_from_bin = false;
bool write_to_bin = false;

char *guide_path = NULL;
char *smooth_path = NULL;
char *ref_res_path = NULL;
char *dpu_res_path = NULL;
int img_height = 64;
int img_width = 64;

bmcv_dpu_fgs_mode dpu_fgs_mode = DPU_FGS_MUX0;
bmcv_dpu_disp_range disp_range = BMCV_DPU_DISP_RANGE_16;
bm_handle_t handle = NULL;

typedef struct dpu_fgs_t {
    int i_test;
} dpu_fgs_t;

extern void dpu_read_bin(const char *path, unsigned char *data, int size);
extern int md5_cmp(unsigned char* got, unsigned char* exp, int size);
extern int md5_get(unsigned char* got, int size, char* md5_str);

extern bm_status_t bm_dpu_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
        bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride, bool bFgs);

extern bm_status_t test_cmodel_fgs(unsigned char *smooth_input,
                                   unsigned char *guide_input,
                                   unsigned char *ref_output,
                                   int width, int height,
                                   bmcv_dpu_fgs_mode fgs_mode,
                                   bmcv_dpu_fgs_attrs *grp);

extern bm_status_t test_cmodel_fgs_u16(unsigned char *smooth_input,
                                       unsigned char *guide_input,
                                       unsigned short *ref_output,
                                       int width, int height,
                                       bmcv_dpu_fgs_mode fgs_mode,
                                       bmcv_dpu_fgs_attrs *grp,
                                       bmcv_dpu_disp_range disp_range);

static void write_bin(void *va, char *name, int size) {
    FILE *fp_dst = fopen(name, "wb");
    fwrite(va, 1, size, fp_dst);
    fclose(fp_dst);
}

void copy_img_data(unsigned char *data, unsigned char *cmodel, int w, int h) {
    for (int i = 0; i < h; i++)
        memcpy(cmodel + i * w, data + i * ALIGN(w, 16), w);
}

static int cmp_u8(unsigned char *got, unsigned char *exp, int width, int height, int idx) {
    for (int i = 0; i < height ; i++) {
        for (int j = 0; j < width; j++) {
            if (got[i * ALIGN(width, 32) + j] != exp[i * width + j]) {
                printf("[Thread %d][u8] Compare failed, at idx = %d, dpu_output = 0x%x, cmodel_output = 0x%x \n", idx, i, got[i * ALIGN(width, 32) + j], exp[i * width + j]);
                return -1;
            }
        }
    }
    return 0;
}

static int cmp_u16(unsigned short *got, unsigned short *exp, int width, int height, int idx) {
    for (int i = 0; i < height ; i++) {
        for (int j = 0; j < width; j++) {
            if (got[i * ALIGN(width, 32) + j] != exp[i * width + j]) {
                printf("[Thread %d][u16] Compare failed, at idx = %d, dpu_output = 0x%x, cmodel_output = 0x%x \n", idx, i, got[i * ALIGN(width, 32) + j], exp[i * width + j]);
                return -1;
            }
        }
    }
    return 0;
}

static int cmp_md5(char* file_name, unsigned char* got, int size, int idx) {
    FILE *ref_fp = fopen(file_name, "rb");
    if (ref_fp == NULL) {
        printf("[Thread %d] Read file from ref_res_path failed. No such file : %s \n", idx, file_name);
        exit(-1);
        return -1;
    }
    unsigned char *ref = malloc(size);
    fread((void *)ref, 1, size, ref_fp);
    fclose(ref_fp);
    char ref_md5[MD5_STRING_LENGTH + 1];
    md5_get(ref, size, ref_md5);
    return (md5_cmp(got, (unsigned char*)ref_md5, size));
}

static int check_file_exist(char* file_name) {
    FILE *ref_fp = fopen(file_name, "rb");
    if (ref_fp == NULL)
        return 1;
    return 0;
}

void fill_img(unsigned char *data, int w, int h) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++)
            data[i * ALIGN(w, 16) + j] = rand() % 256;
    }
}

bm_status_t init_dpu(bm_handle_t handle, bm_image *left,
                     bm_image *right, bm_image *output,
                     bm_image_format_ext dType,
                     unsigned char *leftData,
                     unsigned char *rightData,
                     int uWidth, int uHeight) {
    bm_status_t ret = BM_SUCCESS;
    int src_stride[4], dst_stride[4];
    bm_dpu_image_calc_stride(handle, uHeight, uWidth, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, src_stride, false);
    bm_dpu_image_calc_stride(handle, uHeight, uWidth, FORMAT_GRAY, dType, dst_stride, true);
    ret = bm_image_create(handle, uHeight, uWidth, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, left, src_stride);
    if (ret != BM_SUCCESS) {
        printf("left bm_image create failed. ret = %d \n", ret);
        return BM_ERR_FAILURE;
    }
    ret = bm_image_create(handle, uHeight, uWidth, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, right, src_stride);
    if (ret != BM_SUCCESS) {
        printf("right bm_image create failed. ret = %d \n", ret);
        return BM_ERR_FAILURE;
    }
    ret = bm_image_create(handle, uHeight, uWidth, FORMAT_GRAY, dType, output, dst_stride);
    if (ret != BM_SUCCESS) {
        printf("output bm_image create failed. ret = %d \n", ret);
        return BM_ERR_FAILURE;
    }
    if (bm_image_alloc_dev_mem(*left, BMCV_HEAP1_ID) != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_leftImg failed \n");
        return BM_ERR_FAILURE;
    }
    if (bm_image_alloc_dev_mem(*right, BMCV_HEAP1_ID) != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_rightImg failed \n");
        return BM_ERR_FAILURE;
    }
    if (bm_image_alloc_dev_mem(*output, BMCV_HEAP1_ID) != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_dispImg failed \n");
        return BM_ERR_FAILURE;
    }
    ret = bm_image_copy_host_to_device(*left, (void**)&leftData);
    if (ret != BM_SUCCESS) {
        printf("left_img bm_image_copy_host_to_device failed \n");
        exit(-1);
    }
    ret = bm_image_copy_host_to_device(*right, (void**)&rightData);
    if (ret != BM_SUCCESS) {
        printf("right_img bm_image_copy_host_to_device failed \n");
        exit(-1);
    }
    return ret;
}

bm_status_t deinit_dpu(bm_image *left, bm_image *right, bm_image *output) {
    bm_image_destroy(left);
    bm_image_destroy(right);
    bm_image_destroy(output);
    return BM_SUCCESS;
}

static void *test_dpu_fgs(void* args) {
    dpu_fgs_t dpu_fgs_thread_arg = *(dpu_fgs_t*)args;
    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval timediff;
    unsigned long long time_single, time_total = 0, time_avg = 0;
    unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
    int img_size = 0, dsize = 0;
    bm_status_t ret = BM_SUCCESS;
    bm_image smooth_img, guide_img;
    bm_image disp_img;
    bm_image_data_format_ext output_dtype = DATA_TYPE_EXT_1N_BYTE;
    bmcv_dpu_fgs_attrs dpu_fgs_attr = {
        .fgs_max_count = 19,
        .fgs_max_t = 3,
        .fxbase_line = 864000,
        .depth_unit_en = 1
    };
#if SLEEP_ON
    int fps = 7;
    int sleep_time = 1000000 / fps;
#endif
    img_size = img_height * ALIGN(img_width, 16);
    (dpu_fgs_mode == DPU_FGS_MUX0) ? (output_dtype = DATA_TYPE_EXT_1N_BYTE) :
    (output_dtype = DATA_TYPE_EXT_U16);
    dsize = (output_dtype == DATA_TYPE_EXT_1N_BYTE) ? 1 : 2;
    unsigned char *smoothImg = (unsigned char*)malloc(img_size * sizeof(unsigned char));
    unsigned char *guideImg = (unsigned char*)malloc(img_size * sizeof(unsigned char));
    memset(smoothImg, 0, img_size * sizeof(unsigned char));
    memset(guideImg, 0, img_size * sizeof(unsigned char));
    if (read_from_bin) {
        if (check_file_exist(guide_path)) {
            printf("[Thread %d] Read file from guide_path failed. No such file : %s \n", dpu_fgs_thread_arg.i_test, guide_path);
            exit(-1);
        }
        else if (check_file_exist(smooth_path)) {
            printf("[Thread %d] Read file from smooth_path failed. No such file : %s \n", dpu_fgs_thread_arg.i_test, smooth_path);
            exit(-1);
        }
        else if (check_file_exist(ref_res_path)) {
            printf("[Thread %d] Read file from ref_res_path failed. No such file : %s \n", dpu_fgs_thread_arg.i_test, ref_res_path);
            exit(-1);
        }
        dpu_read_bin(guide_path, guideImg, img_size * sizeof(unsigned char));
        dpu_read_bin(smooth_path, smoothImg, img_size * sizeof(unsigned char));
    }
    else {
        fill_img(guideImg, img_width, img_height);
        fill_img(smoothImg, img_width, img_height);
    }
    ret = init_dpu(handle, &smooth_img, &guide_img, &disp_img, output_dtype, smoothImg, guideImg, img_width, img_height);
    if (ret != BM_SUCCESS) {
        printf("[Thread %d] Init fgs dpu failed, ret is %d \n", dpu_fgs_thread_arg.i_test, ret);
        exit(-1);
    }
    for (int i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_dpu_fgs_disp(handle, &guide_img, &smooth_img, &disp_img, &dpu_fgs_attr, dpu_fgs_mode);
        gettimeofday(&tv_end, NULL);
        timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
        timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
        time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
#if SLEEP_ON
        if (time_single < sleep_time)
            usleep((sleep_time - time_single));
        gettimeofday(&tv_end, NULL);
        timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
        timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
        time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
#endif
        if (time_single > time_max)
            time_max = time_single;
        if (time_single < time_min)
            time_min = time_single;
        time_total = time_total + time_single;
        if (ret != BM_SUCCESS) {
            printf("[Thread %d] bmcv_dpu_fgs_disp failed. \n", dpu_fgs_thread_arg.i_test);
            deinit_dpu(&smooth_img, &guide_img, &disp_img);
            free(smoothImg);
            free(guideImg);
            exit(-1);
        }
    }
    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = img_size * fps_actual / 1024 / 1024;
    char fmt_str[100];
    format_to_str(disp_img.image_format, fmt_str);
    if (read_from_bin) {
        printf("------Thread %d------ \nwidth         = %d \nheight        = %d \nloop          = %d \ndpu_fgs_mode  = %d \n", dpu_fgs_thread_arg.i_test, img_width, img_height, loop_time, dpu_fgs_mode);
        printf("guide_path    = %s \nsmooth_path   = %s \nref_res_path  = %s \ndpu_res_path  = %s \n", guide_path, smooth_path, ref_res_path, dpu_res_path);
        printf("read_from_bin = %d \nwrite_to_bin  = %d \nfmt_str       = %s \nimg_size      = %d*%d \ntime_max      = %llu \ntime_avg      = %llu \nfps           = %llu \npixel_per_sec = %lluM pps \n", read_from_bin, write_to_bin, fmt_str, guide_img.width, guide_img.height, time_max, time_avg, fps_actual, pixel_per_sec);
    }
    else {
        printf("------Thread %d------ \nwidth         = %d \nheight        = %d \nloop          = %d \ndpu_fgs_mode  = %d \n", dpu_fgs_thread_arg.i_test, img_width, img_height, loop_time, dpu_fgs_mode);
        printf("read_from_bin = %d \nwrite_to_bin  = %d \nfmt_str       = %s \nimg_size      = %d*%d \ntime_max      = %llu \ntime_avg      = %llu \nfps           = %llu \npixel_per_sec = %lluM pps \n", read_from_bin, write_to_bin, fmt_str, guide_img.width, guide_img.height, time_max, time_avg, fps_actual, pixel_per_sec);
    }
    if (dpu_fgs_thread_arg.i_test == 0) {
        int dst_img_size = ALIGN(img_width, 32) * img_height;
        if (read_from_bin) {
            unsigned char *u8_dpu_res = (unsigned char*) malloc(dst_img_size * dsize);
            ret =  bm_image_copy_device_to_host(disp_img, (void**)&u8_dpu_res);
            if (ret != BM_SUCCESS) {
                printf("[Thread %d] bm_image_copy_device_to_host fialed, ret = %d\n", dpu_fgs_thread_arg.i_test, ret);
                free(u8_dpu_res);
                free(smoothImg);
                free(guideImg);
                deinit_dpu(&smooth_img, &guide_img, &disp_img);
                exit(-1);
            }
            ret = cmp_md5(ref_res_path, u8_dpu_res, img_size, dpu_fgs_thread_arg.i_test);
            if (ret != BM_SUCCESS) {
                write_bin((void *)u8_dpu_res, "error_cmp.bin", dst_img_size * dsize);
                printf("[Thread %d] bm_write_bin save error_cmp.bin \n", dpu_fgs_thread_arg.i_test);
                free(u8_dpu_res);
                free(smoothImg);
                free(guideImg);
                deinit_dpu(&smooth_img, &guide_img, &disp_img);
                exit(-1);
            }
            if (write_to_bin) {
                write_bin((void *)u8_dpu_res, dpu_res_path, dst_img_size * dsize);
                printf("[Thread %d] bm_write_bin write %s \n", dpu_fgs_thread_arg.i_test, dpu_res_path);
            }
            free(u8_dpu_res);
        }
        else {
            unsigned char *smooth_img_cmodel = (unsigned char*) malloc (img_height * img_width * sizeof(unsigned char));
            unsigned char *guide_img_cmodel = (unsigned char*) malloc (img_height * img_width * sizeof(unsigned char));
            copy_img_data(smoothImg, smooth_img_cmodel, img_width, img_height);
            copy_img_data(guideImg, guide_img_cmodel, img_width, img_height);
            if (dpu_fgs_mode == DPU_FGS_MUX0) {
                unsigned char *cpu_res = (unsigned char*) malloc (img_height * img_width * dsize);
                unsigned char *dpu_res = (unsigned char*) malloc (dst_img_size * dsize);
                ret = test_cmodel_fgs(smooth_img_cmodel, guide_img_cmodel, cpu_res, guide_img.width, guide_img.height, dpu_fgs_mode, &dpu_fgs_attr);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d] test_cmodel_fgs_u8 failed, ret = %d \n", dpu_fgs_thread_arg.i_test, ret);
                    free(cpu_res);
                    free(dpu_res);
                    free(smooth_img_cmodel);
                    free(guide_img_cmodel);
                    free(smoothImg);
                    free(guideImg);
                    deinit_dpu(&smooth_img, &guide_img, &disp_img);
                    exit(-1);
                }
                ret = bm_image_copy_device_to_host(disp_img, (void**)&dpu_res);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d] bm_image_copy_device_to_host fialed, ret = %d\n", dpu_fgs_thread_arg.i_test, ret);
                    free(cpu_res);
                    free(dpu_res);
                    free(smooth_img_cmodel);
                    free(guide_img_cmodel);
                    free(smoothImg);
                    free(guideImg);
                    deinit_dpu(&smooth_img, &guide_img, &disp_img);
                    exit(-1);
                }
                ret = cmp_u8(dpu_res, cpu_res, img_width, img_height, dpu_fgs_thread_arg.i_test);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d][u8] DPU and CPU failed to compare \n", dpu_fgs_thread_arg.i_test);
                    free(cpu_res);
                    free(dpu_res);
                    free(smooth_img_cmodel);
                    free(guide_img_cmodel);
                    free(smoothImg);
                    free(guideImg);
                    deinit_dpu(&smooth_img, &guide_img, &disp_img);
                    exit(-1);
                }
                else
                    printf("[Thread %d][u8] Compare DPU result with CPU result successfully! \n", dpu_fgs_thread_arg.i_test);
                if (write_to_bin) {
                    write_bin((void *)dpu_res, dpu_res_path, dst_img_size * dsize);
                    printf("[Thread %d] bm_write_bin save %s \n", dpu_fgs_thread_arg.i_test, dpu_res_path);
                }
                free(cpu_res);
                free(dpu_res);
            }
            else {
                unsigned short *cpu_res = (unsigned short*) malloc (img_height * img_width * dsize);
                unsigned short *dpu_res = (unsigned short*) malloc (dst_img_size * dsize);
                ret = test_cmodel_fgs_u16(smooth_img_cmodel, guide_img_cmodel, cpu_res,
                                          guide_img.width, guide_img.height, dpu_fgs_mode, &dpu_fgs_attr, disp_range);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d] test_cmodel_fgs_u16 failed, ret = %d \n", dpu_fgs_thread_arg.i_test, ret);
                    free(cpu_res);
                    free(dpu_res);
                    free(smooth_img_cmodel);
                    free(guide_img_cmodel);
                    free(smoothImg);
                    free(guideImg);
                    deinit_dpu(&smooth_img, &guide_img, &disp_img);
                    exit(-1);
                }
                ret = bm_image_copy_device_to_host(disp_img, (void**)&dpu_res);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d] bm_image_copy_device_to_host fialed, ret = %d\n", dpu_fgs_thread_arg.i_test, ret);
                    free(cpu_res);
                    free(dpu_res);
                    free(smooth_img_cmodel);
                    free(guide_img_cmodel);
                    free(smoothImg);
                    free(guideImg);
                    deinit_dpu(&smooth_img, &guide_img, &disp_img);
                    exit(-1);
                }
                ret = cmp_u16(dpu_res, cpu_res, img_width, img_height, dpu_fgs_thread_arg.i_test);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d][u16] DPU and CPU failed to compare \n", dpu_fgs_thread_arg.i_test);
                    free(cpu_res);
                    free(dpu_res);
                    free(smooth_img_cmodel);
                    free(guide_img_cmodel);
                    free(smoothImg);
                    free(guideImg);
                    deinit_dpu(&smooth_img, &guide_img, &disp_img);
                    exit(-1);
                }
                else
                    printf("[Thread %d][u16] Compare DPU result with CPU result successfully! \n", dpu_fgs_thread_arg.i_test);
                if (write_to_bin) {
                    write_bin((void *)dpu_res, dpu_res_path, dst_img_size * dsize);
                    printf("[Thread %d] bm_write_bin save %s \n", dpu_fgs_thread_arg.i_test, dpu_res_path);
                }
                free(cpu_res);
                free(dpu_res);
            }
            free(guide_img_cmodel);
            free(smooth_img_cmodel);
        }
    }
    deinit_dpu(&smooth_img, &guide_img, &disp_img);
    free(smoothImg);
    free(guideImg);
    return 0;
}

int main(int argc, char *args[]) {
    srand((unsigned int)time(NULL));
    img_height = ALIGN((rand() % (IMG_MAX_HEIGHT - IMG_MIN_HEIGHT + 1) + IMG_MIN_HEIGHT), 2);
    img_width = ALIGN((rand() % (IMG_MAX_WIDTH - IMG_MIN_WIDTH + 1) + IMG_MIN_WIDTH), IMG_ALIGN);
    dpu_fgs_mode = (rand() % 2) + DPU_FGS_MUX0;
    bm_status_t ret = BM_SUCCESS;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d \n", ret);
        return -1;
    }
    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: \n");
        printf("%s thread_num | loop | width | height | guide_path | smooth_path | ref_res_path | dpu_res_path \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 \n", args[0]);
        printf("%s 1 1 \n", args[0]);
        printf("%s 1 1 512 \n", args[0]);
        printf("%s 1 1 512 284 \n", args[0]);
        printf("%s 1 1 128 128 /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin fgs_res.bin \n", args[0]);
        printf("* Need to set guide_path, smooth_path, ref_res_path, dpu_res_path at same time \n");
        printf("* Range of width: %d~%d \n", IMG_MIN_WIDTH, IMG_MAX_WIDTH);
        printf("* Range of height: %d~%d \n", IMG_MIN_HEIGHT, IMG_MAX_HEIGHT);
        return 0;
    }
    else {
        switch (argc) {
        case 1 ... 5: {
            int *vars[] = {&thread_num, &loop_time, &img_width, &img_height};
            for (int i = 1; i < argc; ++i)
                *vars[i - 1] = atoi(args[i]);
            if ((img_width % 32) != 0) dpu_fgs_mode = DPU_FGS_MUX0;
            break;
        }
        case 9: {
            dpu_fgs_mode = DPU_FGS_MUX0;
            read_from_bin = true;
            write_to_bin = true;
            int *vars[] = {&thread_num, &loop_time, &img_width, &img_height};
            for (int i = 1; i < 5; ++i)
                *vars[i - 1] = atoi(args[i]);
            char **strs[] = {&guide_path, &smooth_path, &ref_res_path, &dpu_res_path};
            for (int i = 5; i < argc; ++i)
                *strs[i - 5] = args[i];
            break;
        }
        default:
            printf("[TEST dpu fgs] Thread nums should be 1~5,9 \n");
            exit(-1);
            break;
        }
    }
    dpu_fgs_t dpu_fgs_thread_arg[thread_num];
#ifdef __linux__
    pthread_t pid[thread_num];
    for (int i = 0; i < thread_num; i++) {
        dpu_fgs_thread_arg[i].i_test = i;
        if (pthread_create(pid + i, NULL, test_dpu_fgs, (void *)(dpu_fgs_thread_arg + i))) {
            printf("Create thread failed \n");
            exit(-1);
        }
    }
    for (int i = 0; i < thread_num; i++) {
        int ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed \n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    printf("--------ALL THREADS TEST OVER---------\n");
#endif
    return 0;
}