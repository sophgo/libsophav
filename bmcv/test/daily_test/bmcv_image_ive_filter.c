#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main(){
    int dev_id = 0;
    unsigned char u8Norm = 4;
    int height = 1080, width = 1920;
    bm_image_format_ext fmt = FORMAT_GRAY; // 14 4 6
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_filter.bin";
    bm_handle_t handle = NULL;

    signed char arr3by3[25] = { 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 2, 4,
                        2, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0 };

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    bm_image src, dst;
    int stride[4];

    bmcv_ive_filter_ctrl filterAttr;
    memcpy(filterAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));
    filterAttr.u8_norm = u8Norm;

    // calc ive image stride && create bm image struct
    int data_size = 1;
    stride[0] = align_up(width, 16) * data_size;

    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

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
    ret = bmcv_ive_filter(handle, src, dst, filterAttr);

    bm_image_get_byte_size(dst, image_byte_size);
    byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
    memset(output_ptr, 0, sizeof(byte_size));

    void* out_ptr[4] = {(void*)output_ptr,
                        (void*)((char*)output_ptr + image_byte_size[0]),
                        (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                        (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

    ret = bm_image_copy_device_to_host(dst, (void **)out_ptr);

    FILE *filter_fp = fopen(dst_name, "wb");
    fwrite((void *)output_ptr, 1, width * height, filter_fp);
    fclose(filter_fp);

    free(input_data);
    free(output_ptr);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    bm_dev_free(handle);
    return 0;
}