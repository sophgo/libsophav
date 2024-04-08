#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "bm_ioctl.h"
#include "linux/cvi_vc_drv_ioctl.h"
#include "bm_vpu_logging.h"


#define PAGE_SIZE ((unsigned long)getpagesize())


int bmenc_chn_open(char* dev_name)
{

    int fd;

    fd = open(dev_name, O_RDWR | O_DSYNC | O_CLOEXEC);
    if (fd == -1) {
        BMVPU_ENC_ERROR("cannot open '%s'\n", dev_name);
        return -1;
    }


    return fd;
}

void bmenc_chn_close(int fd) {

    if (fd < 0) {
        return;
    }
    close(fd);
    return;
}



///////////////
// ENC IOCTL //
///////////////
int  bmenc_ioctl_get_ext_addr(int chn_fd, int *ext_addr)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_GET_EXT_ADDR, ext_addr);
    return ret;
}

int bmenc_ioctl_set_chn(int chn_fd, int *chn_id)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VCODEC_SET_CHN, chn_id);
    return ret;
}

int bmenc_ioctl_get_chn(int chn_fd, int *is_jpu, int *chn_id)
{
    int ret = 0;
    *chn_id = ioctl(chn_fd, CVI_VC_VCODEC_GET_CHN, is_jpu);
    if (*chn_id < 0) {
        return -1;
    }
    return ret;
}

int bmenc_ioctl_create_chn(int chn_fd, VENC_CHN_ATTR_S* pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_CREATE_CHN, pstAttr);

    return ret;
}

int bmenc_ioctl_destroy_chn(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_DESTROY_CHN);
    return ret;
}

int bmenc_ioctl_set_rc_params(int chn_fd, VENC_RC_PARAM_S *pstRcParam)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_SET_RC_PARAM, pstRcParam);
    return ret;
}

int bmenc_ioctl_set_h264VUI(int chn_fd, VENC_H264_VUI_S *pH264Vui)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_SET_H264_VUI, pH264Vui);
    return ret;
}

int bmenc_ioctl_set_h265VUI(int chn_fd, VENC_H265_VUI_S *pH265Vui)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_SET_H265_VUI, pH265Vui);
    return ret;
}

int bmenc_ioctl_get_chn_attr(int chn_fd, VENC_CHN_ATTR_S* pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_GET_CHN_ATTR, pstAttr);

    return 0;
}

int bmenc_ioctl_start_recv_frame(int chn_fd, VENC_RECV_PIC_PARAM_S* pstRecvParam)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_START_RECV_FRAME, pstRecvParam);
    return ret;
}

int bmenc_ioctl_stop_recv_frame(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_STOP_RECV_FRAME);
    return ret;
}

int  bmenc_ioctl_encode_header(int chn_fd, VENC_ENCODE_HEADER_S* pstEncodeHeader)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_GET_HEADER, pstEncodeHeader);
    return ret;
}
int bmenc_ioctl_send_frame(int chn_fd, VIDEO_FRAME_INFO_EX_S* pstFrameEx)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_SEND_FRAME, pstFrameEx);
    return ret;
}

int bmenc_ioctl_get_stream(int chn_fd, VENC_STREAM_EX_S* pstStreamEx)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_GET_STREAM, pstStreamEx);
    return ret;
}

int bmenc_ioctl_release_stream(int chn_fd, VENC_STREAM_S* pstStream)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_RELEASE_STREAM, pstStream);
    return ret;
}

int bmenc_ioctl_roi(int chn_fd, VENC_CUSTOM_MAP_S* proiAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_SET_CUSTOM_MAP, proiAttr);
    return ret;
}

int bmenc_ioctl_get_intinal_info(int chn_fd, VENC_INITIAL_INFO_S *pinfo)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_GET_INTINAL_INFO, pinfo);
    return ret;
}

int  bmenc_ioctl_query_status(int chn_fd, VENC_CHN_STATUS_S* pstatus)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VENC_QUERY_STATUS, pstatus);
    return ret;
}

