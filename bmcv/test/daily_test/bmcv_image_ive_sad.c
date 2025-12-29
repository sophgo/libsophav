#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

const int sad_write_img(char* file_name, unsigned char *data,
                        int sad_width, int sad_height, int dsize,
                        int stride){
    unsigned char* tmp = malloc(sad_width * dsize);
    FILE *fp = fopen(file_name, "wb");
    for(int h = 0; h < sad_height; h++, data += stride){
        memcpy(tmp, data, sad_width * dsize);
        fwrite((void *)tmp, 1, sad_width * dsize, fp);
    }
    fclose(fp);
    free(tmp);
    return 0;
}


int main(){
    int dev_id = 0;
    int height = 1080, width = 1920;
    bm_image_format_ext fmt = FORMAT_GRAY;
    bmcv_ive_sad_mode sadMode = BM_IVE_SAD_MODE_MB_8X8;
    bmcv_ive_sad_out_ctrl sadOutCtrl = BM_IVE_SAD_OUT_BOTH;
    int u16_thr = 0x800;
    int u8_min_val = 2;
    int u8_max_val = 30;
    char *src1_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin", *src2_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin";
    char *dstSad_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_Sad.bin", *dstThr_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_sad_thr.bin";
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    int dst_width = 0, dst_height = 0;
    bm_image src[2];
    bm_image dstSad, dstSadThr;
    int stride[4];
    int dstSad_stride[4];

    bmcv_ive_sad_attr sadAttr;
    bmcv_ive_sad_thresh_attr sadthreshAttr;
    sadAttr.en_mode = sadMode;
    sadAttr.en_out_ctrl = sadOutCtrl;
    sadthreshAttr.u16_thr = u16_thr;
    sadthreshAttr.u8_min_val = u8_min_val;
    sadthreshAttr.u8_max_val = u8_max_val;

    bm_image_data_format_ext dst_sad_dtype = DATA_TYPE_EXT_1N_BYTE;
    bool flag = false;

    // calc ive image stride && create bm image struct
    int data_size = 1;
    stride[0] = align_up(width, 16) * data_size;
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src[0], stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src[1], stride);

    dst_sad_dtype = DATA_TYPE_EXT_U16;
    data_size = 2;
    dstSad_stride[0] = align_up(width, 16) * data_size;
    bm_image_create(handle, height, width, fmt, dst_sad_dtype, &dstSad, dstSad_stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dstSadThr, stride);

    data_size = sizeof(unsigned short);
    dst_width = width  / 8;
    dst_height = height / 8;

    ret = bm_image_alloc_dev_mem(src[0], BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(src[1], BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dstSad, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dstSadThr, BMCV_HEAP1_ID);
    if (dstSad.data_type == DATA_TYPE_EXT_1N_BYTE){
        flag = true;
    }

    int byte_size;
    unsigned char *input_data;
    int image_byte_size[4] = {0};
    char *filename[] = {src1_name, src2_name};
    for (int i = 0; i < 2; i++) {
        bm_image_get_byte_size(src[i], image_byte_size);
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
        bm_image_copy_host_to_device(src[i], in_ptr);
    }

    ret = bmcv_ive_sad(handle, src, &dstSad, &dstSadThr, &sadAttr, &sadthreshAttr);

    unsigned char* sad_res = (unsigned char*)malloc(width * height * data_size);
    unsigned char* thr_res = (unsigned char*)malloc(width * height);
    memset(sad_res, 0, width * height * data_size);
    memset(thr_res, 0, width * height);

    ret = bm_image_copy_device_to_host(dstSad, (void **)&sad_res);
    ret = bm_image_copy_device_to_host(dstSadThr, (void **)&thr_res);

    int sad_stride = (flag) ? dstSad_stride[0] / 2 : dstSad_stride[0];
    sad_write_img(dstSad_name, sad_res, dst_width, dst_height, data_size, sad_stride);
    sad_write_img(dstThr_name, thr_res, dst_width, dst_height, sizeof(unsigned char), stride[0]);
    free(sad_res);
    free(thr_res);

    bm_image_destroy(&src[0]);
    bm_image_destroy(&src[1]);
    bm_image_destroy(&dstSad);
    bm_image_destroy(&dstSadThr);

    bm_dev_free(handle);
    return 0;
}