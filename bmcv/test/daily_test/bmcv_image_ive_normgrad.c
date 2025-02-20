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
    bmcv_ive_normgrad_outmode enMode = BM_IVE_NORM_GRAD_OUT_HOR_AND_VER;
    bm_image_format_ext fmt = FORMAT_GRAY;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dst_hName = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_normgrad_h.bin", *dst_vName = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_normgrad_v.bin";

    bm_handle_t handle = NULL;
    /* 3 by 3*/
    signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                    2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image src;
    bm_image dst_H, dst_V, dst_conbine_HV;
    int src_stride[4];
    int dst_stride[4];

    bmcv_ive_normgrad_ctrl normgrad_attr;
    normgrad_attr.u8_norm = 8;
    normgrad_attr.en_mode = enMode;
    (memcpy(normgrad_attr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char)));

    // calc ive image stride && create bm image struct
    int data_size = 1;
    src_stride[0] = align_up(width, 16) * data_size;
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);

    int image_byte_size[4] = {0};
    bm_image_get_byte_size(src, image_byte_size);
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
    bm_image_copy_host_to_device(src, in_ptr);

    dst_stride[0] = align_up(width, 16) * data_size;
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, &dst_H, dst_stride);
    ret = bm_image_alloc_dev_mem(dst_H, BMCV_HEAP1_ID);


    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, &dst_V, dst_stride);
    ret = bm_image_alloc_dev_mem(dst_V, BMCV_HEAP1_ID);

    ret = bmcv_ive_norm_grad(handle, &src, &dst_H, &dst_V, &dst_conbine_HV, normgrad_attr);

    signed char* ive_h_res = malloc(width * height * sizeof(signed char));
    signed char* ive_v_res = malloc(width * height * sizeof(signed char));
    unsigned short* ive_combine_res = malloc(width * height * sizeof(unsigned short));

    memset(ive_h_res, 0, width * height * sizeof(signed char));
    memset(ive_v_res, 0, width * height * sizeof(signed char));
    memset(ive_combine_res, 1, width * height * sizeof(unsigned short));

    ret = bm_image_copy_device_to_host(dst_H, (void**)&ive_h_res);
    ret = bm_image_copy_device_to_host(dst_V, (void**)&ive_v_res);

    FILE *iveH_fp = fopen(dst_hName, "wb");
    fwrite((void *)ive_h_res, sizeof(signed char), width * height, iveH_fp);
    fclose(iveH_fp);

    FILE *iveV_fp = fopen(dst_vName, "wb");
    fwrite((void *)ive_v_res, sizeof(signed char), width * height, iveV_fp);
    fclose(iveV_fp);

    free(input_data);
    free(ive_h_res);
    free(ive_v_res);
    free(ive_combine_res);

    bm_image_destroy(&dst_H);
    bm_image_destroy(&dst_V);

    bm_dev_free(handle);
    return ret;
}