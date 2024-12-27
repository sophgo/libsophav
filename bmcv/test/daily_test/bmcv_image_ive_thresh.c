#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

int main() {
    int dev_id = 0;
    int height = 1080, width = 1920;
    bmcv_ive_thresh_mode thresh_mode = IVE_THRESH_MIN_MID_MAX; /* IVE_THRESH_MODE_MIN_MID_MAX */
    int low_thr = 236, high_thr = 249, min_val = 166, mid_val = 219, max_val = 60;
    bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_rgb.bin", *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_thresh.bin";

    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bmcv_ive_thresh_attr thresh_attr;
    bm_image src, dst;
    int src_stride[4];
    int dst_stride[4];

    // calc ive image stride
    int data_size = 1;
    src_stride[0] = align_up(width, 16) * data_size;
    dst_stride[0] = align_up(width, 16) * data_size;
    // create bm image struct
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

    // alloc bm image memory
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    // read image data from input files
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


    memset(&thresh_attr, 0, sizeof(bmcv_ive_thresh_attr));
    thresh_attr.low_thr = low_thr;
    thresh_attr.high_thr = high_thr;
    thresh_attr.min_val = min_val;
    thresh_attr.mid_val = mid_val;
    thresh_attr.max_val = max_val;

    ret = bmcv_ive_thresh(handle, src, dst, thresh_mode, thresh_attr);

    unsigned char *ive_res = (unsigned char*) malloc(width * height * sizeof(unsigned char));
    memset(ive_res, 0, width * height * sizeof(unsigned char));

    ret = bm_image_copy_device_to_host(dst, (void **)&ive_res);

    FILE *fp = fopen(dst_name, "wb");
    fwrite((void *)ive_res, 1, width * height * sizeof(unsigned char), fp);
    fclose(fp);

    free(input_data);
    free(ive_res);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    bm_dev_free(handle);

    return ret;
}