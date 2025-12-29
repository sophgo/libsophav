#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main(){
    int dev_id = 0;
    bmcv_resize_algorithm resize_mode = BMCV_INTER_AREA;
    bm_image_format_ext format = FORMAT_GRAY;
    int src_width = 1920, src_height = 1080;
    int dst_width = 400;
    int dst_height = 300;

    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_resize.bin";

    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image src, dst;
    int src_stride[4], dst_stride[4];

    int data_size= 1;
    int byte_size;
    int image_byte_size[4] = {0};
    // calc ive image stride && create bm image struct
    src_stride[0] = align_up(src_width, 16) * data_size;
    bm_image_create(handle, src_height, src_width, format, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);

    dst_stride[0] = align_up(dst_width, 16) * data_size;
    bm_image_create(handle, dst_height, dst_width, format, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    bm_image_get_byte_size(src, image_byte_size);
    byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
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

    ret = bmcv_ive_resize(handle, src, dst, resize_mode);

    bm_image_get_byte_size(dst, image_byte_size);
    byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
    memset(output_ptr, 0, sizeof(byte_size));

    void* out_ptr[4] = {(void*)output_ptr,
                    (void*)((char*)output_ptr + image_byte_size[0]),
                    (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                    (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

    ret = bm_image_copy_device_to_host(dst, (void **)out_ptr);

    FILE *ive_fp = fopen(dst_name, "wb");
    fwrite((void *)output_ptr, 1, byte_size, ive_fp);
    fclose(ive_fp);

    free(input_data);
    free(output_ptr);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    bm_dev_free(handle);
    return 0;
}