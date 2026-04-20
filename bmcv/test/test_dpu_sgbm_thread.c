#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"

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

char *left_path = NULL;
char *right_path = NULL;
char *ref_res_path = NULL;
char *dpu_res_path = NULL;
char dpu_output_path[1024];
int img_height = 64;
int img_width = 64;
int dev_id = 0;
int sgbm_mode = 0;
int rang = 0;

bmcv_dpu_sgbm_mode dpu_sgbm_mode = DPU_SGBM_MUX2;
bmcv_dpu_disp_range  disp_range = BMCV_DPU_DISP_RANGE_16;
bm_handle_t handle = NULL;

typedef struct dpu_sgbm_t {
    int i_test;
} dpu_sgbm_t;

extern void dpu_read_bin(const char *path, unsigned char *data, int size);
extern int md5_cmp(unsigned char* got, unsigned char* exp, int size);
extern int md5_get(unsigned char* got, int size, char* md5_str);

extern bm_status_t bm_dpu_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
        bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride, bool bFgs);

extern bm_status_t test_cmodel_sgbm(unsigned char *left_input, unsigned char *right_input,
                                    unsigned char *ref_output, int width, int height,
                                    bmcv_dpu_sgbm_mode sgbm_mode, bmcv_dpu_sgbm_attrs *grp);

extern bm_status_t test_cmodel_sgbm_u16(unsigned char *left_input, unsigned char *right_input,
                                        unsigned short *ref_output, int width, int height,
                                        bmcv_dpu_sgbm_mode sgbm_mode, bmcv_dpu_sgbm_attrs *grp);

static void write_bin(void *va, char *name, int size) {
    FILE *fp_dst = fopen(name, "wb");
    fwrite(va, 1, size, fp_dst);
    fclose(fp_dst);
}

void fill_img(unsigned char *data, int w, int h) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++)
            data[i * ALIGN(w, 16) + j] = rand() % 256;
    }
}

void copy_img_data(unsigned char *data, unsigned char *cmodel, int w, int h) {
    for (int i = 0; i < h; i++)
        memcpy(cmodel + i * w, data + i * ALIGN(w, 16), w);
}

static int cmp_u8(unsigned char *got, unsigned char *exp, int width, int height, int idx) {
    for (int i = 0; i < height ; i++) {
        for (int j = 0; j < width; j++) {
            if (got[i * ALIGN(width, 16) + j] != exp[i * width + j]) {
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
            if (got[i * ALIGN(width, 16) + j] != exp[i * width + j]) {
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
        return -1;
    }
    unsigned char *ref = malloc(size);
    fread((void *)ref, 1, size, ref_fp);
    fclose(ref_fp);
    char ref_md5[MD5_STRING_LENGTH + 1];
    printf("ref ");
    md5_get(ref, size, ref_md5);
    return (md5_cmp(got, (unsigned char*)ref_md5, size));
}

static int check_file_exist(char* file_name) {
    FILE *ref_fp = fopen(file_name, "rb");
    if (ref_fp == NULL)
        return 1;
    return 0;
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
    bm_dpu_image_calc_stride(handle, uHeight, uWidth, FORMAT_GRAY, dType, dst_stride, false);
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

static void *test_dpu_sgbm(void* args) {
    dpu_sgbm_t dpu_sgbm_thread_arg = *(dpu_sgbm_t*)args;
    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval timediff;
    unsigned long long time_single, time_total = 0, time_avg = 0;
    unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
    int img_size = 0, dsize = 0;
    bm_status_t ret = BM_SUCCESS;
    bm_image left_img, right_img;
    bm_image disp_img;
    bm_image_data_format_ext output_dtype = DATA_TYPE_EXT_1N_BYTE;
    bmcv_dpu_sgbm_attrs dpu_sgbm_attr = {
        .bfw_mode_en = 4,
        .disp_range_en = disp_range,
        .disp_start_pos = 0,
        .dcc_dir_en = 1,
        .dpu_census_shift = 1,
        .dpu_rshift1 = 3,
        .dpu_rshift2 = 2,
        .dcc_dir_en = 1,
        .dpu_ca_p1 = 1800,
        .dpu_ca_p2 = 14400,
        .dpu_uniq_ratio = 25,
        .dpu_disp_shift = 4
    };
#if SLEEP_ON
    int fps = 7;
    int sleep_time = 1000000 / fps;
#endif
    img_size = img_height * ALIGN(img_width, 16);
    (dpu_sgbm_mode == DPU_SGBM_MUX1) ? (output_dtype = DATA_TYPE_EXT_U16) :
    (output_dtype = DATA_TYPE_EXT_1N_BYTE);
    dsize = (output_dtype == DATA_TYPE_EXT_1N_BYTE) ? 1 : 2;
    unsigned char *leftImg = (unsigned char*)malloc(img_size * sizeof(unsigned char));
    unsigned char *rightImg = (unsigned char*)malloc(img_size * sizeof(unsigned char));
    memset(leftImg, 0, img_size * sizeof(unsigned char));
    memset(rightImg, 0, img_size * sizeof(unsigned char));
    if (read_from_bin) {
        if (check_file_exist(left_path)) {
            printf("[Thread %d] Read file from left_path failed. No such file : %s \n", dpu_sgbm_thread_arg.i_test, left_path);
            exit(-1);
        }
        else if (check_file_exist(right_path)) {
            printf("[Thread %d] Read file from right_path failed. No such file : %s \n", dpu_sgbm_thread_arg.i_test, right_path);
            exit(-1);
        }
        else if (check_file_exist(ref_res_path)) {
            printf("[Thread %d] Read file from ref_res_path failed. No such file : %s \n", dpu_sgbm_thread_arg.i_test, ref_res_path);
            exit(-1);
        }
        dpu_read_bin(left_path, leftImg, img_size * sizeof(unsigned char));
        dpu_read_bin(right_path, rightImg, img_size * sizeof(unsigned char));
    }
    else {
        fill_img(leftImg, img_width, img_height);
        fill_img(rightImg, img_width, img_height);
    }
    ret = init_dpu(handle, &left_img, &right_img, &disp_img, output_dtype, leftImg, rightImg, img_width, img_height);
    if (ret != BM_SUCCESS) {
        printf("[Thread %d] Init sgbm dpu failed, ret is %d \n", dpu_sgbm_thread_arg.i_test, ret);
        exit(-1);
    }
    for (int i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_dpu_sgbm_disp(handle, &left_img, &right_img, &disp_img, &dpu_sgbm_attr, dpu_sgbm_mode);
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
            printf("[Thread %d] bmcv_dpu_sgbm_disp failed. The width should %d align. \n", dpu_sgbm_thread_arg.i_test, IMG_ALIGN);
            deinit_dpu(&left_img, &right_img, &disp_img);
            free(leftImg);
            free(rightImg);
            exit(-1);
        }
    }
    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = img_size * fps_actual / 1024 / 1024;
    char fmt_str[100];
    sprintf(dpu_output_path, "%s%s", ref_res_path, "_output");
    format_to_str(disp_img.image_format, fmt_str);
    if (read_from_bin) {
        printf("------Thread %d------ \nwidth         = %d \nheight        = %d \nloop          = %d \ndpu_sgbm_mode  = %d \n", dpu_sgbm_thread_arg.i_test, img_width, img_height, loop_time, dpu_sgbm_mode);
        printf("left_path    = %s \nright_path   = %s \nref_res_path  = %s \ndpu_res_path  = %s \n", left_path, right_path, ref_res_path, dpu_res_path);
        printf("read_from_bin = %d \nwrite_to_bin  = %d \nfmt_str       = %s \nimg_size      = %d*%d \ntime_max      = %llu \ntime_avg      = %llu \nfps           = %llu \npixel_per_sec = %lluM pps \n", read_from_bin, write_to_bin, fmt_str, left_img.width, left_img.height, time_max, time_avg, fps_actual, pixel_per_sec);
    }
    else {
        printf("------Thread %d------ \nwidth         = %d \nheight        = %d \nloop          = %d \ndpu_sgbm_mode  = %d \n", dpu_sgbm_thread_arg.i_test, img_width, img_height, loop_time, dpu_sgbm_mode);
        printf("read_from_bin = %d \nwrite_to_bin  = %d \nfmt_str       = %s \nimg_size      = %d*%d \ntime_max      = %llu \ntime_avg      = %llu \nfps           = %llu \npixel_per_sec = %lluM pps \n", read_from_bin, write_to_bin, fmt_str, left_img.width, left_img.height, time_max, time_avg, fps_actual, pixel_per_sec);
    }
    if (dpu_sgbm_thread_arg.i_test == 0) {
        if (read_from_bin) {
            unsigned char *u8_dpu_res = (unsigned char*)malloc(img_size * dsize);
            ret =  bm_image_copy_device_to_host(disp_img, (void**)&u8_dpu_res);
            if (ret != BM_SUCCESS) {
                printf("[Thread %d] bm_image_copy_device_to_host fialed, ret = %d\n", dpu_sgbm_thread_arg.i_test, ret);
                free(u8_dpu_res);
                free(leftImg);
                free(rightImg);
                deinit_dpu(&left_img, &right_img, &disp_img);
                exit(-1);
            }
            ret = cmp_md5(ref_res_path, u8_dpu_res, img_size, dpu_sgbm_thread_arg.i_test);
            if (ret != BM_SUCCESS) {
                write_bin((void *)u8_dpu_res, "error_cmp.bin", img_size * dsize);
                printf("[Thread %d] bm_write_bin save error_cmp.bin \n", dpu_sgbm_thread_arg.i_test);
                free(u8_dpu_res);
                free(leftImg);
                free(rightImg);
                deinit_dpu(&left_img, &right_img, &disp_img);
                exit(-1);
            }
            if (write_to_bin) {
                write_bin((void *)u8_dpu_res, dpu_res_path, img_size * dsize);
                printf("[Thread %d] bm_write_bin write %s \n", dpu_sgbm_thread_arg.i_test, dpu_res_path);
            }
            free(u8_dpu_res);
        }
        else {
            unsigned char *left_img_cmodel = (unsigned char*) malloc (img_height * img_width * sizeof(unsigned char));
            unsigned char *right_img_cmodel = (unsigned char*) malloc (img_height * img_width * sizeof(unsigned char));
            copy_img_data(leftImg, left_img_cmodel, img_width, img_height);
            copy_img_data(rightImg, right_img_cmodel, img_width, img_height);
            if (dpu_sgbm_mode == DPU_SGBM_MUX1) {
                unsigned short *cpu_res = (unsigned short*) malloc (img_height * img_width * dsize);
                unsigned short *dpu_res = (unsigned short*) malloc (img_size * dsize);
                ret = test_cmodel_sgbm_u16(left_img_cmodel, right_img_cmodel, cpu_res, left_img.width, left_img.height, dpu_sgbm_mode, &dpu_sgbm_attr);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d] test_cmodel_sgbm_u16 failed, ret = %d \n", dpu_sgbm_thread_arg.i_test, ret);
                    free(cpu_res);
                    free(dpu_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                }
                ret =  bm_image_copy_device_to_host(disp_img, (void**)&dpu_res);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d] bm_image_copy_device_to_host fialed, ret = %d\n", dpu_sgbm_thread_arg.i_test, ret);
                    free(cpu_res);
                    free(dpu_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                }
                ret = cmp_u16(dpu_res, cpu_res, img_width, img_height, dpu_sgbm_thread_arg.i_test);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d][u16] DPU and CPU failed to compare \n", dpu_sgbm_thread_arg.i_test);
                    free(cpu_res);
                    free(dpu_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                }
                else
                    printf("[Thread %d][u16] Compare DPU result with CPU result successfully! \n", dpu_sgbm_thread_arg.i_test);
                if (write_to_bin) {
                    write_bin((void *)dpu_res, dpu_res_path, img_size * dsize);
                    printf("[Thread %d] bm_write_bin save %s \n", dpu_sgbm_thread_arg.i_test, dpu_res_path);
                }
                free(cpu_res);
                free(dpu_res);
            }
            else {
                unsigned char *cpu_res = (unsigned char*) malloc (img_height * img_width * dsize);
                unsigned char *dpu_res = (unsigned char*) malloc (img_size * dsize);
                ret = test_cmodel_sgbm(left_img_cmodel, right_img_cmodel, cpu_res, left_img.width, left_img.height, dpu_sgbm_mode, &dpu_sgbm_attr);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d] test_cmodel_sgbm_u8 failed, ret = %d \n", dpu_sgbm_thread_arg.i_test, ret);
                    free(cpu_res);
                    free(dpu_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                }
                ret =  bm_image_copy_device_to_host(disp_img, (void**)&dpu_res);
                if (ret != BM_SUCCESS) {
                    printf("bm_image_copy_device_to_host fialed, ret = %d\n", ret);
                    free(cpu_res);
                    free(dpu_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                }
                ret =  cmp_u8(dpu_res, cpu_res, img_width, img_height, dpu_sgbm_thread_arg.i_test);
                if (ret != BM_SUCCESS) {
                    printf("[Thread %d][u8] DPU and CPU failed to compare \n", dpu_sgbm_thread_arg.i_test);
                    free(cpu_res);
                    free(dpu_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                }
                else
                    printf("[Thread %d][u8] Compare DPU result with CPU result successfully! \n", dpu_sgbm_thread_arg.i_test);
                if (write_to_bin) {
                    write_bin((void *)dpu_res, dpu_res_path, img_size * dsize);
                    printf("[Thread %d] bm_write_bin save %s \n", dpu_sgbm_thread_arg.i_test, dpu_res_path);
                }
                free(cpu_res);
                free(dpu_res);
            }
            free(left_img_cmodel);
            free(right_img_cmodel);
        }
    }
    deinit_dpu(&left_img, &right_img, &disp_img);
    free(leftImg);
    free(rightImg);
    return 0;
}

int main(int argc, char **args) {
    srand((unsigned int)time(NULL));
    img_height = ALIGN((rand() % (IMG_MAX_HEIGHT - IMG_MIN_HEIGHT + 1) + IMG_MIN_HEIGHT), 2);
    img_width = ALIGN((rand() % (IMG_MAX_WIDTH - IMG_MIN_WIDTH + 1) + IMG_MIN_WIDTH), IMG_ALIGN);
    dpu_sgbm_mode = rand() % 3 + DPU_SGBM_MUX0;
    bm_status_t ret = BM_SUCCESS;
    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: \n");
        printf("%s thread_num | loop | width | height | left_path | right_path | ref_res_path | dpu_res_path \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 \n", args[0]);
        printf("%s 1 1 \n", args[0]);
        printf("%s 1 1 512 \n", args[0]);
        printf("%s 1 1 512 284 \n", args[0]);
        printf("%s 1 1 128 128 /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin sgbm_res.bin \n", args[0]);
        printf("* Need to set left_path, right_path, ref_res_path, dpu_res_path at same time \n");
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
            if ((img_width % 32) != 0 && (img_width % 16) != 0) dpu_sgbm_mode = DPU_SGBM_MUX2;
            break;
        }
        case 9: {
            dpu_sgbm_mode = DPU_SGBM_MUX2;
            read_from_bin = true;
            write_to_bin = true;
            int *vars[] = {&thread_num, &loop_time, &img_width, &img_height};
            for (int i = 1; i < 5; ++i)
                *vars[i - 1] = atoi(args[i]);
            char **strs[] = {&left_path, &right_path, &ref_res_path, &dpu_res_path};
            for (int i = 5; i < argc; ++i)
                *strs[i - 5] = args[i];
            break;
        }
        case 11: {
            dpu_sgbm_mode = DPU_SGBM_MUX2;
            read_from_bin = true;
            write_to_bin = false;
            int *vars_1[] = {&img_width, &img_height, &sgbm_mode};
            for (int i = 1; i < 4; ++i)
                *vars_1[i - 1] = atoi(args[i]);
            char **strs[] = {&left_path, &right_path, &ref_res_path};
            for (int i = 4; i < 7; ++i)
                *strs[i - 4] = args[i];
            int *vars_2[] = {&rang, &dev_id, &thread_num, &loop_time};
            for (int i = 7; i < argc; ++i)
                *vars_2[i - 7] = atoi(args[i]);
            dpu_sgbm_mode = (bmcv_dpu_sgbm_mode)sgbm_mode;
            disp_range = (bmcv_dpu_disp_range)rang;
            break;
        }
        default:
            printf("[TEST dpu sbgm] Thread nums should be 1~5,9 \n");
            exit(-1);
            break;
        }
    }
    ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d \n", ret);
        return -1;
    }
    dpu_sgbm_t dpu_sgbm_thread_arg[thread_num];
#ifdef __linux__
    pthread_t pid[thread_num];
    for (int i = 0; i < thread_num; i++) {
        dpu_sgbm_thread_arg[i].i_test = i;
        if (pthread_create(pid + i, NULL, test_dpu_sgbm, (void *)(dpu_sgbm_thread_arg + i))) {
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