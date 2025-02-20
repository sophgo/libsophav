#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

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
    int dev_id = 0;
    bmcv_ive_16bit_to_8bit_mode mode = BM_IVE_S16_TO_S8;
    unsigned char u8Numerator = 41;
    unsigned short u16Denominator = 18508;
    signed char s8Bias = 0;
    int height = 1080, width = 1920;
    bm_image_data_format_ext srcDtype = DATA_TYPE_EXT_S16;
    bm_image_data_format_ext dstDtype = DATA_TYPE_EXT_1N_BYTE_SIGNED;
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    char *ref_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_ive_16bitto8bit.bin";
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image src, dst;
    int srcStride[4], dstStride[4];

    // config setting
    bmcv_ive_16bit_to_8bit_attr attr;
    attr.mode = mode;
    attr.u16_denominator = u16Denominator;
    attr.u8_numerator = u8Numerator;
    attr.s8_bias = s8Bias;

    int data_size = 1;
    srcStride[0] = align_up(width, 16) * data_size;
    dstStride[0] = align_up(width, 16) * data_size;

    bm_image_create(handle, height, width, FORMAT_GRAY, srcDtype, &src, srcStride);
    bm_image_create(handle, height, width, FORMAT_GRAY, dstDtype, &dst, dstStride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    unsigned char* input_data = (unsigned char*)malloc(height * width * sizeof(unsigned char));
    readBin(src_name, input_data);
    bm_image_copy_host_to_device(src, (void **)&input_data);

    ret = bmcv_ive_16bit_to_8bit(handle, src, dst, attr);

    int image_byte_size[4] = {0};
    bm_image_get_byte_size(dst, image_byte_size);
    int byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
    unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
    memset(output_ptr, 0, sizeof(byte_size));

    void* out_ptr[4] = {(void*)output_ptr,
                        (void*)((char*)output_ptr + image_byte_size[0]),
                        (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                        (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

    ret = bm_image_copy_device_to_host(dst, (void **)out_ptr);

    FILE *fp_dst = fopen(ref_name, "wb");
    if (fwrite((void *)output_ptr, 1, byte_size, fp_dst) < (unsigned int)byte_size){
        printf("file size is less than %d required bytes\n", byte_size);
    };
    fclose(fp_dst);

    free(input_data);
    free(output_ptr);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    return 0;
}
