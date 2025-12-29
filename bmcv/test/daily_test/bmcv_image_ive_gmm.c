#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main(){
    int dev_id = 0;
    int height = 1080, width = 1920;
    bm_image_format_ext src_fmt = FORMAT_GRAY;
    char *input_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin";
    char *dstFg_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_gmm_Fg.bin", *dstBg_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_gmm_Bg.bin";
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    bm_image src;
    bm_image dst_fg, dst_bg;
    bm_device_mem_t dst_model;
    int stride[4];

    unsigned int u32FrameNumMax = 32;
    unsigned int u32FrmCnt = 0;

    bmcv_ive_gmm_ctrl gmmAttr;
    gmmAttr.u0q16_bg_ratio = 45875;
    gmmAttr.u0q16_init_weight = 3277;
    gmmAttr.u22q10_noise_var = 225 * 1024;
    gmmAttr.u22q10_max_var = 2000 * 1024;
    gmmAttr.u22q10_min_var = 200 * 1024;
    gmmAttr.u8q8_var_thr = (unsigned short)(256 * 6.25);
    gmmAttr.u8_model_num = 3;

    int model_len = width * height * gmmAttr.u8_model_num * 8;

    unsigned char* inputData = malloc(width * height * u32FrameNumMax * sizeof(unsigned char));
    FILE *input_fp = fopen(input_name, "rb");
    fread((void *)inputData, 1, width * height * u32FrameNumMax * sizeof(unsigned char), input_fp);
    fclose(input_fp);

    unsigned char* srcData = malloc(width * height * sizeof(unsigned char));
    unsigned char* ive_fg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char* ive_bg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char* model_data = malloc(model_len * sizeof(unsigned char));

    memset(srcData, 0, width * height * sizeof(unsigned char));
    memset(ive_fg_res, 0, width * height * sizeof(unsigned char));
    memset(ive_bg_res, 0, width * height * sizeof(unsigned char));
    memset(model_data, 0, model_len * sizeof(unsigned char));

    // calc ive image stride && create bm image struct
    int data_size = 1;
    stride[0] = align_up(width, 16) * data_size;
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_fg, stride);
    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_bg, stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst_fg, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst_bg, BMCV_HEAP1_ID);

    ret = bm_malloc_device_byte(handle, &dst_model, model_len);

    ret = bm_memcpy_s2d(handle, dst_model, model_data);
    for(u32FrmCnt = 0; u32FrmCnt < u32FrameNumMax; u32FrmCnt++){
        if(width > 480 ){
            for(int j = 0; j < 288; j++){
                memcpy(srcData + (j * width),
                        inputData + (u32FrmCnt * 352 * 288 + j * 352), 352);
                memcpy(srcData + (j * width + 352),
                        inputData + (u32FrmCnt * 352 * 288 + j * 352), 352);
            }
        } else {
            for(int j = 0; j < 288; j++){
                memcpy(srcData + j * stride[0],
                        inputData + u32FrmCnt * width * 288 + j * width, width);
                int s = stride[0] - width;
                memset(srcData + j * stride[0] + width, 0, s);
            }
        }

        ret = bm_image_copy_host_to_device(src, (void**)&srcData);

        if(u32FrmCnt >= 500)
            gmmAttr.u0q16_learn_rate = 131;  //0.02
        else
            gmmAttr.u0q16_learn_rate = 65535 / (u32FrmCnt + 1);

        ret = bmcv_ive_gmm(handle, src, dst_fg, dst_bg, dst_model, gmmAttr);
    }

    ret = bm_image_copy_device_to_host(dst_fg, (void **)&ive_fg_res);
    ret = bm_image_copy_device_to_host(dst_bg, (void **)&ive_bg_res);

    FILE *fg_fp = fopen(dstFg_name, "wb");
    fwrite((void *)ive_fg_res, 1, width * height, fg_fp);
    fclose(fg_fp);

    FILE *bg_fp = fopen(dstBg_name, "wb");
    fwrite((void *)ive_bg_res, 1, width * height, bg_fp);
    fclose(bg_fp);

    free(inputData);
    free(ive_fg_res);
    free(srcData);
    free(ive_bg_res);
    free(model_data);
    bm_image_destroy(&src);
    bm_image_destroy(&dst_fg);
    bm_image_destroy(&dst_bg);
    bm_free_device(handle, dst_model);

    bm_dev_free(handle);
    return 0;
}