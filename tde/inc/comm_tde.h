/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_tde.h
 * Description:
 *   tde interfaces.
 */

#ifndef COMMON_TDE_H
#define COMMON_TDE_H

//#include "cvi_type.h"
#include "tde_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/* tde start err no. */
#define ERR_TDE_BASE ((int)(((0x80UL + 0x20UL) << 24) | (100 << 16) | (4 << 13) | 1))

typedef enum {
    ERR_TDE_DEV_NOT_OPEN = ERR_TDE_BASE, /* tde device not open yet */
    ERR_TDE_DEV_OPEN_FAILED,                /* open tde device failed */
    ERR_TDE_NULL_PTR,                       /* input parameters contain null ptr */
    ERR_TDE_NO_MEM,                         /* malloc failed  */
    ERR_TDE_INVALID_HANDLE,                 /* invalid job handle */
    ERR_TDE_INVALID_PARAM,                  /* invalid parameter */
    ERR_TDE_NOT_ALIGNED,                    /* aligned error for position, stride, width */
    ERR_TDE_MINIFICATION,                   /* invalid minification */
    ERR_TDE_CLIP_AREA,                      /* clip area and operation area have no intersection */
    ERR_TDE_JOB_TIMEOUT,                    /* blocked job wait timeout */
    ERR_TDE_UNSUPPORTED_OPERATION,          /* unsupported operation */
    ERR_TDE_QUERY_TIMEOUT,                  /* query time out */
    ERR_TDE_INTERRUPT,                      /* blocked job was interrupted */
    ERR_TDE_BUTT,
} tde_err_code;

/* RGB and packet YUV formats and semi-planar YUV format */
typedef enum {
    TDE_COLOR_FORMAT_RGB444 = 0,       /* RGB444 format */
    TDE_COLOR_FORMAT_BGR444,           /* BGR444 format */
    TDE_COLOR_FORMAT_RGB555,           /* RGB555 format */
    TDE_COLOR_FORMAT_BGR555,           /* BGR555 format */
    TDE_COLOR_FORMAT_RGB565,           /* RGB565 format */
    TDE_COLOR_FORMAT_BGR565,           /* BGR565 format */
    TDE_COLOR_FORMAT_RGB888,           /* RGB888 format */
    TDE_COLOR_FORMAT_BGR888,           /* BGR888 format */
    TDE_COLOR_FORMAT_ARGB4444,         /* ARGB4444 format */
    TDE_COLOR_FORMAT_ABGR4444,         /* ABGR4444 format */
    TDE_COLOR_FORMAT_RGBA4444,         /* RGBA4444 format */
    TDE_COLOR_FORMAT_BGRA4444,         /* BGRA4444 format */
    TDE_COLOR_FORMAT_ARGB1555,         /* ARGB1555 format */
    TDE_COLOR_FORMAT_ABGR1555,         /* ABGR1555 format */
    TDE_COLOR_FORMAT_RGBA1555,         /* RGBA1555 format */
    TDE_COLOR_FORMAT_BGRA1555,         /* BGRA1555 format */
    TDE_COLOR_FORMAT_ARGB8565,         /* ARGB8565 format */
    TDE_COLOR_FORMAT_ABGR8565,         /* ABGR8565 format */
    TDE_COLOR_FORMAT_RGBA8565,         /* RGBA8565 format */
    TDE_COLOR_FORMAT_BGRA8565,         /* BGRA8565 format */
    TDE_COLOR_FORMAT_ARGB8888,         /* ARGB8888 format */
    TDE_COLOR_FORMAT_ABGR8888,         /* ABGR8888 format */
    TDE_COLOR_FORMAT_RGBA8888,         /* RGBA8888 format */
    TDE_COLOR_FORMAT_BGRA8888,         /* BGRA8888 format */
    TDE_COLOR_FORMAT_RABG8888,         /* RABG8888 format */
    /* 1-bit palette format without alpha component. Each pixel occupies one bit. */
    TDE_COLOR_FORMAT_CLUT1,
    /* 2-bit palette format without alpha component. Each pixel occupies two bits. */
    TDE_COLOR_FORMAT_CLUT2,
    /* 4-bit palette format without alpha component. Each pixel occupies four bits. */
    TDE_COLOR_FORMAT_CLUT4,
    /* 8-bit palette format without alpha component. Each pixel occupies eight bits. */
    TDE_COLOR_FORMAT_CLUT8,
    /* 4-bit palette format with alpha component. Each pixel occupies 8 bit. */
    TDE_COLOR_FORMAT_ACLUT44,
    /* 8-bit palette format with alpha component. Each pixel occupies 16 bit. */
    TDE_COLOR_FORMAT_ACLUT88,
    TDE_COLOR_FORMAT_A1,                 /* Alpha format. Each pixel occupies one bit. */
    TDE_COLOR_FORMAT_A8,                 /* Alpha format. Each pixel occupies eight bits. */
    TDE_COLOR_FORMAT_YCbCr888,           /* 33：YCbCr888 */
    TDE_COLOR_FORMAT_AYCbCr8888,         /* 34：AYCbCr8888 */
    TDE_COLOR_FORMAT_YCbCr422,           /* 35：PKYUYV */
    TDE_COLOR_FORMAT_PKGYVYU,            /* 36：PKYVYU */
    TDE_COLOR_FORMAT_PKGUYVY,            /* 37：PKUYVY */
    TDE_COLOR_FORMAT_PKGVYUY,            /* 38：PKVYUY */
    TDE_COLOR_FORMAT_PKGVUYY,            /* 39：PKVUYY */
    TDE_COLOR_FORMAT_PKGYYUV,            /* 40：PKYYUV */
    TDE_COLOR_FORMAT_PKGUVYY,            /* 41：PKUVYY */
    TDE_COLOR_FORMAT_PKGYYVU,            /* 42：PKYYVU */
    TDE_COLOR_FORMAT_JPG_YCbCr400MBP,    /* 43：SP400 */
    TDE_COLOR_FORMAT_JPG_YCbCr422MBHP,   /* 44：SP422h(1*2)(像素宽度为偶数) */
    TDE_COLOR_FORMAT_JPG_YCbCr422MBVP,   /* 45：SP422V(2*1)(像素高度为偶数) */
    TDE_COLOR_FORMAT_MP1_YCbCr420MBP,    /* 46：SP420(cd hi像素宽高为偶数) */
    TDE_COLOR_FORMAT_MP2_YCbCr420MBP,    /* 47：SP420(cd hi像素宽高为偶数) */
    TDE_COLOR_FORMAT_MP2_YCbCr420MBI,    /* 48：SP420(cd hi像素宽高为偶数) */
    TDE_COLOR_FORMAT_JPG_YCbCr420MBP,    /* 49：SP420(cd hi像素宽高为偶数) */
    TDE_COLOR_FORMAT_JPG_YCbCr444MBP,    /* 50：SP444(cd hi像素宽高为偶数) */
    TDE_COLOR_FORMAT_MAX                 /* 51：End of enumeration */
} tde_color_format;

/* Definition of the semi-planar YUV format */
typedef enum {
    TDE_MB_COLOR_FORMAT_JPG_YCbCr400MBP = 0, /* Semi-planar YUV400 format, for JPG decoding */
    /* Semi-planar YUV422 format, horizontal sampling, for JPG decoding */
    TDE_MB_COLOR_FORMAT_JPG_YCbCr422MBHP,
    TDE_MB_COLOR_FORMAT_JPG_YCbCr422MBVP, /* Semi-planar YUV422 format, vertical sampling, for JPG decoding */
    TDE_MB_COLOR_FORMAT_MP1_YCbCr420MBP,  /* Semi-planar YUV420 format */
    TDE_MB_COLOR_FORMAT_MP2_YCbCr420MBP,  /* Semi-planar YUV420 format */
    TDE_MB_COLOR_FORMAT_MP2_YCbCr420MBI,  /* Semi-planar YUV420 format */
    TDE_MB_COLOR_FORMAT_JPG_YCbCr420MBP,  /* Semi-planar YUV420 format, for JPG pictures */
    TDE_MB_COLOR_FORMAT_JPG_YCbCr444MBP,  /* Semi-planar YUV444 format, for JPG pictures */
    TDE_MB_COLOR_FORMAT_MAX
} tde_mb_color_format;

/* Structure of the bitmap information set by customers */
typedef struct {
    td_phys_addr_t phys_addr; /* Header address of a bitmap or the Y component */
    unsigned int phys_len;
    tde_color_format color_format; /* Color format */
    unsigned int height; /* Bitmap height */
    unsigned int width; /* Bitmap width */
    unsigned int stride; /* Stride of a bitmap or the Y component */
    unsigned char is_ycbcr_clut; /* Whether the CLUT is in the YCbCr space. */
    unsigned char alpha_max_is_255; /* The maximum alpha value of a bitmap is 255 or 128. */
    unsigned char support_alpha_ex_1555; /* Whether to enable the alpha extension of an ARGB1555 bitmap. */
    unsigned char alpha0;        /* Values of alpha0 and alpha1, used as the ARGB1555 format */
    unsigned char alpha1;        /* Values of alpha0 and alpha1, used as the ARGB1555 format */
    td_phys_addr_t cbcr_phys_addr;    /* Address of the CbCr component, pilot */
    unsigned int cbcr_phys_len;
    unsigned int cbcr_stride;  /* Stride of the CbCr component, pilot */
    /* <Address of the color look-up table (CLUT), for color extension or color correction */
    td_phys_addr_t clut_phys_addr;
    unsigned int clut_phys_len;
} tde_surface;

/* Definition of the semi-planar YUV data */
typedef struct {
    tde_mb_color_format mb_color_format; /* YUV format */
    td_phys_addr_t y_addr;             /* Physical address of the Y component */
    unsigned int y_len;
    unsigned int y_width;            /* Width of the Y component */
    unsigned int y_height;           /* Height of the Y component */
    unsigned int y_stride;           /* Stride of the Y component, indicating bytes in each row */
    td_phys_addr_t cbcr_phys_addr;      /* Width of the UV component */
    unsigned int cbcr_phys_len;
    unsigned int cbcr_stride;        /* Stride of the UV component, indicating the bytes in each row */
} tde_mb_surface;

/* Definition of the TDE rectangle */
typedef struct {
    int pos_x;   /* Horizontal coordinate */
    int pos_y;   /* Vertical coordinate */
    unsigned int width;  /* Width */
    unsigned int height; /* Height */
} tde_rect;

/* dma module */
typedef struct {
    tde_surface *dst_surface;
    tde_rect *dst_rect;
} tde_none_src;

/* single source */
typedef struct {
    tde_surface *src_surface;
    tde_surface *dst_surface;
    tde_rect *src_rect;
    tde_rect *dst_rect;
} tde_single_src;

/* mb source */
typedef struct {
    tde_mb_surface *mb_surface;
    tde_surface *dst_surface;
    tde_rect *src_rect;
    tde_rect *dst_rect;
} tde_mb_src;

/* double source */
typedef struct {
    tde_surface *bg_surface;
    tde_surface *fg_surface;
    tde_surface *dst_surface;
    tde_rect *bg_rect;
    tde_rect *fg_rect;
    tde_rect *dst_rect;
} tde_double_src;

/* Logical operation type */
typedef enum {
    TDE_ALPHA_BLENDING_NONE = 0x0,     /* No alpha and raster of operation (ROP) blending */
    TDE_ALPHA_BLENDING_BLEND = 0x1,    /* Alpha blending */
    TDE_ALPHA_BLENDING_ROP = 0x2,      /* ROP blending */
    TDE_ALPHA_BLENDING_COLORIZE = 0x4, /* Colorize operation */
    TDE_ALPHA_BLENDING_MAX = 0x8       /* End of enumeration */
} tde_alpha_blending;

/* Definition of ROP codes */
typedef enum {
    TDE_ROP_BLACK = 0,   /* Blackness */
    TDE_ROP_NOTMERGEPEN, /* ~(S2 | S1) */
    TDE_ROP_MASKNOTPEN,  /* ~S2&S1 */
    TDE_ROP_NOTCOPYPEN,  /* ~S2 */
    TDE_ROP_MASKPENNOT,  /* S2&~S1 */
    TDE_ROP_NOT,         /* ~S1 */
    TDE_ROP_XORPEN,      /* S2^S1 */
    TDE_ROP_NOTMASKPEN,  /* ~(S2 & S1) */
    TDE_ROP_MASKPEN,     /* S2&S1 */
    TDE_ROP_NOTXORPEN,   /* ~(S2^S1) */
    TDE_ROP_NOP,         /* S1 */
    TDE_ROP_MERGENOTPEN, /* ~S2|S1 */
    TDE_ROP_COPYPEN,     /* S2 */
    TDE_ROP_MERGEPENNOT, /* S2|~S1 */
    TDE_ROP_MERGEPEN,    /* S2|S1 */
    TDE_ROP_WHITE,       /* Whiteness */
    TDE_ROP_MAX
} tde_rop_mode;

/* Definition of the blit mirror */
typedef enum {
    TDE_MIRROR_NONE = 0,   /* No mirror */
    TDE_MIRROR_HOR, /* Horizontal mirror */
    TDE_MIRROR_VER,   /* Vertical mirror */
    TDE_MIRROR_BOTH,       /* Horizontal and vertical mirror */
    TDE_MIRROR_MAX
} tde_mirror_mode;

/* Clip operation type */
typedef enum {
    TDE_CLIP_MODE_NONE = 0, /* No clip */
    TDE_CLIP_MODE_INSIDE,   /* Clip the data within the rectangle to output and discard others */
    TDE_CLIP_MODE_OUTSIDE,  /* Clip the data outside the rectangle to output and discard others */
    TDE_CLIP_MODE_MAX
} tde_clip_mode;

/* Scaling mode for the macroblock */
typedef enum {
    TDE_MB_RESIZE_NONE = 0,       /* No scaling */
    TDE_MB_RESIZE_QUALITY_LOW,    /* Low-quality scaling */
    TDE_MB_RESIZE_QUALITY_MIDDLE, /* Medium-quality scaling */
    TDE_MB_RESIZE_QUALITY_HIGH,   /* High-quality scaling */
    TDE_MB_RESIZE_MAX
} tde_mb_resize;

/* Definition of fill colors */
typedef struct {
    tde_color_format color_format; /* TDE pixel format */
    unsigned int color_value;         /* Fill colors that vary according to pixel formats */
} tde_fill_color;

/* Definition of colorkey modes */
typedef enum {
    TDE_COLORKEY_MODE_NONE = 0,   /* No colorkey */
    /* When performing the colorkey operation on the foreground bitmap,
    you need to perform this operation before the CLUT for color extension
    and perform this operation after the CLUT for color correction. */
    TDE_COLORKEY_MODE_FG,
    TDE_COLORKEY_MODE_BG, /* Perform the colorkey operation on the background bitmap */
    TDE_COLORKEY_MODE_MAX
} tde_colorkey_mode;

/* Definition of colorkey range */
typedef struct {
    unsigned char min_component;   /* Minimum value of a component */
    unsigned char max_component;   /* Maximum value of a component */
    unsigned char is_component_out;    /* The colorkey of a component is within or beyond the range. */
    unsigned char is_component_ignore; /* Whether to ignore a component. */
    unsigned char component_mask;  /* Component mask */
} tde_colorkey_component;

/* Definition of colorkey values */
typedef union {
    struct {
        tde_colorkey_component alpha; /* Alpha component */
        tde_colorkey_component red;   /* Red component */
        tde_colorkey_component green; /* Green component */
        tde_colorkey_component blue;  /* Blue component */
    } argb_colorkey;                     /* AUTO:tde_colorkey_mode:TDE_COLORKEY_MODE_NONE; */
    struct {
        tde_colorkey_component alpha; /* Alpha component */
        tde_colorkey_component y;     /* Y component */
        tde_colorkey_component cb;    /* Cb component */
        tde_colorkey_component cr;    /* Cr component */
    } ycbcr_colorkey;                    /* AUTO:tde_colorkey_mode:TDE_COLORKEY_MODE_FG; */
    struct {
        tde_colorkey_component alpha; /* Alpha component */
        tde_colorkey_component clut;  /* Palette component */
    } clut_colorkey;                     /* AUTO:tde_colorkey_mode:TDE_COLORKEY_MODE_BG; */
} tde_colorkey;

/* Definition of alpha output sources */
typedef enum {
    TDE_OUT_ALPHA_FROM_NORM = 0,    /* Output from the result of alpha blending or anti-flicker */
    TDE_OUT_ALPHA_FROM_BG,  /* Output from the background bitmap */
    TDE_OUT_ALPHA_FROM_FG,  /* Output from the foreground bitmap */
    TDE_OUT_ALPHA_FROM_GLOBALALPHA, /* Output from the global alpha */
    TDE_OUT_ALPHA_FROM_MAX
} tde_out_alpha_from;

/* Definition of filtering */
typedef enum {
    TDE_FILTER_MODE_COLOR = 0, /* Filter the color */
    TDE_FILTER_MODE_ALPHA,     /* Filter the alpha channel */
    TDE_FILTER_MODE_BOTH,      /* Filter the color and alpha channel */
    TDE_FILTER_MODE_NONE,      /* No filter */
    TDE_FILTER_MODE_MAX
} tde_filter_mode;

/* blend mode */
typedef enum {
    TDE_BLEND_ZERO = 0x0,
    TDE_BLEND_ONE,
    TDE_BLEND_SRC2COLOR,
    TDE_BLEND_INVSRC2COLOR,
    TDE_BLEND_SRC2ALPHA,
    TDE_BLEND_INVSRC2ALPHA,
    TDE_BLEND_SRC1COLOR,
    TDE_BLEND_INVSRC1COLOR,
    TDE_BLEND_SRC1ALPHA,
    TDE_BLEND_INVSRC1ALPHA,
    TDE_BLEND_SRC2ALPHASAT,
    TDE_BLEND_MAX
} tde_blend_mode;

/* Alpha blending command. You can set parameters or select Porter or Duff. */
/* pixel = (source * fs + destination * fd),
   sa = source alpha,
   da = destination alpha */
typedef enum {
    TDE_BLEND_CMD_NONE = 0x0, /* fs: sa      fd: 1.0-sa */
    TDE_BLEND_CMD_CLEAR,      /* fs: 0.0     fd: 0.0 */
    TDE_BLEND_CMD_SRC,        /* fs: 1.0     fd: 0.0 */
    TDE_BLEND_CMD_SRCOVER,    /* fs: 1.0     fd: 1.0-sa */
    TDE_BLEND_CMD_DSTOVER,    /* fs: 1.0-da  fd: 1.0 */
    TDE_BLEND_CMD_SRCIN,      /* fs: da      fd: 0.0 */
    TDE_BLEND_CMD_DSTIN,      /* fs: 0.0     fd: sa */
    TDE_BLEND_CMD_SRCOUT,     /* fs: 1.0-da  fd: 0.0 */
    TDE_BLEND_CMD_DSTOUT,     /* fs: 0.0     fd: 1.0-sa */
    TDE_BLEND_CMD_SRCATOP,    /* fs: da      fd: 1.0-sa */
    TDE_BLEND_CMD_DSTATOP,    /* fs: 1.0-da  fd: sa */
    TDE_BLEND_CMD_ADD,        /* fs: 1.0     fd: 1.0 */
    TDE_BLEND_CMD_XOR,        /* fs: 1.0-da  fd: 1.0-sa */
    TDE_BLEND_CMD_DST,        /* fs: 0.0     fd: 1.0 */
    TDE_BLEND_CMD_CONFIG,     /* You can set the parameteres. */
    TDE_BLEND_CMD_MAX
} tde_blend_cmd;

/* Options for the alpha blending operation */
typedef struct {
    unsigned char global_alpha_en;       /* Global alpha enable */
    unsigned char pixel_alpha_en;        /* Pixel alpha enable */
    unsigned char src1_alpha_premulti;       /* Src1 alpha premultiply enable */
    unsigned char src2_alpha_premulti;       /* Src2 alpha premultiply enable */
    tde_blend_cmd blend_cmd;        /* Alpha blending command */
    /* Src1 blending mode select. It is valid when blend_cmd is set to TDE_BLEND_CMD_CONFIG. */
    tde_blend_mode src1_blend_mode;
    /* Src2 blending mode select. It is valid when blend_cmd is set to TDE_BLEND_CMD_CONFIG. */
    tde_blend_mode src2_blend_mode;
} tde_blend_opt;

/* CSC parameter option */
typedef struct {
    unsigned char src_csc_user_en;  /* User-defined ICSC parameter enable */
    unsigned char src_csc_param_reload_en; /* User-defined ICSC parameter reload enable */
    unsigned char dst_csc_user_en;  /* User-defined OCSC parameter enable */
    unsigned char dst_csc_param_reload_en; /* User-defined OCSC parameter reload enable */
    td_phys_addr_t src_csc_param_addr; /* ICSC parameter address. The address must be 128-bit aligned. */
    int src_csc_param_len;
    td_phys_addr_t dst_csc_param_addr; /* OCSC parameter address. The address must be 128-bit aligned. */
    int dst_csc_param_len;
} tde_csc_opt;

/* Definition of blit operation options */
typedef struct {
    tde_alpha_blending alpha_blending_cmd; /* Logical operation type */

    tde_rop_mode rop_color; /* ROP type of the color space */

    tde_rop_mode rop_alpha; /* ROP type of the alpha component */

    tde_colorkey_mode colorkey_mode; /* Colorkey mode */

    tde_colorkey colorkey_value; /* Colorkey value */

    tde_clip_mode clip_mode; /* Perform the clip operation within or beyond the area */

    tde_rect clip_rect; /* Definition of the clipping area */

    unsigned char resize; /* Whether to scale */

    tde_filter_mode filter_mode; /* Filtering mode during scaling */

    tde_mirror_mode mirror; /* Mirror type */

    unsigned char clut_reload; /* Whether to reload the CLUT */

    unsigned char global_alpha; /* Global alpha value */

    tde_out_alpha_from out_alpha_from; /* Source of the output alpha */

    unsigned int color_resize; /* Colorize value */

    tde_blend_opt blend_opt;

    tde_csc_opt csc_opt;
    unsigned char is_compress;
    unsigned char is_decompress;
} tde_opt;

/* Definition of macroblock operation options */
typedef struct {
    tde_clip_mode clip_mode; /* Clip mode */

    tde_rect clip_rect; /* Definition of the clipping area */

    tde_mb_resize resize_en; /* Scaling information */

    /* If the alpha value is not set, the maximum alpha value is output by default. */
    unsigned char is_set_out_alpha;

    unsigned char out_alpha; /* Global alpha for operation */
} tde_mb_opt;

/* Definition of the pattern filling operation */
typedef struct {
    tde_alpha_blending alpha_blending_cmd; /* Logical operation type */

    tde_rop_mode rop_color; /* ROP type of the color space */

    tde_rop_mode rop_alpha; /* ROP type of the alpha component */

    tde_colorkey_mode colorkey_mode; /* Colorkey mode */

    tde_colorkey colorkey_value; /* Colorkey value */

    tde_clip_mode clip_mode; /* Clip mode */

    tde_rect clip_rect; /* Clipping area */

    unsigned char clut_reload; /* Whether to reload the CLUT */

    unsigned char global_alpha; /* Global alpha */

    tde_out_alpha_from out_alpha_from; /* Source of the output alpha */

    unsigned int color_resize; /* Colorize value */

    tde_blend_opt blend_opt; /* Options of the blending operation */

    tde_csc_opt csc_opt; /* CSC parameter option */
} tde_pattern_fill_opt;

/* Definition of rotation directions */
typedef enum {
    TDE_ROTATE_CLOCKWISE_90 = 0, /* Rotate 90 degree clockwise */
    TDE_ROTATE_CLOCKWISE_180,    /* Rotate 180 degree clockwise */
    TDE_ROTATE_CLOCKWISE_270,    /* Rotate 270 degree clockwise */
    TDE_ROTATE_MAX
} tde_rotate_angle;

/* Definition of corner_rect */
typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int inner_color;
    unsigned int outer_color;
} tde_corner_rect_info;

typedef struct {
    tde_rect *corner_rect_region;
    tde_corner_rect_info *corner_rect_info;
} tde_corner_rect;

typedef struct {
    int start_x;
    int start_y;
    int end_x;
    int end_y;
    unsigned int thick;
    unsigned int color;
} tde_line;

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
#endif /* COMMON_TDE_H */
