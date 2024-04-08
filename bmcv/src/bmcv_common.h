#ifndef BMCV_COMMON_H
#define BMCV_COMMON_H

#include "bmcv_api_ext_c.h"
#ifdef __x86_64__
void print_trace(void);
#else
#define print_trace()                                                                              \
  do {                                                                                             \
  } while (0)
#endif


#if defined (__cplusplus)
extern "C" {
#endif

#ifndef FW_MAX_SHAPE_DIMS
#define FW_MAX_SHAPE_DIMS 8
#endif  // FW_MAX_SHAPE_DIMS

#define WARP_MAX_W (256)
#define WARP_MAX_H (256)

#define IS_NAN(x) ((((x >> 23) & 0xff) == 0xff) && ((x & 0x7fffff) != 0))
#define ABS(x) ((x) > 0 ? (x) : (-(x)))

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

#ifdef __linux__
#define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)
#else
#define ALIGN(x, a) __ALIGN_MASK(x, (int)(a)-1)
#endif

#define DISTANCE_DIM_MAX 8

typedef union {
  int ival;
  float fval;
} IF_VAL;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char  s8;
typedef signed short s16;
typedef signed int   s32;
typedef signed long long int s64;

#define INLINE inline
#define UNUSED(x) (void)(x)

#define INT8_SIZE 1
#define FLOAT_SIZE 4

#define bm_min(x, y) (((x)) < ((y)) ? (x) : (y))
#define bm_max(x, y) (((x)) > ((y)) ? (x) : (y))

#ifdef SOC_MODE
struct ce_base {
    u32 alg;
    u32 enc;
    u64 src;
    u64 dst;
    u64 len;
    u64 dstlen;
};

#define CE_BASE_IOC_TYPE                ('C')
#define CE_BASE_IOC_IOR(nr, type)       _IOR(CE_BASE_IOC_TYPE, nr, type)
/* do one shot operation on physical address */
#define CE_BASE_IOC_OP_PHY      \
        CE_BASE_IOC_IOR(0, struct ce_base)

enum {
        CE_BASE_IOC_ALG_BASE64,
        CE_BASE_IOC_ALG_MAX,
};
#endif


#ifndef USING_CMODEL
#define hang(_ret) while (1)
#else
#define hang(_ret) exit(_ret)
#endif

#define ASSERT(_cond) ASSERT_INFO(_cond, "none.")

typedef struct bm_api_cv_absdiff {
  int channel;
  u64 input1_addr[3];
  u64 input2_addr[3];
  u64 output_addr[3];
  int width[3];
  int height[3];
  int input1_str[3];
  int input2_str[3];
  int output_str[3];
} __attribute__((packed)) bm_api_cv_absdiff_t;

typedef struct {
  u64 input_img_addr;
  int img_w;
  int img_w_stride;
  int img_h;
  float alpha_0;
  float beta_0;
  float alpha_1;
  float beta_1;
  float alpha_2;
  float beta_2;
  int convert_storage_mode;
  int image_num;
  int input_img_data_type;
  int output_img_data_type;
  u64 output_img_addr;
} __attribute__((packed)) bm_api_cv_convert_to_t;

typedef struct bm_api_cv_axpy {
  u64 A_global_offset;
  u64 X_global_offset;
  u64 Y_global_offset;
  u64 F_global_offset;
  int input_n;
  int input_c;
  int input_h;
  int input_w;
} __attribute__((packed)) bm_api_cv_axpy_t;

typedef struct bm_api_cv_add_weighted {
  int channel;
  u64 input1_addr[3];
  u64 input2_addr[3];
  u64 output_addr[3];
  int width[3];
  int height[3];
  int input1_str[3];
  int input2_str[3];
  int output_str[3];
  float alpha;
  float beta;
  float gamma;
} __attribute__((packed)) bm_api_cv_add_weighted_t;

typedef struct bm_api_cv_bitwise {
  int channel;
  u64 input1_addr[3];
  u64 input2_addr[3];
  u64 output_addr[3];
  int width[3];
  int height[3];
  int input1_str[3];
  int input2_str[3];
  int output_str[3];
  int op;
} __attribute__((packed)) bm_api_cv_bitwise_t;

typedef struct bm_api_cv_min_max {
  u64 inputAddr;
  u64 minAddr;
  u64 maxAddr;
  int len;
  int mode;
} __attribute__((packed)) bm_api_cv_min_max_t;

typedef struct bm_api_cv_cmulp {
  u64 XR;
  u64 XI;
  u64 PR;
  u64 PI;
  u64 YR;
  u64 YI;
  int batch;
  int len;
} __attribute__((packed)) bm_api_cv_cmulp_t;

typedef struct bm_api_cv_distance {
  u64 Xaddr;
  u64 Yaddr;
  int dim;
  float pnt[DISTANCE_DIM_MAX];
  int len;
  int dtype;
} __attribute__((packed)) bm_api_cv_distance_t;

typedef struct bm_api_cv_hamming_distance {
  u64 input_query_global_offset;
  u64 input_database_global_offset;
  u64 output_global_offset;
  int bits_len;
  int left_row;
  int right_row;
} __attribute__((packed)) bm_api_cv_hamming_distance_t;

typedef struct bm_api_cv_sort {
  u64 src_data_addr;
  u64 src_index_addr;
  int data_cnt;
  u64 dst_data_addr;
  u64 dst_index_addr;
  int sort_cnt;
  int order;
  int index_enable;
  int auto_index;
} __attribute__((packed)) bm_api_cv_sort_t;

typedef struct bm_api_laplacian {
  int channel;
  u64 input_addr[3];
  u64 kernel_addr;
  u64 output_addr[3];
  u32 width[3];
  u32 height[3];
  int kw;
  int kh;
  u32 stride_i[3];
  u32 stride_o[3];
  float delta;
  int is_packed;
} __attribute__((packed)) bm_api_laplacian_t;

typedef struct bm_api_cv_as_strided {
  u64 input_addr;
  u64 output_addr;
  int input_row;
  int input_col;
  int output_row;
  int output_col;
  int row_stride;
  int col_stride;
} __attribute__((packed)) bm_api_cv_as_strided_t;

typedef struct bm_api_cv_transpose {
  u64 input_global_mem_addr;
  u64 output_global_mem_addr;
  int channel;
  int height;
  int width;
  int type_len;
} __attribute__((packed)) bm_api_cv_transpose_t;

typedef struct bm_api_cv_threshold {
  int channel;
  u64 input_addr[3];
  u64 output_addr[3];
  int width[3];
  int height[3];
  int input_str[3];
  int output_str[3];
  int type;
  u32 thresh;
  u32 max_value;
} __attribute__((packed)) bm_api_cv_threshold_t;

typedef struct bm_api_cv_quantify {
  int channel;
  u64 input_addr[3];
  u64 output_addr[3];
  int width[3];
  int height[3];
  int input_str[3];
  int output_str[3];
} __attribute__((packed)) bm_api_cv_quantify_t;

typedef struct bm_api_cv_filter {
  int channel;
  unsigned long long input_addr[3];
  unsigned long long kernel_addr;
  unsigned long long output_addr[3];
  int width[3];
  int height[3];
  int kw;
  int kh;
  int stride_i[3];
  int stride_o[3];
  float delta;
  int is_packed;
  int out_type;
}__attribute__((packed)) bm_api_cv_filter_t;

typedef struct bm_api_cv_bayer2rgb {
  int width;
  int height;
  int dst_fmt;
  int src_type;
  u64 input_addr;
  u64 output_addr;
  u64 sys_mem_addr_temp_ul;
  u64 sys_mem_addr_temp_br;
  u64 sys_mem_addr_temp_g;
  u64 sys_mem_addr_temp_b;
  u64 input_addr_padding_up_left;
  u64 input_addr_padding_bottom_right;
  u64 kernel_addr;
}__attribute__((packed)) bm_api_cv_bayer2rgb_t;

typedef struct bm_api_cv_copy_to_st {
  u64 input_image_addr;
  u64 output_image_addr;
  u64 padding_image_addr;
  int C;
  int input_w_stride;
  int input_w;
  int input_h;
  int padding_w_stride;
  int padding_w;
  int padding_h;
  int data_type;
  int bgr_or_rgb;
  int planner_or_packed;
  int padding_b;
  int padding_g;
  int padding_r;
  int if_padding;
} __attribute__((packed)) bm_api_cv_copy_to_t;

typedef struct sgcv_affine_matrix_s{
    float m[6];
} __attribute__((packed)) sgcv_affine_matrix;

typedef struct sg_api_cv_warp {
  u64 input_image_addr;
  u64 output_image_addr;
  u64 index_image_addr;
  u64 input_image_addr_align;
  u64 out_image_addr_align;
  sgcv_affine_matrix m;
  int src_w_stride;
  int dst_w_stride;
  int image_n;
  int image_sh;
  int image_sw;
  int image_dh;
  int image_dw;
  int type;
} __attribute__((packed)) sg_api_cv_warp_t;

typedef struct {
  u64 input_image_addr;
  u64 output_image_addr;
  u64 index_image_addr;
  u64 input_image_addr_align_r;
  u64 input_image_addr_align_g;
  u64 input_image_addr_align_b;
  u64 out_image_addr_align_a;
  u64 out_image_addr_align_b;
  u64 system_image_addr_lu;
  u64 system_image_addr_ld;
  u64 system_image_addr_ru;
  u64 system_image_addr_rd;
  bm_image_data_format_ext input_format;
  bm_image_data_format_ext output_format;
  sgcv_affine_matrix m;
  int image_c;
  int image_sh;
  int image_sw;
  int image_dh;
  int image_dw;
  int type;
} __attribute__((packed)) sg_api_cv_warp_bilinear_t;

typedef struct sgcv_perspective_matrix_s{
    float m[9];
} __attribute__((packed)) sgcv_perspective_matrix;

typedef struct sg_api_cv_warp_perspective {
  unsigned long long input_image_addr;
  unsigned long long output_image_addr;
  unsigned long long index_image_addr;
  unsigned long long input_image_addr_align;
  unsigned long long out_image_addr_align;
  sgcv_perspective_matrix m;
  int src_w_stride;
  int dst_w_stride;
  int image_n;
  int image_sh;
  int image_sw;
  int image_dh;
  int image_dw;
  int type;
} __attribute__((packed)) sg_api_cv_warp_perspective_1684x_t;

typedef struct bm_api_cv_pyramid {
  u64 input_addr;
  u64 kernel_addr;
  u64 output_addr;
  int width;
  int height;
  int ow;
  int oh;
  int stride_i;
  int stride_o;
}__attribute__((packed)) bm_api_cv_pyramid_t;

typedef struct sg_api_batch_topk_t {
  u64 input_data_addr;
  u64 input_index_addr;
  u64 output_data_addr;
  u64 output_index_addr;
  u64 buffer_addr;
  int input_index_valid;
  int k;
  int descending;
  int batchs;
  int batch_num;
  int batch_stride;
  int dtype;
}__attribute__((packed)) sg_api_batch_topk_t;

typedef struct bm_api_cv_matmul {
  int M;
  int N;
  int K;
  u64 A_addr;
  u64 B_addr;
  u64 C_addr;
  int A_sign;
  int B_sign;
  int rshift_bit;
  int result_type;
  int is_B_trans;
  float alpha;
  float beta;
}__attribute__((packed)) bm_api_cv_matmul_t;

typedef struct bm_api_cv_calc_hist_index {
  u64 Xaddr;
  u64 Yaddr;
  float a;
  float b;
  int len;
  int xdtype;
  float upper;
}__attribute__((packed)) bm_api_cv_calc_hist_index_t;

typedef struct bm_api_cv_gemm {
  u64 A_global_offset;
  u64 B_global_offset;
  u64 C_global_offset;
  u64 Y_global_offset;
  int M;
  int N;
  int K;
  int is_A_trans;
  int is_B_trans;
  float alpha;
  float beta;
  bm_image_data_format_ext in_dtype;
  bm_image_data_format_ext out_dtype;
} __attribute__((packed)) bm_api_cv_gemm_t;

typedef struct bm_api_cv_feature_match_1684x {
  u64 input_data_global_addr;
  u64 db_data_global_addr;
  u64 db_feature_global_addr;
  u64 db_data_trans_addr;
  u64 output_temp_addr;
  u64 output_sorted_similarity_global_addr;
  u64 output_sorted_index_global_addr;
  int batch_size;
  int feature_size;
  int db_size;
} __attribute__((packed)) bm_api_cv_feature_match_1684x_t;

typedef struct bm_api_cv_feature_match_fix8b_1684x {
  u64 input_data_global_addr;
  u64 db_data_global_addr;
  u64 output_similarity_global_addr;
  u64 output_sorted_index_global_addr;
  u64 arm_tmp_buffer;
  int batch_size;
  int feature_size;
  int db_size;
  int sort_cnt;
  int rshiftbits;
} __attribute__((packed)) bm_api_cv_feature_match_fix8b_1684x_t;

typedef struct bm_api_cv_hist_balance1 {
    u64 Xaddr;
    int len;
    u64 cdf_addr;
} __attribute__((packed)) bm_api_cv_hist_balance_t1;

typedef struct bm_api_cv_hist_balance2 {
    u64 Xaddr;
    int len;
    float cdf_min;
    u64 cdf_addr;
    u64 Yaddr;
} __attribute__((packed)) bm_api_cv_hist_balance_t2;

#ifdef _WIN32

DECL_EXPORT int clock_gettime(int dummy, struct timespec* ct);
DECL_EXPORT int gettimeofday(struct timeval* tp, void* tzp);

#endif

#if defined (__cplusplus)
}
#endif

/********************************************/
#endif  // BMCV_API_STRUCT_H
