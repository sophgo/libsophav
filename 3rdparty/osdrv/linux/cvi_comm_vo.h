/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_comm_vo.h
 * Description:
 *   The common data type defination for VO module.
 */

#ifndef __CVI_COMM_VO_H__
#define __CVI_COMM_VO_H__

#include <linux/cvi_type.h>
#include <linux/cvi_common.h>
#include <linux/cvi_comm_video.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


/* VO video output interface type */
#define VO_INTF_CVBS (0x01L << 0)
#define VO_INTF_YPBPR (0x01L << 1)
#define VO_INTF_VGA (0x01L << 2)
#define VO_INTF_BT656 (0x01L << 3)
#define VO_INTF_BT1120 (0x01L << 6)
#define VO_INTF_LCD (0x01L << 7)
#define VO_INTF_LCD_18BIT (0x01L << 10)
#define VO_INTF_LCD_24BIT (0x01L << 11)
#define VO_INTF_LCD_30BIT (0x01L << 12)
#define VO_INTF_MIPI (0x01L << 13)
#define VO_INTF_MIPI_SLAVE (0x01L << 14)
#define VO_INTF_HDMI (0x01L << 15)
#define VO_INTF_I80 (0x01L << 16)

#define VO_GAMMA_NODENUM 65
typedef CVI_U32 VO_INTF_TYPE_E;

typedef enum _VO_INTF_SYNC_E {
	VO_OUTPUT_PAL = 0, /* PAL standard*/
	VO_OUTPUT_NTSC, /* NTSC standard */

	VO_OUTPUT_1080P24, /* 1920 x 1080 at 24 Hz. */
	VO_OUTPUT_1080P25, /* 1920 x 1080 at 25 Hz. */
	VO_OUTPUT_1080P30, /* 1920 x 1080 at 30 Hz. */

	VO_OUTPUT_720P50, /* 1280 x  720 at 50 Hz. */
	VO_OUTPUT_720P60, /* 1280 x  720 at 60 Hz. */
	VO_OUTPUT_1080P50, /* 1920 x 1080 at 50 Hz. */
	VO_OUTPUT_1080P60, /* 1920 x 1080 at 60 Hz. */

	VO_OUTPUT_576P50, /* 720  x  576 at 50 Hz. */
	VO_OUTPUT_480P60, /* 720  x  480 at 60 Hz. */

	VO_OUTPUT_800x600_60, /* VESA 800 x 600 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1024x768_60, /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1280x1024_60, /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1366x768_60, /* VESA 1366 x 768 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1440x900_60, /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT Compliant */
	VO_OUTPUT_1280x800_60, /* 1280*800@60Hz VGA@60Hz*/
	VO_OUTPUT_1600x1200_60, /* VESA 1600 x 1200 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1680x1050_60, /* VESA 1680 x 1050 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1920x1200_60, /* VESA 1920 x 1600 at 60 Hz (non-interlaced) CVT (Reduced Blanking)*/
	VO_OUTPUT_640x480_60, /* VESA 640 x 480 at 60 Hz (non-interlaced) CVT */
	VO_OUTPUT_720x1280_60, /* For MIPI DSI Tx 720 x1280 at 60 Hz */
	VO_OUTPUT_1080x1920_60, /* For MIPI DSI Tx 1080x1920 at 60 Hz */
	VO_OUTPUT_480x800_60, /* For MIPI DSI Tx 480x800 at 60 Hz */
	VO_OUTPUT_1440P60, /* 2560 x 1440 at 60 Hz. */
	VO_OUTPUT_2160P24, /* 3840 x 2160 at 24 Hz. */
	VO_OUTPUT_2160P25, /* 3840 x 2160 at 25 Hz. */
	VO_OUTPUT_2160P30, /* 3840 x 2160 at 30 Hz. */
	VO_OUTPUT_2160P50, /* 3840 x 2160 at 50 Hz. */
	VO_OUTPUT_2160P60, /* 3840 x 2160 at 60 Hz. */
	VO_OUTPUT_USER, /* User timing. */

	VO_OUTPUT_BUTT

} VO_INTF_SYNC_E;

typedef enum _VO_CSC_MATRIX_E {
	VO_CSC_MATRIX_IDENTITY = 0,
	VO_CSC_MATRIX_BT601_TO_BT709,
	VO_CSC_MATRIX_BT709_TO_BT601,

	VO_CSC_MATRIX_BT601_TO_RGB_PC,
	VO_CSC_MATRIX_BT709_TO_RGB_PC,

	VO_CSC_MATRIX_RGB_TO_BT601_PC,
	VO_CSC_MATRIX_RGB_TO_BT709_PC,

	VO_CSC_MATRIX_RGB_TO_BT2020_PC,
	VO_CSC_MATRIX_BT2020_TO_RGB_PC,

	VO_CSC_MATRIX_RGB_TO_BT601_TV,
	VO_CSC_MATRIX_RGB_TO_BT709_TV,

	VO_CSC_MATRIX_BUTT
} VO_CSC_MATRIX_E;

typedef enum _VO_I80_FORMAT {
	VO_I80_FORMAT_RGB444 = 0,
	VO_I80_FORMAT_RGB565,
	VO_I80_FORMAT_RGB666,
	VO_I80_FORMAT_MAX
} VO_I80_FORMAT;

enum VO_PATTERN_MODE {
	VO_PAT_OFF = 0,
	VO_PAT_SNOW,
	VO_PAT_AUTO,
	VO_PAT_RED,
	VO_PAT_GREEN,
	VO_PAT_BLUE,
	VO_PAT_COLORBAR,
	VO_PAT_GRAY_GRAD_H,
	VO_PAT_GRAY_GRAD_V,
	VO_PAT_BLACK,
	VO_PAT_MAX,
};

enum VO_LVDS_LANE_ID {
	VO_LVDS_LANE_CLK = 0,
	VO_LVDS_LANE_0,
	VO_LVDS_LANE_1,
	VO_LVDS_LANE_2,
	VO_LVDS_LANE_3,
	VO_LVDS_LANE_MAX,
};


/*gpio*/
enum GPIO_NUM_E {
GPIOD_00 = 404,
GPIOD_01,   GPIOD_02,   GPIOD_03,   GPIOD_04,   GPIOD_05,
GPIOD_06,   GPIOD_07,   GPIOD_08,   GPIOD_09,   GPIOD_10,
GPIOD_11,
GPIOC_00 = 416,
GPIOC_01,   GPIOC_02,   GPIOC_03,   GPIOC_04,   GPIOC_05,
GPIOC_06,   GPIOC_07,   GPIOC_08,   GPIOC_09,   GPIOC_10,
GPIOC_11,   GPIOC_12,   GPIOC_13,   GPIOC_14,   GPIOC_15,
GPIOC_16,   GPIOC_17,   GPIOC_18,   GPIOC_19,   GPIOC_20,
GPIOC_21,   GPIOC_22,   GPIOC_23,   GPIOC_24,   GPIOC_25,
GPIOC_26,   GPIOC_27,   GPIOC_28,   GPIOC_29,   GPIOC_30,
GPIOC_31,
GPIOB_00 = 448,
GPIOB_01,   GPIOB_02,   GPIOB_03,   GPIOB_04,   GPIOB_05,
GPIOB_06,   GPIOB_07,   GPIOB_08,   GPIOB_09,   GPIOB_10,
GPIOB_11,   GPIOB_12,   GPIOB_13,   GPIOB_14,   GPIOB_15,
GPIOB_16,   GPIOB_17,   GPIOB_18,   GPIOB_19,   GPIOB_20,
GPIOB_21,   GPIOB_22,   GPIOB_23,   GPIOB_24,   GPIOB_25,
GPIOB_26,   GPIOB_27,   GPIOB_28,   GPIOB_29,   GPIOB_30,
GPIOB_31,
GPIOA_00 = 480,
GPIOA_01,   GPIOA_02,   GPIOA_03,   GPIOA_04,   GPIOA_05,
GPIOA_06,   GPIOA_07,   GPIOA_08,   GPIOA_09,   GPIOA_10,
GPIOA_11,   GPIOA_12,   GPIOA_13,   GPIOA_14,   GPIOA_15,
GPIOA_16,   GPIOA_17,   GPIOA_18,   GPIOA_19,   GPIOA_20,
GPIOA_21,   GPIOA_22,   GPIOA_23,   GPIOA_24,   GPIOA_25,
GPIOA_26,   GPIOA_27,   GPIOA_28,   GPIOA_29,   GPIOA_30,
GPIOA_31,
#ifdef ARCH_CV182X
GPIOE_00 = 380,
GPIOE_01,   GPIOE_02,   GPIOE_03,   GPIOE_04,   GPIOE_05,
GPIOE_06,   GPIOE_07,   GPIOE_08,   GPIOE_09,   GPIOE_10,
GPIOE_11,   GPIOE_12,   GPIOE_13,   GPIOE_14,   GPIOE_15,
GPIOE_16,   GPIOE_17,   GPIOE_18,   GPIOE_19,   GPIOE_20,
GPIOE_21,   GPIOE_22,   GPIOE_23,
GPIO_MIN = GPIOE_00,
#else
GPIO_MIN = GPIOD_00,
#endif
GPIO_MAX = GPIOA_31,
};

enum GPIO_ACTIVE_E {
	GPIO_ACTIVE_LOW,
	GPIO_ACTIVE_HIGH,
	GPIO_ACTIVE_BUFF
};

enum VO_LVDS_OUT_BIT_E {
	VO_LVDS_OUT_6BIT = 0,
	VO_LVDS_OUT_8BIT,
	VO_LVDS_OUT_10BIT,
	VO_LVDS_OUT_MAX,
};

enum VO_LVDS_MODE_E {
	VO_LVDS_MODE_JEIDA = 0,
	VO_LVDS_MODE_VESA,
	VO_LVDS_MODE_MAX,
};

/*
 * u32Priority: Video out overlay priority.
 * stRect: Rectangle of video output channel.
 */
typedef struct _VO_CHN_ATTR_S {
	CVI_U32 u32Priority;
	RECT_S stRect;
} VO_CHN_ATTR_S;

/*
 * u32ChnBufUsed: Channel buffer that been occupied.
 */
typedef struct _VO_QUERY_STATUS_S {
	CVI_U32 u32ChnBufUsed;
} VO_QUERY_STATUS_S;

/*
 * bSynm: sync mode(0:timing,as BT.656; 1:signal,as LCD)
 * bIop: interlaced or progressive display(0:i; 1:p)
 * u16FrameRate: frame-rate
 * u16Vact: vertical active area
 * u16Vbb: vertical back blank porch
 * u16Vfb: vertical front blank porch
 * u16Hact: horizontal active area
 * u16Hbb: horizontal back blank porch
 * u16Hfb: horizontal front blank porch
 * u16Hpw: horizontal pulse width
 * u16Vpw: vertical pulse width
 * bIdv: inverse data valid of output
 * bIhs: inverse horizontal synch signal
 * bIvs: inverse vertical synch signal
 */
typedef struct _VO_SYNC_INFO_S {
	CVI_BOOL bSynm;
	CVI_BOOL bIop;
	CVI_U16 u16FrameRate;

	CVI_U16 u16Vact;
	CVI_U16 u16Vbb;
	CVI_U16 u16Vfb;

	CVI_U16 u16Hact;
	CVI_U16 u16Hbb;
	CVI_U16 u16Hfb;

	CVI_U16 u16Hpw;
	CVI_U16 u16Vpw;

	CVI_BOOL bIdv;
	CVI_BOOL bIhs;
	CVI_BOOL bIvs;
} VO_SYNC_INFO_S;

/* Define I80's lane (0~3)
 *
 * CS: Chip Select
 * RS(DCX): Data/Command
 * WR: MCU Write to bus
 * RD: MCU Read from bus
 */
typedef struct _VO_I80_LANE_S {
	CVI_U8 CS;
	CVI_U8 RS;
	CVI_U8 WR;
	CVI_U8 RD;
} VO_I80_LANE_S;

/* Define I80's config
 *
 * lane_s: lane mapping
 * fmt: format of data
 * cycle_time: cycle time of WR/RD, unit ns, max 250
 */
typedef struct _VO_I80_CFG_S {
	VO_I80_LANE_S lane_s;
	VO_I80_FORMAT fmt;
	CVI_U16 cycle_time;
} VO_I80_CFG_S;

/* Define I80's cmd
 *
 * delay: ms to delay after instr
 * data_type: Data(1)/Command(0)
 * data: data to send
 */
typedef struct _VO_I80_INSTR_S {
	CVI_U8	delay;
	CVI_U8  data_type;
	CVI_U8	data;
} VO_I80_INSTR_S;

struct VO_LVDS_CTL_PIN_S {
	CVI_U32 gpio_num;
	enum GPIO_ACTIVE_E active;
};

/* Define LVDS's config
 *
 * lvds_vesa_mode: true for VESA mode; false for JEIDA mode
 * data_big_endian: true for big endian; true for little endian
 * lane_id: lane mapping, -1 no used
 * lane_pn_swap: lane pn-swap if true
 */
typedef struct _VO_LVDS_ATTR_S {
	enum VO_LVDS_OUT_BIT_E out_bits;
	CVI_U8 chn_num;
	CVI_BOOL data_big_endian;
	enum VO_LVDS_LANE_ID lane_id[VO_LVDS_LANE_MAX];
	CVI_BOOL lane_pn_swap[VO_LVDS_LANE_MAX];
	struct _VO_SYNC_INFO_S stSyncInfo;
	CVI_U32 pixelclock;
	struct VO_LVDS_CTL_PIN_S backlight_pin;
	enum VO_LVDS_MODE_E lvds_vesa_mode;
} VO_LVDS_ATTR_S;

/*
 * u32BgColor: Background color of a device, in RGB format.
 * enIntfType: Type of a VO interface.
 * enIntfSync: Type of a VO interface timing.
 * stSyncInfo: Information about VO interface timings if customed type.
 * sti80Cfg: attritube for i80 interface if IntfType is i80
 * stLvdsAttr: attritube for lvds interface if IntfType is lvds
 */
typedef struct _VO_PUB_ATTR_S {
	CVI_U32 u32BgColor;
	VO_INTF_TYPE_E enIntfType;
	VO_INTF_SYNC_E enIntfSync;
	VO_SYNC_INFO_S stSyncInfo;
	union {
		VO_I80_CFG_S sti80Cfg;
		VO_LVDS_ATTR_S stLvdsAttr;
	};
} VO_PUB_ATTR_S;

/*
 * stDispRect: Display resolution
 * stImageSize: Original ImageSize.
 *              Only useful if vo support scaling, otherwise, it should be the same width stDispRect.
 * u32DispFrmRt: frame rate.
 * enPixFormat: Pixel format of the video layer
 */
typedef struct _VO_VIDEO_LAYER_ATTR_S {
	RECT_S stDispRect;
	SIZE_S stImageSize;
	CVI_U32 u32DispFrmRt;
	PIXEL_FORMAT_E enPixFormat;
} VO_VIDEO_LAYER_ATTR_S;

/*
 * enCscMatrix: CSC matrix
 */
typedef struct _VO_CSC_S {
	VO_CSC_MATRIX_E enCscMatrix;
} VO_CSC_S;

typedef CVI_S32 (*pfnVoDevPmOps)(void *pvData);

typedef struct _VO_PM_OPS_S {
	pfnVoDevPmOps pfnPanelSuspend;
	pfnVoDevPmOps pfnPanelResume;
} VO_PM_OPS_S;

typedef struct _VO_GAMMA_INFO_S {
	VO_DEV s32VoDev;
	CVI_BOOL enable;
	CVI_BOOL osd_apply;
	CVI_U32 value[VO_GAMMA_NODENUM];
} VO_GAMMA_INFO_S;

typedef struct _VO_BIN_INFO_S {
	VO_GAMMA_INFO_S gamma_info;
	CVI_U32 guard_magic;
} VO_BIN_INFO_S;



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __CVI_COMM_VO_H__ */
