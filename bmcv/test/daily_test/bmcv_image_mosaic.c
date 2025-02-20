#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

void readBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

int main() {
    int src_h = 1080, src_w = 1920, dev_id = 0;
    bm_image_format_ext src_fmt = FORMAT_RGB_PACKED;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin";
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_mosaic.bin";

    bmcv_rect_t rect = {.start_x = 100, .start_y = 100, .crop_w = 500, .crop_h = 500};
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);

    unsigned char* input_data = malloc(src_h * src_w * 3);
    unsigned char* output_tpu = malloc(src_h * src_w * 3);
    readBin(src_name, input_data, src_h * src_w * 3);
    memset(output_tpu, 0, src_h * src_w * 3);

    // convert_ctx ctx = *(convert_ctx*)arg;
    bm_image src;
    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    ret = bm_image_alloc_dev_mem(src,BMCV_HEAP1_ID);
    unsigned char *in1_ptr[1] = {input_data};
    bm_image_copy_host_to_device(src, (void **)(in1_ptr));
    bmcv_image_mosaic(handle, 1, src, &rect, false);


    int image_byte_size[4] = {0};
    bm_image_get_byte_size(src, image_byte_size);
    int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
    void* out_ptr[4] = {(void*)output_ptr,
                        (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                        (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                        (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
    bm_image_copy_device_to_host(src, (void **)out_ptr);
    writeBin(dst_name, output_ptr, src_h * src_w *3);


    free(input_data);
    free(output_ptr);
    bm_image_destroy(&src);
    return ret;
}