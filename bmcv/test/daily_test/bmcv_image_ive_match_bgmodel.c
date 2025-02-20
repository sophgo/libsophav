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
    bm_image_format_ext fmt = FORMAT_GRAY;
    char *input_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *ref_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_match_bgmodel.bin";

    bm_handle_t handle = NULL;

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    int srcStride[4];

    int frameNum;
    int frameNumMax = 100;
    int frmCnt = 0;

    int buf_size = frameNumMax * width * height;
    unsigned char *input_data = (unsigned char*)malloc(buf_size * sizeof(unsigned char));
    unsigned char *src_data = (unsigned char*)malloc(buf_size * sizeof(unsigned char));
    unsigned char *bgmodel_res = (unsigned char*) malloc(width * sizeof(bmcv_ive_bg_model_pix) * height);

    memset(input_data, 0, buf_size * sizeof(unsigned char));
    memset(src_data, 0, buf_size * sizeof(unsigned char));
    memset(bgmodel_res, 0, width * sizeof(bmcv_ive_bg_model_pix) * height);

    FILE *input_fp = fopen(input_name, "rb");
    if(input_fp == NULL){
        printf("%s : No such file \n", ref_name);
        exit(-1);
    }
    fread((void *)input_data, 1, buf_size * sizeof(unsigned char), input_fp);
    fclose(input_fp);

    // create src image
    bm_image src, stFgFlag, stDiffFg, stBgModel;
    bm_device_mem_t stStatData;
    bmcv_ive_match_bgmodel_attr matchBgmodel_attr;

    int u8Stride[4] = {0}, s16Stride[4] = {0}, bgModelStride[4] = {0};

    int data_size = 1;
    u8Stride[0] = align_up(width, 16) * data_size;
    bgModelStride[0] = align_up(width * sizeof(bmcv_ive_bg_model_pix), 16) * data_size;
    data_size = 2;
    s16Stride[0] = align_up(width, 16) * data_size;

    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, u8Stride);
    ret = bm_image_create(handle, height, width * sizeof(bmcv_ive_bg_model_pix), fmt, DATA_TYPE_EXT_1N_BYTE, &stBgModel, bgModelStride);
    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stFgFlag, u8Stride);
    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_S16, &stDiffFg, s16Stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(stFgFlag, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(stDiffFg, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(stBgModel, BMCV_HEAP1_ID);
    ret = bm_malloc_device_byte(handle, &stStatData, sizeof(bmcv_ive_bg_stat_data));

    // init match_bgmodel config
    matchBgmodel_attr.u32_cur_frm_num = 0;
    matchBgmodel_attr.u32_pre_frm_num = 0;
    matchBgmodel_attr.u16_time_thr = 20;
    matchBgmodel_attr.u8_diff_thr_crl_coef = 0;
    matchBgmodel_attr.u8_diff_max_thr = 10;
    matchBgmodel_attr.u8_diff_min_thr = 10;
    matchBgmodel_attr.u8_diff_thr_inc = 0;
    matchBgmodel_attr.u8_fast_learn_rate = 4;
    matchBgmodel_attr.u8_det_chg_region = 1;

    data_size = 1;
    srcStride[0] = align_up(width, 16) * data_size;

    // create dst image
    bm_image stBgImg, stChgStaLife;
    bmcv_ive_update_bgmodel_attr update_bgmodel_attr;

    int bg_img_stride[4] = {0}, chg_sta_stride[4] = {0};

    data_size = 1;
    bg_img_stride[0] = align_up(width, 16) * data_size;

    data_size = 4;
    chg_sta_stride[0] = align_up(width, 16) * data_size;

    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stBgImg, bg_img_stride);
    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U32, &stChgStaLife, chg_sta_stride);
    ret = bm_image_alloc_dev_mem(stBgImg, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(stChgStaLife, BMCV_HEAP1_ID);

    // init update_bgmodel config
    update_bgmodel_attr.u32_cur_frm_num = 0;
    update_bgmodel_attr.u32_pre_chk_time = 0;
    update_bgmodel_attr.u32_frm_chk_period = 30;
    update_bgmodel_attr.u32_init_min_time = 25;
    update_bgmodel_attr.u32_sty_bg_min_blend_time = 100;
    update_bgmodel_attr.u32_sty_bg_max_blend_time = 1500;
    update_bgmodel_attr.u16_fg_max_fade_time = 15;
    update_bgmodel_attr.u16_bg_max_fade_time = 60;
    update_bgmodel_attr.u8_sty_bg_acc_time_rate_thr = 80;
    update_bgmodel_attr.u8_chg_bg_acc_time_rate_thr = 60;
    update_bgmodel_attr.u8_dyn_bg_acc_time_thr = 0;
    update_bgmodel_attr.u8_dyn_bg_depth = 3;
    update_bgmodel_attr.u8_bg_eff_sta_rate_thr = 90;
    update_bgmodel_attr.u8_acce_bg_learn = 0;
    update_bgmodel_attr.u8_det_chg_region = 1;

    unsigned char* fg_flag = malloc(width * height);
    unsigned char* bg_model = (unsigned char*)malloc(width * sizeof(bmcv_ive_bg_model_pix) * height);

    bmcv_ive_bg_stat_data *stat = malloc(sizeof(bmcv_ive_bg_stat_data));

    memset(fg_flag, 0, width * height);
    memset(bg_model, 0, width * sizeof(bmcv_ive_bg_model_pix) * height);

    for(int i = 0; i < 1; i++){
        int updCnt = 5;
        int preUpdTime = 0;
        int preChkTime = 0;
        int frmUpdPeriod = 10;
        int frmChkPeriod = 30;

        ret = bm_image_copy_host_to_device(stFgFlag, (void**)&fg_flag);
        ret = bm_image_copy_host_to_device(stBgModel, (void**)&bg_model);

        // config setting
        matchBgmodel_attr.u32_cur_frm_num = frmCnt;
        for(frmCnt = 0; frmCnt < frameNumMax; frmCnt++){
            frameNum = frmCnt + 1;
            if(width > 480){
                for(int j = 0; j < 288; j++){
                    memcpy(src_data + (j * width),
                        input_data + (frmCnt * 352 * 288 + j * 352), 352);
                    memcpy(src_data + (j * width + 352),
                        input_data + (frmCnt * 352 * 288 + j * 352), 352);
                }
            } else {
                for(int j = 0; j < 288; j++){
                    memcpy(src_data + j * srcStride[0],
                        input_data + frmCnt * width * 288 + j * width, width);
                    int s = srcStride[0] - width;
                    memset(src_data + j * srcStride[0] + width, 0, s);
                }
            }

            ret = bm_image_copy_host_to_device(src, (void**)&src_data);

            matchBgmodel_attr.u32_pre_frm_num = matchBgmodel_attr.u32_cur_frm_num;
            matchBgmodel_attr.u32_cur_frm_num = frameNum;
            ret = bmcv_ive_match_bgmodel(handle, src, stBgModel,
                            stFgFlag, stDiffFg, stStatData, matchBgmodel_attr);

            ret = bm_memcpy_d2s(handle, (void*)stat, stStatData);

            if(updCnt == 0 || frameNum >= preUpdTime + frmUpdPeriod){
                updCnt++;
                preUpdTime = frameNum;
                update_bgmodel_attr.u32_cur_frm_num = frameNum;
                update_bgmodel_attr.u32_pre_chk_time = preChkTime;
                update_bgmodel_attr.u32_frm_chk_period = 0;
                if(frameNum >= preChkTime + frmChkPeriod){
                    update_bgmodel_attr.u32_frm_chk_period = frmChkPeriod;
                    preChkTime = frameNum;
                }

                ret = bmcv_ive_update_bgmodel(handle, &src, &stBgModel, &stFgFlag, &stBgImg, &stChgStaLife, stStatData, update_bgmodel_attr);
                ret = bm_memcpy_d2s(handle, (void*)stat, stStatData);
            }
        }

        ret = bm_image_copy_device_to_host(stBgModel, (void**)&bgmodel_res);
    }
    free(input_data);
    free(src_data);
    free(fg_flag);
    free(bg_model);
    free(stat);

    FILE *fp = fopen(ref_name, "wb");
    fwrite((void *)bgmodel_res, 1, width * sizeof(bmcv_ive_bg_model_pix) * height, fp);
    fclose(fp);

    free(bgmodel_res);

    bm_image_destroy(&src);
    bm_image_destroy(&stFgFlag);
    bm_image_destroy(&stDiffFg);
    bm_image_destroy(&stBgModel);
    bm_free_device(handle, stStatData);

    bm_image_destroy(&stBgImg);
    bm_image_destroy(&stChgStaLife);

    bm_dev_free(handle);
    return ret;
}
