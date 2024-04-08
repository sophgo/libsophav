/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_comm_sys.h
 * Description:
 *   The common sys type defination.
 */

#ifndef __CVI_COMM_SYS_H__
#define __CVI_COMM_SYS_H__

#include <linux/cvi_comm_video.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define BIND_DEST_MAXNUM 64
#define BIND_NODE_MAXNUM 64

typedef struct _MMF_BIND_DEST_S {
	CVI_U32   u32Num;
	MMF_CHN_S astMmfChn[BIND_DEST_MAXNUM];
} MMF_BIND_DEST_S;

typedef struct _BIND_NODE_S {
	CVI_BOOL bUsed;
	MMF_CHN_S src;
	MMF_BIND_DEST_S dsts;
} BIND_NODE_S;

typedef enum _VI_VPSS_MODE_E {
	VI_OFFLINE_VPSS_OFFLINE = 0,
	VI_OFFLINE_VPSS_ONLINE,
	VI_ONLINE_VPSS_OFFLINE,
	VI_ONLINE_VPSS_ONLINE,
	VI_BE_OFL_POST_OL_VPSS_OFL,
	VI_BE_OFL_POST_OFL_VPSS_OFL,
	VI_BE_OL_POST_OFL_VPSS_OFL,
	VI_BE_OL_POST_OL_VPSS_OFL,
	VI_VPSS_MODE_BUTT
} VI_VPSS_MODE_E;


typedef struct _VI_VPSS_MODE_S {
	VI_VPSS_MODE_E aenMode[VI_MAX_PIPE_NUM];
} VI_VPSS_MODE_S;

typedef struct _CVI_CDMA_2D_S {
	CVI_U64 u64PhyAddrSrc;
	CVI_U64 u64PhyAddrDst;
	CVI_U16 u16Width;
	CVI_U16 u16Height;
	CVI_U16 u16StrideSrc;
	CVI_U16 u16StrideDst;
	CVI_U16 u16FixedValue;
	CVI_BOOL bEnableFixed;
} CVI_CDMA_2D_S;

typedef struct _CVI_TDMA_2D_S {
	uint64_t paddr_src;
	uint64_t paddr_dst;
	uint32_t w_bytes;
	uint32_t h;
	uint32_t stride_bytes_src;
	uint32_t stride_bytes_dst;
} CVI_TDMA_2D_S;

typedef struct _VPSS_VENC_WRAP_PARAM_S {
	CVI_BOOL bAllOnline;

	CVI_U32 u32FrameRate;
	CVI_U32 u32FullLinesStd;

	SIZE_S stLargeStreamSize;
	SIZE_S stSmallStreamSize;
} VPSS_VENC_WRAP_PARAM_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* __CVI_COMM_SYS_H__ */

