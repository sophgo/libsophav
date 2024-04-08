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

#ifndef _FRM_RC_H_
#define _FRM_RC_H_

#include "hevc_enc_rc.h"

typedef struct {
    rc_pic_t rc_pic;
} frm_rc_t;

#ifdef __cplusplus
extern "C" {
#endif
void frm_rc_seq_init(
    //IN
    frm_rc_t *rc,
    int bps,            //unit is bits per second
    int buf_size_ms,    //HRD buffer size in unit of millisecond
    int frame_rate,    
    int pic_width,
    int pic_height,
    int intra_period,   //0(only first), 1(all intra),..
    int rc_mode,
    int gop_size,
    int dqp[MAX_GOP_SIZE],
    int rc_initial_qp,
    int is_first_pic,
    int gamma,
    int rc_weight_factor
#ifdef CLIP_PIC_DELTA_QP
    ,int max_delta_qp_minus,
    int max_delta_qp_plus
#endif
    ); //-1(internally calculate initial QP), 0~51(use this for initial QP)

void frm_rc_pic_init(
    //IN
    frm_rc_t *rc,
    int is_intra,
    int min_qp,
    int max_qp,
    //OUT
#ifdef CLIP_PIC_DELTA_QP
    int *max_delta_qp_minus,
    int *max_delta_qp_plus,
#endif
    int *out_qp,
    int *pic_target_bit,
    int *hrd_buf_level,
    int *hrd_buf_size
); //when en_roi=1, it's non-ROI QP. otherwise it's frame QP.

void frm_rc_pic_end(
    //IN
    frm_rc_t *rc,
    int real_pic_bit,
    int avg_qp,
    int frame_skip_flag);

#ifdef __cplusplus
}
#endif

#endif 

