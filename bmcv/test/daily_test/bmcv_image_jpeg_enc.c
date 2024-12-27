#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include "bmcv_api_ext_c.h"
#include <assert.h>
#include <math.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main() {
    int dev_id = -1;
    int dev_cnt;
    int ret = 0;
    bm_dev_getcount(&dev_cnt);
    if (dev_id >= dev_cnt) {
        printf("[TEST JPEG] dev_id should less than device count, only detect %d devices\n", dev_cnt);
        exit(-1);
    }
    printf("device count = %d\n", dev_cnt);
    bm_handle_t handle[dev_cnt];

    for (int i = 0; i < dev_cnt; i++) {
        int id;
        if (dev_id != -1) {
            dev_cnt = 1;
            id = dev_id;
        } else {
            id = i;
        }
        bm_status_t req = bm_dev_request(handle + i, id);
        if (req != BM_SUCCESS) {
            printf("create bm handle for dev%d failed!\n", id);
            exit(-1);
        }
    }
    for (int j = 0; j < dev_cnt; j++) {
        char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_yuv420.bin";
        int format = FORMAT_YUV420P;
        int image_n = 1;
        int image_h = 1080;
        int image_w = 1920;
        int* stride = (int*)malloc(3 * sizeof(int));
        stride[0] = align_up(image_w, 16);
        stride[1] = align_up(image_w / 2, 16);
        stride[2] = align_up(image_w / 2, 16);


        bm_image src[4];
        for (int i = 0; i < image_n; i++) {
            bm_image_create(handle[j], image_h, image_w, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, src + i, stride);
            bm_image_alloc_dev_mem(src[i], BMCV_HEAP1_ID);
        }
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(src[0], image_byte_size);
        for (int i = 0; i < 4; i++) {
            printf("src_%d_byte_size: %d\n", i, image_byte_size[i]);
        }
        int byte_size = image_w * image_h * 3 / 2;
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
        bm_image_copy_host_to_device(src[0], in_ptr);
        void* jpeg_data[4] = {NULL, NULL, NULL, NULL};
        size_t* size = (size_t*)malloc(image_n * sizeof(size_t));
        ret = bmcv_image_jpeg_enc(handle[j], image_n, src, jpeg_data, size, 95);
        if (ret != BM_SUCCESS) exit(-1);

        for (int i = 0; i < image_n; i++) {
            free(jpeg_data[i]);
            bm_image_destroy(&src[i]);
        }
        free(input_data);
        free(size);
        free(stride);
    }
    for (int i = 0; i < dev_cnt; i++) {
        bm_dev_free(handle[i]);
    }
    return ret;
}
