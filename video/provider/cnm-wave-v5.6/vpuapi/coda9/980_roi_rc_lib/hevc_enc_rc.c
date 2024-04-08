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

#include <assert.h>
#include <math.h>
#include "hevc_enc_rc.h"
//#include "fw_util.h"

enum {
    CBR,
    VBR,
    ABR
};

#define MAX_ROI_LEVEL   8
#define LOG2_CTB_SIZE   6
#define CTB_SIZE        (1<<LOG2_CTB_SIZE)


double toDouble(FLOAT_T x)
{
#ifdef OPT_FLOAT
    return x.sig * pow(2.0,x.exp);
#else
    return x;
#endif
}

static int32_t fw_util_min(int32_t x, int32_t y)
{
    return (x<y) ? x : y;
}

static int32_t fw_util_max(int32_t x, int32_t y)
{
    return (x>y) ? x : y;
}

static int32_t fw_util_min_max(int32_t min, int32_t max , int32_t x)
{
    return (x<min) ? min : (x>max) ? max : x;
}

static FLOAT_T calc_qp_from_bpp(
    rq_model_t *m,
    FLOAT_T bpp)
{
    //double qp = m->a * log2(bpp) + m->b;
    FLOAT_T qp = FADD(FMUL(m->a, FLOG2(bpp)), m->b);
    return qp;
}

static FLOAT_T calc_bpp_from_qp(
    rq_model_t *m,
    FLOAT_T qp)
{
    //double bpp = pow2((qp - m->b) / m->a);
    FLOAT_T bpp = FPOW2(FDIV(FSUB(qp, m->b), m->a));
    return bpp;
}

static void init_rq_model(
    rc_pic_t *rc,
    int intra_qp,
    fw_gop_entry_t gop[MAX_GOP_SIZE])
{
    //intra model
    rc->intra_model.bpp = rc->avg_bpp;
    rc->intra_model.dqp = 0;

    //inter model
    if (rc->intra_period != 1) //not all intra
    {
        int i;
        int anchor_pos = 0;

        //set anchor model
        for (i=0; i<rc->gop_size; i++)
        {
            if (gop[i].curr_poc == rc->gop_size)
            {
                anchor_pos = i;
                rc->anchor_model = &rc->inter_model[i];
                break;
            }
        }

        //set inter
        for (i=0; i<rc->gop_size; i++)
        {
            rq_model_t *model = &rc->inter_model[i];
            model->dqp = gop[i].qp - gop[anchor_pos].qp;
            model->bpp = rc->avg_bpp;
        }

        rc->intra_model.dqp = intra_qp - gop[anchor_pos].qp;
    }    
}

static void update_init_beta(rc_pic_t *rc, int delta)
{
    int i;
    FLOAT_T f_delta = FINIT(delta, 0);
    rc->intra_model.b = FADD(rc->intra_model.b , f_delta);

    rc->intra_model.b = FMINMAX(FINIT(10, 0), FINIT(50, 0), rc->intra_model.b);

    for (i=0; i<rc->gop_size; i++)
    {
        rq_model_t *model = &rc->inter_model[i];

        model->b = FADD(model->b , f_delta);

        model->b = FMINMAX(FINIT(10, 0), FINIT(50, 0), model->b);


    }

}



static void alloc_gop_bit(
    rc_pic_t *rc)
{
    int i;

    if (rc->intra_period == 1)  //all intra
    {
        rc->intra_model.bpp = rc->avg_bpp;
    }
    else if (rc->bit_alloc_mode == BIT_ALLOC_FIXED_RATIO)
    {
        int total_ratio = 0;
        FLOAT_T gop_bpp = FINIT(0,0);
        FLOAT_T anchor_qp;
        FLOAT_T ratio;

        //inter
        for (i=0; i<rc->gop_size; i++)
            total_ratio += rc->fixed_bit_ratio[i];            

        for (i=0; i<rc->gop_size; i++)
        {
            rq_model_t *model = &rc->inter_model[i];

            //model->bpp = rc->avg_bpp * rc->gop_size * rc->fixed_bit_ratio[i] / total_ratio;
            model->bpp = FDIV(FMUL(FMUL(rc->avg_bpp,FINIT(rc->gop_size,0)),FINIT(rc->fixed_bit_ratio[i],0)),FINIT(total_ratio,0));
            if (model == rc->anchor_model && rc->intra_period != 0)
            {
                //gop_bpp += model->bpp * (1.0 - (double)rc->gop_size / rc->intra_period);
                gop_bpp = FADD(gop_bpp,FMUL(model->bpp,FSUB(FINIT(1,0),FDIV(FINIT(rc->gop_size,0),FINIT(rc->intra_period,0)))));
            }
            else
            {
                //gop_bpp += model->bpp;
                gop_bpp = FADD(gop_bpp,model->bpp);
            }
        }

        //intra
        anchor_qp = calc_qp_from_bpp(rc->anchor_model, rc->anchor_model->bpp);
        //rc->intra_model.bpp = calc_bpp_from_qp(&rc->intra_model, anchor_qp + rc->intra_model.dqp);
        rc->intra_model.bpp = calc_bpp_from_qp(&rc->intra_model,FADD(anchor_qp,FINIT(rc->intra_model.dqp,0)));
        if (rc->intra_period > 0)
        {
            //gop_bpp += rc->intra_model.bpp * rc->gop_size / rc->intra_period;
            gop_bpp = FADD(gop_bpp,FDIV(FMUL(rc->intra_model.bpp,FINIT(rc->gop_size,0)),FINIT(rc->intra_period,0)));
        }

        //adjust
        //ratio = rc->avg_bpp * rc->gop_size / gop_bpp;
        ratio = FDIV(FMUL(rc->avg_bpp,FINIT(rc->gop_size,0)),gop_bpp);
        //rc->intra_model.bpp *= ratio;
        rc->intra_model.bpp = FMUL(rc->intra_model.bpp,ratio);
        for (i=0; i<rc->gop_size; i++)
        {
            //rc->inter_model[i].bpp *= ratio;
            rc->inter_model[i].bpp = FMUL(rc->inter_model[i].bpp,ratio);
        }
    }
    else //adaptive bit allocation
    {
        FLOAT_T anchor_qp;
        FLOAT_T gop_bpp;
        FLOAT_T ratio;

        anchor_qp = calc_qp_from_bpp(rc->anchor_model, rc->anchor_model->bpp);

        //intra
        //rc->intra_model.bpp = calc_bpp_from_qp(&rc->intra_model, anchor_qp + rc->intra_model.dqp);
        rc->intra_model.bpp = calc_bpp_from_qp(&rc->intra_model,FADD(anchor_qp,FINIT(rc->intra_model.dqp,0)));
        if (rc->intra_period == 0)
            gop_bpp = FINIT(0,0);
        else
        {
            //gop_bpp = rc->intra_model.bpp * rc->gop_size / rc->intra_period;
            gop_bpp = FDIV(FMUL(rc->intra_model.bpp,FINIT(rc->gop_size,0)),FINIT(rc->intra_period,0));
        }

        //inter
        for (i=0; i<rc->gop_size; i++)
        {
            rq_model_t *model = &rc->inter_model[i];

            //model->bpp = calc_bpp_from_qp(model, anchor_qp + model->dqp);
            model->bpp = calc_bpp_from_qp(model, FADD(anchor_qp,FINIT(model->dqp,0)));
            if (model == rc->anchor_model && rc->intra_period != 0)
            {
                //gop_bpp += model->bpp * (1.0 - (double)rc->gop_size / rc->intra_period);
                gop_bpp = FADD(gop_bpp,FMUL(model->bpp,FSUB(FINIT(1,0),FDIV(FINIT(rc->gop_size,0),FINIT(rc->intra_period,0)))));
            }
            else
            {
                //gop_bpp += model->bpp;
                gop_bpp = FADD(gop_bpp,model->bpp);
            }
        }

        //adjust
        //ratio = rc->avg_bpp * rc->gop_size / gop_bpp;
        ratio = FDIV(FMUL(rc->avg_bpp,FINIT(rc->gop_size,0)),gop_bpp);
        //rc->intra_model.bpp *= ratio;
        rc->intra_model.bpp = FMUL(rc->intra_model.bpp,ratio);
        for (i=0; i<rc->gop_size; i++)
        {
            //rc->inter_model[i].bpp *= ratio;
            rc->inter_model[i].bpp = FMUL(rc->inter_model[i].bpp,ratio);
        }
    }
}

static void update_rq_model(
    rq_model_t *model,
    FLOAT_T bpp,
    int qp,
    int weight_factor)
{
    FLOAT_T qp_est = calc_qp_from_bpp(model, bpp);
    //double error  = qp - qp_est;
    FLOAT_T error = FSUB(FINIT(qp,0),qp_est);
    FLOAT_T log2r  = FLOG2(bpp);
    //double gain   = 0.5 / (log2r * log2r + 1.0); //0.5 for smoothing
    //FLOAT_T gain   = FDIV(FINIT(1,-1),FADD(FMUL(log2r,log2r),FINIT(1,0)));
    FLOAT_T gain   = FDIV(FINIT(1,-weight_factor),FADD(FMUL(log2r,log2r),FINIT(1,0)));

    FLOAT_T new_b;

    //update a
    //model->a += gain * error * log2r;
    model->a = FADD(model->a,FMUL(FMUL(gain,error),log2r));
    //model->a = double_min_max(-4.0, -2.0, model->a);
    model->a = FMINMAX(FINIT(-4,0), FINIT(-2,0), model->a);

    //update b
    //new_b    = qp - model->a * log2r;
    new_b    = FSUB(FINIT(qp,0),FMUL(model->a,log2r));

    error = FSUB(new_b, model->b);
    model->b = FADD( model->b , FMUL(FINIT(1, -weight_factor), error)); //FMUL(FINIT(1,-4),FADD(model->b,new_b));
    
    //model->b = 0.5 * (model->b + new_b); //0.5 for smoothing
   //model->b = FMUL(FINIT(1,-1),FADD(model->b,new_b));
}

static void init_rc_buf(
    rc_pic_t *rc,
    int xmit_bps,
    int buf_size_ms)
{
    rc->buf_size = (xmit_bps / 1000) * buf_size_ms;

    if (rc->mode == CBR)
        rc->init_level = rc->buf_size / 4;
    else //VBR or ABR
        rc->init_level = 0;

#ifdef PARA_CHANGE_RC_SEQ_INIT
    if(!rc->para_changed)
        rc->buf_level = rc->init_level;
#else
    rc->buf_level = rc->init_level;
#endif
}

static void update_rc_buf(
    rc_pic_t *rc,
    int pic_bit)
{
    rc->buf_level += pic_bit;
    rc->buf_level -= rc->xmit_pic_bit;

    if (rc->mode == CBR || rc->mode == VBR)
        rc->buf_level = fw_util_min_max(0, rc->buf_size, rc->buf_level);
    else //ABR
        rc->buf_level = fw_util_min_max(-(1<<30), 1<<30, rc->buf_level);
}

static int alloc_pic_bit(
    rc_pic_t *rc)
{
    FLOAT_T weight;
    int pic_bit;

    

    //weight=1 when buf_level=init_level
    //weight=0 when buf_level=buf_size
    //weight = (double)(rc->buf_level - rc->buf_size) / (rc->init_level - rc->buf_size);

    if(INT32_MIN + rc->buf_size >= rc->buf_level)
        weight = FDIV(FINIT(INT32_MIN,0),FINIT(rc->init_level - rc->buf_size,0));
    else
        weight = FADD(FINIT(1,0),FMUL(FDIV(FINIT(rc->buf_level - rc->init_level,0),FINIT(rc->buf_size - rc->init_level,0)), FSUB(FINIT(rc->gamma,-8), FINIT(1,0))));

    //weight = double_min_max(0.1, 2.0, weight);
    weight = FMINMAX(FINIT(26,-8), FINIT(2,0), weight);
    //pic_bit = (int)(rc->curr_model->bpp * weight * rc->num_pixel_in_pic + 0.5);

     pic_bit = FINT(FMUL(FMUL(rc->curr_model->bpp,weight),FINIT(rc->num_pixel_in_pic,0)));
/*
    //to prevent overflow
    if (rc->mode == CBR || rc->mode == VBR)
        pic_bit = fw_util_min(pic_bit, rc->buf_size - rc->buf_level);

    //to prevent underflow
    if (rc->mode == CBR) //VBR never generate underflow
        pic_bit = fw_util_max(pic_bit, rc->xmit_pic_bit - rc->buf_level);
*/
    pic_bit = fw_util_max(pic_bit, 1);

    return pic_bit; 
}

static int calc_pic_qp(
    rc_pic_t *rc,
    int pic_bit)
{
    //double bpp = (double)pic_bit / rc->num_pixel_in_pic;
    FLOAT_T bpp = FDIV(FINIT(pic_bit,0),FINIT(rc->num_pixel_in_pic,0));
    FLOAT_T qp = calc_qp_from_bpp(rc->curr_model, bpp);
    //int int_qp = (int)(qp + 0.5);
    int int_qp = FINT(qp);
    int_qp = fw_util_min_max(rc->min_qp, rc->max_qp, int_qp);

    return int_qp;
}


//************** ROI **************************************//
static void roi_pic_init(
    //IN
    int roi_dqp,
    unsigned char *roi_qp_map, //mem size = ctu_width * ctu_height (bytes)
    int mb_width,
    int mb_height,
    int pic_qp,
    int min_qp,
    int max_qp,    
    //OUT    
    uint32_t* roi_base_qp)
{
    int ctu_addr;
    int level;
    int num_mb_in_pic = mb_width * mb_height;
    int num_mb_in_level[MAX_ROI_LEVEL+1];
    int qp0; //non-roi QP
    int sum = 0;

    //init roi level
    for (level=0; level<=MAX_ROI_LEVEL; level++)
        num_mb_in_level[level] = 0;

    //count num_ctu of each level
    for (ctu_addr=0; ctu_addr<num_mb_in_pic; ctu_addr++)
    {
        int roi_level = roi_qp_map[ctu_addr];
        assert(roi_level>=0 && roi_level<=MAX_ROI_LEVEL);
        num_mb_in_level[roi_level]++;
    }

    //pic_qp = avg(qp[i])
    //       = qp0 - avg(qp0 - qp[i])
    //       = qp0 - avg(dqp[i])
    for (level=1; level<=MAX_ROI_LEVEL; level++)
        sum += num_mb_in_level[level] * roi_dqp * level;
    qp0 = pic_qp + (sum + (num_mb_in_pic>>1)) / num_mb_in_pic;

    *roi_base_qp = fw_util_min_max(min_qp, max_qp, qp0);
}

static void roi_gen_qp_map(uint32_t mb_num_x,  uint32_t mb_num_y, uint8_t *roi_map, uint8_t average_qp,int roi_dqp )
{
    uint32_t map_size      = mb_num_x * mb_num_y;
    uint32_t i;

    for(i = 0 ; i < map_size ; i++)
        roi_map[i] = MAX(0, MIN(51, (roi_map[i]*roi_dqp) + average_qp));
}

#define MB_SIZE 16
#define LOG2_MB_SIZE 4

void rc_init(rc_pic_t *rc)
{
    int i;

    rc->frame_num = 0;
    rc->para_changed = 0;
#ifdef CLIP_PIC_DELTA_QP
    rc->last_anchor_qp = -1;
#endif
    rc->enc_order_in_gop = 0;
 
    rc->intra_model.a = FINIT(-845, -8);//-3.3;
    rc->intra_model.b = FINIT(7373, -8);//28.8    

    for (i=0; i<MAX_GOP_SIZE; i++)
    {
        rc->inter_model[i].a = FINIT(-512, -8);//-2.0
        rc->inter_model[i].b = FINIT(6195, -8);//24.2
    }


}


void rc_seq_init(
    //IN
    rc_pic_t *rc,
    int enc_bps,
    int xmit_bps,
    int buf_size_ms,
    int bit_alloc_mode, //0(adaptive), 1(fixed bit ratio)
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
    int para_change,
    int weight_factor,
    int gamma,
#ifdef CLIP_PIC_DELTA_QP
    int max_delta_qp_minus,
    int max_delta_qp_plus,
#endif
    //OUT
    int *rc_buf_size)
{
    rc->para_changed = para_change;
    rc->frame_num = 0;
    rc->num_pixel_in_pic = pic_width * pic_height;
    rc->gop_size = gop_size;
    rc->intra_period = intra_period;
    rc->bit_alloc_mode = bit_alloc_mode;
    rc->mb_width  = (pic_width  + MB_SIZE - 1) >> LOG2_MB_SIZE;
    rc->mb_height = (pic_height + MB_SIZE - 1) >> LOG2_MB_SIZE;
#ifdef CLIP_PIC_DELTA_QP
    rc->max_delta_qp_minus = max_delta_qp_minus;
    rc->max_delta_qp_plus = max_delta_qp_plus;
#endif

        rc->mode = CBR;

    if (rc->mode != VBR) //ABR or CBR
        xmit_bps = enc_bps;

    rc->avg_pic_bit  = enc_bps / frame_rate;
    rc->xmit_pic_bit = xmit_bps / frame_rate;
    //rc->avg_bpp = (double)rc->avg_pic_bit / rc->num_pixel_in_pic;
    rc->gamma = gamma;
    rc->weight_factor = weight_factor;
    rc->avg_bpp = FDIV(FINIT(rc->avg_pic_bit,0),FINIT(rc->num_pixel_in_pic,0));

    rc->initial_qp = rc_initial_qp;

    if (bit_alloc_mode == BIT_ALLOC_FIXED_RATIO)
    {
        int i;

        for (i=0; i<gop_size; i++)
            rc->fixed_bit_ratio[i] = fixed_bit_ratio[i];
    }

    init_rc_buf(rc, xmit_bps, buf_size_ms);

    init_rq_model(rc, intra_qp, gop);

    *rc_buf_size = rc->buf_size;
}


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
    int *hrd_buf_size)
{
    int is_anchor;
    int pic_bit;

    rc->min_qp = min_qp;
    rc->max_qp = max_qp;

    if (is_intra)
        rc->curr_model = &rc->intra_model;
    else
        rc->curr_model = &rc->inter_model[enc_order_in_gop];

    is_anchor = is_intra || (rc->curr_model == rc->anchor_model);

    //init gop
    if (is_anchor || rc->para_changed)
    {
        alloc_gop_bit(rc);
        rc->para_changed = 0;
    }

    pic_bit = alloc_pic_bit(rc);

    *pic_qp = calc_pic_qp(rc, pic_bit);
    if (rc->initial_qp >= 0)
    {
        update_init_beta(rc, rc->initial_qp - *pic_qp);
        *pic_qp = rc->initial_qp;
        rc->initial_qp = -1;
    }

#ifdef CLIP_PIC_DELTA_QP
    if(rc->last_anchor_qp != -1)
    {
        int max_qp = CLIP3(0, 51, rc->last_anchor_qp + rc->curr_model->dqp + rc->max_delta_qp_plus);
        int min_qp = CLIP3(0, 51, rc->last_anchor_qp + rc->curr_model->dqp - rc->max_delta_qp_minus);

        *pic_qp = CLIP3(min_qp, max_qp, *pic_qp);

        *max_delta_qp_plus = max_qp - *pic_qp;
        *max_delta_qp_minus = *pic_qp - min_qp;

    }
    else
    {
        int max_qp = CLIP3(0, 51, rc->last_anchor_qp + rc->curr_model->dqp + 51);
        int min_qp = CLIP3(0, 51, rc->last_anchor_qp + rc->curr_model->dqp - 51);

        *pic_qp = CLIP3(min_qp, max_qp, *pic_qp);

        *max_delta_qp_plus = max_qp - *pic_qp;
        *max_delta_qp_minus = *pic_qp - min_qp;


    }
    
#endif
    
    *pic_target_bit = pic_bit;
    *hrd_buf_level = rc->buf_level;
    *hrd_buf_size = rc->buf_size;

}

void rc_pic_end(
    //IN
    rc_pic_t *rc,
    int avg_qp,
    int real_pic_bit,
    int frm_skip_flag)
{
    //double bpp = (double)real_pic_bit / rc->num_pixel_in_pic;
    FLOAT_T bpp = FDIV(FINIT(real_pic_bit,0),FINIT(rc->num_pixel_in_pic,0));

    if (frm_skip_flag == 0)
    {
        update_rq_model(rc->curr_model, bpp, avg_qp, rc->weight_factor);

#ifdef CLIP_PIC_DELTA_QP
        rc->last_anchor_qp = avg_qp - rc->curr_model->dqp;
#endif
    }

    update_rc_buf(rc, real_pic_bit);

    rc->frame_num++;
}

