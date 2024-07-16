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

int stitch_set_src_attr(int fd, stitch_src_attr *attr)
{
	return ioctl(fd, STITCH_SET_SRC_ATTR, attr);
}

int stitch_get_src_attr(int fd, stitch_src_attr *attr)
{
	return ioctl(fd, STITCH_GET_SRC_ATTR, attr);
}

int stitch_set_chn_attr(int fd, stitch_chn_attr *attr)
{
	return ioctl(fd, STITCH_SET_CHN_ATTR, attr);
}

int stitch_get_chn_attr(int fd, stitch_chn_attr *attr)
{
	return ioctl(fd, STITCH_GET_CHN_ATTR, attr);
}

int stitch_set_op_attr(int fd, stitch_op_attr *attr)
{
	return ioctl(fd, STITCH_SET_OP_ATTR, attr);
}

int stitch_get_op_attr(int fd, stitch_op_attr *attr)
{
	return ioctl(fd, STITCH_GET_OP_ATTR, attr);
}

int stitch_set_wgt_attr(int fd, stitch_bld_wgt_attr *attr)
{
	return ioctl(fd, STITCH_SET_WGT_ATTR, attr);
}

int stitch_get_wgt_attr(int fd, stitch_bld_wgt_attr *attr)
{
	return ioctl(fd, STITCH_GET_WGT_ATTR, attr);
}

int stitch_set_regx(int fd, unsigned char regx)
{
	return ioctl(fd, STITCH_SET_REGX, &regx);
}

int stitch_dev_enable(int fd)
{
	return ioctl(fd, STITCH_DEV_ENABLE);
}

int stitch_dev_disable(int fd)
{
	return ioctl(fd, STITCH_DEV_DISABLE);
}

int stitch_reset(int fd)
{
	return ioctl(fd, STITCH_RST);
}

int stitch_send_src_frame(int fd, struct stitch_src_frm_cfg *cfg)
{
	return ioctl(fd, STITCH_SEND_SRC_FRM, cfg);
}

int stitch_send_chn_frame(int fd, struct stitch_chn_frm_cfg *cfg)
{
	return ioctl(fd, STITCH_SEND_CHN_FRM, cfg);
}

int stitch_get_chn_frame(int fd, struct stitch_chn_frm_cfg *cfg)
{
	return ioctl(fd, STITCH_GET_CHN_FRM, cfg);
}

int stitch_release_chn_frame(int fd, struct stitch_chn_frm_cfg *cfg)
{
	return ioctl(fd, STITCH_RLS_CHN_FRM, cfg);
}

int stitch_attach_vbpool(int fd, const struct stitch_vb_pool_cfg *cfg)
{
	return ioctl(fd, STITCH_ATTACH_VB_POOL, cfg);
}

int stitch_detach_vbpool(int fd, const struct stitch_vb_pool_cfg *cfg)
{
	return ioctl(fd, STITCH_DETACH_VB_POOL, cfg);
}

int stitch_dump_reginfo(int fd)
{
	return ioctl(fd, STITCH_DUMP_REGS);
}

