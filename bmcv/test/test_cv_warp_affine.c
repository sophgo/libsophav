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
#define UNUSED(x) (void)(x)
#define IMG_C 3
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

static  int image_sh = 1080;
static  int image_sw = 1920;
static  int image_dh = 112;
static  int image_dw = 112;
static int flag = 0;
static int is_bilinear = 0;
static bm_handle_t handle;

static int get_source_idx(int idx, int *matrix, int image_n) {
    for (int i = 0; i < image_n; i++) {
        if (idx < matrix[i]) {
            return i;
        }
    }
    printf("%s:%d[%s] Error:get source idx error\n",__FILE__, __LINE__, __FUNCTION__);
    exit(-1);
}

float det(float arcs[3][3],int n){
    if(n == 1){
        return arcs[0][0];
    }
    float ans = 0;
    float temp[3][3];
    int i,j,k;
    for(i = 0;i < n;i++){
        for(j = 0;j < n - 1;j++){
            for(k = 0;k < n - 1;k++){
                temp[j][k] = arcs[j+1][(k >= i) ? k+1 : k];
            }
        }
        float t = det(temp, n-1);
        if(i%2 == 0){
            ans += arcs[0][i] * t;
        }
        else{
            ans -=  arcs[0][i] * t;
        }
    }
    return ans;
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

        for (int i = pad_h_value * image_sw; i < pad_h_value * image_sw + image_n * image_c * image_sh * image_sw;i++)
            res_temp[i] = res[i];

        for (int i = pad_h_value * image_sw + image_n * image_c * image_sh * image_sw;i <  pad_h_value * image_sw + image_n * image_c * image_sh * image_sw + pad_h_value * image_sw;i++)
            res_temp[i] = 0;
    }

    if (image_dw > image_sw){
        int pad_w_value = (image_dw - image_sw) / 2;
        for (int i = 0;i < image_dh;i++){
            for (int i = 0;i < pad_w_value;i++)
                res_temp_bak[i] = 0;
            for (int i = pad_w_value;i < pad_w_value + image_sw;i++)
                res_temp_bak[i] = res_temp[i-pad_w_value];
            for (int i = pad_w_value + image_sw;i < pad_w_value + image_sw + pad_w_value + image_sw;i++)
                res_temp_bak[i] = 0;
        }
    }

    free(res);
    free(res_temp);
    return res_temp_bak;
}

unsigned char fetch_pixel_1(int x_idx, int y_idx, int c_idx, int width, int height, int w_stride, unsigned char* image)
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

    unsigned char pixel_lu = image[width * height * c_idx + ceil_y * width + floor_x];
    unsigned char pixel_ld = image[width * height * c_idx + floor_y * width + floor_x];
    unsigned char pixel_ru = image[width * height * c_idx + ceil_y * width + ceil_x];
    unsigned char pixel_rd = image[width * height * c_idx + floor_y * width + ceil_x];

    float coef_x_ceil = x_idx - floor_x;
    float coef_y_ceil = y_idx - floor_y;

    float coef_x_floor = 1.0 - coef_x_ceil;
    float coef_y_floor = 1.0 - coef_y_ceil;

    unsigned char mix = (unsigned char)(coef_x_floor * coef_y_ceil * pixel_lu +
                coef_x_floor * coef_y_floor * pixel_ld +
                coef_x_ceil * coef_y_ceil * pixel_ru +
                coef_x_ceil * coef_y_floor * pixel_rd + 0.5f);
    return mix;
};

void inverse_matrix(float matrix[3][3], float matrix_inv[2][3]){
    float det_value = det(matrix, 3);
    matrix_inv[0][0] = matrix[1][1] / det_value;
    matrix_inv[0][1] = -matrix[0][1] / det_value;
    matrix_inv[0][2] = (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) / det_value;
    matrix_inv[1][0] = - matrix[1][0] / det_value;
    matrix_inv[1][1] = matrix[0][0] / det_value;
    matrix_inv[1][2] = (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) / det_value;
}

static int bmcv_affine_nearest_cmp_1n(
                            unsigned char * p_exp,
                            unsigned char * p_got,
                            unsigned char * src,
                            int *           map,
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
                unsigned char got = p_got[c * image_dw_stride * image_dh +
                                          y * image_dw_stride + x * w_stride];
                unsigned char exp = p_exp[c * image_dw_stride * image_dh +
                                          y * image_dw_stride + x * w_stride];
                if (got != exp) {
                    // loopup up/down left/right by 1 pixel in source image.
                    int sx = map_x[y * image_dw + x];
                    int sy = map_y[y * image_dw + x];
                    //if(sx >= image_sw || sx < 0 || sy >= image_sh || sy < 0)
                    //    continue;
                    unsigned char left = fetch_pixel_1(
                        sx + 1, sy, c, image_sw, image_sh, w_stride, src);
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

static int bmcv_affine_bilinear_cmp_1n(unsigned char * p_exp,
                                       unsigned char * p_got,
                                       unsigned char * src,
                                       float*          map,
                                       int             image_c,
                                       int             image_sh,
                                       int             image_sw,
                                       int             image_dh,
                                       int             image_dw) {
    // generate map.
    float* map_x = map;
    float* map_y = map + image_dh * image_dw;

    for (int c = 0; c < image_c; c++) {
        for (int y = 0; y < image_dh; y++) {
            for (int x = 0; x < image_dw; x++) {
                unsigned char got = p_got[c * image_dw * image_dh +
                                          y * image_dw + x];
                unsigned char exp = p_exp[c * image_dw * image_dh +
                                          y * image_dw + x];
                if (got != exp) {
                    // loopup up/down left/right by 1 pixel in source image.
                    float sx = map_x[y * image_dw + x];
                    float sy = map_y[y * image_dw + x];
                    //if(sx >= image_sw || sx < 0 || sy >= image_sh || sy < 0)
                    //    continue;
                    unsigned char mix_pixel =
                        fetch_pixel_2(sx, sy, c, image_sw, image_sh, src);

                    if (abs(mix_pixel - got) <= 1) {
                        continue;
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


static int bmcv_warp_cmp(unsigned char*   p_exp,
                         unsigned char*   p_got,
                         unsigned char*   src,
                         int*  map,
                         int       matrix_num[4],
                         int       image_n,
                         int       image_c,
                         int       image_sh,
                         int       image_sw,
                         int       image_dh,
                         int       image_dw,
                         int      is_bilinear) {
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
        if (is_bilinear)
            ret = bmcv_affine_bilinear_cmp_1n(p_exp + dst_offset,
                                              p_got + dst_offset,
                                              src + src_offset,
                                              (float*)map + i * image_dh * image_dw * 2,
                                              image_c,
                                              image_sh,
                                              image_sw,
                                              image_dh,
                                              image_dw);
        else
            ret = bmcv_affine_nearest_cmp_1n(p_exp + dst_offset,
                                             p_got + dst_offset,
                                             src + src_offset,
                                             (int*)map + i * image_dh * image_dw * 2,
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

static int bmcv_affine_nearest_1n_ref(
                                    // input
                                    unsigned char *src_image,
                                    float *        trans_mat,
                                    int            image_c,
                                    int            image_sh,
                                    int            image_sw,
                                    int            image_dh,
                                    int            image_dw,
                                    // output
                                    int *          map,
                                    unsigned char *dst_image,
                                    bool use_opencv) {

    UNUSED(image_c);
    float* tensor_S = (float*)malloc(image_dh * image_dw * 2 * sizeof(float));
    float *tensor_SX    = tensor_S;
    float *tensor_SY    = tensor_SX + image_dh * image_dw;
    int *tensor_DX      = map;
    int *tensor_DY      = tensor_DX + image_dh * image_dw;
    int             dst_w     = image_dw;
    int             dst_h     = image_dh;
    int             src_w     = image_sw;
    int             src_h     = image_sh;

    if (use_opencv){
        float mat_array[6];
        float matrix_tem[3][3];
        float matrix_tem_inv[2][3];
        memset(matrix_tem, 0, sizeof(matrix_tem));
        memset(matrix_tem_inv, 0, sizeof(matrix_tem_inv));
        for(int a = 0;a < 6;a++){
            matrix_tem[(a/3)][(a%3)] = trans_mat[a];
        }
        matrix_tem[2][0] = 0;
        matrix_tem[2][1] = 0;
        matrix_tem[2][2] = 1;
        inverse_matrix(matrix_tem, matrix_tem_inv);
        for(int a = 0;a < 6;a++){
            mat_array[a] = matrix_tem_inv[(a/3)][(a%3)];
        }
        float           m0        = mat_array[0];
        float           m1        = mat_array[1];
        float           m2        = mat_array[2];
        float           m3        = mat_array[3];
        float           m4        = mat_array[4];
        float           m5        = mat_array[5];

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
                // tensor_DX[y * dst_w + x] = (unsigned short)(dx + 0.5f);
                // tensor_DY[y * dst_w + x] = (unsigned short)(dy + 0.5f);
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
    }else{
        float           m0        = trans_mat[0];
        float           m1        = trans_mat[1];
        float           m2        = trans_mat[2];
        float           m3        = trans_mat[3];
        float           m4        = trans_mat[4];
        float           m5        = trans_mat[5];
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
                // tensor_DX[y * dst_w + x] = (unsigned short)(dx + 0.5f);
                // tensor_DY[y * dst_w + x] = (unsigned short)(dy + 0.5f);
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
            unsigned short sx = tensor_DX[y * dst_w + x];
            unsigned short sy = tensor_DY[y * dst_w + x];
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

static int bmcv_affine_bilinear_1n_ref(
                                    // input
                                    unsigned char *src_image,
                                    float *        trans_mat,
                                    int            image_c,
                                    int            image_sh,
                                    int            image_sw,
                                    int            image_dh,
                                    int            image_dw,
                                    // output
                                    float *        map_index,
                                    unsigned char *dst_image,
                                    bool opencv_mode) {
    UNUSED(image_c);
    float* tensor_S = (float*)malloc(image_dh *image_dw * 2 * sizeof(float));
    float *tensor_SX = tensor_S;
    float *tensor_SY = tensor_SX + image_dh * image_dw;
    float *tensor_DX = map_index;
    float *tensor_DY = tensor_DX + image_dh * image_dw;
    int             dst_w     = image_dw;
    int             dst_h     = image_dh;
    int             src_w     = image_sw;
    int             src_h     = image_sh;

    if (opencv_mode){
        float mat_array[6];
        float matrix_tem[3][3];
        float matrix_tem_inv[2][3];
        memset(matrix_tem, 0, sizeof(matrix_tem));
        memset(matrix_tem_inv, 0, sizeof(matrix_tem_inv));
        for(int a = 0;a < 6;a++){
            matrix_tem[(a/3)][(a%3)] = trans_mat[a];
        }
        matrix_tem[2][0] = 0;
        matrix_tem[2][1] = 0;
        matrix_tem[2][2] = 1;
        inverse_matrix(matrix_tem, matrix_tem_inv);
        for(int a = 0;a < 6;a++){
            mat_array[a] = matrix_tem_inv[(a/3)][(a%3)];
        }
        float           m0        = mat_array[0];
        float           m1        = mat_array[1];
        float           m2        = mat_array[2];
        float           m3        = mat_array[3];
        float           m4        = mat_array[4];
        float           m5        = mat_array[5];

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
                tensor_DX[y * dst_w + x] = dx;
                tensor_DY[y * dst_w + x] = dy;
            }
        }
    } else {
        float           m0        = trans_mat[0];
        float           m1        = trans_mat[1];
        float           m2        = trans_mat[2];
        float           m3        = trans_mat[3];
        float           m4        = trans_mat[4];
        float           m5        = trans_mat[5];

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
                tensor_DX[y * dst_w + x] = dx;
                tensor_DY[y * dst_w + x] = dy;
            }
        }
    }

    int w_stride = 1;
    int src_w_stride = src_w * w_stride;
    int dst_w_stride = dst_w * w_stride;

    if (image_c == 3) {
        // warp in source image directly.
        unsigned char *sb = src_image;
        unsigned char *sg = sb + src_w_stride * src_h;
        unsigned char *sr = sg + src_w_stride * src_h;
        unsigned char *db = dst_image;
        unsigned char *dg = db + dst_w_stride * dst_h;
        unsigned char *dr = dg + dst_w_stride * dst_h;
        tensor_DX         = map_index;
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
    }else {
        // warp in source image directly.
        unsigned char *sb = src_image;
        unsigned char *db = dst_image;
        tensor_DX         = map_index;
        tensor_DY         = tensor_DX + dst_h * dst_w;
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                float sx = tensor_DX[y * dst_w + x];
                float sy = tensor_DY[y * dst_w + x];
                db[y * dst_w_stride + x * w_stride] =
                    fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sb);
            }
        }
    }
    free(tensor_S);
    return 0;
}


static int bmcv_warp_ref(
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
    int is_bilinear,
    // output
    int* map,
    unsigned char* dst_image,
    bool use_opencv)
{
    int output_num      = 0;
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
        if (is_bilinear)
            ret = bmcv_affine_bilinear_1n_ref(
                src_image + src_offset,
                trans_mat + i * 6,
                image_c,
                image_sh,
                image_sw,
                image_dh,
                image_dw,
                (float *)map + i * image_dh * image_dw * 2,
                dst_image + dst_offset,
                use_opencv);
        else
            ret = bmcv_affine_nearest_1n_ref(
                src_image + src_offset,
                trans_mat + i * 6,
                image_c,
                image_sh,
                image_sw,
                image_dh,
                image_dw,
                (int *)map + i * image_dh * image_dw * 2,
                dst_image + dst_offset,
                use_opencv);
        if (ret != BM_SUCCESS)
            return ret;
    }
    return ret;
}

static int bmcv_warp_tpu(bm_handle_t handle,
                                 // input
                                 unsigned char* src_data,
                                 float* trans_mat,
                                 int matrix_num[4],
                                 int image_n,
                                 int image_c,
                                 int image_sh,
                                 int image_sw,
                                 int image_dh,
                                 int image_dw,
                                 int is_bilinear,
                                 // output
                                 unsigned char* dst_data,
                                 bool use_opencv) {
    bm_status_t ret = BM_SUCCESS;
    int output_num = 0;
    for (int i = 0; i < image_n; i++) {
        output_num += matrix_num[i];
    }

    bmcv_affine_image_matrix matrix_image[4];
    bmcv_affine_matrix *     matrix = (bmcv_affine_matrix *)(trans_mat);
    for (int i = 0; i < image_n; i++) {
        matrix_image[i].matrix_num = matrix_num[i];
        matrix_image[i].matrix     = matrix;
        matrix += matrix_num[i];
    }

    bm_image                 src_img[4];
    bm_image_format_ext      image_format = FORMAT_BGR_PLANAR;
    bm_image_data_format_ext data_type    = DATA_TYPE_EXT_1N_BYTE;

    for (int i = 0; i < image_n; i++){
        bm_image_create(
            handle, image_sh, image_sw, image_format, data_type, src_img + i, NULL);
        int stride = 0;
        bm_image_get_stride(src_img[i], &stride);

        void *ptr = (void *)(src_data + 3 * stride * image_sh * i);

        bm_image_copy_host_to_device(src_img[i], (void **)(&ptr));
    }

    // create dst image.
    bm_image* dst_img = (bm_image*)malloc(image_n * sizeof(bm_image));

    for (int i = 0; i < image_n; i++) {
        bm_image_create(
            handle, image_dh, image_dw, image_format, data_type, dst_img + i, NULL);
    }
    struct timeval t1, t2;
    if (!use_opencv) {
        printf("normal mode\n");
        gettimeofday(&t1, NULL);
        ret = bmcv_image_warp_affine(handle, image_n, matrix_image, src_img, dst_img, is_bilinear);
        gettimeofday(&t2, NULL);
        if (BM_SUCCESS != ret) {
            free(dst_img);
            return -1;
        }
    }else{
        printf("opencv mode\n");
        gettimeofday(&t1, NULL);
        ret = bmcv_image_warp_affine_similar_to_opencv(handle, image_n, matrix_image, src_img, dst_img, is_bilinear);
        gettimeofday(&t2, NULL);
        if (BM_SUCCESS != ret) {
            free(dst_img);
            return -1;
        }
    }
    printf("Warp_affine TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    // std::cout << "---warp_affine TPU using time= " << ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec) << "(us)" << std::endl;

    int size = 0;
    bm_image_get_byte_size(dst_img[0], &size);
    unsigned char* temp_out = (unsigned char*)malloc(output_num * size * sizeof(unsigned char));

    for (int i = 0; i < image_n; i++) {
        void *ptr = (void *)(temp_out + size * i);
        bm_image_copy_device_to_host(dst_img[i], (void **)&ptr);
    }
    memcpy(dst_data, temp_out, image_n * image_c * image_dh * image_dw);

    for (int i = 0; i < image_n; i++){
        bm_image_destroy(&src_img[i]);
    }
    free(dst_img);
    free(temp_out);
    return 0;
}

static int test_cv_warp_single_case(unsigned char* src_image,
                                     float*         trans_mat,
                                     int            matrix_num[4],
                                     int            image_n,
                                     int            image_c,
                                     int            image_sh,
                                     int            image_sw,
                                     int            image_dh,
                                     int            image_dw,
                                     bool           use_opencv,
                                     int           is_bilinear) {
    int ret = 0;
    int output_num = 0;
    struct timeval t1, t2;
    for (int i = 0; i < image_n; i++) {
        output_num += matrix_num[i];
    }

    int* map = (int*)malloc(output_num *image_dh *image_dw * 2 * sizeof(int));
    unsigned char* dst_image_ref = (unsigned char*)malloc(output_num * image_c * image_dh * image_dw * sizeof(unsigned char));
    unsigned char* dst_image_tpu = (unsigned char*)malloc(output_num * image_c * image_dh * image_dw * sizeof(unsigned char));

    // calculate use reference for compare.
    gettimeofday(&t1, NULL);
    ret = bmcv_warp_ref(src_image,
                        trans_mat,
                        matrix_num,
                        image_n,
                        image_c,
                        image_sh,
                        image_sw,
                        image_dh,
                        image_dw,
                        is_bilinear,
                        map,
                        dst_image_ref,
                        use_opencv);
    gettimeofday(&t2, NULL);
    printf("Warp_affine CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    if (ret) {
        printf("run bm_warp_ref failed ret = %d\n", ret);
        return -1;
    }

    // calculate use NPU.
    ret = bmcv_warp_tpu(handle,
                        src_image,
                        trans_mat,
                        matrix_num,
                        image_n,
                        image_c,
                        image_sh,
                        image_sw,
                        image_dh,
                        image_dw,
                        is_bilinear,
                        dst_image_tpu,
                        use_opencv);
    if (ret) {
        printf("run bm_warp failed ret = %d\n", ret);
        return -1;
    }
    // compare.
    ret = bmcv_warp_cmp(dst_image_ref,
                                dst_image_tpu,
                                src_image,
                                map,
                                matrix_num,
                                image_n,
                                image_c,
                                image_sh,
                                image_sw,
                                image_dh,
                                image_dw,
                                is_bilinear);

    if (ret) {
        printf("cv_warp comparing failed\n");
        return -1;
    }

    free(map);
    free(dst_image_ref);
    free(dst_image_tpu);

    return ret;
}

static int test_cv_warp_random(int trials) {
    for (int idx_trial = 0; idx_trial < trials; idx_trial++) {
        int ret =0;
        int image_c = IMG_C;
        int matrix_num[4] = {0};

        printf("is_bilinear: %d \n", is_bilinear);
        int image_n = rand() % 0x03 + 1;
        bool use_opencv = rand() % 0x02 ? true : false;
        use_opencv = false;
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
        matrix_num[0] = 1;
        matrix_num[1] = 1;
        matrix_num[2] = 1;
        matrix_num[3] = 1;

        printf("image_n %d input[%dx%d] output[%dx%d]\n",
                image_n,
                image_sw,
                image_sh,
                image_dw,
                image_dh);
        int output_num = 0;
        for (int i = 0; i < image_n; i++){
            output_num += matrix_num[i];
        }
        if (!output_num){
            return -1;
        }

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
        ret = test_cv_warp_single_case(src_data, trans_mat, matrix_num, image_n, image_c,
                                        image_sh, image_sw, image_dh, image_dw, use_opencv, is_bilinear);
        if (ret) {
            printf("------Test warp_affine Failed------\n");
            free(trans_mat);
            return -1;
        }
        printf("------Test warp_affine Successed!------\n");
        free(trans_mat);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int         dev_id = 0;
    int seed           = -1;
    bool fixed         = false;
    bm_status_t ret    = bm_dev_request(&handle, dev_id);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    int test_loop_times = 1;

    if (argc > 1){
        flag = atoi(argv[1]);
    }

    if (argc > 2){
        flag = atoi(argv[1]);
        is_bilinear = atoi(argv[2]);
        image_sh    = atoi(argv[3]);
        image_sw    = atoi(argv[4]);
        image_dh    = atoi(argv[5]);
        image_dw    = atoi(argv[6]);
    }

    if (test_loop_times > 1500 || test_loop_times < 1) {
        printf("[TEST WARP AFFINE] loop times should be 1~1500\n");
        exit(-1);
    }
    printf("[TEST WARP AFFINE] test starts... LOOP times will be %d\n", test_loop_times);

    for (int loop_idx = 0; loop_idx < test_loop_times; loop_idx++) {
        printf("------[TEST WARP AFFINE] LOOP %d\n", loop_idx);

        struct timespec tp;
        clock_gettime(0, &tp);
        seed = (fixed) ? seed : tp.tv_nsec;
        srand(seed);
        printf("random seed %d\n", seed);

        ret = test_cv_warp_random(2);
        if (ret != 0) {
            printf("Test cv_warp_affine failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    printf("------[TEST WARP AFFINE] ALL TEST PASSED!------\n");

    return ret;
}
