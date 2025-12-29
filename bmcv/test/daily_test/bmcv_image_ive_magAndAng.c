#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main(){
    int dev_id = 0;
    int height = 1080, width = 1920;
    // int templateSize = 0; // 0: 3x3 1:5x5
    unsigned short thrValue = 0;
    bm_image_format_ext fmt = FORMAT_GRAY;
    bmcv_ive_mag_and_ang_outctrl enMode = BM_IVE_MAG_AND_ANG_OUT_ALL;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dst_magName = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_mag.bin", *dst_angName = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_ang.bin";

    /* 3 by 3*/
    signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                    2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    bm_image src;
    bm_image dst_mag, dst_ang;
    int stride[4];
    int magStride[4];

    bmcv_ive_mag_and_ang_ctrl magAndAng_attr;
    memset(&magAndAng_attr, 0, sizeof(bmcv_ive_mag_and_ang_ctrl));
    magAndAng_attr.en_out_ctrl = enMode;
    magAndAng_attr.u16_thr = thrValue;
    memcpy(magAndAng_attr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

    // calc ive image stride && create bm image struct
    int data_size = 1;
    stride[0] = align_up(width, 16) * data_size;
    data_size = 2;
    magStride[0] = align_up(width, 16) * data_size;

    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &dst_mag, magStride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst_mag, BMCV_HEAP1_ID);

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


    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst_ang, stride);
    ret = bm_image_alloc_dev_mem(dst_ang, BMCV_HEAP1_ID);

    ret = bmcv_ive_mag_and_ang(handle, &src, &dst_mag, &dst_ang, magAndAng_attr);

    unsigned short* magOut_res = malloc(width * height * sizeof(unsigned short));
    unsigned char*  angOut_res = malloc(width * height * sizeof(unsigned char));
    memset(magOut_res, 0, width * height * sizeof(unsigned short));
    memset(angOut_res, 0, width * height * sizeof(unsigned char));

    ret = bm_image_copy_device_to_host(dst_mag, (void **)&magOut_res);
    ret = bm_image_copy_device_to_host(dst_ang, (void **)&angOut_res);


    FILE *mag_fp = fopen(dst_magName, "wb");
    fwrite((void *)magOut_res, 1, width * height, mag_fp);
    fclose(mag_fp);

    FILE *ang_fp = fopen(dst_angName, "wb");
    fwrite((void *)angOut_res, 1, width * height, ang_fp);
    fclose(ang_fp);

    free(input_data);
    free(magOut_res);
    free(angOut_res);

    bm_image_destroy(&src);
    bm_image_destroy(&dst_mag);
    bm_image_destroy(&dst_ang);

    bm_dev_free(handle);
    return 0;
}