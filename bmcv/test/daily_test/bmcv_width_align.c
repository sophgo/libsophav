#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "bmcv_internal.h"

static void readBin(const char* path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

static void write_bin(const char *output_path, unsigned char *output_data, int size)
{
    FILE *fp_dst = fopen(output_path, "wb");

    if (fp_dst == NULL) {
        printf("unable to open output file %s\n", output_path);
        return;
    }

    if(fwrite(output_data, sizeof(unsigned char), size, fp_dst) != 0) {
        printf("write image success\n");
    }
    fclose(fp_dst);
}
static int bmcv_width_align_cmp(unsigned char *p_exp, unsigned char *p_got, int count) {
    int ret = 0;
    for (int j = 0; j < count; j++) {
        if (p_exp[j] != p_got[j]) {
            printf("error: when idx=%d,  exp=%d but got=%d\n", j, (int)p_exp[j], (int)p_got[j]);
            return -1;
        }
    }
    return ret;
}

int main() {
    int image_w = 1920;
    int image_h = 1080;

    int                      default_stride[3] = {0};
    int                      src_stride[3]     = {0};
    int                      dst_stride[3]     = {0};
    bm_image_format_ext      image_format      = FORMAT_GRAY;
    bm_image_data_format_ext data_type         = DATA_TYPE_EXT_1N_BYTE;
    int                      raw_size          = 0;

    default_stride[0] = image_w;
    src_stride[0]     = image_w + rand() % 16;
    dst_stride[0]     = image_w + rand() % 16;

    raw_size = image_h * image_w;
    unsigned char* raw_image = (unsigned char*)malloc(image_w * image_h * sizeof(unsigned char));
    unsigned char* dst_data = (unsigned char*)malloc(image_w * image_h * sizeof(unsigned char));
    unsigned char* src_image = (unsigned char*)malloc(src_stride[0] * image_h * sizeof(unsigned char));
    unsigned char* dst_image = (unsigned char*)malloc(dst_stride[0] * image_h * sizeof(unsigned char));

    const char* input_path = "/home/linaro/gray.bin";
    const char* output_path = "/home/linaro/gray_out.bin";
    readBin(input_path, raw_image, raw_size);

    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    bm_image src_img, dst_img;

    // calculate use reference for compare.
    unsigned char *src_s_offset;
    unsigned char *src_d_offset;
    for (int i = 0; i < image_h; i++) {
        src_s_offset = raw_image + i * default_stride[0];
        src_d_offset = src_image + i * src_stride[0];
        memcpy(src_d_offset, src_s_offset, image_w);
    }

    // create source image.
    bm_image_create(handle, image_h, image_w,image_format, data_type, &src_img, src_stride);
    bm_image_create(handle, image_h, image_w, image_format, data_type, &dst_img, dst_stride);

    int size[3] = {0};
    bm_image_get_byte_size(src_img, size);
    u8 *host_ptr_src[] = {src_image,
                            src_image + size[0],
                            src_image + size[0] + size[1]};
    bm_image_get_byte_size(dst_img, size);
    u8 *host_ptr_dst[] = {dst_image,
                            dst_image + size[0],
                            dst_image + size[0] + size[1]};

    ret = bm_image_copy_host_to_device(src_img, (void **)(host_ptr_src));
    if (ret != BM_SUCCESS) {
        printf("test data prepare failed");
        return ret;
    }

    ret = bmcv_width_align(handle, src_img, dst_img);
    if (ret != BM_SUCCESS) {
        printf("bmcv width align failed");
        return ret;
    }
    ret = bm_image_copy_device_to_host(dst_img, (void **)(host_ptr_dst));
    if (ret != BM_SUCCESS) {
        printf("test data copy_back failed");
        return ret;
    }
    bm_image_destroy(&src_img);
    bm_image_destroy(&dst_img);

    unsigned char *dst_s_offset;
    unsigned char *dst_d_offset;
    for (int i = 0; i < image_h; i++) {
        dst_s_offset = dst_image + i * dst_stride[0];
        dst_d_offset = dst_data + i * default_stride[0];
        memcpy(dst_d_offset, dst_s_offset, image_w);
    }
    write_bin(output_path, dst_data, raw_size);

    // compare.
    int cmp_res = bmcv_width_align_cmp(raw_image, dst_data, raw_size);
    if (cmp_res != 0) {
        printf("cv_width_align comparing failed\n");
        ret = BM_ERR_FAILURE;
        return ret;
    }
    printf("------[TEST WIDTH ALIGN] ALL TEST PASSED!\n");
    free(raw_image);
    free(dst_data);
    free(src_image);
    free(dst_image);
    return 0;
}