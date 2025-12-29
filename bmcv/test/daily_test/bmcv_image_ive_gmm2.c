#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main(){
    int dev_id = 0;
    bool pixel_ctrl = false;
    gmm2_life_update_factor_mode
            life_update_enMode = LIFE_UPDATE_FACTOR_MODE_GLB;
    int height = 1080, width = 1920;
    bm_image_format_ext src_fmt = FORMAT_GRAY;
    char *input_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dstFg_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_gmm2_Fg.bin", *dstBg_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_gmm2_Bg.bin";

    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    bm_image src, src_factor;
    bm_image dst_fg, dst_bg, dst_model_match_model_info;
    bm_device_mem_t dst_model;
    int stride[4], factorStride[4];
    unsigned int u32FrameNumMax = 32;
    unsigned int u32FrmCnt = 0;
    unsigned int u32FrmNum = 0;

    bmcv_ive_gmm2_ctrl gmm2Attr;
    gmm2Attr.u16_var_rate = 1;
    gmm2Attr.u8_model_num = 3;
    gmm2Attr.u9q7_max_var = (16 * 16) << 7;
    gmm2Attr.u9q7_min_var = (8 * 8) << 7;
    gmm2Attr.u8_glb_sns_factor = 8;
    gmm2Attr.en_sns_factor_mode = SNS_FACTOR_MODE_GLB;
    gmm2Attr.u16_freq_thr = 12000;
    gmm2Attr.u16_freq_init_val = 20000;
    gmm2Attr.u16_freq_add_factor = 0xEF;
    gmm2Attr.u16_freq_redu_factor = 0xFF00;
    gmm2Attr.u16_life_thr = 5000;
    gmm2Attr.en_life_update_factor_mode = life_update_enMode;

    unsigned char* inputData = malloc(width * height * u32FrameNumMax * sizeof(unsigned char));
    FILE *input_fp = fopen(input_name, "rb");
    fread((void *)inputData, sizeof(unsigned char), width * height * u32FrameNumMax, input_fp);
    fclose(input_fp);

    unsigned char* srcData = malloc(width * height * sizeof(unsigned char));
    unsigned short* srcFactorData = malloc(width * height * sizeof(unsigned short));
    memset(srcData, 0, width * height * sizeof(unsigned char));
    memset(srcFactorData, 0, width * height * sizeof(unsigned short));

    int model_len = width * height * gmm2Attr.u8_model_num * 8;
    unsigned char* model_data = malloc(model_len * sizeof(unsigned char));
    memset(model_data, 0, model_len * sizeof(unsigned char));

    unsigned char* ive_fg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char* ive_bg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char* ive_pc_match_res = malloc(width * height * sizeof(unsigned char));

    memset(ive_fg_res, 0, width * height * sizeof(unsigned char));
    memset(ive_bg_res, 0, width * height * sizeof(unsigned char));
    memset(ive_pc_match_res, 0, width * height * sizeof(unsigned char));

    // calc ive image stride && create bm image struct
    int data_size = 1;
    stride[0] = align_up(width, 16) * data_size;

    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);

    factorStride[0] = align_up(width, 16) * data_size;

    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_U16, &src_factor, factorStride);
    ret = bm_image_alloc_dev_mem(src_factor, BMCV_HEAP1_ID);
    ret = bm_image_copy_host_to_device(src_factor, (void **)&srcFactorData);

    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_fg, stride);
    ret = bm_image_alloc_dev_mem(dst_fg, BMCV_HEAP1_ID);

    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_bg, stride);
    ret = bm_image_alloc_dev_mem(dst_bg, BMCV_HEAP1_ID);

    bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_model_match_model_info, stride);
    ret = bm_image_alloc_dev_mem(dst_model_match_model_info, BMCV_HEAP1_ID);

    ret = bm_malloc_device_byte(handle, &dst_model, model_len);

    ret = bm_memcpy_s2d(handle, dst_model, model_data);

    for(u32FrmCnt = 0; u32FrmCnt < u32FrameNumMax; u32FrmCnt++){
        if(width > 480){
            for(int i = 0; i < 288; i++){
                memcpy(srcData + (i * width),
                        inputData + (u32FrmCnt * 352 * 288 + i * 352), 352);
                memcpy(srcData + (i * width + 352),
                        inputData + (u32FrmCnt * 352 * 288 + i * 352), 352);

            }
        } else {
            for(int i = 0; i < 288; i++){
                memcpy(srcData + i * stride[0],
                        inputData + u32FrmCnt * width * 288 + i * width, width);
                int s = stride[0] - width;
                memset(srcData + i * stride[0] + width, 0, s);
            }
        }

        ret = bm_image_copy_host_to_device(src, (void**)&srcData);

        u32FrmNum = u32FrmCnt + 1;
        if(gmm2Attr.u8_model_num == 1)
            gmm2Attr.u16_freq_redu_factor = (u32FrmNum >= 500) ? 0xFFA0 : 0xFC00;
        else
            gmm2Attr.u16_glb_life_update_factor =
                                    (u32FrmNum >= 500) ? 4 : 0xFFFF / u32FrmNum;

        if(pixel_ctrl && u32FrmNum > 16)
            gmm2Attr.en_life_update_factor_mode = LIFE_UPDATE_FACTOR_MODE_PIX;

        ret = bmcv_ive_gmm2(handle, &src, &src_factor, &dst_fg, &dst_bg, &dst_model_match_model_info, dst_model, gmm2Attr);
    }

    ret = bm_image_copy_device_to_host(dst_fg, (void **)&ive_fg_res);
    ret = bm_image_copy_device_to_host(dst_bg, (void **)&ive_bg_res);
    ret = bm_image_copy_device_to_host(dst_model_match_model_info, (void **)&ive_pc_match_res);


    FILE *fg_fp = fopen(dstFg_name, "wb");
    fwrite((void *)ive_fg_res, 1, width * height, fg_fp);
    fclose(fg_fp);

    FILE *bg_fp = fopen(dstBg_name, "wb");
    fwrite((void *)ive_bg_res, 1, width * height, bg_fp);
    fclose(bg_fp);

    free(inputData);
    free(srcData);
    free(srcFactorData);
    free(model_data);
    free(ive_fg_res);
    free(ive_bg_res);
    free(ive_pc_match_res);
    bm_image_destroy(&src);
    bm_image_destroy(&src_factor);
    bm_image_destroy(&dst_fg);
    bm_image_destroy(&dst_model_match_model_info);
    bm_free_device(handle, dst_model);

    bm_dev_free(handle);
    return 0;
}
