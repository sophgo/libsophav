#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))


int main(){
    int dev_id = 0;
    int height = 1080, width = 1920;
    bm_image_format_ext fmt = FORMAT_GRAY;
    bmcv_ive_ccl_mode enMode = BM_IVE_CCL_MODE_8C;
    unsigned short u16InitAreaThr = 4;
    unsigned short u16Step = 2;

    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *ive_res_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_ccl.bin";
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    bm_image src_dst_img;
    bm_device_mem_t pst_blob;
    bmcv_ive_ccl_attr ccl_attr;
    bmcv_ive_ccblob *ccblob = NULL;
    int stride[4];

    ccl_attr.en_mode = enMode;
    ccl_attr.u16_init_area_thr = u16InitAreaThr;
    ccl_attr.u16_step = u16Step;

    ccblob = (bmcv_ive_ccblob *)malloc(sizeof(bmcv_ive_ccblob));
    memset(ccblob, 0, sizeof(bmcv_ive_ccblob));

    // calc ive image stride && create bm image struct
    int data_size = 1;
    stride[0] = align_up(width, 16) * data_size;
    bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src_dst_img, stride);

    ret = bm_image_alloc_dev_mem(src_dst_img, BMCV_HEAP1_ID);
    ret = bm_malloc_device_byte(handle, &pst_blob, sizeof(bmcv_ive_ccblob));

    int image_byte_size[4] = {0};
    bm_image_get_byte_size(src_dst_img, image_byte_size);
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
    bm_image_copy_host_to_device(src_dst_img, in_ptr);

    ret = bmcv_ive_ccl(handle, src_dst_img, pst_blob, ccl_attr);

    ret = bm_memcpy_d2s(handle, (void*)ccblob, pst_blob);
    FILE *fp = fopen(ive_res_name, "wb");
    fwrite((void *)ccblob, 1, sizeof(bmcv_ive_ccblob), fp);
    fclose(fp);

    bm_dev_free(handle);
    return ret;
}