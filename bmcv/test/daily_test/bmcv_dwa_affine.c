#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

int main() {
    int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080, dev_id = 0;
    bm_image_format_ext fmt = FORMAT_YUV420P;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_yuv420.bin", *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_dwa_affine.bin";
    bm_handle_t handle = NULL;
    bmcv_affine_attr_s affine_attr = {0};
    affine_attr.u32RegionNum = 4;
    affine_attr.stDestSize.u32Width = 400;
    affine_attr.stDestSize.u32Height = 400;
    // bmcv_point2f_s faces[9][4] = {0};
    bmcv_point2f_s faces[9][4] = {
        { {.x = 722.755, .y = 65.7575}, {.x = 828.402, .y = 80.6858}, {.x = 707.827, .y = 171.405}, {.x = 813.474, .y = 186.333} },
        { {.x = 494.919, .y = 117.918}, {.x = 605.38,  .y = 109.453}, {.x = 503.384, .y = 228.378}, {.x = 613.845, .y = 219.913} },
        { {.x = 1509.06, .y = 147.139}, {.x = 1592.4,  .y = 193.044}, {.x = 1463.15, .y = 230.48 }, {.x = 1546.5,  .y = 276.383} },
        { {.x = 1580.21, .y = 66.7939}, {.x = 1694.1,  .y = 70.356 }, {.x = 1576.65, .y = 180.682}, {.x = 1690.54, .y = 184.243} },
        { {.x = 178.76,  .y = 90.4814}, {.x = 286.234, .y = 80.799 }, {.x = 188.442, .y = 197.955}, {.x = 295.916, .y = 188.273} },
        { {.x = 1195.57, .y = 139.226}, {.x = 1292.69, .y = 104.122}, {.x = 1230.68, .y = 236.34}, {.x = 1327.79, .y = 201.236}, },
        { {.x = 398.669, .y = 109.872}, {.x = 501.93, .y = 133.357}, {.x = 375.184, .y = 213.133}, {.x = 478.445, .y = 236.618}, },
        { {.x = 845.989, .y = 94.591}, {.x = 949.411, .y = 63.6143}, {.x = 876.966, .y = 198.013}, {.x = 980.388, .y = 167.036}, },
        { {.x = 1060.19, .y = 58.7882}, {.x = 1170.61, .y = 61.9105}, {.x = 1057.07, .y = 169.203}, {.x = 1167.48, .y = 172.325}, },
    };
    memcpy(affine_attr.astRegionAttr, faces, sizeof(faces));

    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image src, dst;

    bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    int image_byte_size[4] = {0};
    bm_image_get_byte_size(src, image_byte_size);
    int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    unsigned char *input_data = (unsigned char*)malloc(byte_size);
    FILE *fp_src = fopen(src_name, "rb");
    if (fread((void*)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
        printf("file size is less than required bytes%d\n", byte_size);
    }
    fclose(fp_src);
    void* in_ptr[4] = {(void *)input_data,
                        (void *)((unsigned char*)input_data + image_byte_size[0]),
                        (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1]),
                        (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
    bm_image_copy_host_to_device(src, in_ptr);

    bmcv_dwa_affine(handle, src, dst, affine_attr);

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

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    free(input_data);
    free(output_ptr);

    bm_dev_free(handle);

    return 0;
}

