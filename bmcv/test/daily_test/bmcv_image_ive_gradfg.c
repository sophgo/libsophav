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
    bmcv_ive_gradfg_mode gradfg_mode = GRAD_FG_MODE_USE_CUR_GRAD;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_gradfg.bin";

    bm_handle_t handle = NULL;
    /* 3 by 3*/
    signed char arr3by3[25] = { 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0, -2, 0,
                    2, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0 };
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image stBgdiffFg;
    bm_image stCurGrad, stBgGrad, stGragFg;
    int u8Stride[4], u16Stride[4];

    // config setting

    // normGrad config
    bmcv_ive_normgrad_ctrl normGradAttr;
    normGradAttr.en_mode = BM_IVE_NORM_GRAD_OUT_COMBINE;
    normGradAttr.u8_norm = 8;
    memcpy(&normGradAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

    bmcv_ive_gradfg_attr gradFgAttr;
    gradFgAttr.en_mode = gradfg_mode;
    gradFgAttr.u16_edw_factor = 1000;
    gradFgAttr.u8_crl_coef_thr = 80;
    gradFgAttr.u8_mag_crl_thr = 4;
    gradFgAttr.u8_min_mag_diff = 2;
    gradFgAttr.u8_noise_val = 1;
    gradFgAttr.u8_edw_dark = 1;

    // calc ive image stride && create bm image struct
    int data_size = 1;
    u8Stride[0] = align_up(width, 16) * data_size;
    data_size = 2;
    u16Stride[0] = align_up(width, 16) * data_size;


    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stBgdiffFg, u8Stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &stCurGrad, u16Stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &stBgGrad, u16Stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stGragFg, u8Stride);

    ret = bm_image_alloc_dev_mem(stBgdiffFg, BMCV_HEAP1_ID);

    int image_byte_size[4] = {0};
    bm_image_get_byte_size(stBgdiffFg, image_byte_size);
    int byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    unsigned char *input_data = (unsigned char *)malloc(byte_size);
    FILE *fp_src = fopen(src_name, "rb");
    if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
    printf("file size is less than required bytes%d\n", byte_size);
    };
    fclose(fp_src);
    void* in_ptr[4] = {(void *)input_data,
                        (void *)((unsigned char*)input_data + image_byte_size[0]),
                        (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1]),
                        (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
    bm_image_copy_host_to_device(stBgdiffFg, in_ptr);


    ret = bm_image_alloc_dev_mem(stCurGrad, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(stBgGrad, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(stGragFg, BMCV_HEAP1_ID);

    // Create Fake Data.
    ret = bmcv_ive_norm_grad(handle, &stBgdiffFg, NULL, NULL, &stCurGrad, normGradAttr);

    normGradAttr.u8_norm = 2;
    ret = bmcv_ive_norm_grad(handle, &stBgdiffFg, NULL, NULL, &stBgGrad, normGradAttr);
    ret = bmcv_ive_gradfg(handle, stBgdiffFg, stCurGrad, stBgGrad, stGragFg, gradFgAttr);

    unsigned char *gradFg_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    memset(gradFg_res, 0, width * height * sizeof(unsigned char));

    ret = bm_image_copy_device_to_host(stGragFg, (void**)&gradFg_res);

    FILE *ive_fp = fopen(dst_name, "wb");
    fwrite((void *)gradFg_res, 1, width * height, ive_fp);
    fclose(ive_fp);
    free(gradFg_res);

    bm_image_destroy(&stBgdiffFg);
    bm_image_destroy(&stCurGrad);
    bm_image_destroy(&stBgGrad);
    bm_image_destroy(&stGragFg);

    bm_dev_free(handle);
    return 0;
}