#ifndef _BM_VPUENC_IOCTL_H_
#define _BM_VPUENC_IOCTL_H_


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "linux/comm_venc.h"

typedef struct _venc_stream_ex_s {
        venc_stream_s *pstStream;
        int s32MilliSec;
} venc_stream_ex_s;

typedef struct _venc_user_data_s {
        unsigned char *pu8Data;
        unsigned int u32Len;
} venc_user_data_;

typedef struct _video_frame_info_ex_s {
        video_frame_info_s *pstFrame;
        int s32MilliSec;
} video_frame_info_ex_s;

typedef struct _user_frame_info_ex_s {
        user_frame_info_s *pstUserFrame;
        int s32MilliSec;
} user_frame_info_ex_s;

typedef enum {
        REG_CTRL = 0,
        REG_REMAP,
} REG_TYPE;

int  bmenc_chn_open(char* dev_name);
void bmenc_chn_close(int fd);
int  bmenc_ioctl_get_ext_addr(int chn_fd, int *ext_addr);
int  bmenc_ioctl_set_chn(int chn_fd, int *chn_id);
int  bmenc_ioctl_get_chn(int chn_fd, int *is_jpu, int *chn_id);
int  bmenc_ioctl_create_chn(int chn_fd, venc_chn_attr_s* pstAttr);
int  bmenc_ioctl_destroy_chn(int chn_fd);
int  bmenc_ioctl_set_rc_params(int chn_fd, venc_rc_param_s *pstRcParam);
int  bmenc_ioctl_set_h264VUI(int chn_fd, venc_h264_vui_s *pH264Vui);
int  bmenc_ioctl_set_h265VUI(int chn_fd, venc_h265_vui_s *pH265Vui);
int  bmenc_ioctl_get_chn_attr(int chn_fd, venc_chn_attr_s* pstAttr);
int  bmenc_ioctl_start_recv_frame(int chn_fd, venc_recv_pic_param_s* pstRecvParam);
int  bmenc_ioctl_stop_recv_frame(int chn_fd);
int  bmenc_ioctl_encode_header(int chn_fd, venc_encode_header_s* pstEncodeHeader);
int  bmenc_ioctl_send_frame(int chn_fd, video_frame_info_ex_s* pstFrameEx);
int  bmenc_ioctl_get_stream(int chn_fd, venc_stream_ex_s* pstStreamEx);
int  bmenc_ioctl_release_stream(int chn_fd, venc_stream_s* pstStream);
int  bmenc_ioctl_roi(int chn_fd, venc_custom_map_s* proiAttr);
int  bmenc_ioctl_get_intinal_info(int chn_fd, venc_initial_info_s *pinfo);
int  bmenc_ioctl_query_status(int chn_fd, venc_chn_status_s* pstatus);
int  bmenc_ioctl_enc_set_extern_buf(int chn_fd, venc_extern_buf_s* extern_buf);
int  bmenc_ioctl_enc_alloc_physical_memory(int chn_fd, venc_phys_buf_s* phys_buf);
int  bmenc_ioctl_enc_free_physical_memory(int chn_fd, venc_phys_buf_s* phys_buf);


#endif /*_BM_VPUENC_IOCTL_H_*/
