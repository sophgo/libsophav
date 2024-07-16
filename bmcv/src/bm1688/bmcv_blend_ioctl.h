#ifndef MODULES_VPU_INCLUDE_STITCH_IOCTL_H_
#define MODULES_VPU_INCLUDE_STITCH_IOCTL_H_

#include <sys/ioctl.h>
#include "bmcv_a2_common_internal.h"



int  stitch_init(int  fd);
int  stitch_deinit(int  fd);
int  stitch_set_src_attr(int  fd, stitch_src_attr *attr);
int  stitch_get_src_attr(int  fd, stitch_src_attr *attr);
int  stitch_set_chn_attr(int  fd, stitch_chn_attr *attr);
int  stitch_get_chn_attr(int  fd, stitch_chn_attr *attr);
int  stitch_set_op_attr(int  fd, stitch_op_attr *attr);
int  stitch_get_op_attr(int  fd, stitch_op_attr *attr);
int  stitch_set_wgt_attr(int  fd, stitch_bld_wgt_attr *attr);
int  stitch_get_wgt_attr(int  fd, stitch_bld_wgt_attr *attr);
int  stitch_set_regx(int  fd, unsigned char regx);
int  stitch_dev_enable(int  fd);
int  stitch_dev_disable(int  fd);
int  stitch_reset(int  fd);
int  stitch_send_src_frame(int  fd, struct stitch_src_frm_cfg *cfg);
int  stitch_send_chn_frame(int  fd, struct stitch_chn_frm_cfg *cfg);
int  stitch_get_chn_frame(int  fd, struct stitch_chn_frm_cfg *cfg);
int  stitch_release_chn_frame(int  fd, struct stitch_chn_frm_cfg *cfg);
int  stitch_attach_vbpool(int  fd, const struct stitch_vb_pool_cfg *cfg);
int  stitch_detach_vbpool(int  fd, const struct stitch_vb_pool_cfg *cfg);
int  stitch_dump_reginfo(int  fd);

#endif /* MODULES_VPU_INCLUDE_STITCH_IOCTL_H_ */
