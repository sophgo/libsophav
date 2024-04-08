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

#include "main_helper.h"
#include "debug.h"
#include <stdarg.h>
#include "vpuapi/product.h"

#define PRE_CYCLE_COUNT 12  //max 120fps
typedef struct {
    osal_file_t fp;
    Uint32      referenceClk;
    Uint32      preCycles[PRE_CYCLE_COUNT];
    Uint32      fps;
    Uint32      num;            /**< Number of decoded frames */
    Uint32      failureCount;
    Uint64      totalCycles;
    Uint32      sumMhzPerfps;
    Uint32      minMhzPerfps;
    Uint32      maxMhzPerfps;
    double      nsPerCycle;     /**< Nanosecond per cycle */
    BOOL        isEnc;
} PerformanceMonitor;

PFCtx PFMonitorSetup(
    Uint32  coreIdx,
    Uint32  instanceIndex,
    Uint32  targetClkInHz,
    Uint32  fps,
    char*   strLogDir,
    BOOL    isEnc
    )
{
    PerformanceMonitor* pm = NULL;
    char                path[128];
    osal_file_t         fp = NULL;
    Uint32              revision;
    Uint32              inMHz;
    Uint32              movingCount;
    Int32 productId   = VPU_GetProductId(coreIdx);

    if (fps != 30 && fps != 60  && fps != 120) {
        /* Currently, it doesn't support other framerate */
        VLOG(INFO, "%s:%d fps parameter shall be 30, 60 or 120\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    if (strLogDir) {
        sprintf(path, "./report/pf/%s/", strLogDir);
        MkDir(path);
        sprintf(path, "./report/pf/%s/report_performance_%d_0.txt", strLogDir, coreIdx);
    }
    else {
        sprintf(path, "./report/pf/report_performance_%d_0.txt", coreIdx);
        MkDir("./report/pf/");
    }

    if ((fp=osal_fopen(path, "w")) == NULL) {
        VLOG(ERR, "%s:%d Failed to create file %s\n", __FUNCTION__, __LINE__, path);
        return NULL;
    }

    pm = (PerformanceMonitor*)osal_malloc(sizeof(PerformanceMonitor));
    if (pm == NULL) {
        osal_fclose(fp);
        return NULL;
    }

    inMHz            = targetClkInHz/1000000;
    movingCount      = fps/10;
    pm->fp           = fp;
    pm->referenceClk = targetClkInHz;
    pm->nsPerCycle   = 1000/(double)inMHz;
    pm->fps          = fps;
    pm->num          = 0;
    pm->failureCount = 0;
    pm->totalCycles  = 0;
    pm->minMhzPerfps = 0x7fffffff;
    pm->maxMhzPerfps = 0;
    pm->sumMhzPerfps = 0;
    pm->isEnc        = isEnc;

    VPU_GetVersionInfo(coreIdx, NULL, &revision, NULL);

    if ( isEnc == TRUE) {
        VLOG(ERR, "%s:%d Check SUPPORT_ENC_PERFORMANCE to measure encoder performance \n",__FUNCTION__, __LINE__);
        osal_fprintf(fp, "You didn't enable SUPPORT_ENC_PERFORMANCE. performance cycles are not checked correctly\n");
    }

    osal_fprintf(fp, "#Rev.%d\n", revision);
    osal_fprintf(fp, "#Target Clock: %dMHz\n", targetClkInHz/1000000);
    osal_fprintf(fp, "#PASS CONDITION : MovingSum(%dframes) <= 100ms, %dFps\n", movingCount, pm->fps);
    if ( productId == PRODUCT_ID_521 || productId == PRODUCT_ID_511) {
        osal_fprintf(fp, "#================================================================================================================================================================\n");
        osal_fprintf(fp, "#             One frame                      %d Frames moving sum                  Average                   %dfps                              Cycles\n", movingCount, pm->fps);
        osal_fprintf(fp, "#      ---------------------------     ----------------------------------------------------------- ---------------------------    -------------------------------\n");
        osal_fprintf(fp, "# No.    cycles      time(ms)             cycles     time(ms)    PASS       cycles      time(ms)   min(MHz) avg(MHz) max(MHz)       PREPARE  PROCESSING %s\n", isEnc?"ENCODING":"DECODING");
        osal_fprintf(fp, "#================================================================================================================================================================\n");
    } else if (productId == PRODUCT_ID_517) {
        osal_fprintf(fp, "#===============================================================================================================================================================\n");
        osal_fprintf(fp, "#                           One frame                                          %d Frames moving sum                  Average                   %dfps            \n", movingCount, pm->fps);
        osal_fprintf(fp, "#      ----------------------------------------------------------         ---------------------------------------------------------- ---------------------------\n");
        osal_fprintf(fp, "# No.      seek       parse    decoding   cycles      time(ms)              cycles     time(ms)    PASS       cycles      time(ms)   min(MHz) avg(MHz) max(MHz) \n");
        osal_fprintf(fp, "#=============================================================================================================================\n");
    }
    else {
        osal_fprintf(fp, "#=============================================================================================================================\n");
        osal_fprintf(fp, "#             One frame                      %d Frames moving sum                  Average                   %dfps            \n", movingCount, pm->fps);
        osal_fprintf(fp, "#      ---------------------------     ----------------------------------------------------------- ---------------------------\n");
        osal_fprintf(fp, "# No.    cycles      time(ms)             cycles     time(ms)    PASS       cycles      time(ms)   min(MHz) avg(MHz) max(MHz) \n");
        osal_fprintf(fp, "#=============================================================================================================================\n");
    }
    return (PFCtx)pm;
}

void PFMonitorRelease(
    PFCtx   context
    )
{
    PerformanceMonitor* pm = (PerformanceMonitor*)context;
    osal_file_t         fp;
    Uint32              minMHz, avgMHz, maxMHz;
    Uint32              expectedCpf;            //!<< Expected cycles per frame
    Uint32              avgCycles;
    BOOL                pass = TRUE;
    if (pm == NULL) {
        VLOG(ERR, "%s:%d NULL Context\n", __FUNCTION__, __LINE__);
        return;
    }

    if (pm->fps == 0 || pm->num == 0) {
        VLOG(WARN, "%s:%d divisor must be a integer(not zero)\n", __FUNCTION__, __LINE__);
        return;
    }

    expectedCpf = pm->referenceClk / pm->fps;
    avgCycles   = (Uint32)(pm->totalCycles / pm->num);

    fp = pm->fp;
    minMHz = pm->minMhzPerfps;
    avgMHz = pm->sumMhzPerfps/pm->num;
    maxMHz = pm->maxMhzPerfps;
    if (pm->failureCount > 0 || avgCycles > expectedCpf ) {
        VLOG(INFO, "expectedCpf: %d avgCycles: %d\n", expectedCpf, avgCycles);
        pass = FALSE;
    }
    osal_fprintf(fp, "#=============================================================================================================================\n");
    osal_fprintf(fp, "# %dFPS SUMMARY(required clock)         : MIN(%dMHz) AVG(%dMHz) MAX(%dMHz) \n", pm->fps, minMHz, avgMHz, maxMHz);
    osal_fprintf(fp, "# NUMBER OF FAILURE MOVING SUM(%dFRAMES) : %d\n", pm->fps/10, pm->failureCount);
    osal_fprintf(fp, "#%s\n", pass == TRUE ? "SUCCESS" : "FAILURE");

    osal_fclose(pm->fp);

    osal_free(pm);

    return;
}

void PFMonitorUpdate(
    Uint32  coreIdx,
    PFCtx   context,
    Uint32  cycles,
    ...
    )
{
    PerformanceMonitor* pm = (PerformanceMonitor*)context;
    osal_file_t         fp;
    Uint32              cyclesInMs;
    Uint32              TotalcyclesInMs;
    Uint32              movingSum3InCycle; /* 3 frame moving sum */
    Uint32              movingSum3InMs;    /* 3 frame moving sum in millisecond */
    Uint32              count;
    Uint32*             preCycles;
    Uint32              mhzPerfps;
    Uint8               resultMsg;
    Uint32              movingCount;
    Int32 productId   = ProductVpuGetId(coreIdx);

    if (pm == NULL) {
        VLOG(ERR, "%s:%d NULL Context\n", __FUNCTION__, __LINE__);
        return;
    }

    fp          = pm->fp;
    count       = pm->num;
    preCycles   = pm->preCycles;
    mhzPerfps   = (Uint32)(cycles*pm->fps / 1000000.0 + 0.5);

    pm->totalCycles += cycles;
    pm->sumMhzPerfps += mhzPerfps;
    pm->minMhzPerfps = (pm->minMhzPerfps>mhzPerfps) ? mhzPerfps : pm->minMhzPerfps;
    pm->maxMhzPerfps = (pm->maxMhzPerfps<mhzPerfps) ? mhzPerfps : pm->maxMhzPerfps;

    movingCount = pm->fps/10; //30 fps = 3, 60fps = 6, 120fps = 12

    count++;
    if (count < movingCount) {
        movingSum3InCycle = 0;
        preCycles[count-1] = cycles;
    }
    else {
        if (movingCount == 3) {
            movingSum3InCycle = preCycles[0] + preCycles[1] + cycles;
            preCycles[0]      = preCycles[1];
            preCycles[1]      = cycles;
        }
        else if (movingCount == 6) {
            movingSum3InCycle = preCycles[0] + preCycles[1] + preCycles[2] + preCycles[3] + preCycles[4] + cycles;
            preCycles[0]      = preCycles[1];
            preCycles[1]      = preCycles[2];
            preCycles[2]      = preCycles[3];
            preCycles[3]      = preCycles[4];
            preCycles[4]      = cycles;
        }
        else if (movingCount == 12) {
            movingSum3InCycle = preCycles[0] + preCycles[1] + preCycles[2] + preCycles[3] + preCycles[4] + cycles;
            preCycles[0]      = preCycles[1];
            preCycles[1]      = preCycles[2];
            preCycles[2]      = preCycles[3];
            preCycles[3]      = preCycles[4];
            preCycles[4]      = preCycles[5];
            preCycles[5]      = preCycles[6];
            preCycles[6]      = preCycles[7];
            preCycles[7]      = preCycles[8];
            preCycles[8]      = preCycles[9];
            preCycles[9]      = preCycles[10];
            preCycles[10]     = cycles;
        }
        else {
            VLOG(ERR, "not support pfs=%d,movingCount=%d\n", pm->fps, movingCount);
            return;
        }
    }

    cyclesInMs      = (Uint32)((cycles * pm->nsPerCycle)/1000000);
    TotalcyclesInMs = (Uint32)((pm->totalCycles * pm->nsPerCycle)/1000000);
    movingSum3InMs  = (Uint32)((movingSum3InCycle * pm->nsPerCycle)/1000000);

    if (movingSum3InMs > 100) {
        resultMsg = 'X';
        pm->failureCount++;
    }
    else {
        resultMsg = 'O';
    }

	if (productId == PRODUCT_ID_521 || productId == PRODUCT_ID_627 || productId == PRODUCT_ID_637)
    {
        Uint32  encPrepareCycle;
        Uint32  encProcessingCycle;
        Uint32  encEncodingCycle;
        va_list ap;

        va_start(ap, cycles);
        encPrepareCycle = va_arg(ap, Uint32);
        encProcessingCycle = va_arg(ap, Uint32);
        encEncodingCycle = va_arg(ap, Uint32);
        va_end(ap);
        if ((count%pm->fps) == 0) {
            Uint32 avgMhzPerfps = pm->sumMhzPerfps / count;
            osal_fprintf(fp, "%5d %10d %10d             %10d %10d     %c      %10d   %7d      %8d %8d %8d   %10d %10d %10d\n",
                count, cycles, cyclesInMs, movingSum3InCycle, movingSum3InMs, resultMsg, pm->totalCycles/count, TotalcyclesInMs/count,
                pm->minMhzPerfps, avgMhzPerfps, pm->maxMhzPerfps, encPrepareCycle, encProcessingCycle, encEncodingCycle);
        }
        else {
            osal_fprintf(fp, "%5d %10d %10d             %10d %10d     %c      %10d   %7d                                   %10d %10d %10d\n",
                count, cycles, cyclesInMs, movingSum3InCycle, movingSum3InMs, resultMsg, pm->totalCycles/count, TotalcyclesInMs/count, encPrepareCycle, encProcessingCycle, encEncodingCycle);
        }
    } else if (productId == PRODUCT_ID_517 || productId == PRODUCT_ID_617) {
        Uint32  decSeekCycle;
        Uint32  decParseCycle;
        Uint32  decodedCycle;
        va_list ap;

        va_start(ap, cycles);
        decSeekCycle = va_arg(ap, Uint32);
        decParseCycle = va_arg(ap, Uint32);
        decodedCycle = va_arg(ap, Uint32);
        va_end(ap);

        if ((count%pm->fps) == 0) {
            Uint32 avgMhzPerfps = pm->sumMhzPerfps / count;
            osal_fprintf(fp, "%5d %10d %10d %10d %10d %10d             %10d %10d     %c      %10d   %7d      %8d %8d %8d\n",
                count, decSeekCycle, decParseCycle, decodedCycle, cycles, cyclesInMs, movingSum3InCycle, movingSum3InMs, resultMsg, pm->totalCycles/count, TotalcyclesInMs/count,
                pm->minMhzPerfps, avgMhzPerfps, pm->maxMhzPerfps);
        }
        else {
            osal_fprintf(fp, "%5d %10d %10d %10d %10d %10d             %10d %10d     %c      %10d   %7d\n",
                count, decSeekCycle, decParseCycle, decodedCycle, cycles, cyclesInMs, movingSum3InCycle, movingSum3InMs, resultMsg, pm->totalCycles/count, TotalcyclesInMs/count);
        }
    } else if ( productId == PRODUCT_ID_980 || productId == PRODUCT_ID_960 ) {
        if ((count%pm->fps) == 0) {
            Uint32 avgMhzPerfps = pm->sumMhzPerfps / count;
            osal_fprintf(fp, "%5d %10d %10d             %10d %10d     %c      %10d   %7d      %8d %8d %8d\n",
                count, cycles, cyclesInMs, movingSum3InCycle, movingSum3InMs, resultMsg, pm->totalCycles/count, TotalcyclesInMs/count,
                pm->minMhzPerfps, avgMhzPerfps, pm->maxMhzPerfps);
        }
        else {
            osal_fprintf(fp, "%5d %10d %10d             %10d %10d     %c      %10d   %7d\n",
                count, cycles, cyclesInMs, movingSum3InCycle, movingSum3InMs, resultMsg, pm->totalCycles/count, TotalcyclesInMs/count);
        }
    }
    else {
        VLOG(ERR, "%s:%d need to check product id\n", __FUNCTION__, __LINE__);
        exit(1);
    }

    pm->num = count;

    return;
}

