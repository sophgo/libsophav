#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

int main() {
    int src_h = 1080, src_w = 1920, dev_id = 0;
    bm_image_format_ext fmt = FORMAT_YUV420P;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_yuv420.bin", *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_dwa_gdc.bin";
    bm_handle_t handle = NULL;
    bmcv_gdc_attr ldc_attr = {true, 0, 0, 0, 0, 0, -200, };
    fmt = FORMAT_RGB_PLANAR;
    ldc_attr.grid_info.size = 0;
    ldc_attr.grid_info.u.system.system_addr = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image src, dst;
    int dst_w, dst_h;

    bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);

    dst_w = src_w;
    dst_h = src_h;
    bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    int image_byte_size[4] = {0};
    bm_image_get_byte_size(src, image_byte_size);
    for (int i = 0; i < 4; i++) {
        printf("image_byte_size[%d] is : %d\n", i, image_byte_size[i]);
    }
    int byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    // int byte_size = src_w * src_h * 3 / 2;
    unsigned char *input_data = (unsigned char *)malloc(byte_size);
    FILE *fp_src = fopen(src_name, "rb");
    if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
    printf("file size is less than required bytes%d\n", byte_size);
    };
    fclose(fp_src);
    bm_image_copy_host_to_device(src, (void *)&input_data);

    bmcv_dwa_gdc(handle, src, dst, ldc_attr);

    unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
    void* out_ptr[4] = {(void*)output_ptr,
                        (void*)((unsigned char*)output_ptr + dst_w * dst_h),
                        (void*)((unsigned char*)output_ptr + 5 / 4 * dst_w * dst_w)};
    bm_image_copy_device_to_host(dst, (void **)out_ptr);

    FILE *fp_dst = fopen(dst_name, "wb");
    if (fwrite((void *)input_data, 1, byte_size, fp_dst) < (unsigned int)byte_size){
        printf("file size is less than %d required bytes\n", byte_size);
    };
    fclose(fp_dst);

    free(input_data);
    free(output_ptr);
    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    bm_dev_free(handle);

    return 0;
}