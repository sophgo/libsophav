#ifndef BMCV_A2_DPU_CMODEL_H
#define BMCV_A2_DPU_CMODEL_H

#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif

#include "bmcv_a2_common_internal.h"

typedef enum frame_type_{
    TOP_FIELD = 0,
    BOT_FIELE = 1,
    PROGRESSIVE = 2,
    RAW = 3,
} frame_type;

typedef struct bm_frame_{
    frame_type type;
    bm_image_format_ext format;
    int            width;
    int            height;
    int            channel;
    int            depth;
    int            *data;
} bm_frame;

#define ASR(x, n)              ((x) < 0? ~((~(x)) >> (n)) : (x) >> (n))
#define MIN(x, y)              (((x) < (y)) ? (x) : (y) )
#define MAX(x, y)              (((x) > (y)) ? (x) : (y) )
#define CLIP(x, xl, xh)        (MIN(MAX((x), (xl)), (xh)))
#define MINMAX(x, min, max)    (MIN(MAX((x), (min)), (max)))
#define SIGN(x)                (((x) < 0) ? -1 : 1)
#define LIMIT(a, up, low)      (MIN(MAX((a), (low)), (up)))

#define DPU_DEPTH_BITS (16)     // Use 16,8 for check
#define DPU_COST_BITS  (16)
#define DPU_SOBEL_BITS (8)
#define DPU_MAX_COST   (32767)

#define MIN4(a, b, c, d) (MIN((a), MIN((b), (MIN((c), (d))))))
#define GET_LR_PTR_FROM_PAD(ptr, x, depth) ptr + (x + 1) * (depth) + 1

#define set_array_val(array, array_size, val)       \
    for (int i = 0; i < array_size; i++) {       \
        array[i] = val;                            \
    }                                               \

#define swap(x, y)                                  \
    do {                                            \
        __typeof__(x) SWAP = x;                     \
        x = y;                                      \
        y = SWAP;                                   \
    } while (0)                                     \

// MIRROR
// -1 to 1, -2 to 2 ...
// width to width - 2, width + 1 to width - 3 ...
#define MIRROR(x, width)      ( ((x) < 0)? (-(x)):(((x) > (width)-1)? (2*(width)-2 -(x)):(x)) )

/// Access PIXEL(x,y,ch) in a Frame:
///   frame:    pointer of a frame struct
///   x,y:      pixel location
///   ch:       pixel channel
#define SETPIXEL(frame, x, y, ch, val) \
    (*(frame->data + (MIRROR((y), frame->height) * frame->width + MIRROR((x),frame->width)) * frame->channel + (ch)) = val)
#define PIXEL(frame, x, y, ch) \
    (*(frame->data + (MIRROR((y), frame->height) * frame->width + MIRROR((x),frame->width)) * frame->channel + (ch)))

#define DPU_MAX_COST (32767)

// COMMON Functions
bm_status_t bm_frame_create(bm_frame *frame, frame_type type, bm_image_format_ext format,
                            int width, int height, int channel, int depth);
bm_status_t bm_frame_copy_data(bm_frame *frame, frame_type type, bm_image_format_ext format,
                            int width, int height, int channel, int depth, unsigned char * input_img);
int bm_frame_destroy(bm_frame *frame);

// LOCAL Functions
bm_status_t cmodel_dpu_adcost(bm_frame *src1, bm_frame *src2, bm_frame *dst);
bm_status_t cmodel_dpu_add(bm_frame *src1, bm_frame *src2, bm_frame *dst, bmcv_dpu_sgbm_attrs *grp);
bm_status_t cmodel_dpu_boxfilter(bm_frame *src, bm_frame *dst, bmcv_dpu_sgbm_attrs *grp, int width, int height);
bm_status_t cmodel_dpu_btDistanceCost(bm_frame *pImg1, bm_frame *pImg2, bm_frame *dst, bmcv_dpu_sgbm_attrs *grp, int width, int height);
bm_status_t cmodel_dpu_dcc(bm_frame *src, bm_frame *dst, bmcv_dpu_sgbm_attrs *grp, int width, int height);
bm_status_t cmodel_dpu_dispinterp(bm_frame *src_cost, bm_frame *src_disp, bm_frame *dst_disp, bmcv_dpu_sgbm_attrs *grp, int width, int height);
bm_status_t cmodel_dpu_lrCheck(bm_frame *src_cost, bm_frame *src_disp, bm_frame *dst_disp, bmcv_dpu_sgbm_attrs *grp);
bm_status_t cmodel_dpu_median3x3(bm_frame *src_disp, bm_frame *dst_disp, int width, int height);
bm_status_t cmodel_dpu_census(bm_frame *src_disp, bm_frame *dst_disp, bmcv_dpu_sgbm_attrs *grp, int width, int height);
bm_status_t cmodel_dpu_UniquenessCheck(bm_frame *src_cost, bm_frame *src_disp, bm_frame *dst_disp, bmcv_dpu_sgbm_attrs *grp, int width, int height);
bm_status_t cmodel_dpu_wta(bm_frame *src_cost, bm_frame *dst_disp, bmcv_dpu_sgbm_attrs *grp, int width, int height);
bm_status_t cmodel_dpu_dispU16toU8(bm_frame *src, bm_frame *dst, bmcv_dpu_sgbm_attrs *grp, int width, int height);
bm_status_t cmodel_dpu_dispU8toDepthU16(bm_frame *dispData, bm_frame *depthData, bmcv_dpu_disp_range dispRange, bmcv_dpu_fgs_attrs *fgs_grp, int width, int height);
bm_status_t cmodel_dpu_dccPair(bm_frame *src, bm_frame *dst, bmcv_dpu_sgbm_attrs *grp, int width, int height);
bm_status_t cmodel_dpu_fgs(bm_frame *smooth_img, bm_frame *guide_img, bmcv_dpu_fgs_attrs *fgs_grp);

// FGS Functions
void read_row_uint8();
void write_row_uint8();
void read_row_uint16();
void write_row_uint16();
void calc_left_to_right(int t, int y);
void calc_right_to_left(int t, int y);
void calc_bottom_to_top(int t, int y);
unsigned int quan_div(unsigned int divisor, unsigned int dividend);

// add for print
unsigned char* rev_smooth_img_curr_row_buffer;
bm_frame* revu_frame;

// TEST Functions
bm_status_t test_cmodel_sgbm(unsigned char *left_input,
                             unsigned char *right_input,
                             unsigned char *ref_output,
                             int width,
                             int height,
                             bmcv_dpu_sgbm_mode sgbm_mode,
                             bmcv_dpu_sgbm_attrs *grp);

bm_status_t test_cmodel_sgbm_u16(unsigned char *left_input,
                                 unsigned char *right_input,
                                 unsigned short *ref_output,
                                 int width,
                                 int height,
                                 bmcv_dpu_sgbm_mode sgbm_mode,
                                 bmcv_dpu_sgbm_attrs *grp);

bm_status_t test_cmodel_fgs(unsigned char *smooth_input,
                            unsigned char *guide_input,
                            unsigned char *ref_output,
                            int width,
                            int height,
                            bmcv_dpu_fgs_mode fgs_mode,
                            bmcv_dpu_fgs_attrs *grp);

bm_status_t test_cmodel_fgs_u16(unsigned char *smooth_input,
                                unsigned char *guide_input,
                                unsigned short *ref_output,
                                int width,
                                int height,
                                bmcv_dpu_fgs_mode fgs_mode,
                                bmcv_dpu_fgs_attrs *grp,
                                bmcv_dpu_disp_range disp_range);

bm_status_t test_cmodel_online(unsigned char *left_input,
                               unsigned char *right_input,
                               unsigned char *ref_output,
                               int width,
                               int height,
                               bmcv_dpu_online_mode online_mode,
                               bmcv_dpu_sgbm_attrs *grp,
                               bmcv_dpu_fgs_attrs *fgs_grp);

bm_status_t test_cmodel_online_u16(unsigned char *left_input,
                                   unsigned char *right_input,
                                   unsigned short *ref_output,
                                   int width,
                                   int height,
                                   bmcv_dpu_online_mode online_mode,
                                   bmcv_dpu_sgbm_attrs *grp,
                                   bmcv_dpu_fgs_attrs *fgs_grp);
#endif