#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int writeBin(const char* path, void* output_data, int size)
{
    int len = 0;
    FILE* fp_dst = fopen(path, "wb+");

    if (fp_dst == NULL) {
        perror("Error opening file\n");
        return -1;
    }

    len = fwrite((void*)output_data, 1, size, fp_dst);
    if (len < size) {
        printf("file size = %d is less than required bytes = %d\n", len, size);
        return -1;
    }

    fclose(fp_dst);
    return 0;
}

static int readBin(const char* path, void* input_data)
{
    int len;
    int size;
    FILE* fp_src = fopen(path, "rb");

    if (fp_src == NULL) {
        perror("Error opening file\n");
        return -1;
    }

    fseek(fp_src, 0, SEEK_END);
    size = ftell(fp_src);
    fseek(fp_src, 0, SEEK_SET);

    len = fread((void*)input_data, 1, size, fp_src);
    if (len < size) {
        printf("file size = %d is less than required bytes = %d\n", len, size);
        return -1;
    }

    fclose(fp_src);
    return 0;
}




int main() {
    int rot_angle = 90;
    int src_w = 1920, src_h = 1080, dst_w = 1080, dst_h = 1920, dev_id = 0;
    bm_image_format_ext src_fmt = FORMAT_RGB_PLANAR, dst_fmt = FORMAT_RGB_PLANAR;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgbp.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_rotate.bin";

    bm_handle_t handle = NULL;
    bm_status_t ret;
    ret = bm_dev_request(&handle, dev_id);
    bm_image src, dst;

    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    unsigned char* input_data = malloc(src_w * src_h * 3);
    unsigned char* in_ptr[3] = {input_data, input_data + src_w * src_h, input_data + src_w * src_h * 2};
    readBin(src_name, input_data);
    bm_image_copy_host_to_device(src, (void **)in_ptr);
    bmcv_image_rotate(handle, src, dst, rot_angle);
    bm_image_copy_device_to_host(dst, (void **)in_ptr);

    writeBin(dst_name, input_data, src_w * src_h * 3);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    free(input_data);
    bm_dev_free(handle);

    return ret;
}
