#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

int main() {
    bm_status_t ret = BM_SUCCESS;
    bm_handle_t handle = NULL;
    int dev_id = 0;

    char *src_name = "/home/linaro/A2test/bmcv/test/res/dc_src_2240x2240_R.yuv", *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_dwa_fisheye_grid_info.bin", *grid_name = "/home/linaro/A2test/bmcv/test/res/R_grid_info_68_68_4624_70_70_dst_2240x2240_src_2240x2240.dat";

    int src_h = 2240, src_w = 2240;
    int dst_w = 2240, dst_h = 2240;
    bm_image src, dst;
    bm_image_format_ext fmt = FORMAT_YUV420P;
    ret = (int)bm_dev_request(&handle, dev_id);
    bmcv_fisheye_attr_s fisheye_attr = {0};
    fisheye_attr.grid_info.size = 446496;     // 注意：用户需根据实际的Grid_Info文件大小（字节数）进行设置
    fisheye_attr.bEnable = true;

    // create bm image
    bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
    // read image data from input files
    int image_byte_size[4] = {0};
    bm_image_get_byte_size(src, image_byte_size);
    int byte_size = src_w * src_h * 3 / 2;
    unsigned char *input_data = (unsigned char *)malloc(byte_size);
    FILE *fp_src = fopen(src_name, "rb");
    if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
      printf("file size is less than required bytes%d\n", byte_size);
    };
    fclose(fp_src);
    void* in_ptr[3] = {(void *)input_data, (void *)((unsigned char*)input_data + src_w * src_h), (void *)((unsigned char*)input_data + 5 / 4 * src_w * src_h)};
    bm_image_copy_host_to_device(src, in_ptr);
    // read grid_info data
    char *buffer = (char *)malloc(fisheye_attr.grid_info.size);
    memset(buffer, 0, fisheye_attr.grid_info.size);

    FILE *fp = fopen(grid_name, "rb");
    fread(buffer, 1, fisheye_attr.grid_info.size, fp);
    fclose(fp);
    fisheye_attr.grid_info.u.system.system_addr = (void *)buffer;

    bmcv_dwa_fisheye(handle, src, dst, fisheye_attr);
    bm_image_get_byte_size(dst, image_byte_size);
    unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
    void* out_ptr[3] = {(void*)output_ptr, (void*)((unsigned char*)output_ptr + dst_w * dst_h), (void*)((unsigned char*)output_ptr + 5 / 4 * dst_w * dst_h)};
    bm_image_copy_device_to_host(dst, (void **)out_ptr);

    FILE *fp_dst = fopen(dst_name, "wb");
    if (fwrite((void *)output_ptr, 1, byte_size, fp_dst) < (unsigned int)byte_size){
        printf("file size is less than %d required bytes\n", byte_size);
    };
    fclose(fp_dst);
    return ret;
}
