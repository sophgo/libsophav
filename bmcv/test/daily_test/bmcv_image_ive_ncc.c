#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main(){
    int dev_id = 0;
    int height = 288, width = 352;;
    bm_image_format_ext src_fmt = FORMAT_GRAY;
    char *src1_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin", *src2_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_ncc.bin";
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image src1, src2;
    bm_device_mem_t dst;
    int src_stride[4];

    // calc ive image stride && create bm image struct
    int data_size = 1;
    src_stride[0] = align_up(width, 16) * data_size;

    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src1, src_stride);
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src2, src_stride);
    ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP1_ID);

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

    int data_len = sizeof(bmcv_ive_ncc_dst_mem_t);

    ret = bm_malloc_device_byte(handle, &dst, data_len);

    ret = bmcv_ive_ncc(handle, src1, src2, dst);

    unsigned long long *ncc_result = malloc(data_len);
    ret = bm_memcpy_d2s(handle, ncc_result, dst);
    unsigned long long *numerator = ncc_result;
    unsigned long long *quadSum1 = ncc_result + 1;
    unsigned long long *quadSum2 = quadSum1 + 1;
    float fr = (float)((double)*numerator / (sqrt((double)*quadSum1) * sqrt((double)*quadSum2)));
    printf("bmcv ive NCC value is %f \n", fr);

    FILE *ncc_result_fp = fopen(dst_name, "wb");
    fwrite((void *)ncc_result, 1, data_len, ncc_result_fp);
    fclose(ncc_result_fp);

    free(input_data);
    free(ncc_result);

    bm_free_device(handle, dst);
    bm_dev_free(handle);
    return 0;
}