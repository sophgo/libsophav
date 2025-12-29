#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"

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

static unsigned char* image_read_2(
                    int            image_n,
                    int            image_c,
                    int            image_sh,
                    int            image_sw,
                    int            image_dh,
                    int            image_dw) {

    unsigned char* res          = (unsigned char*) malloc(image_n * image_c * image_sh * image_sw * sizeof(unsigned char));
    unsigned char* res_temp     = (unsigned char*) malloc(image_n * image_c * image_dh * image_dw * sizeof(unsigned char));
    unsigned char* res_temp_bak = (unsigned char*) malloc(image_n * image_c * image_dh * image_dw * sizeof(unsigned char));

    for (int i = 0; i < image_n * image_c * image_sh * image_sw; i++)
    {
        res[i] = i % 255;
    }

    if (image_dh <= image_sh && image_dw <= image_sw)
        return res;

    if (image_dh > image_sh){
        int pad_h_value = (image_dh - image_sh) / 2;
        for (int i = 0;i < pad_h_value * image_sw;i++)
            res_temp[i] = 0;

        for (int i = pad_h_value * image_sw, j = 0; i < pad_h_value * image_sw + image_n * image_c * image_sh * image_sw;i++,j++)
            res_temp[i] = res[j];

        for (int i = pad_h_value * image_sw + image_n * image_c * image_sh * image_sw;i <  pad_h_value * image_sw + image_n * image_c * image_sh * image_sw + pad_h_value * image_sw;i++)
            res_temp[i] = 0;
    }

    if (image_dw > image_sw){
        int pad_w_value = (image_dw - image_sw) / 2;
        int j = 0;
        for (int i = 0;i < image_dh;i++){
            for (;j < pad_w_value + i * image_dw;j++)
                res_temp_bak[j] = 0;
            for (;j < pad_w_value + image_sw + i * image_dw;j++)
                res_temp_bak[j] = res_temp[j - pad_w_value - i * image_dw + i * image_sw];
            for (;j < pad_w_value + pad_w_value + image_sw + i * image_dw;j++)
                res_temp_bak[j] = 0;
        }
    }

    free(res);
    free(res_temp);
    return res_temp_bak;
}

int main() {
    int image_sh = 1080;
    int image_sw = 1920;
    int image_dh = 112;
    int image_dw = 112;
    int is_bilinear = 0;
    bm_handle_t handle;
    int dev_id = 0;
    bm_status_t ret = bm_dev_request(&handle, dev_id);

    int image_c = 3;
    int image_n = 1;
    int output_num = 1;

    unsigned char* src_data = image_read_2(image_n, image_c, image_sh, image_sw, image_dh, image_dw);
    float* trans_mat = (float*)malloc(output_num * 6 * sizeof(float));

    for (int i = 0; i < output_num; i++){
        trans_mat[0 + i * 6] = 3.84843f;
        trans_mat[1 + i * 6] = -0.0248411f;
        trans_mat[2 + i * 6] = 916.203f;
        trans_mat[3 + i * 6] = 0.0248411;
        trans_mat[4 + i * 6] = 3.84843f;
        trans_mat[5 + i * 6] = 55.9748f;
    }

    unsigned char* dst_image_tpu = (unsigned char*)malloc(output_num * image_c * image_dh * image_dw * sizeof(unsigned char));

    bmcv_affine_image_matrix matrix_image[4];
    bmcv_affine_matrix *     matrix = (bmcv_affine_matrix *)(trans_mat);
    for (int i = 0; i < image_n; i++) {
        matrix_image[i].matrix_num = 1;
        matrix_image[i].matrix     = matrix;
        matrix += 1;
    }

    bm_image                 src_img[4];
    bm_image_format_ext      image_format = FORMAT_BGR_PLANAR;
    bm_image_data_format_ext data_type    = DATA_TYPE_EXT_1N_BYTE;

    for (int i = 0; i < image_n; i++){
        bm_image_create(handle, image_sh, image_sw, image_format, data_type, src_img + i, NULL);
        int stride = 0;
        bm_image_get_stride(src_img[i], &stride);
        void *ptr = (void *)(src_data + 3 * stride * image_sh * i);
        bm_image_copy_host_to_device(src_img[i], (void **)(&ptr));
    }

    // create dst image.
    bm_image* dst_img = (bm_image*)malloc(image_n * sizeof(bm_image));

    for (int i = 0; i < image_n; i++) {
        bm_image_create(handle, image_dh, image_dw, image_format, data_type, dst_img + i, NULL);
    }
    ret = bmcv_image_warp_affine(handle, image_n, matrix_image, src_img, dst_img, is_bilinear);

    int size = 0;
    bm_image_get_byte_size(dst_img[0], &size);
    unsigned char* temp_out = (unsigned char*)malloc(output_num * size * sizeof(unsigned char));
    for (int i = 0; i < image_n; i++) {
        void *ptr = (void *)(temp_out + size * i);
        bm_image_copy_device_to_host(dst_img[i], (void **)&ptr);
    }
    memcpy(dst_image_tpu, temp_out, image_n * image_c * image_dh * image_dw);

    for (int i = 0; i < image_n; i++){
        bm_image_destroy(&src_img[i]);
    }
    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_warp_affine_dst.bin";
    writeBin(dst_name, temp_out,  size);
    writeBin("/home/linaro/A2test/bmcv/test/res/out/daily_test_image_warp_affine_src.bin", src_data, image_sh * image_sw * 3);
    free(src_data);
    free(dst_img);
    free(temp_out);

    free(dst_image_tpu);


    return ret;
}
