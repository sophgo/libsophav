#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "bmcv_a2_common_internal.h"

int stitch_init(int fd)
{
    return ioctl(fd, STITCH_INIT);
}

int stitch_deinit(int fd)
{
    return ioctl(fd, STITCH_DEINIT);
}

int stitch_set_src_attr(int fd, stitch_grp_src_attr *attr)
{
    return ioctl(fd, STITCH_SET_SRC_ATTR, attr);
}

int stitch_get_src_attr(int fd, stitch_grp_src_attr *attr)
{
    return ioctl(fd, STITCH_GET_SRC_ATTR, attr);
}

int stitch_set_chn_attr(int fd, stitch_grp_chn_attr *attr)
{
    return ioctl(fd, STITCH_SET_CHN_ATTR, attr);
}

int stitch_get_chn_attr(int fd, stitch_grp_chn_attr *attr)
{
    return ioctl(fd, STITCH_GET_CHN_ATTR, attr);
}

int stitch_set_op_attr(int fd, stitch_grp_op_attr *attr)
{
    return ioctl(fd, STITCH_SET_OP_ATTR, attr);
}

int stitch_get_op_attr(int fd, stitch_grp_op_attr *attr)
{
    return ioctl(fd, STITCH_GET_OP_ATTR, attr);
}

int stitch_set_wgt_attr(int fd, stitch_grp_bld_wgt_attr *attr)
{
    return ioctl(fd, STITCH_SET_WGT_ATTR, attr);
}

int stitch_get_wgt_attr(int fd, stitch_grp_bld_wgt_attr *attr)
{
    return ioctl(fd, STITCH_GET_WGT_ATTR, attr);
}

int stitch_set_regx(int fd, unsigned char regx)
{
    return ioctl(fd, STITCH_SET_REGX, &regx);
}

int  stitch_grp_enable(int  fd, stitch_grp *grp_id)
{
    return ioctl(fd, STITCH_GRP_ENABLE, grp_id);
}

int  stitch_grp_disable(int  fd, stitch_grp *grp_id)
{
    return ioctl(fd, STITCH_GRP_DISABLE, grp_id);
}

int stitch_reset(int fd)
{
    return ioctl(fd, STITCH_RST);
}

int stitch_send_src_frame(int fd, struct stitch_grp_src_frm_cfg *cfg)
{
    return ioctl(fd, STITCH_SEND_SRC_FRM, cfg);
}

int stitch_send_chn_frame(int fd, struct stitch_grp_chn_frm_cfg *cfg)
{
    return ioctl(fd, STITCH_SEND_CHN_FRM, cfg);
}

int stitch_get_chn_frame(int fd, struct stitch_grp_chn_frm_cfg *cfg)
{
    return ioctl(fd, STITCH_GET_CHN_FRM, cfg);
}

int stitch_release_chn_frame(int fd, struct stitch_grp_chn_frm_cfg *cfg)
{
    return ioctl(fd, STITCH_RLS_CHN_FRM, cfg);
}

int stitch_attach_vbpool(int fd, const struct stitch_grp_vb_pool_cfg *cfg)
{
    return ioctl(fd, STITCH_ATTACH_VB_POOL, cfg);
}

int stitch_detach_vbpool(int fd, const struct stitch_grp_vb_pool_cfg *cfg)
{
    return ioctl(fd, STITCH_DETACH_VB_POOL, cfg);
}

int stitch_dump_reginfo(int fd)
{
    return ioctl(fd, STITCH_DUMP_REGS);
}

int stitch_init_grp(int fd, stitch_grp *grp_id)
{
    return ioctl(fd, STITCH_INIT_GRP, grp_id);
}

int stitch_deinit_grp(int fd, stitch_grp *grp_id)
{
    return ioctl(fd, STITCH_DEINIT_GRP, grp_id);
}

int stitch_get_available_grp(int fd, stitch_grp *grp_id)
{
    return ioctl(fd, STITCH_GET_AVAIL_GROUP, grp_id);
}
