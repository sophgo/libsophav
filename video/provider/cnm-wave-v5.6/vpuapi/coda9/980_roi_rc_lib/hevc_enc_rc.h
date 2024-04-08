//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
// 
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
// 
// The entire notice above must be reproduced on all authorized copies.
// 
// Description  : 
//-----------------------------------------------------------------------------

#ifndef _HEVC_ENC_RC_H_
#define _HEVC_ENC_RC_H_

#include "util_float.h"

#define MAX_GOP_SIZE    8
#define PARA_CHANGE_RC_SEQ_INIT 

#define CLIP3(a, b, c)                  (MIN(b, MAX(a, c)))
#define MAX(a, b)                       (((a) > (b)) ? (a) : (b))
#define MIN(a, b)                       (((a) < (b)) ? (a) : (b))


enum {
    BIT_ALLOC_ADAPTIVE,
    BIT_ALLOC_FIXED_RATIO,
};

typedef struct {
    int dqp;    //delta qp relative anchor qp
    FLOAT_T a;   //QP = a*log2R + b
    FLOAT_T b;
    FLOAT_T bpp; //from alloc_gop_bit()
} rq_model_t;

typedef struct {
    //int32_t pic_type;
    int32_t curr_poc;
    int32_t qp;
    //int32_t temporal_id;
    //int32_t ref_poc[2];
} fw_gop_entry_t;

typedef struct {
    int num_pixel_in_pic;
    int mb_width;
    int mb_height;
    int gop_size;
    int intra_period;
    int bit_alloc_mode;

    int fixed_bit_ratio[MAX_GOP_SIZE];

    int mode;
    int avg_pic_bit;
    int xmit_pic_bit;

    rq_model_t intra_model;
    rq_model_t inter_model[MAX_GOP_SIZE];
    rq_model_t *curr_model;
    rq_model_t *anchor_model;
    FLOAT_T avg_bpp;
    int min_qp;
    int max_qp;
    int buf_level;
    int init_level;
    int buf_size;

    int frame_num;  //for trace
    int initial_qp;
    int max_intra_bit;
    int para_changed;
    int weight_factor;
    int gamma;

#ifdef CLIP_PIC_DELTA_QP
    int last_anchor_qp;
    int max_delta_qp_plus;
    int max_delta_qp_minus;
#endif
    int enc_order_in_gop;

} rc_pic_t;

#ifdef __cplusplus
extern "C" {
#endif

    void rc_init(rc_pic_t *rc);

    void rc_seq_init(
        //IN
        rc_pic_t *rc,
        int enc_bps,
        int xmit_bps,
        int buf_size_ms,
        int bit_alloc_mode,
        int fixed_bit_ratio[MAX_GOP_SIZE],
        int frame_rate,    
        int gop_size,
        fw_gop_entry_t gop[MAX_GOP_SIZE],
        int pic_width,
        int pic_height,
        int intra_period,
        int rc_mode,
        int intra_qp,
        int rc_initial_qp,
        int is_para_change,
        int weight_factor,
        int gamma,
#ifdef CLIP_PIC_DELTA_QP
        int max_delta_qp_minus,
        int max_delta_qp_plus,
#endif
        //OUT
        int *hrd_buf_size);

    void rc_pic_init(
        //IN
        rc_pic_t *rc,
        int is_intra,
        int enc_order_in_gop,
        int min_qp,
        int max_qp,
        //OUT
#ifdef CLIP_PIC_DELTA_QP
        int *max_delta_qp_minus,
        int *max_delta_qp_plus,
#endif
        int *pic_qp,
        int *pic_target_bit,
        int *hrd_buf_level,
        int *hrd_buf_size);

    void rc_pic_end(
        //IN
        rc_pic_t *rc,
        int avg_qp,
        int real_pic_bit,
        int frm_skip_flag);

#ifdef __cplusplus
}
#endif

#endif
 
