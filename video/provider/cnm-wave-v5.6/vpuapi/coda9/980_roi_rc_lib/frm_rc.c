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

#include "frm_rc.h"
#include <assert.h>

void frm_rc_seq_init(
    //IN
    frm_rc_t *rc,
    int bps,
    int buf_size_ms,
    int frame_rate,    
    int pic_width,
    int pic_height,
    int intra_period,
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
    )
{
    int fixed_bit_ratio[MAX_GOP_SIZE];
    int enc_bps  = bps;
    int xmit_bps = 0; //ABR
    int bit_alloc_mode = BIT_ALLOC_ADAPTIVE;
    fw_gop_entry_t gop[MAX_GOP_SIZE];
    int intra_qp = 32; //any value is OK
    int hrd_buf_size;
    int i;

    rc->rc_pic.gop_size = gop_size;

    if(gop_size == 0)
    {
        gop[0].curr_poc = 1;
        gop[0].qp = intra_qp;
    }
    else
    {
        gop[gop_size-1].curr_poc = gop_size;
        gop[gop_size-1].qp       = intra_qp + dqp[gop_size-1];

        for(i = 0; i< gop_size-1 ;i++)
        {
            gop[i].curr_poc = i;
            gop[i].qp = intra_qp + dqp[i];
        }
    }

    if(is_first_pic == 1)
        rc_init(&rc->rc_pic);
    
    rc_seq_init(&rc->rc_pic, enc_bps, xmit_bps, buf_size_ms, bit_alloc_mode, fixed_bit_ratio,
        frame_rate, gop_size, gop, pic_width, pic_height, intra_period, rc_mode, intra_qp,
        rc_initial_qp, !is_first_pic,
        rc_weight_factor, 
        gamma,
#ifdef CLIP_PIC_DELTA_QP
        max_delta_qp_minus,
        max_delta_qp_plus,
#endif
        &hrd_buf_size);
}

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
    int *target_pic_bit,
    int *hrd_buf_level,
    int *hrd_buf_size)
{
    int pic_qp;
    int pic_target_bit;
    int pic_buf_level;
    int pic_buf_size;
#ifdef CLIP_PIC_DELTA_QP
    int row_max_dqp_minus;
    int row_max_dqp_plus;
#endif

    if((rc->rc_pic.enc_order_in_gop + 1) > rc->rc_pic.gop_size)
        rc->rc_pic.enc_order_in_gop = 0;
    rc_pic_init(&rc->rc_pic, is_intra, rc->rc_pic.enc_order_in_gop, min_qp, max_qp,
#ifdef CLIP_PIC_DELTA_QP
        &row_max_dqp_minus, &row_max_dqp_plus,
#endif
        &pic_qp, &pic_target_bit, &pic_buf_level, &pic_buf_size);

    *out_qp = pic_qp;

    rc->rc_pic.enc_order_in_gop++;

    *target_pic_bit = pic_target_bit;
    *hrd_buf_level = pic_buf_level;
    *hrd_buf_size = pic_buf_size;
#ifdef CLIP_PIC_DELTA_QP
    *max_delta_qp_plus = row_max_dqp_plus;
    *max_delta_qp_minus = row_max_dqp_minus;
#endif

}

void frm_rc_pic_end(
    //IN
    frm_rc_t *rc,
    int real_pic_bit,
    int avg_qp,
    int frm_skip_flag
)
{
    //int frm_skip_flag = 0;
    //int avg_qp = rc->average_pic_qp;

    rc_pic_end(&rc->rc_pic, avg_qp, real_pic_bit, frm_skip_flag);
}
 
