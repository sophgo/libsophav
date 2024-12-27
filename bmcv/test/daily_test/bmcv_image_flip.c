#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

int readBin(const char * path, unsigned char* input_data, int size) {
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
};

    fclose(fp_src);
    return 0;
}

int writeBin(const char * path, unsigned char* input_data, int size) {
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
    return 0;
}

int main() {
    int src_h = 1080;
    int src_w = 1920;
    int dst_w = 1920;
    int dst_h = 1080;
    bm_image_format_ext src_fmt = FORMAT_GRAY;
    bm_image_format_ext dst_fmt = 14;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_flip.bin";
    bmcv_flip_mode flip_mode = HORIZONTAL_FLIP;
    bm_handle_t handle;
    bm_status_t ret = 0;
    bm_image src, dst;
    unsigned char* data_tpu = (unsigned char*)malloc(src_w * src_h * sizeof(unsigned char));
    ret = bm_dev_request(&handle, 0);

    ret = readBin(src_name, data_tpu, src_h * src_w);

    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

    ret = bm_image_alloc_dev_mem(src,BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst,BMCV_HEAP1_ID);

    unsigned char* in_ptr[1] = {0};

    in_ptr[0] = data_tpu;

    ret = bm_image_copy_host_to_device(src, (void**)in_ptr);
    bmcv_image_flip(handle, src, dst, flip_mode);
    ret = bm_image_copy_device_to_host(dst, (void**)in_ptr);
    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    ret = writeBin(dst_name, data_tpu, src_w * src_h);


    return ret;
}

