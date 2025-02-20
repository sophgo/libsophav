#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main() {
    int dev_id = 0;
    int height = 1080, width = 1920;
    bm_image_format_ext fmt = FORMAT_GRAY;
    char *src1_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin", *src2_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_frame_diff_motion.bin";
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image src1, src2, dst;
    int stride[4];

    // mask data
    unsigned char arr[25] = {0, 0, 255, 0, 0, 0, 0, 255, 0, 0, 255,
            255, 255, 255, 255, 0, 0, 255, 0, 0, 0, 0, 255, 0, 0};

    // config setting(Sub->threshold->erode->dilate)
    bmcv_ive_frame_diff_motion_attr attr;
    attr.sub_mode = IVE_SUB_ABS;
    attr.thr_mode = IVE_THRESH_BINARY;
    attr.u8_thr_min_val = 0;
    attr.u8_thr_max_val = 255;
    attr.u8_thr_low = 30;

    memcpy(&attr.au8_erode_mask, &arr, 25 * sizeof(unsigned char));
    memcpy(&attr.au8_dilate_mask, &arr, 25 * sizeof(unsigned char));

    // calc ive image stride && create bm image struct
    int data_size = 1;
    stride[0] = align_up(width, 16) * data_size;

    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src1, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src2, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

    ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    // read image data from input files
    int byte_size;
    unsigned char *input_data;
    int image_byte_size[4] = {0};
    char *filename[] = {src1_name, src2_name};
    bm_image src_images[] = {src1, src2};
    for (int i = 0; i < 2; i++) {
        bm_image_get_byte_size(src_images[i], image_byte_size);
        byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        input_data = (unsigned char *)malloc(byte_size);
        FILE *fp_src = fopen(filename[i], "rb");
        if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
        printf("file size is less than required bytes%d\n", byte_size);
        };
        fclose(fp_src);
        void* in_ptr[4] = {(void *)input_data,
                            (void *)((unsigned char*)input_data + image_byte_size[0]),
                            (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1]),
                            (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_host_to_device(src_images[i], in_ptr);
    }

    ret = bmcv_ive_frame_diff_motion(handle, src1, src2, dst, attr);


    unsigned char *ive_add_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    memset(ive_add_res, 0, width * height * sizeof(unsigned char));

    ret = bm_image_copy_device_to_host(dst, (void **)&ive_add_res);
    if(ret != BM_SUCCESS){
        printf("dst bm_image_copy_device_to_host failed, ret is %d \n", ret);
        exit(-1);
    }
    FILE *fp = fopen(dst_name, "wb");
    fwrite((void *)ive_add_res, 1, width * height * sizeof(unsigned char), fp);
    fclose(fp);

    free(input_data);
    free(ive_add_res);
    bm_image_destroy(&src1);
    bm_image_destroy(&src2);
    bm_image_destroy(&dst);

    bm_dev_free(handle);

    return 0;
}