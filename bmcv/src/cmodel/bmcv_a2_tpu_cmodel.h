#ifndef BMCV_A2_TPU_CMODEL_H
#define BMCV_A2_TPU_CMODEL_H

#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif

#include "bmcv_a2_common_internal.h"
#include "test_misc.h"
#define CHN_NUM_MAX 3
#define DIM_MAX 8
#define IMAGE_CHN_NUM_MAX 3
#define CMP_ERR_THRESHOLD 1e-6
#define GRAY_SERIES 256
#define KERNEL_LENGTH 9
#define UINT_MIN 0
#define SATURATE(a, s, e) ((a) > (e) ? (e) : ((a) < (s) ? (s) : (a)))
#define DTYPE_F32 4
#define DTYPE_U8 1
#define UNUSED(x) (void)(x)
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))
#define bm_min(x, y) (((x)) < ((y)) ? (x) : (y))
#define bm_max(x, y) (((x)) > ((y)) ? (x) : (y))
#define M_PI 3.14159265358979323846

enum operation{
    AND = 7,
    OR = 8,
    XOR = 9,
};

enum Win_mode {
    HANN = 0,
    HAMM = 1,
};

enum Pad_mode {
    CONSTANT = 0,
    REFLECT = 1,
};

typedef struct {
    float alpha;
    float beta;
} convert_to_arg_t;

struct row_para {
    int input_row;
    int output_row;
    int row_stride;
};

struct col_para {
    int input_col;
    int output_col;
    int col_stride;
};
struct row_col_para {
    struct row_para row;
    struct col_para col;
};

typedef struct {
    int first;
    float second;
} Pair;

struct frame_size {
    int height;
    int width;
};

struct hist_para {
    int chn_num; /*1 or 2 or 3 */
    int out_dim; /*if C=1, dim =1; if C=2, dim = 1 / 2; if C=3; dim = 1 / 2 / 3*/
    int data_type; /* 0 is float, 1 is uint8 */
    int op; /* 0 is no weighted, 1 is weighted */
};

struct Matpara {
    int M;
    int K;
    int N;
    int trans_A;
    int trans_B;
    int signA;
    int signB;
    int right_shift_bit;
    int result_type;
    float alpha;
    float beta;
};
/*
  test_cv_absdiff
*/
int absdiff_cmodel(unsigned char* input1,
                          unsigned char* input2,
                          unsigned char* output,
                          int img_size);
/*
  test_cv_add_weight
*/
int cpu_add_weighted(unsigned char* input1, unsigned char* input2, unsigned char* output,
                            int img_size,  float alpha, float beta, float gamma);
/*
  test_cv_as_strided
*/
int cpu_as_strided(float* input, float* output, struct row_col_para row_col);
/*
  test_cv_base64
*/
int base64_encode(void *dst, void *src, int size, char *box);
int base64_decode(void *dst, void *src, unsigned long size, char *box);
/*
  test_cv_batch_topk
*/
void topk_reference(int k, int batch, int batch_num, int batch_stride, int descending, bool bottom_index_valid,
                    float* bottom_data, int* bottom_index, float* top_data_ref, int* top_index_ref);
/*
  test_cv_bayer2rgb
*/
void bayer2rgb_cpu(unsigned char* dstImgCpu_r, unsigned char* dstImgCpu_g, unsigned char* dstImgCpu_b,
                   unsigned char* srcImg, int height, int width, int src_type);
/*
  test_cv_bitwise
*/
int cpu_bitwise(unsigned char* input1, unsigned char* input2, unsigned char* output,
                        int len, int op);
/*
  test_cv_calc_hist
*/
int cpu_calc_hist(void* input_host, float* output_cpu, struct frame_size* frame,
                        struct hist_para* para, const float* weighted);
/*
  test_cv_cmulp
*/
int cpu_cmul(float* XRHost, float* XIHost, float* PRHost, float* PIHost,
                    float* cpu_YR, float* cpu_YI, int L, int batch);
/*
  test_cv_convert_to
*/
int32_t convert_to_ref_float( float            *src,
                              float            *dst,
                              convert_to_arg_t *convert_to_arg,
                              int               image_num,
                              int               image_channel,
                              int               image_h,
                              int               image_w_stride,
                              int               image_w,
                              int               convert_format);
int32_t convert_to_ref_int8( signed char            *src,
                             signed char            *dst,
                             convert_to_arg_t *convert_to_arg,
                             int               image_num,
                             int               image_channel,
                             int               image_h,
                             int               image_w_stride,
                             int               image_w,
                             int               convert_format);
/*
  test_cv_copy_to
*/
int packedToplanar_float(float * packed, int channel, int width, int height);
int copy_to_ref_v2_float(float * src,
                                float * dst,
                                int channel,
                                int in_h,
                                int in_w,
                                int out_h,
                                int out_w,
                                int start_x,
                                int start_y,
                                int ispacked);
/*
  test_cv_distance
*/
int cpu_distance_fp32(float* XHost, float* cpu_out, int L, int dim, float pnt32[]);
int cpu_distance_fp16(fp16* XHost16, fp16* cpu_out_fp16, int L,
                            int dim, fp16 pnt16[]);
/*
  test_cv_draw_lines
*/
int draw_line_cpu(unsigned char* input, int height, int width, int line_num, int format,
                        const bmcv_point_t* start, const bmcv_point_t* end,
                        unsigned char color[3], int thickness);
/*
  test_cv_feature_match
*/
int calc_sqrt_transposed(float** feature, int rows, int cols, float* db_feature);
int cpu_feature_match(float** input_data, int batch_size, int feature_size, float** db_data,
                            int db_size, float** match_res);
int cpu_feature_match_fix8b(int8_t** input_data, int batch_size, int feature_size,
                                    int8_t** db_data, int db_size, int rshiftbits,
                                    short** match_res);
/*
  test_cv_gaussian_blur
*/
int gaussian_blur_cpu_ref(unsigned char *input, unsigned char *output, int channel, bool is_packed,
                                 int height, int width, int kw, int kh, float sigmaX, float sigmaY);
/*
  test_cv_gemm
*/
int cpu_gemm(bool if_A_trans, bool if_B_trans, int M, int N, int K, float alpha, float* src_A,
                    int lda, float* src_B, int ldb, float beta, float* src_C);
/*
  test_cv_hist_balance
*/
int cpu_hist_balance(uint8_t* input_host, uint8_t* output_cpu, int height, int width);
/*
  test_cv_hm_distance
*/
void native_cal_hammingDistance(int *input1, int *input2, int *output, int bits_len,
                                       int input1_num, int input2_num);
/*
  test_cv_laplace
*/
int cpu_lap(unsigned char* src, unsigned char* dst, int width, int height, int ksize);
/*
  test_cv_matmul
*/
int cpu_matmul(signed char* src_A, signed char* src_B, void* output, struct Matpara para);
/*
  test_cv_min_max
*/
int cpu_min_max(float* XHost, int L, float* min_cpu, float* max_cpu);
/*
  test_cv_nms
*/
int cpu_nms(face_rect_t* proposals, const float nms_threshold, face_rect_t* nmsProposals,
                    int size, int* result_size);
/*
  test_cv_put_text
*/
int get_image_offset(int format, int width, int height, int* offset_list);
int put_text_cpu(unsigned char* input, int height, int width, const char* text,
                        bmcv_point_t org, int fontFace, float fontScale, int format,
                        unsigned char color[3], int thickness);
/*
  test_cv_pyramid
*/
int cpu_pyramid_down(unsigned char* input, unsigned char* output,
                            int height, int width, int oh, int ow);
/*
  test_cv_quantify
*/
int quantify_cpu(
        float* input,
        unsigned char* output,
        int height,
        int width);
/*
  test_cv_threshold
*/
int threshold_cpu(
        unsigned char* input,
        unsigned char* output,
        int height,
        int width,
        unsigned char threshold,
        unsigned char max_val,
        int type);
/*
  test_cv_transpose
*/
int cpu_transpose(void* input, void* output, int channel, int height,
                        int width, int stride, int dtype);
/*
  test_cv_warp_affine
*/
int get_source_idx(int idx, int *matrix, int image_n);
unsigned char fetch_pixel_2(float x_idx, float y_idx, int c_idx, int width, int height, unsigned char* image);
int bmcv_warp_ref(
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
    bool use_opencv);
/*
  test_cv_warp_perspective
*/
int bmcv_warp_perspective_ref(
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
    float* tensor_S);
/*
  test_cv_sort
*/
void mergeSort(sort_t ref_res[], int left, int right, bool is_ascend);
/*
  test_cv_fft
*/
int cpu_fft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int length, int batch, bool forward);
int cpu_fft_2d(float *in_real, float *in_imag, float *out_real,
                        float *out_imag, int M, int N, bool forward);
/*
  test_cv_stft
*/
int cpu_stft(float* XRHost, float* XIHost, float* YRHost_cpu, float* YIHost_cpu, int L,
            int batch, int n_fft, int hop_length, int num_frames, int pad_mode, int win_mode, bool norm);
/*
  test_cv_isft
*/
int cpu_istft(float* input_R, float* input_I, float* out_R, float* out_I, int L,
                    int batch, int n_fft, bool real_input, int win_mode, int hop_length, bool norm);
#endif