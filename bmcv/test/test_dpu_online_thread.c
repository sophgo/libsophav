#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "bmcv_api_ext_c.h"

#define IMG_MAX_HEIGHT 1080
#define IMG_MAX_WIDHT  1920
#define IMG_MIN_HEIGHT 64
#define IMG_MIN_WIDHT  64
#define IMG_ALIGN      4
#define MD5_DIGEST_LENGTH 16
#define MD5_STRING_LENGTH (MD5_DIGEST_LENGTH * 2)
#define SLEEP_ON 0

#define ALIGN(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

extern void dpu_read_bin(const char *path, unsigned char *data, int size);
extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);
extern int md5_get(unsigned char* got, int size, char* md5_str);

extern bm_status_t bm_dpu_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride, bool bFgs);

extern bm_status_t test_cmodel_online(unsigned char *left_input, unsigned char *right_input,
                                      unsigned char *ref_output, int width, int height,
                                      bmcv_dpu_online_mode online_mode,
                                      bmcv_dpu_sgbm_attrs *grp,
                                      bmcv_dpu_fgs_attrs *fgs_grp);

extern bm_status_t test_cmodel_online_u16(unsigned char *left_input, unsigned char *right_input,
                                          unsigned short *ref_output, int width, int height,
                                          bmcv_dpu_online_mode online_mode,
                                          bmcv_dpu_sgbm_attrs *grp,
                                          bmcv_dpu_fgs_attrs *fgs_grp);


typedef struct dpu_online_ctx_{
    int loop;
    int i;
} dpu_online_ctx;

bm_handle_t handle = NULL;
char *left_name = "./dpu_data/sofa_left_img_512x284.bin";
char *right_name = "./dpu_data/sofa_right_img_512x284.bin";
char *ref_res_name = "./dpu_data/fgs_512x284_res.bin";
char *dpu_res_name = "./out/online_res.bin";
int img_width = 512;
int img_height = 284;
int test_loop_times  = 1;
int test_threads_num = 1;
int dev_id = 0;
int bWrite = 0;
int rand_mode = 0;
bmcv_dpu_online_mode dpu_mode = DPU_ONLINE_MUX0;
bmcv_dpu_disp_range  disp_range = BMCV_DPU_DISP_RANGE_16;

bm_status_t set_sgbm_default_param(bmcv_dpu_sgbm_attrs *grp_) {
    grp_->bfw_mode_en = 4;
    grp_->disp_range_en = 1;
    grp_->disp_start_pos = 0;
    grp_->dcc_dir_en = 1;
    grp_->dpu_census_shift = 1;
    grp_->dpu_rshift1 = 3;
    grp_->dpu_rshift2 = 2;
    grp_->dcc_dir_en = 1;
    grp_->dpu_ca_p1 = 1800;
    grp_->dpu_ca_p2 = 14400;
    grp_->dpu_uniq_ratio = 25;
    grp_->dpu_disp_shift = 4;
    return BM_SUCCESS;
}

void fill_img(unsigned char *data, int w, int h){
    for(int i = 0; i < h; i++){
        for(int j = 0; j < w; j++){
            data[i * ALIGN(w, 16) + j] = rand() % 256;
        }
    }
}

void copy_img_data(unsigned char *data, unsigned char *cmodel, int w, int h){
    for(int i = 0; i < h; i++){
        memcpy(cmodel + i * w, data + i * ALIGN(w, 16), w);
    }
}

int cmp_u8(unsigned char *got, unsigned char *exp, int width, int height)
{
    for(int i = 0; i < height ; i++){
        for(int j = 0; j < width; j++){
            if(got[i * ALIGN(width, 32) + j] != exp[i * width + j]){
                printf("Compare failed, at idx = %d, dpu_output = 0x%x, cmodel_output = 0x%x \n",
                           i, got[i * ALIGN(width, 32) + j], exp[i * width + j]);
                return -1;
            }
        }
    }
    return 0;
}

int cmp_u16(unsigned short *got, unsigned short *exp, int width, int height)
{
    for(int i = 0; i < height ; i++){
        for(int j = 0; j < width; j++){
            if(got[i * ALIGN(width, 32) + j] != exp[i * width + j]){
                printf("Compare failed, at idx = %d, dpu_output = 0x%x, cmodel_output = 0x%x \n",
                           i, got[i * ALIGN(width, 32) + j], exp[i * width + j]);
                return -1;
            }
        }
    }
    return 0;
}

int cmp_md5(char* file_name, unsigned char* got, int size){
    FILE *ref_fp = fopen(file_name, "rb");
    if(ref_fp == NULL){
        printf("%s : No such file \n", file_name);
        return -1;
    }

    unsigned char* ref = malloc(size);
    fread((void *)ref, 1, size, ref_fp);
    fclose(ref_fp);

    char ref_md5[MD5_STRING_LENGTH + 1];
    printf("ref ");
    md5_get(ref, size, ref_md5);

    return (md5_cmp(got, (unsigned char*)ref_md5, size));
}

bm_status_t init_dpu(bm_handle_t handle, bm_image *left,
                     bm_image *right, bm_image *output,
                     bm_image_format_ext dType,
                     unsigned char* leftData,
                     unsigned char* rightData,
                     int uWidth, int uHeight){
    bm_status_t ret = BM_SUCCESS;
    int src_stride[4], dst_stride[4];

    bm_dpu_image_calc_stride(handle, uHeight, uWidth, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, src_stride, false);
    bm_dpu_image_calc_stride(handle, uHeight, uWidth, FORMAT_GRAY, dType, dst_stride, true);

    ret = bm_image_create(handle, uHeight, uWidth, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, left, src_stride);
    if(ret != BM_SUCCESS){
        printf("left bm_image create failed. ret = %d \n", ret);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_create(handle, uHeight, uWidth, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, right, src_stride);
    if(ret != BM_SUCCESS){
        printf("right bm_image create failed. ret = %d \n", ret);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_create(handle, uHeight, uWidth, FORMAT_GRAY, dType, output, dst_stride);
    if(ret != BM_SUCCESS){
        printf("output bm_image create failed. ret = %d \n", ret);
        return BM_ERR_FAILURE;
    }

    if(bm_image_alloc_dev_mem(*left, BMCV_HEAP1_ID) != BM_SUCCESS){
        printf("bm_image_alloc_dev_mem_leftImg failed \n");
        return BM_ERR_FAILURE;
    }

    if(bm_image_alloc_dev_mem(*right, BMCV_HEAP1_ID) != BM_SUCCESS){
        printf("bm_image_alloc_dev_mem_rightImg failed \n");
        return BM_ERR_FAILURE;
    }

    if(bm_image_alloc_dev_mem(*output, BMCV_HEAP1_ID) != BM_SUCCESS){
        printf("bm_image_alloc_dev_mem_dispImg failed \n");
        return BM_ERR_FAILURE;
    }

    ret = bm_image_copy_host_to_device(*left, (void**)&leftData);
    if(ret != BM_SUCCESS){
        printf("left_img bm_image_copy_host_to_device failed \n");
        exit(-1);
    }

    ret = bm_image_copy_host_to_device(*right, (void**)&rightData);
    if(ret != BM_SUCCESS){
        printf("right_img bm_image_copy_host_to_device failed \n");
        exit(-1);
    }

    return ret;
}

bm_status_t deinit_dpu(bm_image *left, bm_image *right, bm_image *output)
{
    bm_image_destroy(left);
    bm_image_destroy(right);
    bm_image_destroy(output);

    return BM_SUCCESS;
}

static void * test_dpu_online(void* arg)
{
    dpu_online_ctx ctx = *(dpu_online_ctx*)arg;
    struct timeval tv_start;
    struct timeval tv_end;
    struct timeval timediff;
    unsigned long long time_single, time_total = 0, time_avg = 0;
    unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
    unsigned int i = 0, loop_time = 0;

    bm_status_t ret = BM_SUCCESS;
    bm_image_data_format_ext output_dtype = DATA_TYPE_EXT_1N_BYTE;
    bm_image left_img, right_img;
    bm_image disp_img;

    loop_time = ctx.loop;

    // set dpu config
    bmcv_dpu_sgbm_attrs sgbmAttrs;
    set_sgbm_default_param(&sgbmAttrs);
    sgbmAttrs.disp_range_en = disp_range;

    bmcv_dpu_fgs_attrs  fgsAttrs;
    fgsAttrs.depth_unit_en = 1;
    fgsAttrs.fgs_max_count = 19;
    fgsAttrs.fgs_max_t = 3;
    fgsAttrs.fxbase_line = 864000;

    // get img size
    (dpu_mode == DPU_ONLINE_MUX0) ? (output_dtype = DATA_TYPE_EXT_1N_BYTE) :
                                    (output_dtype = DATA_TYPE_EXT_U16);
    int dsize = (output_dtype == DATA_TYPE_EXT_1N_BYTE) ? 1 : 2;

    int img_size = ALIGN(img_width, 16) * img_height;
    printf("idx %d, img_width is %d , img_height is %d , dpu_mode is %d \n", ctx.i, img_width, img_height, dpu_mode);

    unsigned char *leftImg = (unsigned char*)malloc(img_size * sizeof(unsigned char));
    unsigned char *rightImg = (unsigned char*)malloc(img_size * sizeof(unsigned char));
    memset(leftImg, 0, img_size * sizeof(unsigned char));
    memset(rightImg, 0, img_size * sizeof(unsigned char));

    if(rand_mode){
        fill_img(leftImg, img_width, img_height);
        fill_img(rightImg, img_width, img_height);
    } else {
        dpu_read_bin(left_name, leftImg, img_size * sizeof(unsigned char));
        dpu_read_bin(right_name, rightImg, img_size * sizeof(unsigned char));
    }

    ret = init_dpu(handle, &left_img, &right_img, &disp_img,
                          output_dtype, leftImg, rightImg, img_width, img_height);
    if(ret != BM_SUCCESS){
        printf("init dpu sgbm failed, ret is %d \n", ret);
        exit(-1);
    }

    for (i = 0; i < loop_time; i++) {
        gettimeofday(&tv_start, NULL);
        ret = bmcv_dpu_online_disp(handle, &left_img,
                    &right_img, &disp_img, &sgbmAttrs, &fgsAttrs, dpu_mode);
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
            printf("bmcv_dpu_online_disp failed. ret = %d\n", ret);
            deinit_dpu(&left_img, &right_img, &disp_img);
            free(leftImg);
            free(rightImg);
            exit(-1);
        }
    }

    if(ctx.i == 0){
        int dst_img_size = ALIGN(img_width, 32) * img_height;

        if(rand_mode){
            // cmp cmodel
            if(dpu_mode == DPU_ONLINE_MUX0){
                unsigned char* u8_ref_res = (unsigned char*) malloc(img_height * img_width * dsize);
                unsigned char* u8_dpu_res = (unsigned char*) malloc(dst_img_size * dsize);

                unsigned char* left_img_cmodel = (unsigned char*) malloc (img_height * img_width * sizeof(unsigned char));
                unsigned char* right_img_cmodel = (unsigned char*) malloc (img_height * img_width * sizeof(unsigned char));

                copy_img_data(leftImg, left_img_cmodel, img_width, img_height);
                copy_img_data(rightImg, right_img_cmodel, img_width, img_height);

                // calc cmodel res
                ret = test_cmodel_online(left_img_cmodel, right_img_cmodel, u8_ref_res,
                            left_img.width, left_img.height, dpu_mode, &sgbmAttrs, &fgsAttrs);

                // d2s
                ret = bm_image_copy_device_to_host(disp_img, (void**)&u8_dpu_res);
                if(ret != BM_SUCCESS){
                    printf("bm_image_copy_device_to_host fialed, ret = %d\n", ret);
                    free(u8_dpu_res);
                    free(u8_ref_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                }

                // cmp
                int cmp = cmp_u8(u8_dpu_res, u8_ref_res, img_width, img_height);
                if(cmp != 0){
                    printf("[bmcv dpu online] idx %d, u8_dpu_res cmp failed \n", ctx.i);
                    free(u8_dpu_res);
                    free(u8_ref_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                } else {
                    printf("[bmcv dpu online] idx %d, u8_dpu_res cmp successful\n", ctx.i);
                }

                if(bWrite) bm_write_bin(disp_img, dpu_res_name);

                free(u8_dpu_res);
                free(u8_ref_res);
                free(left_img_cmodel);
                free(right_img_cmodel);
            } else {
                unsigned short *u16_ref_res = (unsigned short*) malloc (img_height * img_width * dsize);
                unsigned short *u16_dpu_res= (unsigned short*) malloc (dst_img_size * dsize);

                unsigned char* left_img_cmodel = (unsigned char*) malloc (img_height * img_width * sizeof(unsigned char));
                unsigned char* right_img_cmodel = (unsigned char*) malloc (img_height * img_width * sizeof(unsigned char));

                copy_img_data(leftImg, left_img_cmodel, img_width, img_height);
                copy_img_data(rightImg, right_img_cmodel, img_width, img_height);

                ret = test_cmodel_online_u16(left_img_cmodel, right_img_cmodel, u16_ref_res,
                         left_img.width, left_img.height, dpu_mode, &sgbmAttrs, &fgsAttrs);

                ret =  bm_image_copy_device_to_host(disp_img, (void**)&u16_dpu_res);
                if(ret != BM_SUCCESS){
                    printf("bm_image_copy_device_to_host fialed, ret = %d\n", ret);
                    free(u16_dpu_res);
                    free(u16_ref_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                }

                // cmp
                int cmp = cmp_u16(u16_dpu_res, u16_ref_res, img_width, img_height);

                if(cmp != 0){
                    printf("[bmcv dpu online] idx %d, u16_dpu_res cmp failed \n", ctx.i);
                    free(u16_dpu_res);
                    free(u16_ref_res);
                    free(left_img_cmodel);
                    free(right_img_cmodel);
                    free(leftImg);
                    free(rightImg);
                    deinit_dpu(&left_img, &right_img, &disp_img);
                    exit(-1);
                } else {
                    printf("[bmcv dpu online] idx %d, u16_dpu_res cmp successful\n", ctx.i);
                }

                if(bWrite) bm_write_bin(disp_img, dpu_res_name);

                free(u16_dpu_res);
                free(u16_ref_res);
                free(left_img_cmodel);
                free(right_img_cmodel);
            }

        } else {
            // cmp md5
            unsigned char* u8_dpu_res = (unsigned char*) malloc(dst_img_size * dsize);
            ret =  bm_image_copy_device_to_host(disp_img, (void**)&u8_dpu_res);
            if(ret != BM_SUCCESS){
                printf("bm_image_copy_device_to_host fialed, ret = %d\n", ret);
                free(u8_dpu_res);
                free(leftImg);
                free(rightImg);
                deinit_dpu(&left_img, &right_img, &disp_img);
                exit(-1);
            }

            // md5cmp
            int cmp = cmp_md5(ref_res_name, u8_dpu_res, img_size);

            if(cmp != 0){
                bm_write_bin(disp_img, "error_cmp.bin");
                free(u8_dpu_res);
                free(leftImg);
                free(rightImg);
                deinit_dpu(&left_img, &right_img, &disp_img);
                exit(-1);
            }

            if(bWrite)
                bm_write_bin(disp_img, dpu_res_name);

            free(u8_dpu_res);
        }
    }

    time_avg = time_total / loop_time;
    fps_actual = 1000000 / time_avg;
    pixel_per_sec = img_size * fps_actual/1024/1024;

    deinit_dpu(&left_img, &right_img, &disp_img);
    free(leftImg);
    free(rightImg);

    char fmt_str[100];
    format_to_str(disp_img.image_format, fmt_str);

    printf("[bmcv dpu sgbm]idx:%d, rand_mode is %d, fmt is %s, size is %d*%d, \n",ctx.i, rand_mode, fmt_str, left_img.width, left_img.height);
    printf("[bmcv dpu sgbm]idx:%d, loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
        ctx.i, loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

    return 0;

}

static void print_help(char **argv){
    printf("please follow this order:\n \
        %s width height dpu_mode left_name right_name  ref_name disp_range dev_id test_threads_num test_loop_times bWrite dpu_res_name\n \
        %s thread_num loop_num\n", argv[0], argv[0]);
};

int main(int argc, char **argv)
{
    if(argc >= 7){
        img_width = atoi(argv[1]);
        img_height = atoi(argv[2]);
        dpu_mode = atoi(argv[3]);
        left_name = argv[4];
        right_name = argv[5];
        ref_res_name = argv[6];
        if(argc > 7)  disp_range = atoi(argv[7]);
        if(argc > 8) dev_id = atoi(argv[8]);
        if(argc > 9) test_threads_num = atoi(argv[9]);
        if(argc > 10) test_loop_times = atoi(argv[10]);
        if(argc > 11) bWrite = atoi(argv[11]);
        if(argc > 12) dpu_res_name = argv[12];
    }

    if(argc == 2){
        test_threads_num  = atoi(argv[1]);
    }
    else if (argc == 3 ) {
        test_threads_num = atoi(argv[1]);
        test_loop_times  = atoi(argv[2]);
    }
    else if (argc == 4) {
        rand_mode = atoi(argv[1]);
        test_threads_num = atoi(argv[2]);
        test_loop_times  = atoi(argv[3]);
    } else if (argc > 4 && argc < 6){
        printf("command input error\n");
        print_help(argv);
        exit(-1);
    }

    if (test_threads_num > 8 || test_threads_num < 1) {
        printf("[TEST dpu online] thread nums should be 1~8\n");
        exit(-1);
    }

    bm_status_t ret = BM_SUCCESS;
    ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    if(rand_mode){
        img_height = rand() % (IMG_MAX_HEIGHT - IMG_MIN_HEIGHT + 1) + IMG_MIN_HEIGHT;
        img_width = rand() % (IMG_MAX_WIDHT - IMG_MIN_WIDHT + 1) + IMG_MIN_WIDHT;
        img_height = ALIGN(img_height, 2);
        img_width = ALIGN(img_width, IMG_ALIGN);
        dpu_mode = rand() % 3 + DPU_ONLINE_MUX0;
    }

    dpu_online_ctx ctx[test_threads_num];
    #ifdef __linux__
        pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
        for (int i = 0; i < test_threads_num; i++) {
            ctx[i].i = i;
            ctx[i].loop = test_loop_times;
            if (pthread_create(
                    &pid[i], NULL, test_dpu_online, (void *)(ctx + i))) {
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
