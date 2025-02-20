#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define YUV_8BIT(y, u, v) ((((y)&0xff) << 16) | (((u)&0xff) << 8) | ((v)&0xff))

int main() {
    int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080, dev_id = 0;
    int yuv_8bit_y = 0, yuv_8bit_u = 0, yuv_8bit_v = 0;
    bm_image_format_ext fmt = FORMAT_YUV420P;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_yuv420.bin", *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_dwa_fisheye.bin";
    bm_handle_t handle = NULL;
    bmcv_fisheye_attr_s fisheye_attr = {0};
    fisheye_attr.bEnable = 1;
    fisheye_attr.bBgColor = 1;
    yuv_8bit_y = 0;
    yuv_8bit_u = 128;
    yuv_8bit_v = 128;
    fisheye_attr.u32BgColor = YUV_8BIT(yuv_8bit_y, yuv_8bit_u, yuv_8bit_v);
    fisheye_attr.s32HorOffset = src_w / 2;
    fisheye_attr.s32VerOffset = src_h / 2;
    fisheye_attr.u32TrapezoidCoef = 0;
    fisheye_attr.s32FanStrength = 0;
    fisheye_attr.enMountMode = 0;
    fisheye_attr.enUseMode = 1;
    fisheye_attr.enViewMode = 0;
    fisheye_attr.u32RegionNum = 1;
    fisheye_attr.grid_info.u.system.system_addr = NULL;
    fisheye_attr.grid_info.size = 0;

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    bm_image src, dst;

    bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

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

    bmcv_dwa_fisheye(handle, src, dst, fisheye_attr);

    bm_image_get_byte_size(dst, image_byte_size);
    byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
    void* out_ptr[4] = {(void*)output_ptr,
                        (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                        (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                        (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
    bm_image_copy_device_to_host(dst, (void **)out_ptr);

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

    return 0;
}