#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdlib.h>

#define ALIGN(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
#define SATURATE(a, s, e) ((a) > (e) ? (e) : ((a) < (s) ? (s) : (a)))
#define IMAGE_CHN_NUM_MAX 3

static int line_num = 6;
static bmcv_point_t start[6] = {{0, 0}, {500, 0}, {500, 500}, {0, 500}, {0, 0}, {0, 500}};
static bmcv_point_t end[6] = {{500, 0}, {500, 500}, {0, 500}, {0, 0}, {500, 500}, {500, 0}};
static unsigned char color[3] = {255, 0, 0};
static int thickness = 4;

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
    int format = 0;
    int ret = 0;
    char *input_path = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *output_path = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_draw_lines.bin";
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }
    unsigned char* data_tpu = (unsigned char*)malloc(width * height * IMAGE_CHN_NUM_MAX * sizeof(unsigned char));
    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    int total_size = 0;
    ret = readBin(input_path, data_tpu);

    unsigned char* input = data_tpu;
    const bmcv_point_t* p1 = start;
    const bmcv_point_t* p2 = end;

    bm_image input_img;
    unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
    bmcv_color_t rgb = {color[0], color[1], color[2]};

    ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    ret = bm_image_alloc_dev_mem(input_img, BMCV_HEAP1_ID);

    offset_list[0] = width * height;
    offset_list[1] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;
    offset_list[2] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;

    in_ptr[0] = input;
    in_ptr[1] = input + offset_list[0];
    in_ptr[2] = input + offset_list[0] + offset_list[1];

    ret = bm_image_copy_host_to_device(input_img, (void**)in_ptr);
    bmcv_image_draw_lines(handle, input_img, p1, p2, line_num, rgb, thickness);
    ret = bm_image_copy_device_to_host(input_img, (void**)in_ptr);

    bm_image_destroy(&input_img);

    for (int i = 0; i < IMAGE_CHN_NUM_MAX; ++i) {
        total_size += offset_list[i];
    }

    ret = writeBin(output_path, data_tpu, total_size);

    free(data_tpu);
    bm_dev_free(handle);
    return ret;
}