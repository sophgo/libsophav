#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>

#define NPU_NUM 32
#define KERNEL_SIZE 3 * 3 * 3 * 4 * 32
#define CONVD_MATRIX 12 * 9

const unsigned char convd_kernel_bg8[CONVD_MATRIX] = {1, 0, 1, 0, 0, 0, 1, 0, 1, //Rb
                                                    0, 0, 2, 0, 0, 0, 0, 0, 2, //Rg1
                                                    0, 0, 0, 0, 0, 0, 2, 0, 2, //Rg2
                                                    0, 0, 0, 0, 0, 0, 0, 0, 4, //Rr
                                                    4, 0, 0, 0, 0, 0, 0, 0, 0, //Bb
                                                    2, 0, 2, 0, 0, 0, 0, 0, 0, //Bg1
                                                    2, 0, 0, 0, 0, 0, 2, 0, 0, //Bg2
                                                    1, 0, 1, 0, 0, 0, 1, 0, 1, //Br
                                                    0, 1, 0, 1, 0, 1, 0, 1, 0, //Gb
                                                    0, 0, 0, 0, 0, 4, 0, 0, 0, //Gg1
                                                    0, 0, 0, 0, 0, 0, 0, 4, 0, //Gg2
                                                    0, 1, 0, 1, 0, 1, 0, 1, 0};//Gr
const unsigned char convd_kernel_rg8[CONVD_MATRIX] = {4, 0, 0, 0, 0, 0, 0, 0, 0, //Rr
                                                    2, 0, 2, 0, 0, 0, 0, 0, 0, //Rg1
                                                    2, 0, 0, 0, 0, 0, 2, 0, 0, //Rg2
                                                    1, 0, 1, 0, 0, 0, 1, 0, 1, //Rb
                                                    1, 0, 1, 0, 0, 0, 1, 0, 1, //Br
                                                    0, 0, 2, 0, 0, 0, 0, 0, 2, //Bg1
                                                    0, 0, 0, 2, 0, 2, 0, 0, 0, //Bg2
                                                    0, 0, 0, 0, 0, 0, 0, 0, 4, //Bb
                                                    1, 0, 1, 0, 0, 0, 1, 0, 1, //Gr
                                                    0, 0, 0, 0, 0, 4, 0, 0, 0, //Gg1
                                                    0, 0, 0, 0, 0, 0, 0, 4, 0, //Gg2
                                                    0, 1, 0, 1, 0, 1, 0, 1, 0};//Gb
void readBin(const char *path, unsigned char* input_data, int size) {
    FILE *fp_src = fopen(path, "rb");
    if (fp_src == NULL) {
        printf("无法打开输出文件 %s\n", path);
        return;
    }
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };
    fclose(fp_src);
}


int main() {
    int width = 6000;
    int height = 4000;
    int src_type = 0;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *output_path = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_bayer2rgb.bin";

    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }
    unsigned char* output = (unsigned char*)malloc(width * height * 3);
    unsigned char* input = (unsigned char*)malloc(width * height);
    unsigned char kernel_data[KERNEL_SIZE] = {0};

        // reconstructing kernel data
        for (int i = 0;i < 12;i++) {
        for (int j = 0;j < 9;j++) {
        if (src_type == 0) {
            kernel_data[i * 9 * NPU_NUM + NPU_NUM * j] = convd_kernel_bg8[i * 9 + j];
        } else {
            kernel_data[i * 9 * NPU_NUM + NPU_NUM * j] = convd_kernel_rg8[i * 9 + j];
            }
        }
    }

    readBin(src_name, input, height * width);
    bm_image input_img;
    bm_image output_img;

    ret = bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    bm_image_create(handle, height, width, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input_img, BMCV_HEAP1_ID);
    bm_image_alloc_dev_mem(output_img, BMCV_HEAP1_ID);
    bm_image_copy_host_to_device(input_img, (void **)(&input));
    bmcv_image_bayer2rgb(handle, kernel_data, input_img, output_img);

    void* out_ptr[3] = {(void*)output,
                        (void*)((unsigned char*)output + width * height),
                        (void*)((unsigned char*)output + 2 * width * height)};
    bm_image_copy_device_to_host(output_img, (void **)(out_ptr));
    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);


    FILE *fp_dst = fopen(output_path, "wb");
    fwrite((void *)output, 1, height * width * 3, fp_dst);
    free(output);
    free(input);
    bm_dev_free(handle);
    return ret;
}
