#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdlib.h>

#define IMAGE_CHN_NUM_MAX 3
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)

static bmcv_point_t org = {500, 500};
static int fontScale = 5;
static const char text[30] = "Hello, world!";
static unsigned char color[3] = {255, 0, 0};
static int thickness = 2;


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

int main()
{
    int width = 1920;
    int height = 1080;
    int format = FORMAT_YUV420P;
    int ret = 0;
    char* input_path = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char* output_path = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_put_text.bin";

    int i;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    offset_list[0] = width * height;
    offset_list[1] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;
    offset_list[2] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;

    int total_size = 0;
    unsigned char* data_tpu = (unsigned char*)malloc(width * height * IMAGE_CHN_NUM_MAX * sizeof(unsigned char));

    ret = readBin(input_path, data_tpu);

    bm_image input_img;
    unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
    bmcv_color_t rgb = {color[0], color[1], color[2]};

    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    ret = bm_image_alloc_dev_mem(input_img, BMCV_HEAP1_ID);

    in_ptr[0] = data_tpu;
    in_ptr[1] = data_tpu + offset_list[0];
    in_ptr[2] = data_tpu + offset_list[0] + offset_list[1];

    ret = bm_image_copy_host_to_device(input_img, (void**)in_ptr);
    ret = bmcv_image_put_text(handle, input_img, text, org, rgb, fontScale, thickness);
    ret = bm_image_copy_device_to_host(input_img, (void**)in_ptr);

    bm_image_destroy(&input_img);

    for (i = 0; i < IMAGE_CHN_NUM_MAX; ++i) {
        total_size += offset_list[i];
    }
    ret = writeBin(output_path, data_tpu, total_size);

    free(data_tpu);
    bm_dev_free(handle);
    return ret;
}
