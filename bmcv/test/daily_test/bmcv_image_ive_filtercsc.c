#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main() {
    int dev_id = 0;
    unsigned char u8Norm = 4;
    int height = 1080, width = 1920;
    csc_type_t csc_type = CSC_YCbCr2RGB_BT601;
    bm_image_format_ext src_fmt = 2, dst_fmt = 10; // FORMAT_NV21: 4; yuv444p : 2 FORMAT_RGB_PACKED: 10
    char *src_name = "/home/linaro/A2test/bmcv/test/res/444_1920x1088.yuv";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_filterandcsc.bin";
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    bm_image src, dst;
    int src_stride[4], dst_stride[4];

    // filter data
    signed char arr3by3[25] = {0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 2,
                        4, 2, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0};

    // config setting
    bmcv_ive_filter_ctrl filterAttr;

    filterAttr.u8_norm = u8Norm;
    memcpy(filterAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

    // calc ive image stride && create bm image struct
    int data_size = 1;
    src_stride[0] = align_up(width, 16) * data_size;
    src_stride[1] = align_up(width, 16) * data_size;
    src_stride[2] = align_up(width, 16) * data_size;
    dst_stride[0] = align_up(width*3, 16) * data_size;

    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    int byte_size = width * height * 3;
    unsigned char *input_data = (unsigned char *)malloc(byte_size);
    FILE *fp_src = fopen(src_name, "rb");
    if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
      printf("file size is less than required bytes%d\n", byte_size);
    };
    fclose(fp_src);
    void* in_ptr[3] = {(void *)input_data,
                        (void *)((unsigned char*)input_data + width * height),
                        (void *)((unsigned char*)input_data + 2 * width * height)};
    bm_image_copy_host_to_device(src, in_ptr);

    ret = bmcv_ive_filter_and_csc(handle, src, dst, filterAttr, csc_type);

    unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
    memset(output_ptr, 0, sizeof(byte_size));

    void* out_ptr[3] = {(void*)output_ptr,
                        (void*)((char*)output_ptr + width * height),
                        (void*)((char*)output_ptr + 2 * width * height)};

    ret = bm_image_copy_device_to_host(dst, (void **)out_ptr);
    FILE *fp_dst = fopen(dst_name, "wb");
    if (fwrite((void *)output_ptr, 1, byte_size, fp_dst) < (unsigned int)byte_size){
        printf("file size is less than %d required bytes\n", byte_size);
    };
    fclose(fp_dst);

    free(input_data);
    free(output_ptr);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    bm_dev_free(handle);

    return ret;
}