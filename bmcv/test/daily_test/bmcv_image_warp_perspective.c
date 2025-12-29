#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"

#define BM_MIN(x, y) (((x)) < ((y)) ? (x) : (y))
#define BM_MAX(x, y) (((x)) > ((y)) ? (x) : (y))

#define NO_USE 0
#define MAX_INT (float)(pow(2, 31) - 2)
#define MIN_INT (float)(1 - pow(2, 31))
#define UNUSED(x) (void)(x)

static int image_sh = 500;
static int image_sw = 500;
static int image_dh = 200;
static int image_dw = 200;
static int use_bilinear = 0;
static bm_handle_t handle;

void inverse_matrix(int n, float arcs[3][3], float astar[3][3]);

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


static void my_get_perspective_transform(int* sx, int* sy, int dw, int dh, float* matrix) {
    int A = sx[3] + sx[0] - sx[1] - sx[2];
    int B = sy[3] + sy[0] - sy[1] - sy[2];
    int C = sx[2] - sx[3];
    int D = sy[2] - sy[3];
    int E = sx[1] - sx[3];
    int F = sy[1] - sy[3];
    matrix[8] = 1;
    matrix[7] = ((float)(A * F - B * E) / (float)dh) / (float)(C * F - D * E);
    matrix[6] = ((float)(A * D - B * C) / (float)dw) / (float)(D * E - C * F);
    matrix[0] = (matrix[6] * dw * sx[1] + sx[1] - sx[0]) / dw;
    matrix[1] = (matrix[7] * dh * sx[2] + sx[2] - sx[0]) / dh;
    matrix[2] = sx[0];
    matrix[3] = (matrix[6] * dw * sy[1] + sy[1] - sy[0]) / dw;
    matrix[4] = (matrix[7] * dh * sy[2] + sy[2] - sy[0]) / dh;
    matrix[5] = sy[0];
}

static unsigned char*  image_read(
                    int            image_n,
                    int            image_c,
                    int            image_h,
                    int            image_w) {
    printf("image_n = %d,  image_c = %d,  image_h = %d,  image_w = %d\n", image_n, image_c, image_h, image_w);
    unsigned char* res = (unsigned char*) malloc(image_n * image_c * image_h * image_w * sizeof(unsigned char));
    for (int i = 0; i < image_n * image_c * image_h * image_w; i++)
    {
        res[i] = i % 255;
    }
    return res;
}

static bm_status_t src_data_generation(int i, int* coordinate, float* trans_mat, float* tensor_S) {
    int   border_x1                          = rand() % image_sw;
    int   border_x2                          = rand() % image_sw;
    while (border_x1 == border_x2) border_x2 = rand() % image_sw;
    int   border_y1                          = rand() % image_sh;
    int   border_y2                          = rand() % image_sh;
    while (border_y1 == border_y2) border_y2 = rand() % image_sh;
    int   x_min                              = BM_MIN(border_x1, border_x2);
    int   x_max                              = BM_MAX(border_x1, border_x2);
    int   y_min                              = BM_MIN(border_y1, border_y2);
    int   y_max                              = BM_MAX(border_y1, border_y2);

    int x[4], y[4];
    int sx[4], sy[4];
    int idx = rand() % 4;
    x   [0] = x_min + rand() % (x_max - x_min);
    y   [0] = y_min;
    x   [1] = x_max;
    y   [1] = y_min + rand() % (y_max - y_min);
    x   [2] = x_max - rand() % (x_max - x_min);
    y   [2] = y_max;
    x   [3] = x_min;
    y   [3] = y_max - rand() % (y_max - y_min);
    sx  [0] = x[(0 + idx) % 4];
    sy  [0] = y[(0 + idx) % 4];
    sx  [1] = x[(1 + idx) % 4];
    sy  [1] = y[(1 + idx) % 4];
    sx  [2] = x[(3 + idx) % 4];
    sy  [2] = y[(3 + idx) % 4];
    sx  [3] = x[(2 + idx) % 4];
    sy  [3] = y[(2 + idx) % 4];
    printf("src coordinate: (%d %d) (%d %d) (%d %d) (%d %d)\n", sx[0], sy[0], sx[1], sy[1], sx[2], sy[2], sx[3], sy[3]);

    coordinate[0 + i * 8] = sx[0];
    coordinate[1 + i * 8] = sx[1];
    coordinate[2 + i * 8] = sx[2];
    coordinate[3 + i * 8] = sx[3];
    coordinate[4 + i * 8] = sy[0];
    coordinate[5 + i * 8] = sy[1];
    coordinate[6 + i * 8] = sy[2];
    coordinate[7 + i * 8] = sy[3];

    float matrix_cv[9];
    my_get_perspective_transform(sx, sy, image_dw-1, image_dh-1, matrix_cv);
    trans_mat[0 + i * 9] = matrix_cv[0];
    trans_mat[1 + i * 9] = matrix_cv[1];
    trans_mat[2 + i * 9] = matrix_cv[2];
    trans_mat[3 + i * 9] = matrix_cv[3];
    trans_mat[4 + i * 9] = matrix_cv[4];
    trans_mat[5 + i * 9] = matrix_cv[5];
    trans_mat[6 + i * 9] = matrix_cv[6];
    trans_mat[7 + i * 9] = matrix_cv[7];
    trans_mat[8 + i * 9] = matrix_cv[8];

    printf("trans_mat[0 + i * 9] = %f\n", trans_mat[0 + i * 9]);
    printf("trans_mat[1 + i * 9] = %f\n", trans_mat[1 + i * 9]);
    printf("trans_mat[2 + i * 9] = %f\n", trans_mat[2 + i * 9]);
    printf("trans_mat[3 + i * 9] = %f\n", trans_mat[3 + i * 9]);
    printf("trans_mat[4 + i * 9] = %f\n", trans_mat[4 + i * 9]);
    printf("trans_mat[5 + i * 9] = %f\n", trans_mat[5 + i * 9]);
    printf("trans_mat[6 + i * 9] = %f\n", trans_mat[6 + i * 9]);
    printf("trans_mat[7 + i * 9] = %f\n", trans_mat[7 + i * 9]);
    printf("trans_mat[8 + i * 9] = %f\n", trans_mat[8 + i * 9]);

    float*     tensor_SX  = tensor_S;
    float*     tensor_SY  = tensor_SX + image_dh * image_dw;
    for (int y = 0; y < image_dh; y++) {
        for (int x = 0; x < image_dw; x++) {
            float dx = tensor_SX[y * image_dw + x] * trans_mat[0 + i * 9] +
                tensor_SY[y * image_dw + x] * trans_mat[1 + i * 9] + trans_mat[2 + i * 9];
            float dy = tensor_SX[y * image_dw + x] * trans_mat[3 + i * 9] +
                    tensor_SY[y * image_dw + x] * trans_mat[4 + i * 9] + trans_mat[5 + i * 9];
            float dz = tensor_SX[y * image_dw + x] * trans_mat[6 + i * 9] +
                    tensor_SY[y * image_dw + x] * trans_mat[7 + i * 9] + trans_mat[8 + i * 9];

            dx = dx / dz;
            dy = dy / dz;

            if (dx < MIN_INT || dx > MAX_INT || dy < MIN_INT || dy > MAX_INT || fabs(dz) == 0) {
                printf("--------- the input data is not leagel! --------- \n");
                return BM_ERR_DATA;
            }
        }
    }
    return BM_SUCCESS;
}



int main() {
    int dev_id = 0;
    bm_status_t ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    int image_c = 3;
    int matrix_num[4] = {1, 1, 1, 1};
    int image_n = 1;

    int output_num = 0;
    for (int i = 0; i < image_n; i++) {
        output_num += matrix_num[i];
    }

    unsigned char*        src_data   = image_read(image_n, image_c, image_sh, image_sw);
    float*  trans_mat  = (float*) malloc(output_num * 9 * sizeof(float));
    int*    coordinate = (int*) malloc(output_num * 8 * sizeof(int));
    float*  tensor_S   = (float*) malloc(image_dh *image_dw * 2 * sizeof(float));
    float*      tensor_SX  = tensor_S;
    float*      tensor_SY  = tensor_SX + image_dh * image_dw;
    for (int y = 0; y < image_dh; y++) {
        for (int x = 0; x < image_dw; x++) {
            tensor_SX[y * image_dw + x] = (float)x;
            tensor_SY[y * image_dw + x] = (float)y;
        }
    }

    for (int i = 0; i < output_num; i++) {
        ret = src_data_generation(i, coordinate, trans_mat, tensor_S);
        while (BM_ERR_DATA == ret)
            ret = src_data_generation(i, coordinate, trans_mat, tensor_S);
    }

    bmcv_perspective_image_matrix matrix_image[4];
    bmcv_perspective_matrix* matrix = (bmcv_perspective_matrix *)(trans_mat);
    for (int i = 0; i < image_n; i++) {
        matrix_image[i].matrix_num = matrix_num[i];
        matrix_image[i].matrix = matrix;
        matrix += matrix_num[i];
    }

    bm_image src_img[4];
    bm_image_format_ext image_format = FORMAT_BGR_PLANAR;
    bm_image_data_format_ext data_type = DATA_TYPE_EXT_1N_BYTE;
    int in_image_num = image_n;
    int out_image_num = output_num;

    for (int i = 0; i < in_image_num; i++) {
        bm_image_create(
            handle, image_sh, image_sw, image_format, data_type, src_img + i, NULL);
        int stride = 0;
        // debug
        bm_image_get_stride(src_img[i], &stride);
        void *ptr = (void *)(src_data + 3 * stride * image_sh * i);
        bm_image_copy_host_to_device(src_img[i], (void **)(&ptr));
    }

    // create dst image.
    bm_image* dst_img = (bm_image*)malloc(out_image_num * sizeof(bm_image));

    for (int i = 0; i < out_image_num; i++) {
        bm_image_create(handle, image_dh, image_dw, image_format, data_type, dst_img + i, NULL);
    }
    printf("No coordinate\n");
    ret = bmcv_image_warp_perspective(handle, image_n, matrix_image, src_img, dst_img, use_bilinear);
    int size = 0;
    bm_image_get_byte_size(dst_img[0], &size);
    unsigned char* temp_out = (unsigned char*)malloc(out_image_num * size * sizeof(unsigned char));

    for (int i = 0; i < out_image_num; i++) {
        void *ptr = (void *)(temp_out + size * i);
        bm_image_copy_device_to_host(dst_img[i], (void **)&ptr);
    }

    char *dst_name = "/home/linaro/A2test/bmcv/test/res/out/daily_test_image_warp_perspective_dst.bin";
    writeBin(dst_name, temp_out, size);
    writeBin("/home/linaro/A2test/bmcv/test/res/out/daily_test_image_warp_perspective_src.bin", src_data, image_sh * image_sw * 3);

    free(temp_out);
    free(dst_img);

    free(trans_mat);
    free(coordinate);
    free(tensor_S);

    bm_dev_free(handle);

    return ret;
}
