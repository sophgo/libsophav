#ifndef BMCV_BM1688_VPP_INTERANL_H
#define BMCV_BM1688_VPP_INTERANL_H
#endif

#include "bmcv_a2_common_internal.h"

#define VPSS_MAX_W 8192
#define VPSS_MAX_H 8192
#define VPSS_MIN_W 16
#define VPSS_MIN_H 16

typedef struct border {
	bool rect_border_enable;
	u16 st_x;
	u16 st_y;
	u16 width;
	u16 height;
	u8 value_r;
	u8 value_g;
	u8 value_b;
	u16 thickness;
} border_t;

typedef struct coverex_cfg {
	struct rgn_coverex_param coverex_param[4];
	u8 cover_num;
} coverex_cfg;

typedef struct bm_rgn_cfg {
	struct rgn_param* param;
	u8 rgn_num;
} bmcv_rgn_cfg;

typedef struct bmcv_border {
	u8 border_num;
	border_t border_cfg[4];
} bmcv_border;

typedef struct bmcv_vpss_csc_matrix {
	u16 coef[3][3];
	u8 sub[3];
	u8 add[3];
} bmcv_vpss_csc_matrix;

typedef enum bmcv_vpss_csc_type {
	VPSS_CSC_YCbCr2RGB_BT601 = 0,
	VPSS_CSC_YPbPr2RGB_BT601,
	VPSS_CSC_RGB2YCbCr_BT601,
	VPSS_CSC_YCbCr2RGB_BT709,
	VPSS_CSC_RGB2YCbCr_BT709,
	VPSS_CSC_RGB2YPbPr_BT601,
	VPSS_CSC_YPbPr2RGB_BT709,
	VPSS_CSC_RGB2YPbPr_BT709,
	VPSS_CSC_YPbPr2YPbPr_BT601,
	VPSS_CSC_YCbCr2YCbCr_BT601,
	VPSS_CSC_YPbPr2YPbPr_BT709,
	VPSS_CSC_YCbCr2YCbCr_BT709,
	VPSS_CSC_RGB2RGB,
} bmcv_vpss_csc_type;

typedef struct bmcv_csc_cfg {
	u8 is_fancy;
	u8 is_user_defined_matrix;
	bmcv_flip_mode flip_mode;
	bmcv_vpss_csc_type csc_type;
	bmcv_vpss_csc_matrix csc_matrix;
} bmcv_csc_cfg;

typedef struct stitch_ctx_{
	u16 idx;
	bm_status_t ret;
	bm_handle_t handle;
	bm_image input;
	bm_image output;
	bmcv_rect_t src_crop_rect;
	bmcv_resize_algorithm algorithm;
	bmcv_padding_attr_t padding_attr;
} stitch_ctx;

#ifdef BM_PCIE_MODE
struct vpp_batch_n {
	bm_vpss_cfg *cmd;
};
DECL_EXPORT bm_status_t bm_trigger_vpp(bm_handle_t handle, struct vpp_batch_n* batch);
#endif