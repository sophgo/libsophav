#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"

#define NO_USE 0
#define MAX_INT (float)(pow(2, 31) - 2)
#define MIN_INT (float)(1 - pow(2, 31))
#define UNUSED(x) (void)(x)
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

static int image_sh = 500;
static int image_sw = 500;
static int image_dh = 200;
static int image_dw = 200;
static int flag = 0;
static int use_bilinear = 0;
static bm_handle_t handle;

void inverse_matrix(int n, float arcs[3][3], float astar[3][3]);

static int get_source_idx(int idx, int *matrix, int image_n) {
    for (int i = 0; i < image_n; i++) {
        if (idx < matrix[i]) {
            return i;
        }
    }
    printf("Error:get source idx error\n");
    exit(-1);
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

// static void write_file(char *filename, void* data, size_t size)
// {
//     FILE *fp = fopen(filename, "wb+");
//     if (fp == NULL) {
//         printf("filename is wrong !\r\n");
//         exit(-1);
//     }
//     fwrite(data, size, 1, fp);
//     printf("save to %s %ld bytes\n", filename, size);
//     fclose(fp);
// }

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

static unsigned char fetch_pixel_1(int x_idx, int y_idx, int c_idx, int width, int height, int w_stride, unsigned char* image)
{
    if(x_idx < 0 || x_idx >= width || y_idx < 0 || y_idx >= height)
        return 0;
    return image[width * height * w_stride * c_idx + y_idx * width * w_stride + x_idx * w_stride];
}

unsigned char fetch_pixel_2(float x_idx, float y_idx, int c_idx, int width, int height, unsigned char* image)
{
    x_idx = BM_MIN(width - 1, x_idx);
    x_idx = BM_MAX(0, x_idx);

    y_idx = BM_MIN(height - 1, y_idx);
    y_idx = BM_MAX(0, y_idx);

    int ceil_x  = ceil(x_idx);
    int ceil_y  = ceil(y_idx);
    int floor_x = floor(x_idx);
    int floor_y = floor(y_idx);

    unsigned char pixel_lu = image[width * height * c_idx + floor_y * width + floor_x];
    unsigned char pixel_ld = image[width * height * c_idx + ceil_y * width + floor_x];
    unsigned char pixel_ru = image[width * height * c_idx + floor_y * width + ceil_x];
    unsigned char pixel_rd = image[width * height * c_idx + ceil_y * width + ceil_x];

    float coef_x_ceil = x_idx - floor_x;
    float coef_y_ceil = y_idx - floor_y;

    float coef_x_floor = 1.0 - coef_x_ceil;
    float coef_y_floor = 1.0 - coef_y_ceil;

    unsigned char mix = (unsigned char)(coef_x_floor * coef_y_floor * pixel_lu +
                        coef_x_floor * coef_y_ceil * pixel_ld +
                        coef_x_ceil * coef_y_floor * pixel_ru +
                        coef_x_ceil * coef_y_ceil * pixel_rd + 0.5f);
    return mix;
};

static int bmcv_perspective_nearest_cmp_1n(
                            unsigned char * p_exp,
                            unsigned char * p_got,
                            unsigned char * src,
                            int*            map,
                            int             image_c,
                            int             image_sh,
                            int             image_sw,
                            int             image_dh,
                            int             image_dw) {
    // generate map.
    int* map_x    = map;
    int* map_y    = map + image_dh * image_dw;
    int w_stride = 1;
    int image_dw_stride = image_dw * w_stride;

    for (int c = 0; c < image_c; c++) {
        for (int y = 0; y < image_dh; y++) {
            for (int x = 0; x < image_dw; x++) {
                unsigned char got = p_got[c * image_dw_stride * image_dh + y * image_dw_stride + x * w_stride];
                unsigned char exp = p_exp[c * image_dw_stride * image_dh + y * image_dw_stride + x * w_stride];

                if (got != exp) {
                    // loopup up/down left/right by 1 pixel in source image.
                    int sx = map_x[y * image_dw + x];
                    int sy = map_y[y * image_dw + x];
                    //if(sx >= image_sw || sx < 0 || sy >= image_sh || sy < 0)
                    //    continue;
                    unsigned char left = fetch_pixel_1(sx + 1, sy, c, image_sw, image_sh, w_stride, src);
                    unsigned char right = fetch_pixel_1(sx - 1, sy, c, image_sw, image_sh, w_stride, src);
                    unsigned char up = fetch_pixel_1(sx, sy - 1, c, image_sw, image_sh, w_stride, src);
                    unsigned char down = fetch_pixel_1(sx, sy + 1, c, image_sw, image_sh, w_stride, src);
                    unsigned char left_up = fetch_pixel_1(sx - 1, sy - 1, c, image_sw, image_sh, w_stride, src);
                    unsigned char right_up = fetch_pixel_1(sx + 1, sy - 1, c, image_sw, image_sh, w_stride, src);
                    unsigned char left_down = fetch_pixel_1(sx - 1, sy + 1, c, image_sw, image_sh, w_stride, src);
                    unsigned char right_down = fetch_pixel_1(sx + 1, sy + 1, c, image_sw, image_sh, w_stride, src);
                    if (got == left || got == right || got == up ||
                        got == down || got == left_up || got == left_down ||
                        got == right_up || got == right_down) {
                        // find the neighbour pixel, continue.
                        // printf("c:%d h %d w %d, got:%d exp %d\n", c, y, x,
                        // got, exp); printf("got 0x%x left 0x%x right 0x%x up
                        // 0x%x down 0x%x left_up 0x%x left_down 0x%x right_up
                        // 0x%x right_down 0x%x\n",
                        //        got, left, right, up, down, left_up,
                        //        left_down, right_up, right_down);
                    } else {
                        printf("c:%d h %d w %d, got:%d exp %d\n",
                            c,
                            y,
                            x,
                            got,
                            exp);
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

static int bmcv_perspective_bilinear_cmp_1n(
                            unsigned char * p_exp,
                            unsigned char * p_got,
                            unsigned char * src,
                            float*          map,
                            int             image_c,
                            int             image_sh,
                            int             image_sw,
                            int             image_dh,
                            int             image_dw) {

    // generate map.
    float* map_x    = map;
    float* map_y    = map + image_dh * image_dw;
    int w_stride = 1;
    int image_dw_stride = image_dw * w_stride;

    for (int c = 0; c < image_c; c++) {
        for (int y = 0; y < image_dh; y++) {
            for (int x = 0; x < image_dw; x++) {
                unsigned char got = p_got[c * image_dw_stride * image_dh +
                                          y * image_dw_stride + x * w_stride];
                unsigned char exp = p_exp[c * image_dw_stride * image_dh +
                                          y * image_dw_stride + x * w_stride];
                if (got != exp) {
                    // loopup up/down left/right by 1 pixel in source image.
                    float sx = map_x[y * image_dw + x];
                    float sy = map_y[y * image_dw + x];

                    u8 mix_pixel = fetch_pixel_2(sx, sy, c, image_sw, image_sh, src);

                    if (abs(mix_pixel - got) <= 2) {
                        // find the neighbour pixel, continue.
                        // printf("c:%d h %d w %d, got:%d exp %d\n", c, y, x,
                        // got, exp); printf("got 0x%x left 0x%x right 0x%x up
                        // 0x%x down 0x%x left_up 0x%x left_down 0x%x right_up
                        // 0x%x right_down 0x%x\n",
                        //        got, left, right, up, down, left_up,
                        //        left_down, right_up, right_down);
                    } else {
                        printf("c:%d h %d w %d, got:%d exp %d\n",
                               c,
                               y,
                               x,
                               got,
                               exp);
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

static int bmcv_warp_perspective_cmp(
                         unsigned char* p_exp,
                         unsigned char* p_got,
                         unsigned char* src,
                         int* map,
                         int matrix_num[4],
                         int use_bilinear,
                         int image_n,
                         int image_c,
                         int image_sh,
                         int image_sw,
                         int image_dh,
                         int image_dw) {
    int ret             = 0;
    int output_num      = 0;
    int matrix_sigma[4] = {0};
    for (int i = 0; i < image_n; i++) {
        output_num += matrix_num[i];
        matrix_sigma[i] = output_num;
    }

    for (int i = 0; i < output_num; i++) {
        int s_idx = get_source_idx(i, matrix_sigma, image_n);
        int src_offset = s_idx * image_c * image_sh * image_sw;
        int dst_offset = i * image_c * image_dh * image_dw;
        if (use_bilinear)
            ret = bmcv_perspective_bilinear_cmp_1n(
                                   p_exp + dst_offset,
                                   p_got + dst_offset,
                                   src + src_offset,
                                   (float*)map + i * image_dh * image_dw * 2,
                                   image_c,
                                   image_sh,
                                   image_sw,
                                   image_dh,
                                   image_dw);
        else
            ret = bmcv_perspective_nearest_cmp_1n(
                                   p_exp + dst_offset,
                                   p_got + dst_offset,
                                   src + src_offset,
                                   map + i * image_dh * image_dw * 2,
                                   image_c,
                                   image_sh,
                                   image_sw,
                                   image_dh,
                                   image_dw);
        if (ret != 0) {
            return ret;
        }
    }
    return ret;
}

static int bmcv_perspective_bilinear_1n_ref(
                                    // input
                                    unsigned char *src_image,
                                    float *        trans_mat,
                                    int            image_c,
                                    int            image_sh,
                                    int            image_sw,
                                    int            image_dh,
                                    int            image_dw,
                                    // output
                                    float          *map,
                                    unsigned char  *dst_image,
                                    float*     tensor_S) {
    UNUSED(image_c);
    float* tensor_SX = tensor_S;
    float* tensor_SY = tensor_SX + image_dh * image_dw;
    float *tensor_DX = map;
    float *tensor_DY = tensor_DX + image_dh * image_dw;
    int dst_w = image_dw;
    int dst_h = image_dh;
    int src_w = image_sw;
    int src_h = image_sh;
    float m0 = trans_mat[0];
    float m1 = trans_mat[1];
    float m2 = trans_mat[2];
    float m3 = trans_mat[3];
    float m4 = trans_mat[4];
    float m5 = trans_mat[5];
    float m6 = trans_mat[6];
    float m7 = trans_mat[7];
    float m8 = trans_mat[8];

    // generate the input for calculate coordinate map.
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            tensor_SX[y * dst_w + x] = (float)x;
            tensor_SY[y * dst_w + x] = (float)y;
        }
    }
    // calculate coordinate map.
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float dx = tensor_SX[y * dst_w + x] * m0 +
                       tensor_SY[y * dst_w + x] * m1 + m2;
            float dy = tensor_SX[y * dst_w + x] * m3 +
                       tensor_SY[y * dst_w + x] * m4 + m5;
            float dz = tensor_SX[y * dst_w + x] * m6 +
                       tensor_SY[y * dst_w + x] * m7 + m8;
            dx = dx / dz;
            dy = dy / dz;

            tensor_DX[y * dst_w + x] = dx;
            tensor_DY[y * dst_w + x] = dy;
        }
    }
    int w_stride = 1;
    int src_w_stride = src_w * w_stride;
    int dst_w_stride = dst_w * w_stride;

    // warp in source image directly.
    unsigned char *sb = src_image;
    unsigned char *sg = sb + src_w_stride * src_h;
    unsigned char *sr = sg + src_w_stride * src_h;
    unsigned char *db = dst_image;
    unsigned char *dg = db + dst_w_stride * dst_h;
    unsigned char *dr = dg + dst_w_stride * dst_h;
    tensor_DX         = map;
    tensor_DY         = tensor_DX + dst_h * dst_w;
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float sx = tensor_DX[y * dst_w + x];
            float sy = tensor_DY[y * dst_w + x];
            db[y * dst_w_stride + x * w_stride] =
                fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sb);
            dg[y * dst_w_stride + x * w_stride] =
                fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sg);
            dr[y * dst_w_stride + x * w_stride] =
                fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sr);
        }
    }
    return 0;
}

static int bmcv_perspective_nearest_1n_ref(
                                    // input
                                    unsigned char *src_image,
                                    float *        trans_mat,
                                    int            image_c,
                                    int            image_sh,
                                    int            image_sw,
                                    int            image_dh,
                                    int            image_dw,
                                    // output
                                    int *map,
                                    unsigned char * dst_image) {
    UNUSED(image_c);
    float* tensor_S = (float*)malloc(image_dh *image_dw * 2 * sizeof(float));
    float*     tensor_SX  = tensor_S;
    float*     tensor_SY  = tensor_SX + image_dh * image_dw;
    int        *tensor_DX = map;
    int        *tensor_DY = tensor_DX + image_dh * image_dw;
    int        dst_w      = image_dw;
    int        dst_h      = image_dh;
    int        src_w      = image_sw;
    int        src_h      = image_sh;
    float      m0         = trans_mat[0];
    float      m1         = trans_mat[1];
    float      m2         = trans_mat[2];
    float      m3         = trans_mat[3];
    float      m4         = trans_mat[4];
    float      m5         = trans_mat[5];
    float      m6         = trans_mat[6];
    float      m7         = trans_mat[7];
    float      m8         = trans_mat[8];

    // generate the input for calculate coordinate map.
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            tensor_SX[y * dst_w + x] = (float)x;
            tensor_SY[y * dst_w + x] = (float)y;
        }
    }
    // calculate coordinate map.
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float dx = tensor_SX[y * dst_w + x] * m0 +
                       tensor_SY[y * dst_w + x] * m1 + m2;
            float dy = tensor_SX[y * dst_w + x] * m3 +
                       tensor_SY[y * dst_w + x] * m4 + m5;
            float dz = tensor_SX[y * dst_w + x] * m6 +
                       tensor_SY[y * dst_w + x] * m7 + m8;

            dx = dx / dz;
            dy = dy / dz;

            tensor_DX[y * dst_w + x] = (int)(dx + 0.5f);
            tensor_DY[y * dst_w + x] = (int)(dy + 0.5f);
            tensor_DX[y * dst_w + x] =
                BM_MIN(tensor_DX[y * dst_w + x], image_sw - 1);
            tensor_DX[y * dst_w + x] =
                BM_MAX(tensor_DX[y * dst_w + x], 0);
            tensor_DY[y * dst_w + x] =
                BM_MIN(tensor_DY[y * dst_w + x], image_sh - 1);
            tensor_DY[y * dst_w + x] =
                BM_MAX(tensor_DY[y * dst_w + x], 0);
        }
    }

    int w_stride = 1;
    int src_w_stride = src_w * w_stride;
    int dst_w_stride = dst_w * w_stride;

    // warp in source image directly.
    unsigned char *sb = src_image;
    unsigned char *sg = sb + src_w_stride * src_h;
    unsigned char *sr = sg + src_w_stride * src_h;
    unsigned char *db = dst_image;
    unsigned char *dg = db + dst_w_stride * dst_h;
    unsigned char *dr = dg + dst_w_stride * dst_h;
    tensor_DX         = map;
    tensor_DY         = tensor_DX + dst_h * dst_w;

    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            unsigned short  sx = tensor_DX[y * dst_w + x];
            unsigned short  sy = tensor_DY[y * dst_w + x];
            db[y * dst_w_stride + x * w_stride] =
                sb[sy * src_w_stride + sx * w_stride];
            dg[y * dst_w_stride + x * w_stride] =
                sg[sy * src_w_stride + sx * w_stride];
            dr[y * dst_w_stride + x * w_stride] =
                sr[sy * src_w_stride + sx * w_stride];
        }
    }
    free(tensor_S);
    return 0;
}

static int bmcv_warp_perspective_ref(
    // input
    unsigned char* src_image,
    float* trans_mat,
    int matrix_num[4],
    int image_n,
    int image_c,
    int image_sh,
    int image_sw,
    int image_dh,
    int image_dw,
    int use_bilinear,
    // output
    int* map,
    unsigned char* dst_image,
    float* tensor_S)
{
    int output_num = 0;
    int matrix_sigma[4] = {0};
    bm_status_t ret = BM_SUCCESS;
    for (int i = 0; i < image_n; i++) {
        output_num += matrix_num[i];
        matrix_sigma[i] = output_num;
    }
    for (int i = 0; i < output_num; i++) {
        int s_idx = get_source_idx(i, matrix_sigma, image_n);
        int src_offset = s_idx * image_c * image_sh * image_sw;
        int dst_offset = i * image_c * image_dh * image_dw;
        if (use_bilinear)
            ret = bmcv_perspective_bilinear_1n_ref(
                    src_image + src_offset,
                    trans_mat + i * 9,
                    image_c,
                    image_sh,
                    image_sw,
                    image_dh,
                    image_dw,
                    (float*)map + i * image_dh * image_dw * 2,
                    dst_image + dst_offset,
                    tensor_S);
        else
            ret = bmcv_perspective_nearest_1n_ref(
                    src_image + src_offset,
                    trans_mat + i * 9,
                    image_c,
                    image_sh,
                    image_sw,
                    image_dh,
                    image_dw,
                    map + i * image_dh * image_dw * 2,
                    dst_image + dst_offset);

        if (ret != BM_SUCCESS)
            return ret;
    }
    return ret;
}

static int bmcv_warp_perspective_tpu(
                                 bm_handle_t handle,
                                 // input
                                 unsigned char* src_data,
                                 float* trans_mat,
                                 int* coordinate,
                                 int matrix_num[4],
                                 int src_mode,
                                 int use_bilinear,
                                 int loop_times,
                                 int image_n,
                                 int image_c,
                                 int image_sh,
                                 int image_sw,
                                 int image_dh,
                                 int image_dw,
                                 // output
                                 unsigned char* dst_data) {
    bm_status_t ret = BM_SUCCESS;
    UNUSED(loop_times);
    UNUSED(src_mode);
    int output_num = 0;
    for (int i = 0; i < image_n; i++) {
        output_num += matrix_num[i];
    }
    bmcv_perspective_image_matrix matrix_image[4];
    bmcv_perspective_matrix* matrix = (bmcv_perspective_matrix *)(trans_mat);
    for (int i = 0; i < image_n; i++) {
        matrix_image[i].matrix_num = matrix_num[i];
        matrix_image[i].matrix = matrix;
        matrix += matrix_num[i];
    }

    bmcv_perspective_image_coordinate coordinate_image[4];
    bmcv_perspective_coordinate* coord = (bmcv_perspective_coordinate *)(coordinate);
    for (int i = 0; i < image_n; i++) {
        coordinate_image[i].coordinate_num = matrix_num[i];
        coordinate_image[i].coordinate = coord;
        coord += matrix_num[i];
    }

    bm_image src_img[4];
    bm_image_format_ext image_format = FORMAT_BGR_PLANAR;
    bm_image_data_format_ext data_type = DATA_TYPE_EXT_1N_BYTE;
    int in_image_num = image_n;
    int out_image_num = output_num;
    struct timeval t1, t2;

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
    if (loop_times % 3 == 0) {
        printf("No coordinate\n");
        gettimeofday(&t1, NULL);
        ret = bmcv_image_warp_perspective(handle, image_n, matrix_image, src_img, dst_img, use_bilinear);
        gettimeofday(&t2, NULL);
        if (BM_SUCCESS != ret) {
            free(dst_img);
            return -1;
        }
    }
    if (loop_times % 3 == 1) {
        printf("With coordinate\n");
        gettimeofday(&t1, NULL);
        ret = bmcv_image_warp_perspective_with_coordinate(handle, image_n, coordinate_image, src_img, dst_img, use_bilinear);
        gettimeofday(&t2, NULL);
        if (BM_SUCCESS != ret) {
            free(dst_img);
            return -1;
        }
    }
    if (loop_times % 3 == 2) {
        printf("No coordinate and similar to opencv\n");
        float matrix_tem[3][3];
        float matrix_tem_inv[3][3];
        for (int i = 0; i < image_n; i++) {
            for(int matrix_no = 0; matrix_no < matrix_image[i].matrix_num; matrix_no++){
                memset(matrix_tem, 0, sizeof(matrix_tem));
                memset(matrix_tem_inv, 0, sizeof(matrix_tem_inv));
                for(int a=0;a<9;a++){
                        matrix_tem[(a/3)][(a%3)]=matrix_image[i].matrix->m[a];
                }
                inverse_matrix(3, matrix_tem, matrix_tem_inv);
                for(int a=0;a<9;a++){
                        matrix_image[i].matrix->m[a]=matrix_tem_inv[(a/3)][(a%3)];
                }
            }
        }
        gettimeofday(&t1, NULL);
        ret = bmcv_image_warp_perspective_similar_to_opencv(handle, image_n, matrix_image, src_img, dst_img, use_bilinear);
        gettimeofday(&t2, NULL);
        if (BM_SUCCESS != ret) {
            free(dst_img);
            return -1;
        }
    }
    printf("Warp_perspective TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    // std::cout << "---warp_perspective TPU using time= " << ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec) << "(us)" << std::endl;
    int size = 0;
    bm_image_get_byte_size(dst_img[0], &size);
    unsigned char* temp_out = (unsigned char*)malloc(out_image_num * size * sizeof(unsigned char));

    for (int i = 0; i < out_image_num; i++) {
        void *ptr = (void *)(temp_out + size * i);
        bm_image_copy_device_to_host(dst_img[i], (void **)&ptr);
    }
    memcpy(dst_data, temp_out, output_num * image_c * image_dh * image_dw);

    free(temp_out);
    free(dst_img);
    return 0;
}

static int test_cv_warp_perspective_single_case(
                                     unsigned char*        src_image,
                                     float*         trans_mat,
                                     int*           coordinate,
                                     int            matrix_num[4],
                                     int            src_mode,
                                     int           use_bilinear,
                                     int            loop_times,
                                     int            image_n,
                                     int            image_c,
                                     int            image_sh,
                                     int            image_sw,
                                     int            image_dh,
                                     int            image_dw,
                                     float*     tensor_S) {
    int ret = 0;
    int output_num = 0;
    struct timeval t1, t2;
    for (int i = 0; i < image_n; i++) {
        output_num += matrix_num[i];
    }

    int* map           = (int*) malloc(output_num *image_dh *image_dw * 2 * sizeof(int));
    unsigned char* dst_image_ref = (unsigned char*) malloc(output_num * image_c * image_dh * image_dw * sizeof(unsigned char));
    unsigned char* dst_image_tpu = (unsigned char*) malloc(output_num * image_c * image_dh * image_dw * sizeof(unsigned char));
    // calculate use reference for compare.
    gettimeofday(&t1, NULL);
    ret = bmcv_warp_perspective_ref(src_image,
                                    trans_mat,
                                    matrix_num,
                                    image_n,
                                     image_c,
                                    image_sh,
                                    image_sw,
                                    image_dh,
                                    image_dw,
                                    use_bilinear,
                                    map,                                        dst_image_ref,
                                    tensor_S);
    gettimeofday(&t2, NULL);
    printf("Warp_perspective CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    if (ret) {
        printf("run bm_warp_perspective_ref failed ret = %d\n", ret);
        return -1;
    }
    // calculate use NPU.
    ret = bmcv_warp_perspective_tpu(handle,
                                    src_image,
                                    trans_mat,
                                    coordinate,
                                    matrix_num,
                                    src_mode,
                                    use_bilinear,
                                    loop_times,
                                    image_n,
                                    image_c,
                                    image_sh,
                                    image_sw,
                                    image_dh,
                                    image_dw,
                                    dst_image_tpu);
    if (ret) {
        printf("run bm_warp_perspective on tpu failed ret = %d\n", ret);
        return -1;
    }

    // compare.
    ret = bmcv_warp_perspective_cmp(dst_image_ref,
                                dst_image_tpu,
                                src_image,
                                map,
                                matrix_num,
                                use_bilinear,
                                image_n,
                                image_c,
                                image_sh,
                                image_sw,
                                image_dh,
                                image_dw);
    if (ret) {
        printf("cv_warp_perspective comparing failed\n");
        return -1;
    }

    free(map);
    free(dst_image_ref);
    free(dst_image_tpu);
    return ret;
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

static int test_cv_warp_perspective_random(int trials) {
    int loop_times = 0;
    for (int idx_trial = 0; idx_trial < trials; idx_trial++) {
        int image_c = 3;
        int matrix_num[4] = {1, 1, 1, 1};
        printf("use_bilinear: %d\n", use_bilinear);
        int src_mode = STORAGE_MODE_1N_INT8;
        int image_n = 1;
        if (flag == 1){
            image_sh = (rand() & 0x7fff);
            image_sw = (rand() & 0x7fff);
            while (image_sh > 2160 || image_sh < 1080) {
                image_sh = (rand() & 0x7fff);
            }
            while (image_sw > 3840 || image_sw < 1920) {
                image_sw = (rand() & 0x7fff);
            }

            image_dh = (rand() & 0x3ff) + 1;
            image_dw = (rand() & 0x3ff) + 1;
        }

        if (flag == 2){
            image_sh = (rand() & 0x7fff);
            image_sw = (rand() & 0x7fff);
            while (image_sh > 3840 || image_sh < 2048) {
                image_sh = (rand() & 0x7fff);
            }
            while (image_sw > 3840 || image_sw < 2048) {
                image_sw = (rand() & 0x7fff);
            }

            image_dh = (rand() & 0x7fff) + 1;
            image_dw = (rand() & 0x7fff) + 1;

            while (image_dh > 2048 || image_dh < 1024) {
                image_dh = (rand() & 0x7fff);
            }
            while (image_dw > 2048 || image_dw < 1024) {
                image_dw = (rand() & 0x7fff);
            }
        }

        if (flag == 3){
            image_sh = (rand() & 0x7fff);
            image_sw = (rand() & 0x7fff);
            while (image_sh > 5000 || image_sh < 4096) {
                image_sh = (rand() & 0x7fff);
            }
            while (image_sw > 5000 || image_sw < 4096) {
                image_sw = (rand() & 0x7fff);
            }

            image_dh = (rand() & 0x7fff) + 1;
            image_dw = (rand() & 0x7fff) + 1;

            while (image_dh > 4096 || image_dh < 2048) {
                image_dh = (rand() & 0x7fff);
            }
            while (image_dw > 4096 || image_dw < 2048) {
                image_dw = (rand() & 0x7fff);
            }
        }

        printf("image_n %d input[%dx%d] output[%dx%d]\n",
                image_n,
                image_sw,
                image_sh,
                image_dw,
                image_dh);
        int output_num = 0;
        printf("matrix number: ");
        for (int i = 0; i < image_n; i++) {
            output_num += matrix_num[i];
            printf("%d ", matrix_num[i]);
        }
        printf("total %d\n", output_num);
        if (!output_num) {
            return -1;
        }

        bm_status_t ret        = BM_ERR_FAILURE;
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

        ret = test_cv_warp_perspective_single_case(
                                    src_data,
                                    trans_mat,
                                    coordinate,
                                    matrix_num,
                                    src_mode,
                                    use_bilinear,
                                    loop_times,
                                    image_n,
                                    image_c,
                                    image_sh,
                                    image_sw,
                                    image_dh,
                                    image_dw,
                                    tensor_S);
        if (ret) {
            printf("------Test warp_perspective Failed------\n");
            free(trans_mat);
            free(coordinate);
            free(tensor_S);
            return -1;
        }
        printf("------Test warp_perspective Successed!------\n");

        loop_times++;
        free(trans_mat);
        free(coordinate);
        free(tensor_S);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int test_loop_times = 1;
    int thread_num = 1;
    int seed = -1;
    bool fixed = false;
    int dev_id = 0;
    bm_status_t ret = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    if (argc > 1){
        flag = atoi(argv[1]);
    }

    if (argc > 2){
        flag = atoi(argv[1]);
        use_bilinear = atoi(argv[2]);
        image_sh     = atoi(argv[3]);
        image_sw     = atoi(argv[4]);
        image_dh     = atoi(argv[5]);
        image_dw     = atoi(argv[6]);
    }

    if (test_loop_times > 1500 || test_loop_times < 1) {
        printf("[TEST WARP PERSPECTIVE] loop times should be 1~1500\n");
        exit(-1);
    }
    if (thread_num < 1 || thread_num > 4) {
        printf("[TEST WARP PERSPECTIVE] thread_num should be 1~4\n");
        exit(-1);
    }

    printf("[TEST WARP PERSPECTIVE] test starts... LOOP times will be %d\n", test_loop_times);
    for (int loop_idx = 0; loop_idx < test_loop_times; loop_idx++) {
        printf("------[TEST WARP PERSPECTIVE] LOOP %d ------\n", loop_idx);

        struct timespec tp;
        clock_gettime(0, &tp);

        seed = (fixed) ? seed : tp.tv_nsec;
        srand(seed);
        printf("random seed %d\n", seed);
        ret = test_cv_warp_perspective_random(1);
        if (ret != 0) {
            printf("Test cv_warp_perspective failed\n");
            exit(-1);
        }
    }
    printf("------[TEST WARP PERSPECTIVE] ALL TEST PASSED!\n");
    bm_dev_free(handle);

    return 0;
}