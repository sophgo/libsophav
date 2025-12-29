#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

typedef unsigned int u32;

int main() {
    int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080;
    int dev_id = 0;
    bm_image_format_ext fmt = FORMAT_GRAY;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_dwa_dewarp.bin";
    char *grid_name = "/home/linaro/A2test/bmcv/test/res/grid_info.dat";
    bm_handle_t handle = NULL;
    u32 grid_size = 0;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    FILE *fp = fopen(grid_name, "rb");
    if (!fp) {
        printf("open file:%s failed.\n", grid_name);
        exit(-1);
    }
    char *grid_data = (char *)malloc(grid_size);
    fread(grid_data, 1, grid_size, fp);

    fclose(fp);

    bm_image src, dst;

    dst_w = src_w;
    dst_h = src_h;
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

    bm_device_mem_t dmem;
    dmem.u.system.system_addr = (void *)grid_data;
    dmem.size = grid_size;

    bmcv_dwa_dewarp(handle, src, dst, dmem);

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

    free(grid_data);
    free(input_data);
    free(output_ptr);
    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    bm_dev_free(handle);

    return 0;
}

