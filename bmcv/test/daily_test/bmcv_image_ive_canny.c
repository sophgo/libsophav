#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main(){
    int dev_id = 0;
    int thrSize = 0; //0 -> 3x3; 1 -> 5x5
    int height = 1080, width = 1920;
    bm_image_format_ext fmt = FORMAT_GRAY;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *dstEdge_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_canny.bin";
    bm_handle_t handle = NULL;

    signed char arr5by5[25] = { -1, -2, 0, 2, 1, -4, -8, 0, 8, 4, -6, -12, 0,
                    12, 6, -4, -8, 0, 8, 4, -1, -2, 0, 2, 1 };
    signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                    2, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0 };
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    bm_image src;
    bm_device_mem_t canny_stmem;
    int stride[4];

    int stmem_len = width * height * 4 * (sizeof(unsigned short) + sizeof(unsigned char));

    unsigned char *edge_res = malloc(width * height * sizeof(unsigned char));
    memset(edge_res, 0, width * height * sizeof(unsigned char));

    bmcv_ive_canny_hys_edge_ctrl cannyHysEdgeAttr;
    memset(&cannyHysEdgeAttr, 0, sizeof(bmcv_ive_canny_hys_edge_ctrl));
    cannyHysEdgeAttr.u16_low_thr = (thrSize == 0) ? 42 : 108;
    cannyHysEdgeAttr.u16_high_thr = 3 * cannyHysEdgeAttr.u16_low_thr;
    (thrSize == 0) ? memcpy(cannyHysEdgeAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char)) :
                    memcpy(cannyHysEdgeAttr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char));

    // calc ive image stride && create bm image struct
    int data_size = 1;
    stride[0] = align_up(width, 16) * data_size;
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_malloc_device_byte(handle, &canny_stmem, stmem_len);

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
    cannyHysEdgeAttr.st_mem = canny_stmem;

    ret = bmcv_ive_canny(handle, src, bm_mem_from_system((void *)edge_res), cannyHysEdgeAttr);

    FILE *edge_fp = fopen(dstEdge_name, "wb");
    fwrite((void *)edge_res, 1, width * height, edge_fp);
    fclose(edge_fp);

    free(input_data);
    free(edge_res);
    bm_image_destroy(&src);
    bm_free_device(handle, cannyHysEdgeAttr.st_mem);

    bm_dev_free(handle);
    return 0;
}
