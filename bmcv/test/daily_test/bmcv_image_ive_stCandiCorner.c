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
    int u0q8QualityLevel = 25;
    bm_image_format_ext fmt = FORMAT_GRAY;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_stCandiCorner.bin";

    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image src, dst;
    int stride[4];

    bmcv_ive_stcandicorner_attr stCandiCorner_attr;
    memset(&stCandiCorner_attr, 0, sizeof(bmcv_ive_stcandicorner_attr));

    // calc ive image stride && create bm image struct
    int data_size = 1;
    stride[0] = align_up(width, 16) * data_size;
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    int attr_len = 4 * height * stride[0] + sizeof(bmcv_ive_st_max_eig);
    ret = bm_malloc_device_byte(handle, &stCandiCorner_attr.st_mem, attr_len * sizeof(unsigned char));
    stCandiCorner_attr.u0q8_quality_level = u0q8QualityLevel;

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

    ret = bmcv_ive_stcandicorner(handle, src, dst, stCandiCorner_attr);

    unsigned char *ive_res = (unsigned char *)malloc(width * height * sizeof(unsigned char));
    memset(ive_res, 0, width * height * sizeof(unsigned char));

    ret = bm_image_copy_device_to_host(dst, (void**)&ive_res);

    FILE *ive_fp = fopen(dst_name, "wb");
    fwrite((void *)ive_res, 1, width * height, ive_fp);
    fclose(ive_fp);

    free(input_data);
    free(ive_res);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);
    bm_free_device(handle, stCandiCorner_attr.st_mem);

    bm_dev_free(handle);
    return 0;
}