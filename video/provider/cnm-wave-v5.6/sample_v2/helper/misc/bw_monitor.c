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

#include "vpuapifunc.h"
#include "main_helper.h"
#include "bw_monitor.h"
#include "debug.h"
#include <stdio.h>

#define MBYTE                               (1024*1024)

static char picTypeString[10][4] = {"I", "P", "B", "", "", "IDR", "", "", "", ""};//picType enum in vpuapi.h

/*******************************************************************************
 * WAVE6 Bandwidth monitor registers                                           *
 *******************************************************************************/
#define W6_BW_MON_VCORE0_BASE                   (0x6300)
#define W6_BW_MON_VCORE0_COMMAND                (W6_BW_MON_VCORE0_BASE + 0x00)
#define W6_BW_MON_VCORE1_BASE                   (0xA300)
#define W6_BW_MON_VCORE1_COMMAND                (W6_BW_MON_VCORE1_BASE + 0x00)
#define W6_BW_MON_VCPU_BASE                     (0x0140)
#define W6_BW_MON_VCPU_COMMAND                  (W6_BW_MON_VCPU_BASE + 0x00)

#define W6_BW_MON_OFFSET_COMMAND   0x00
#define W6_BW_MON_OFFSET_MODE      0x04
#define W6_BW_MON_OFFSET_PRI_R     0x20
#define W6_BW_MON_OFFSET_SEC_R     0x24
#define W6_BW_MON_OFFSET_PRI_W     0x28
#define W6_BW_MON_OFFSET_SEC_W     0x2C
#define W6_BW_MON_OFFSET_INFO_CH0  0x30

#define WAVE6_MAX_BW_DATA_CH_SIZE 29
#define WAVE6_CORE_COUNT 3 //vcore0, vcore1, vcpu

typedef void*   BWData;

typedef struct _tag_bw_data_struct {
    Uint32     vcpu_bw_rd;
    Uint32     vcpu_bw_wr;
    Uint32     sdma_bw_rd;
    Uint32     sdma_bw_wr;
    Uint32     vcpu_dma_bw_wr;
    Uint32     vcpu_dma_bw_rd;
    Uint32     vcpu_nbdma_bw_wr;
    Uint32     vcpu_nbdma_bw_rd;
    Uint32     prescan_wr;
    Uint32     prescan_rd;

    Uint32     arid_pri_total;
    Uint32     awid_pri_total;
    Uint32     arid_sec_total;
    Uint32     awid_sec_total;
    Uint32     arid_mclk_total;
    Uint32     awid_mclk_total;
    Uint32     total_write;
    Uint32     total_read;
    Uint32     total_bandwidth;
    Uint32     subblock_bw_pri_rd[16];
    Uint32     subblock_bw_pri_wr[16];
    Uint32     subblock_bw_sec_rd[16];
    Uint32     subblock_bw_sec_wr[16];
    Uint32     subblock_bw_mclk_rd[16];
    Uint32     subblock_bw_mclk_wr[16];
} BWGdiData;

typedef struct _tag_BWMonitorops {
    BWCtx   (*allocate)(Uint32);
    void    (*release)(BWCtx);
    void    (*reset)(BWCtx);
    BWData  (*get_data)(BWCtx, Uint32 numCores, Int32 recon_idx);
    void    (*print)(BWCtx, BWData, Uint32, Int32 recon_idx, Uint32 w6_frame_num);
} BWMonitorOps_t;

#define BW_CONTEXT_COMMON_VARS            \
    BWMonitorOps_t* ops;                  \
    Uint32          coreIndex;            \
    Uint32          instanceIndex;        \
    Uint32          coreCount;            \
    Uint32          productId;            \
    CodecInst*      instance;             \
    Uint32          numFrames;            \
    Uint64          totalPriRead;         \
    Uint64          totalPriWrite;        \
    Uint64          totalSecRead;         \
    Uint64          totalSecWrite;        \
    Uint64          totalProcRead;        \
    Uint64          totalProcWrite;       \
    Uint64          total;                \
    BOOL            enableReportPerFrame; \
    Uint32          bwMode;               \
    osal_file_t     fpBWTotal[WAVE6_CORE_COUNT]; \
    BWData*         data;                 \
    char strLogDir[256];

typedef struct _tag_bw_common_context_struct {
    BW_CONTEXT_COMMON_VARS
} BWCommonCtx;



/************************************************************************/
/* WAVE5 BACKBONE INTERFACE                                             */
/************************************************************************/
#define BWB_BACKBONE_DATA_SIZE                   (17) // total + number of burst length sizes (0~15)

typedef struct _tag_wave_bw_context_struct {
    BW_CONTEXT_COMMON_VARS

    Uint32 arid_prp_total;
    Uint32 awid_prp_total;
    Uint32 arid_fbd_y_total;
    Uint32 awid_fbc_y_total;
    Uint32 arid_fbd_c_total;
    Uint32 awid_fbc_c_total;
    Uint32 arid_pri_total;
    Uint32 awid_pri_total;
    Uint32 arid_sec_total;
    Uint32 awid_sec_total;
    Uint32 arid_proc_total;
    Uint32 awid_proc_total;
    Uint32 awid_bwb_total;
} BackBoneBwCtx;

typedef struct _tag_wave_bw_data_struct {
    Uint32 arid_prp[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_prp[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_fbd_y[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_fbc_y[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_fbd_c[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_fbc_c[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_pri[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_pri[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_sec[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_sec[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_proc[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_proc[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_bwb[BWB_BACKBONE_DATA_SIZE];
} BWBackboneData;

static BWCtx
wave5_bw_monitor_allocate(
    Uint32   coreIndex
    )
{
    BackBoneBwCtx* context;

    context = (BackBoneBwCtx*)osal_malloc(sizeof(BackBoneBwCtx));
    osal_memset((void*)context, 0x00, sizeof(BackBoneBwCtx));

    return context;
}

static void
wave5_bw_monitor_release(
    BWCtx    context
    )
{
    BackBoneBwCtx*  ctx = (BackBoneBwCtx*)context;
    Uint64          avgPriRead, avgPriWrite;
    Uint64          avgSecRead, avgSecWrite;
    Uint32          avgFBCWrite;
    Uint32          avgFBDRead;
    Uint64          avgProcRead, avgProcWrite;
    Uint64          avgPRPRead, avgPRPWrite;
    Uint64          avgBWBWrite;
    Uint64          avgWrite;
    Uint64          avgRead;
    Uint64          avg;

    if (ctx == NULL)
        return;
    
    if (0 == ctx->numFrames) {
        VLOG(ERR, "%s:%d divisor must be a integer(not zero)\n", __FUNCTION__, __LINE__);
        return;
    }

    avgPriRead   = ctx->arid_pri_total / ctx->numFrames;
    avgPriWrite  = ctx->awid_pri_total / ctx->numFrames;
    avgSecRead   = ctx->arid_sec_total / ctx->numFrames;
    avgSecWrite  = ctx->awid_sec_total / ctx->numFrames;
    avgProcRead  = ctx->arid_proc_total / ctx->numFrames;
    avgProcWrite = ctx->awid_proc_total / ctx->numFrames;
    avgFBCWrite  = (ctx->awid_fbc_y_total+ctx->awid_fbc_c_total) / ctx->numFrames;
    avgFBDRead   = (ctx->arid_fbd_y_total+ctx->arid_fbd_c_total) / ctx->numFrames;
    avgPRPRead   = ctx->arid_prp_total / ctx->numFrames;
    avgPRPWrite  = ctx->awid_prp_total / ctx->numFrames;
    avgBWBWrite  = ctx->awid_bwb_total / ctx->numFrames;
    avgWrite     = (ctx->awid_pri_total+ctx->awid_sec_total+ctx->awid_proc_total+ctx->awid_fbc_y_total+ctx->awid_fbc_c_total+ctx->awid_prp_total+ctx->awid_bwb_total) / ctx->numFrames;
    avgRead      = (ctx->arid_pri_total+ctx->arid_sec_total+ctx->arid_proc_total+ctx->arid_fbd_y_total+ctx->arid_fbd_c_total+ctx->arid_prp_total) / ctx->numFrames;
    avg          = ctx->total / ctx->numFrames;

    if (!ctx->bwMode) {
        osal_fprintf(ctx->fpBWTotal[0], "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");

        osal_fprintf(ctx->fpBWTotal[0], "AVER.   %10d %10d %10d %10d %10d %10d %10d  %10d %10d %10d %10d %10d %10d  %10d\n",
            avgPriWrite, avgSecWrite, avgFBCWrite, avgPRPWrite, avgProcWrite, avgBWBWrite, avgWrite,
            avgPriRead,  avgSecRead,  avgFBDRead,  avgPRPRead,  avgProcRead,               avgRead,
            avg);
    }
    osal_free(context);
}

static void
wave5_bw_monitor_reset(
    BWCtx    context
    )
{
    BackBoneBwCtx* ctx = (BackBoneBwCtx*)context;

    if (ctx == NULL)
        return;
}

static void
    wave5_dec_bw_monitor_print(
    BWCtx   context,
    BWData  data,
    Uint32  picType
    )
{
    BWBackboneData* bdata = (BWBackboneData*)data;
    BWCommonCtx*    common = (BWCommonCtx*)context;
    Uint32          total_wr_bw;
    Uint32          total_rd_bw;
    Uint32          idx = 0;
    Uint32          loopCnt = (common->bwMode == 0) ? 1 : BWB_BACKBONE_DATA_SIZE;

    for (idx =0; idx < loopCnt; idx++) {
        total_wr_bw = bdata->awid_pri[idx] + bdata->awid_sec[idx] + bdata->awid_fbc_y[idx] + bdata->awid_fbc_c[idx] + bdata->awid_proc[idx] + bdata->awid_bwb[idx];
        total_rd_bw = bdata->arid_pri[idx] + bdata->arid_sec[idx] + bdata->arid_fbd_y[idx] + bdata->arid_fbd_c[idx] + bdata->arid_proc[idx];
        if (0 == idx) {
            if (common->numFrames == 1) {
                osal_fprintf(common->fpBWTotal[0], "  No.                                    WRITE(B)                                                READ(B)                            TOTAL\n");
                osal_fprintf(common->fpBWTotal[0], "-------------------------------------------------------------------------  -----------------------------------------------------  -----------\n");
                osal_fprintf(common->fpBWTotal[0], "              PRI        SEC        FBC       PROC        BWB       TOTAL        PRI       SEC        FBD       PROC      TOTAL\n");
                if ( common->bwMode == 0 )
                    osal_fprintf(common->fpBWTotal[0], "------------------------------------------------------------------------------------  -----------------------------------------------------  -----------\n");
            }
            if ( common->bwMode != 0 )
                osal_fprintf(common->fpBWTotal[0], "====================================================================================  =====================================================  ===========\n");
            osal_fprintf(common->fpBWTotal[0], "%5d %s %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d  %10d\n", common->numFrames-1, picTypeString[picType],
                bdata->awid_pri[idx], bdata->awid_sec[idx], (bdata->awid_fbc_y[idx]+bdata->awid_fbc_c[idx]), bdata->awid_proc[idx], bdata->awid_bwb[idx], total_wr_bw,
                bdata->arid_pri[idx], bdata->arid_sec[idx], (bdata->arid_fbd_y[idx]+bdata->arid_fbd_c[idx]), bdata->arid_proc[idx], total_rd_bw,
                (total_wr_bw + total_rd_bw));

        } else {
            if (1 == idx) {
                osal_fprintf(common->fpBWTotal[0], "------------------------------------------------------------------------------------  -----------------------------------------------------  -----------\n");
            }
            osal_fprintf(common->fpBWTotal[0], "%5d %s %10d %10d %10d %10d %10d %10d  %10d %10d %10d %10d %10d  %10d\n", idx-1, picTypeString[picType],
                bdata->awid_pri[idx], bdata->awid_sec[idx], (bdata->awid_fbc_y[idx]+bdata->awid_fbc_c[idx]), bdata->awid_proc[idx], bdata->awid_bwb[idx], total_wr_bw,
                bdata->arid_pri[idx], bdata->arid_sec[idx], (bdata->arid_fbd_y[idx]+bdata->arid_fbd_c[idx]), bdata->arid_proc[idx], total_rd_bw,
                (total_wr_bw + total_rd_bw));
        }
    }
    if ( common->bwMode != 0 )
        osal_fprintf(common->fpBWTotal[0], "------------------------------------------------------------------------------------  -----------------------------------------------------  -----------\n");
    osal_fflush(common->fpBWTotal[0]);
}

static void
    wave5_enc_bw_monitor_print(
    BWCtx   context,
    BWData  data,
    Uint32  picType
    )
{
    BWBackboneData* bdata = (BWBackboneData*)data;
    BWCommonCtx*    common = (BWCommonCtx*)context;
    Uint32          total_wr_bw;
    Uint32          total_rd_bw;
    Uint32          idx = 0;
    Uint32          loopCnt = (common->bwMode == 0) ? 1 : BWB_BACKBONE_DATA_SIZE;

    for (idx =0; idx < loopCnt; idx++) {
        total_wr_bw = bdata->awid_pri[idx] + bdata->awid_sec[idx] + bdata->awid_fbc_y[idx] + bdata->awid_fbc_c[idx] + bdata->awid_prp[idx] + bdata->awid_proc[idx] + bdata->awid_bwb[idx];
        total_rd_bw = bdata->arid_pri[idx] + bdata->arid_sec[idx] + bdata->arid_fbd_y[idx] + bdata->arid_fbd_c[idx] + bdata->arid_prp[idx] + bdata->arid_proc[idx];
        if (0 == idx) {
            if (common->numFrames == 1) {
                osal_fprintf(common->fpBWTotal[0], "  No.                                    WRITE(B)                                                            READ(B)                                        TOTAL\n");
                osal_fprintf(common->fpBWTotal[0], "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");
                osal_fprintf(common->fpBWTotal[0], "              PRI        SEC        FBC        PRP       PROC        BWB       TOTAL        PRI       SEC        FBD         PRP       PROC      TOTAL\n");
                if ( common->bwMode == 0 )
                    osal_fprintf(common->fpBWTotal[0], "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");
            }
            if ( common->bwMode != 0 )
                osal_fprintf(common->fpBWTotal[0], "====================================================================================  =================================================================  ===========\n");
            osal_fprintf(common->fpBWTotal[0], "%5d %s %10d %10d %10d %10d %10d %10d %10d  %10d %10d %10d %10d %10d %10d  %10d\n", common->numFrames-1, picTypeString[picType],
                bdata->awid_pri[idx], bdata->awid_sec[idx], (bdata->awid_fbc_y[idx]+bdata->awid_fbc_c[idx]), bdata->awid_prp[idx], bdata->awid_proc[idx], bdata->awid_bwb[idx], total_wr_bw,
                bdata->arid_pri[idx], bdata->arid_sec[idx], (bdata->arid_fbd_y[idx]+bdata->arid_fbd_c[idx]), bdata->arid_prp[idx], bdata->arid_proc[idx],                  total_rd_bw,
                (total_wr_bw + total_rd_bw));
        } else {
            if (1 == idx) {
                osal_fprintf(common->fpBWTotal[0], "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");
            }
            osal_fprintf(common->fpBWTotal[0], "%5d %s %10d %10d %10d %10d %10d %10d %10d  %10d %10d %10d %10d %10d %10d  %10d\n", idx-1, picTypeString[picType],
                bdata->awid_pri[idx], bdata->awid_sec[idx], (bdata->awid_fbc_y[idx]+bdata->awid_fbc_c[idx]), bdata->awid_prp[idx], bdata->awid_proc[idx], bdata->awid_bwb[idx], total_wr_bw,
                bdata->arid_pri[idx], bdata->arid_sec[idx], (bdata->arid_fbd_y[idx]+bdata->arid_fbd_c[idx]), bdata->arid_prp[idx], bdata->arid_proc[idx],                  total_rd_bw,
                (total_wr_bw + total_rd_bw));
        }
    }
    if ( common->bwMode != 0 )
        osal_fprintf(common->fpBWTotal[0], "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");
    osal_fflush(common->fpBWTotal[0]);
}

static void
wave5_bw_monitor_print(
    BWCtx   context,
    BWData  data,
    Uint32  picType,
    Int32   recon_idx,
    Uint32  w6_frame_num
    )
{
    BWCommonCtx*    common = (BWCommonCtx*)context;

    if (common->instance->isDecoder == TRUE) {
        wave5_dec_bw_monitor_print(context, data, picType);
    } else {
        wave5_enc_bw_monitor_print(context, data, picType);
    }
}

static BWData
wave5_bw_monitor_get_data(
    BWCtx    context,
    Uint32   numCores,
    Int32    recon_idx
    )
{
    BackBoneBwCtx*  ctx   = (BackBoneBwCtx*)context;
    BWBackboneData* idata = NULL;
    VPUBWData       bwdata = {0, };
    RetCode         ret = RETCODE_FAILURE;
    Uint32          prevTotal;
    Uint32          idx = 0;
    Uint32          loopCnt = (ctx->bwMode == 0) ? 1 : BWB_BACKBONE_DATA_SIZE;

    idata = (BWBackboneData*)osal_malloc(sizeof(BWBackboneData));

    for (idx = 0; idx < loopCnt; idx++) {
        if (0 < idx) {
            bwdata.bwMode = 1;
            bwdata.burstLengthIdx = idx - 1;
        }
        if (ctx->instance->isDecoder == TRUE) {
            ret = VPU_DecGiveCommand(ctx->instance, GET_BANDWIDTH_REPORT, (void*)&bwdata);
        }
        else {
            ret = VPU_EncGiveCommand(ctx->instance, GET_BANDWIDTH_REPORT, (void*)&bwdata);
        }
        if (ret != RETCODE_SUCCESS) {
            if (NULL != idata) {
                osal_free(idata);
            }
            VLOG(ERR, "%s:%d Failed to VPU_EncGiveCommand(ENC_GET_BW_REPORT), ret(%d)\n", __FUNCTION__, __LINE__, ret);
            return NULL;
        }

        if (ctx->instance->isDecoder == TRUE) {
            bwdata.prpBwRead = 0;
            bwdata.prpBwWrite = 0;
        }

        idata->arid_prp[idx]   = bwdata.prpBwRead;
        idata->awid_prp[idx]   = bwdata.prpBwWrite;
        idata->arid_fbd_y[idx] = bwdata.fbdYRead;
        idata->awid_fbc_y[idx] = bwdata.fbcYWrite;
        idata->arid_fbd_c[idx] = bwdata.fbdCRead;
        idata->awid_fbc_c[idx] = bwdata.fbcCWrite;
        idata->arid_pri[idx]   = bwdata.priBwRead;
        idata->awid_pri[idx]   = bwdata.priBwWrite;
        idata->arid_sec[idx]   = bwdata.secBwRead;
        idata->awid_sec[idx]   = bwdata.secBwWrite;
        idata->arid_proc[idx]  = bwdata.procBwRead;
        idata->awid_proc[idx]  = bwdata.procBwWrite;
        idata->awid_bwb[idx]   = bwdata.bwbBwWrite;

        if (0 == idx) {
            prevTotal   = ctx->total;
            ctx->total += idata->arid_prp[idx];
            ctx->total += idata->awid_prp[idx];
            ctx->total += idata->arid_fbd_y[idx];
            ctx->total += idata->awid_fbc_y[idx];
            ctx->total += idata->arid_fbd_c[idx];
            ctx->total += idata->awid_fbc_c[idx];
            ctx->total += idata->arid_pri[idx];
            ctx->total += idata->awid_pri[idx];
            ctx->total += idata->arid_sec[idx];
            ctx->total += idata->awid_sec[idx];
            ctx->total += idata->arid_proc[idx];
            ctx->total += idata->awid_proc[idx];
            ctx->total += idata->awid_bwb[idx];

            if (prevTotal == ctx->total) {
                // VPU didn't decode a frame.
                return NULL;
            }
            ctx->arid_prp_total   += idata->arid_prp[idx]  ;
            ctx->awid_prp_total   += idata->awid_prp[idx]  ;
            ctx->arid_fbd_y_total += idata->arid_fbd_y[idx];
            ctx->arid_fbd_c_total += idata->arid_fbd_c[idx];
            ctx->awid_fbc_y_total += idata->awid_fbc_y[idx];
            ctx->awid_fbc_c_total += idata->awid_fbc_c[idx];
            ctx->arid_pri_total   += idata->arid_pri[idx]  ;
            ctx->awid_pri_total   += idata->awid_pri[idx]  ;
            ctx->arid_sec_total   += idata->arid_sec[idx]  ;
            ctx->awid_sec_total   += idata->awid_sec[idx]  ;
            ctx->arid_proc_total  += idata->arid_proc[idx] ;
            ctx->awid_proc_total  += idata->awid_proc[idx] ;
            ctx->awid_bwb_total   += idata->awid_bwb[idx]  ;
        }
    }

    return (BWData)idata;
}

static BWMonitorOps_t s_wave_backbone_ops = {
    wave5_bw_monitor_allocate,
    wave5_bw_monitor_release,
    wave5_bw_monitor_reset,
    wave5_bw_monitor_get_data,
    wave5_bw_monitor_print
};

/************************************************************************/
/* WAVE6 BANDWIDTH INTERFACE                                             */
/************************************************************************/
typedef struct _wave6_bw_data {
    Uint32 isExist[WAVE6_CORE_COUNT];
    Uint32 rdata[WAVE6_CORE_COUNT][BWB_BACKBONE_DATA_SIZE][WAVE6_MAX_BW_DATA_CH_SIZE];
    Uint32 wdata[WAVE6_CORE_COUNT][BWB_BACKBONE_DATA_SIZE][WAVE6_MAX_BW_DATA_CH_SIZE];
} wave6_bw_data;

typedef struct _wave6_bw_ctx {
    BW_CONTEXT_COMMON_VARS
    Uint32 w6_frame_num;

    //all data(header only encoding + b frame delay + encoding
    Uint32 total_read_ch[WAVE6_CORE_COUNT][WAVE6_MAX_BW_DATA_CH_SIZE];
    Uint32 total_write_ch[WAVE6_CORE_COUNT][WAVE6_MAX_BW_DATA_CH_SIZE];

    Uint32 total_encoded[WAVE6_CORE_COUNT];
    Uint32 total_encoded_pri_read[WAVE6_CORE_COUNT];
    Uint32 total_encoded_pri_write[WAVE6_CORE_COUNT];
    Uint32 total_encoded_sec_read[WAVE6_CORE_COUNT];
    Uint32 total_encoded_sec_write[WAVE6_CORE_COUNT];

    Uint32 total_encoded_read_ch[WAVE6_CORE_COUNT][WAVE6_MAX_BW_DATA_CH_SIZE];
    Uint32 total_encoded_write_ch[WAVE6_CORE_COUNT][WAVE6_MAX_BW_DATA_CH_SIZE];
} wave6_bw_ctx;

static BWCtx
    wave6_bw_monitor_allocate(
    Uint32   coreIndex
    )
{
    wave6_bw_ctx* context;

    context = (wave6_bw_ctx*)osal_malloc(sizeof(wave6_bw_ctx));
    osal_memset((void*)context, 0x00, sizeof(wave6_bw_ctx));

    return context;
}

static void
    wave6_bw_monitor_release(
    BWCtx    context
    )
{
    wave6_bw_ctx*   ctx = (wave6_bw_ctx*)context;
    Uint64          avg_read[WAVE6_MAX_BW_DATA_CH_SIZE];
    Uint64          avg_write[WAVE6_MAX_BW_DATA_CH_SIZE];
    Uint64          avg;
    Uint32          i, j;

    if (ctx == NULL)
        return;

    if (0 == ctx->w6_frame_num) {
        VLOG(WARN, "%s:%d divisor must be a integer(not zero)\n", __FUNCTION__, __LINE__);
        return;
    }

    for (i = 0; i < WAVE6_CORE_COUNT; i++) {
        for (j = 0; j < WAVE6_MAX_BW_DATA_CH_SIZE; j++) {
            avg_read[j] = ctx->total_encoded_read_ch[i][j]  / ctx->w6_frame_num;
            avg_write[j] = ctx->total_encoded_write_ch[i][j]  / ctx->w6_frame_num;
        }
        ctx->total_encoded[i] = ctx->total_encoded_pri_read[i] + ctx->total_encoded_pri_write[i] + ctx->total_encoded_sec_read[i] + ctx->total_encoded_sec_write[i];
        avg = ctx->total_encoded[i] / ctx->w6_frame_num;

        osal_fprintf(ctx->fpBWTotal[i], "ENC AVER ");
        for (j = 0; j < WAVE6_MAX_BW_DATA_CH_SIZE; j++)
            osal_fprintf(ctx->fpBWTotal[i], "%10d ", avg_write[j]);
        osal_fprintf(ctx->fpBWTotal[i], " %10d  ", (ctx->total_encoded_pri_write[i] + ctx->total_encoded_sec_write[i]) / ctx->w6_frame_num);
        for (j = 0; j < WAVE6_MAX_BW_DATA_CH_SIZE; j++)
            osal_fprintf(ctx->fpBWTotal[i], "%10d ", avg_read[j]);
        osal_fprintf(ctx->fpBWTotal[i], " %10d  ", (ctx->total_encoded_pri_read[i] + ctx->total_encoded_sec_read[i]) / ctx->w6_frame_num);
        osal_fprintf(ctx->fpBWTotal[i], "%10d\n", avg);
        osal_fprintf(ctx->fpBWTotal[i], "\n");
    }
    osal_free(context);
}

static void
    wave6_bw_monitor_reset(
    BWCtx    context
    )
{
    BackBoneBwCtx* ctx = (BackBoneBwCtx*)context;
    if (ctx == NULL)
        return;

    SetClockGate(ctx->coreIndex, TRUE);
    // [2] : que_full_cnt_en
    // [1] : monitor_enable
    // [0] : monitor_reset
    vdi_fio_write_register(ctx->coreIndex, W6_BW_MON_VCORE0_COMMAND, 0x3);
    vdi_fio_write_register(ctx->coreIndex, W6_BW_MON_VCORE1_COMMAND, 0x3);
    vdi_fio_write_register(ctx->coreIndex, W6_BW_MON_VCPU_COMMAND, 0x3);
    SetClockGate(ctx->coreIndex, FALSE);
}

static void
    _wave6_bw_monitor_print(
    BWCtx   context,
    Uint32  isDecoder,
    BWData  data,
    Uint32  picType,
    Int32   recon_idx,
    Int32   w6_frame_num
    )
{
    wave6_bw_data*     bw_data = (wave6_bw_data*)data;
    wave6_bw_ctx*      ctx = (wave6_bw_ctx*)context;
    Uint32             i = 0, j = 0, k = 0;
    Uint32             loopCnt = (ctx->bwMode == 0) ? 1 : BWB_BACKBONE_DATA_SIZE;
    Uint32             total_write;
    Uint32             total_read;
    char               pic_type_string[10];
    char               codec_string[40];
    char               core_string[40];

    if (isDecoder == TRUE) {
        sprintf(codec_string, "codec=decoder\n");
        osal_memcpy(pic_type_string, picTypeString[picType], sizeof(pic_type_string));
    }
    else {
        sprintf(codec_string, "codec=encoder\n");
        if (recon_idx >= 0)
            osal_memcpy(pic_type_string, picTypeString[picType], sizeof(pic_type_string));
        else {
            osal_snprintf(pic_type_string, sizeof(pic_type_string), "%d", recon_idx);
        }
    }
    for (i = 0; i < WAVE6_CORE_COUNT; i++) { //0:vCore0, 1:vCore1, 2:vCPU
        if (i == 0) {
            sprintf(core_string, "core=vCore0\n");
        }
        else if (i == 1) {
            sprintf(core_string, "core=vCore1\n");
        }
        else {
            sprintf(core_string, "core=vCPU\n");
        }
        for (j = 0; j < loopCnt; j++) {
            if (0 == j) {
                if (ctx->numFrames == 1) {
                    osal_fprintf(ctx->fpBWTotal[i], codec_string);
                    osal_fprintf(ctx->fpBWTotal[i], core_string);
                    osal_fprintf(ctx->fpBWTotal[i], "bwMode=%d\n", ctx->bwMode);
                    osal_fprintf(ctx->fpBWTotal[i], "COMMAND_QUEUE=%d\n", COMMAND_QUEUE_DEPTH);
                    osal_fprintf(ctx->fpBWTotal[i], "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------\n");
                    osal_fprintf(ctx->fpBWTotal[i], "                                                                                                                                                                     WRITE(B)                                                                                                                                                                       |                                                                                                                                                                READ(B)                                                                                                                                                                    | Write&Read\n");
                    osal_fprintf(ctx->fpBWTotal[i], "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------\n");

                    if (i == 0 || i == 1) //vCore
                        osal_fprintf(ctx->fpBWTotal[i], "  No. TYPE     PRI      SEC      CH0_0      CH0_1     CH1_4       CH1_0       CH1_1       CH1_2      CH1_3      CH1_4      CH1_5      CH1_6      CH1_7      CH2_0      CH2_1      CH2_2      CH2_3      CH2_4      CH2_5      CH2_6       CH2_7       CH3_0      CH3_1      CH3_2      CH3_3      CH3_4      CH3_5      CH3_6      CH3_7   TOTAL    |       PRI      SEC        CH0_0      CH0_1     CH1_4       CH1_0       CH1_1       CH1_2      CH1_3      CH1_4      CH1_5      CH1_6      CH1_7      CH2_0      CH2_1      CH2_2      CH2_3      CH2_4      CH2_5      CH2_6       CH2_7       CH3_0      CH3_1      CH3_2      CH3_3      CH3_4     CH3_5     CH3_6     CH3_7     TOTAL  |    TOTAL \n");
                    else //vCPU
                        osal_fprintf(ctx->fpBWTotal[i], "  No. TYPE     PRI      SEC      CH0_0      CH0_1     CH0_2       CH0_3       CH0_4       CH0_5      CH0_6      CH0_7      CH1_0      CH1_1      CH1_2      CH1_3      CH1_4      CH1_5      CH1_6      CH1_7      CH2_0      CH2_1       CH2_2       CH2_3      CH2_4      CH2_5      CH2_6      CH2_7      CH3_0      CH3_1      CH3_2   TOTAL    |       PRI      SEC        CH0_0      CH0_1     CH0_2       CH0_3       CH0_4       CH0_5      CH0_6      CH0_7      CH1_0      CH1_1      CH1_2      CH1_3      CH1_4      CH1_5      CH1_6      CH1_7      CH2_0      CH2_1       CH2_2       CH2_3      CH2_4      CH2_5      CH2_6      CH2_7     CH3_0     CH3_1     CH3_7     TOTAL  |    TOTAL \n");

                    if (ctx->bwMode == 0)
                        osal_fprintf(ctx->fpBWTotal[i], "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------\n");
                }
                if (ctx->bwMode != 0)
                    osal_fprintf(ctx->fpBWTotal[i], "====================================================================================================================================================================================================================================================================================================================================================|===========================================================================================================================================================================================================================================================================================================================================|============\n");
                osal_fprintf(ctx->fpBWTotal[i], "%5d %s ", w6_frame_num - 1, pic_type_string);
                for (k = 0; k < WAVE6_MAX_BW_DATA_CH_SIZE; k++)
                    osal_fprintf(ctx->fpBWTotal[i], "%10d ", bw_data->wdata[i][j][k]);

                total_write = bw_data->wdata[i][j][0] + bw_data->wdata[i][j][1];
                osal_fprintf(ctx->fpBWTotal[i], "%10d ", total_write);

                osal_fprintf(ctx->fpBWTotal[i], " | ");
                for (k = 0; k < WAVE6_MAX_BW_DATA_CH_SIZE; k++)
                    osal_fprintf(ctx->fpBWTotal[i], "%10d ", bw_data->rdata[i][j][k]);
                total_read = bw_data->rdata[i][j][0] + bw_data->rdata[i][j][1];
                osal_fprintf(ctx->fpBWTotal[i], "%10d", total_read);
                osal_fprintf(ctx->fpBWTotal[i], " | ");
                osal_fprintf(ctx->fpBWTotal[i], "%10d\n", total_write + total_read);
            }
            else {
                if (1 == j)
                    osal_fprintf(ctx->fpBWTotal[i], "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------\n");
                osal_fprintf(ctx->fpBWTotal[i], "%5d %s ", j - 1, pic_type_string);
                for (k = 0; k < WAVE6_MAX_BW_DATA_CH_SIZE; k++)
                    osal_fprintf(ctx->fpBWTotal[i], "%10d ", bw_data->wdata[i][j][k]);
                total_write = bw_data->wdata[i][j][0] + bw_data->wdata[i][j][1];
                osal_fprintf(ctx->fpBWTotal[i], "%10d ", total_write);
                osal_fprintf(ctx->fpBWTotal[i], " | ");
                for (k = 0; k < WAVE6_MAX_BW_DATA_CH_SIZE; k++)
                    osal_fprintf(ctx->fpBWTotal[i], "%10d ", bw_data->rdata[i][j][k]);
                total_read = bw_data->rdata[i][j][0] + bw_data->rdata[i][j][1];
                osal_fprintf(ctx->fpBWTotal[i], "%10d", total_read);
                osal_fprintf(ctx->fpBWTotal[i], " | ");
                osal_fprintf(ctx->fpBWTotal[i], "%10d\n", total_write + total_read);
            }
        }
        if (ctx->bwMode != 0)
            osal_fprintf(ctx->fpBWTotal[i], "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------\n");
        osal_fflush(ctx->fpBWTotal[i]);
    }
}

static void
    wave6_bw_monitor_print(
    BWCtx   context,
    BWData  data,
    Uint32  picType,
    Int32   recon_idx,
    Uint32  w6_frame_num
    )
{
    wave6_bw_ctx*    common = (wave6_bw_ctx*)context;

    _wave6_bw_monitor_print(context, common->instance->isDecoder, data, picType, recon_idx, w6_frame_num);
    common->w6_frame_num = w6_frame_num;
}

#define BW_TIMEOUT_COUNT 10000
static Uint32 check_bw_ready(
    Uint32 core_idx,
    Uint32 addr
    )
{
    Uint32 status = 0;
    Uint32 timeout = 0;
    timeout = BW_TIMEOUT_COUNT;
    while (((status&0x1) != 1) && timeout) {
        status = vdi_fio_read_register(core_idx, addr) >> 30;
        timeout--;
    }
    if (timeout == 0) {
        VLOG(ERR, "check_bw_ready HANGUP\n");
        CNMErrorSet(CNM_ERROR_HANGUP);
        return FALSE;
    }
    return TRUE;
}

static BWData
    wave6_bw_monitor_get_data(
    BWCtx    context,
    Uint32   numCores,
    Int32    recon_idx
    )
{
    wave6_bw_ctx*   ctx   = (wave6_bw_ctx*)context;
    wave6_bw_data*  bw_data = NULL;
    Uint32          j = 0, k = 0, i = 0;
    Uint32          enable_write = 0;
    Uint32          loopCnt = (ctx->bwMode == 0) ? 1 : BWB_BACKBONE_DATA_SIZE;
    Uint32          enable_burst_length = 0;
    Uint32          core_idx = ctx->coreIndex;
    Uint32          mode;
    Uint32          prevTotal;
    Uint32          burstLengthIdx;
    Uint32          reg_base[WAVE6_CORE_COUNT] = {W6_BW_MON_VCORE0_BASE, W6_BW_MON_VCORE1_BASE, W6_BW_MON_VCPU_BASE};

    bw_data = (wave6_bw_data*)osal_malloc(sizeof(wave6_bw_data));

    SetClockGate(ctx->coreIndex, TRUE);

    for (i = 0; i < ctx->coreCount; i++) { //0:vCore0, 1:vCore1, 2:vCPU
        if (numCores == 1 && i == 1)
            continue; //no vcore1 data
            
        bw_data->isExist[i] = 1;
        for (j = 0; j < loopCnt; j++) {
            osal_memset(bw_data->rdata[i][j], 0, sizeof(bw_data->rdata[i][j]));
            osal_memset(bw_data->wdata[i][j], 0, sizeof(bw_data->wdata[i][j]));

            if (j == 0) {
                enable_burst_length = 0;
                burstLengthIdx = 0;
            }
            else {
                enable_burst_length = 1; //detail data
                burstLengthIdx = j - 1;
            }
            for (enable_write = 0; enable_write < 2; enable_write++) {
                // [31:31] : report Enable
                // [ 5: 5] : READ(0)/WRITE(1) mode
                // [ 4: 4] : burst length en
                // [ 3: 0] : burst length value
                mode = (1ULL << 31) | (enable_write << 5) | (enable_burst_length << 4) | (burstLengthIdx);
                vdi_fio_write_register(core_idx, reg_base[i] + W6_BW_MON_OFFSET_MODE, mode);
                if (check_bw_ready(core_idx, reg_base[i] + W6_BW_MON_OFFSET_MODE) == FALSE) {
                    SetClockGate(ctx->coreIndex, FALSE);
                    return NULL;
                }

                if (enable_write == TRUE) {
                    bw_data->wdata[i][j][0] = vdi_fio_read_register(core_idx, reg_base[i] + W6_BW_MON_OFFSET_PRI_W);
                    bw_data->wdata[i][j][1] = vdi_fio_read_register(core_idx, reg_base[i] + W6_BW_MON_OFFSET_SEC_W);
                    for (k = 2; k < WAVE6_MAX_BW_DATA_CH_SIZE; k++) {
                        bw_data->wdata[i][j][k] = vdi_fio_read_register(core_idx, reg_base[i] + W6_BW_MON_OFFSET_INFO_CH0 + ((k - 2) * 4));
                    }
                }
                else {
                    bw_data->rdata[i][j][0] = vdi_fio_read_register(core_idx, reg_base[i] + W6_BW_MON_OFFSET_PRI_R);
                    bw_data->rdata[i][j][1] = vdi_fio_read_register(core_idx, reg_base[i] + W6_BW_MON_OFFSET_SEC_R);
                    for (k = 2; k < WAVE6_MAX_BW_DATA_CH_SIZE; k++) {
                        bw_data->rdata[i][j][k] = vdi_fio_read_register(core_idx, reg_base[i] + W6_BW_MON_OFFSET_INFO_CH0 + ((k - 2) * 4));
                    }
                }
            }

            if (0 == j) {
                prevTotal = ctx->total;
                for (k = 0; k < WAVE6_MAX_BW_DATA_CH_SIZE; k++) {
                    bw_data->rdata[i][0][k] *= 16;
                    bw_data->wdata[i][0][k] *= 16;
                }
                for (k = 0; k < 2; k++) {//primary, secondary
                    ctx->total_read_ch[i][k]  += bw_data->rdata[i][0][k];
                    ctx->total_write_ch[i][k] += bw_data->wdata[i][0][k];
                    ctx->total += bw_data->rdata[i][0][k];
                    ctx->total += bw_data->wdata[i][0][k];
                }

                if (prevTotal == ctx->total) {
                    // VPU didn't decode a frame.
                    //vdi_fio_write_register(ctx->coreIndex, reg_base[i] + W6_BW_MON_OFFSET_COMMAND, 0x3);
                    //SetClockGate(ctx->coreIndex, FALSE);
                    //return NULL ;
                }
                if (recon_idx >= 0) {
                    //only include encoding bandwidth. 
                    //exclude encoding delay, header only encoding
                    ctx->total_encoded_pri_read[i]  += bw_data->rdata[i][0][0];
                    ctx->total_encoded_pri_write[i] += bw_data->wdata[i][0][0];
                    ctx->total_encoded_sec_read[i]  += bw_data->rdata[i][0][1];
                    ctx->total_encoded_sec_write[i] += bw_data->wdata[i][0][1];
                    for (k = 0; k < WAVE6_MAX_BW_DATA_CH_SIZE; k++) {
                        ctx->total_encoded_read_ch[i][k]  += bw_data->rdata[i][0][k];
                        ctx->total_encoded_write_ch[i][k] += bw_data->wdata[i][0][k];
                    }
                }
            }
        }
        vdi_fio_write_register(ctx->coreIndex, reg_base[i] + W6_BW_MON_OFFSET_COMMAND, 0x3);
    }

    //reset
    SetClockGate(ctx->coreIndex, FALSE);

    return (BWData)bw_data;
}

static BWMonitorOps_t s_wave6_bw_ops = {
    wave6_bw_monitor_allocate,
    wave6_bw_monitor_release,
    wave6_bw_monitor_reset,
    wave6_bw_monitor_get_data,
    wave6_bw_monitor_print
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
BWCtx
BWMonitorSetup(
    CodecInst*  instance,
    BOOL        perFrame,
    Uint32      bwMode,
    char*       strLogDir
    )
{
    Uint32          coreIndex;
    Uint32          productId;
    Uint32          instIndex;
    BWCommonCtx*    common;
    BWMonitorOps_t* bwOps;
    osal_file_t     fp;
    char            path[256];
    Uint32          i;
    Uint32          coreCount = 1;

    coreIndex = instance->coreIdx;
    productId = instance->productId;
    instIndex = instance->instIndex;

    switch (productId) {
    case PRODUCT_ID_521:
        bwOps = &s_wave_backbone_ops;
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        bwOps = &s_wave6_bw_ops;
        coreCount = WAVE6_CORE_COUNT;
        break;
    default:
        VLOG(ERR, "%s:%d Not yet implemented.\n", __FUNCTION__, __LINE__);
        exit(1);
    }

    common = (BWCommonCtx*)bwOps->allocate(coreIndex);
    common->ops             = bwOps;
    common->coreIndex       = coreIndex;
    common->instanceIndex   = instIndex;
    common->productId       = instance->productId;
    common->instance        = instance;
    common->coreCount       = coreCount;
    for (i = 0; i < coreCount; i++) {//0:vcore0, 1:vcore1, 2:vcpu
        if (strLogDir) {
            sprintf(path, "./report/bw/%s/", strLogDir);
            MkDir(path);
            sprintf(path, "./report/bw/%s/report_bandwidth_%d_%d_%d.txt", strLogDir, coreIndex, instIndex, i);
        }
        else {
            sprintf(path, "./report/bw/report_bandwidth_%d_%d_%d.txt", coreIndex, instIndex, i);
            MkDir("./report/bw/");
        }
        if ((fp = osal_fopen(path, "w")) == NULL) {
            VLOG(ERR, "Failed to open %s\n", path);
        }
        common->fpBWTotal[i] = fp;
    }
    common->enableReportPerFrame = perFrame;
    common->bwMode          = bwMode;
    if (strLogDir) {
        sprintf(common->strLogDir, "%s", strLogDir);
    }
    else {
        osal_memset(common->strLogDir, 0x00, sizeof(common->strLogDir)*sizeof(char));
    }

    if (PRODUCT_ID_W6_SERIES(productId))
        bwOps->reset(common);

    return common;
}

void
BWMonitorReset(
    BWCtx    context
    )
{
    BWCommonCtx* common = (BWCommonCtx*)context;

    if (common == NULL)
        return;

    common->ops->reset(context);
}

void
BWMonitorRelease(
    BWCtx    context
    )
{
    BWCommonCtx* common = (BWCommonCtx*)context;
    Uint32 i;

    if (common == NULL)
        return;

    common->ops->release(context);

    for (i = 0; i < common->coreCount ; i++) {
        if (common->fpBWTotal[i])
            osal_fclose(common->fpBWTotal[i]);
    }
}

void
BWMonitorUpdate(
    BWCtx       context,
    Uint32      numCores,
    Int32       recon_idx
    )
{
    BWCommonCtx* common = (BWCommonCtx*)context;

    if (common == NULL || common->fpBWTotal[0] == NULL) {
        return;
    }

    if ((common->data=common->ops->get_data(context, numCores, recon_idx)) == NULL) {
        return;
    }

    common->numFrames++;
}

void
    BWMonitorUpdatePrint(
    BWCtx       context,
    Uint32      picType,
    Int32      recon_idx,
    Uint32      frame_num
    )
{
    BWCommonCtx* common = (BWCommonCtx*)context;

    if (common == NULL || common->fpBWTotal[0] == NULL ) {
        return;
    }
    if (common->data) {
        common->ops->print(context, common->data, picType, recon_idx, frame_num);
        osal_free(common->data);
        common->data = NULL;
    }

    osal_free(common->data);
}

