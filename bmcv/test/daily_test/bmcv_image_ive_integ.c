#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main(){
    int dev_id = 0;
    int height = 1080, width = 1920;
    bm_image_format_ext src_fmt = FORMAT_GRAY;
    ive_integ_out_ctrl_e integ_mode = 0;
    char* src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_integ.bin";

    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bmcv_ive_integ_ctrl_s integ_attr;
    bm_image src;
    bm_device_mem_t dst;
    int src_stride[4];

    // calc ive image stride && create bm image struct
    int data_size = 1;
    src_stride[0] = align_up(width, 16) * data_size;

    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }

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
    data_size = sizeof(unsigned int);

    ret = bm_malloc_device_byte(handle, &dst, height * width * data_size);
    integ_attr.en_out_ctrl = integ_mode;

    ret = bmcv_ive_integ(handle, src, dst, integ_attr);

    unsigned int *dst_intg_u32 =  malloc(width * height * data_size);
    ret = bm_memcpy_d2s(handle, dst_intg_u32, dst);
    FILE *intg_result_fp = fopen(dst_name, "wb");
    fwrite((void *)dst_intg_u32, data_size, width * height, intg_result_fp);
    fclose(intg_result_fp);

    free(dst_intg_u32);

    bm_image_destroy(&src);
    bm_free_device(handle, dst);

    bm_dev_free(handle);
    return 0;
}