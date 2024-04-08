#ifndef _BM_VPUENC_IOCTL_H_
#define _BM_VPUENC_IOCTL_H_


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "linux/cvi_comm_venc.h"

typedef struct _VENC_STREAM_EX_S {
        VENC_STREAM_S *pstStream;
        CVI_S32 s32MilliSec;
} VENC_STREAM_EX_S;

typedef struct _VENC_USER_DATA_S {
        CVI_U8 *pu8Data;
        CVI_U32 u32Len;
} VENC_USER_DATA_S;

typedef struct _VIDEO_FRAME_INFO_EX_S {
        VIDEO_FRAME_INFO_S *pstFrame;
        CVI_S32 s32MilliSec;
} VIDEO_FRAME_INFO_EX_S;

typedef struct _USER_FRAME_INFO_EX_S {
        USER_FRAME_INFO_S *pstUserFrame;
        CVI_S32 s32MilliSec;
} USER_FRAME_INFO_EX_S;

typedef enum {
        REG_CTRL = 0,
        REG_REMAP,
} REG_TYPE;

int  bmenc_chn_open(char* dev_name);
void bmenc_chn_close(int fd);
int  bmenc_ioctl_get_ext_addr(int chn_fd, int *ext_addr);
int  bmenc_ioctl_set_chn(int chn_fd, int *chn_id);
int  bmenc_ioctl_get_chn(int chn_fd, int *is_jpu, int *chn_id);
int  bmenc_ioctl_create_chn(int chn_fd, VENC_CHN_ATTR_S* pstAttr);
int  bmenc_ioctl_destroy_chn(int chn_fd);
int  bmenc_ioctl_set_rc_params(int chn_fd, VENC_RC_PARAM_S *pstRcParam);
int  bmenc_ioctl_set_h264VUI(int chn_fd, VENC_H264_VUI_S *pH264Vui);
int  bmenc_ioctl_set_h265VUI(int chn_fd, VENC_H265_VUI_S *pH265Vui);
int  bmenc_ioctl_get_chn_attr(int chn_fd, VENC_CHN_ATTR_S* pstAttr);
int  bmenc_ioctl_start_recv_frame(int chn_fd, VENC_RECV_PIC_PARAM_S* pstRecvParam);
int  bmenc_ioctl_stop_recv_frame(int chn_fd);
int  bmenc_ioctl_encode_header(int chn_fd, VENC_ENCODE_HEADER_S* pstEncodeHeader);
int  bmenc_ioctl_send_frame(int chn_fd, VIDEO_FRAME_INFO_EX_S* pstFrameEx);
int  bmenc_ioctl_get_stream(int chn_fd, VENC_STREAM_EX_S* pstStreamEx);
int  bmenc_ioctl_release_stream(int chn_fd, VENC_STREAM_S* pstStream);
int  bmenc_ioctl_roi(int chn_fd, VENC_CUSTOM_MAP_S* proiAttr);
int  bmenc_ioctl_get_intinal_info(int chn_fd, VENC_INITIAL_INFO_S *pinfo);
int  bmenc_ioctl_query_status(int chn_fd, VENC_CHN_STATUS_S* pstatus);


#endif /*_BM_VPUENC_IOCTL_H_*/
