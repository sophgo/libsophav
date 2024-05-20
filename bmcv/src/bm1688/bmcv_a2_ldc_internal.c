#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "bmcv_internal.h"
#include "bmcv_a2_ldc_internal.h"
#ifdef __linux__
#include <sys/ioctl.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#endif

bm_meshdata_all ldc_meshdata;

bm_tsk_mesh_attr_s tsk_mesh[LDC_MAX_TSK_MESH];

static inline void COMMON_GetPicBufferConfig(u32 u32Width, u32 u32Height,
                                             PIXEL_FORMAT_E enPixelFormat, DATA_BITWIDTH_E enBitWidth,
                                             COMPRESS_MODE_E enCmpMode, u32 u32Align, bm_vb_cal_config_s *pstCalConfig)
{
    u8  u8BitWidth = 0;
    u32 u32VBSize = 0;
    u32 u32AlignHeight = 0;
    u32 u32MainStride = 0;
    u32 u32CStride = 0;
    u32 u32MainSize = 0;
    u32 u32YSize = 0;
    u32 u32CSize = 0;

    /* u32Align: 0 is automatic mode, alignment size following system. Non-0 for specified alignment size */
    if (u32Align == 0)
        u32Align = DEFAULT_ALIGN;
    else if (u32Align > MAX_ALIGN)
        u32Align = MAX_ALIGN;

    switch (enBitWidth) {
    case DATA_BITWIDTH_8: {
        u8BitWidth = 8;
        break;
    }
    case DATA_BITWIDTH_10: {
        u8BitWidth = 10;
        break;
    }
    case DATA_BITWIDTH_12: {
        u8BitWidth = 12;
        break;
    }
    case DATA_BITWIDTH_14: {
        u8BitWidth = 14;
        break;
    }
    case DATA_BITWIDTH_16: {
        u8BitWidth = 16;
        break;
    }
    default: {
        u8BitWidth = 0;
        break;
    }
    }

    if ((enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420)
        || (enPixelFormat == PIXEL_FORMAT_NV12)
        || (enPixelFormat == PIXEL_FORMAT_NV21)) {
        u32AlignHeight = ALIGN(u32Height, 2);
    } else
        u32AlignHeight = u32Height;

    if (enCmpMode == COMPRESS_MODE_NONE) {
        u32MainStride = ALIGN((u32Width * u8BitWidth + 7) >> 3, u32Align);
        u32YSize = u32MainStride * u32AlignHeight;

        if (enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420) {
            u32CStride = ALIGN(((u32Width >> 1) * u8BitWidth + 7) >> 3, u32Align);
            u32CSize = (u32CStride * u32AlignHeight) >> 1;

            u32MainStride = u32CStride * 2;
            u32YSize = u32MainStride * u32AlignHeight;
            u32MainSize = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_422) {
            u32CStride = ALIGN(((u32Width >> 1) * u8BitWidth + 7) >> 3, u32Align);
            u32CSize = u32CStride * u32AlignHeight;

            u32MainSize = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_RGB_888_PLANAR ||
                enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR ||
                enPixelFormat == PIXEL_FORMAT_HSV_888_PLANAR ||
                enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_444) {
            u32CStride = u32MainStride;
            u32CSize = u32YSize;

            u32MainSize = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_RGB_BAYER_12BPP) {
            u32MainSize = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_YUV_400) {
            u32MainSize = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_NV12 || enPixelFormat == PIXEL_FORMAT_NV21) {
            u32CStride = ALIGN((u32Width * u8BitWidth + 7) >> 3, u32Align);
            u32CSize = (u32CStride * u32AlignHeight) >> 1;

            u32MainSize = u32YSize + u32CSize;
            pstCalConfig->plane_num = 2;
        } else if (enPixelFormat == PIXEL_FORMAT_NV16 || enPixelFormat == PIXEL_FORMAT_NV61) {
            u32CStride = ALIGN((u32Width * u8BitWidth + 7) >> 3, u32Align);
            u32CSize = u32CStride * u32AlignHeight;

            u32MainSize = u32YSize + u32CSize;
            pstCalConfig->plane_num = 2;
        } else if (enPixelFormat == PIXEL_FORMAT_YUYV || enPixelFormat == PIXEL_FORMAT_YVYU ||
                enPixelFormat == PIXEL_FORMAT_UYVY || enPixelFormat == PIXEL_FORMAT_VYUY) {
            u32MainStride = ALIGN(((u32Width * u8BitWidth + 7) >> 3) * 2, u32Align);
            u32YSize = u32MainStride * u32AlignHeight;
            u32MainSize = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_ARGB_1555 || enPixelFormat == PIXEL_FORMAT_ARGB_4444) {
            // packed format
            u32MainStride = ALIGN((u32Width * 16 + 7) >> 3, u32Align);
            u32YSize = u32MainStride * u32AlignHeight;
            u32MainSize = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_ARGB_8888) {
            // packed format
            u32MainStride = ALIGN((u32Width * 32 + 7) >> 3, u32Align);
            u32YSize = u32MainStride * u32AlignHeight;
            u32MainSize = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_FP32_C3_PLANAR) {
            u32MainStride = ALIGN(((u32Width * u8BitWidth + 7) >> 3) * 4, u32Align);
            u32YSize = u32MainStride * u32AlignHeight;
            u32CStride = u32MainStride;
            u32CSize = u32YSize;
            u32MainSize = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_FP16_C3_PLANAR ||
                enPixelFormat == PIXEL_FORMAT_BF16_C3_PLANAR) {
            u32MainStride = ALIGN(((u32Width * u8BitWidth + 7) >> 3) * 2, u32Align);
            u32YSize = u32MainStride * u32AlignHeight;
            u32CStride = u32MainStride;
            u32CSize = u32YSize;
            u32MainSize = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_INT8_C3_PLANAR ||
                enPixelFormat == PIXEL_FORMAT_UINT8_C3_PLANAR) {
            u32CStride = u32MainStride;
            u32CSize = u32YSize;
            u32MainSize = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else {
            // packed format
            u32MainStride = ALIGN(((u32Width * u8BitWidth + 7) >> 3) * 3, u32Align);
            u32YSize = u32MainStride * u32AlignHeight;
            u32MainSize = u32YSize;
            pstCalConfig->plane_num = 1;
        }

        u32VBSize = u32MainSize;
    } else {
        // TODO: compression mode
        pstCalConfig->plane_num = 0;
    }

    pstCalConfig->u32VBSize = u32VBSize;
    pstCalConfig->u32MainStride = u32MainStride;
    pstCalConfig->u32CStride = u32CStride;
    pstCalConfig->u32MainYSize = u32YSize;
    pstCalConfig->u32MainCSize = u32CSize;
    pstCalConfig->u32MainSize = u32MainSize;
    pstCalConfig->u16AddrAlign = u32Align;
}

bm_status_t bm_ldc_comm_cfg_frame(bm_handle_t handle,
                                  bm_device_mem_t dmem,
                                  SIZE_S *stSize,
                                  PIXEL_FORMAT_E enPixelFormat,
                                  VIDEO_FRAME_INFO_S *pstVideoFrame)
{
    bm_vb_cal_config_s st_vb_cal_config;
    if (pstVideoFrame == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Null pointer! \n");
        return BM_ERR_FAILURE;
    }

    COMMON_GetPicBufferConfig(stSize->u32Width, stSize->u32Height, enPixelFormat, DATA_BITWIDTH_8,
                              COMPRESS_MODE_NONE, LDC_ALIGN, &st_vb_cal_config);

    memset(pstVideoFrame, 0, sizeof(*pstVideoFrame));
    pstVideoFrame->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    pstVideoFrame->stVFrame.enPixelFormat = enPixelFormat;
    pstVideoFrame->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    pstVideoFrame->stVFrame.enColorGamut = COLOR_GAMUT_BT601;
    pstVideoFrame->stVFrame.u32Width = stSize->u32Width;
    pstVideoFrame->stVFrame.u32Height = stSize->u32Height;
    pstVideoFrame->stVFrame.u32Stride[0] = st_vb_cal_config.u32MainStride;
    pstVideoFrame->stVFrame.u32Stride[1] = st_vb_cal_config.u32CStride;
    pstVideoFrame->stVFrame.u32TimeRef = 0;
    pstVideoFrame->stVFrame.u64PTS = 0;
    pstVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;
    pstVideoFrame->u32PoolId = 0;
    pstVideoFrame->stVFrame.u32Length[0] = st_vb_cal_config.u32MainYSize;
    pstVideoFrame->stVFrame.u32Length[1] = st_vb_cal_config.u32MainCSize;

    unsigned long long phy_addr = dmem.u.device.device_addr;

    pstVideoFrame->stVFrame.u64PhyAddr[0] = phy_addr;
    pstVideoFrame->stVFrame.u64PhyAddr[1] = pstVideoFrame->stVFrame.u64PhyAddr[0]
        + ALIGN(st_vb_cal_config.u32MainYSize, st_vb_cal_config.u16AddrAlign);

    if (st_vb_cal_config.plane_num == 3) {
        pstVideoFrame->stVFrame.u32Stride[2] = st_vb_cal_config.u32CStride;
        pstVideoFrame->stVFrame.u32Length[2] = st_vb_cal_config.u32MainCSize;
        pstVideoFrame->stVFrame.u64PhyAddr[2] = pstVideoFrame->stVFrame.u64PhyAddr[1]
            + ALIGN(st_vb_cal_config.u32MainCSize, st_vb_cal_config.u16AddrAlign);
    }
    return BM_SUCCESS;
}

static u8 ldc_get_valid_tsk_mesh_by_name(const char *name)
{
    u8 i = LDC_MAX_TSK_MESH;
    if (!name) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "tsk mesh name is null.\n");
        return i;
    }

    for (i = 0; i < LDC_MAX_TSK_MESH; i++) {
        if (strcmp(tsk_mesh[i].Name, name) == 0 && tsk_mesh[i].paddr && tsk_mesh[i].vaddr) {
            // for debug
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "got remain tsk mesh[%d-%s-%llu]\n", i, tsk_mesh[i].Name, tsk_mesh[i].paddr);
            break;
        }
    }

    if (i == LDC_MAX_TSK_MESH) {
        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "start alloc new tsk mesh[%s]\n", name);
    }
    return i;
}

static u8 ldc_get_valid_tsk_mesh_by_name2(const char *name)
{
    u8 i = LDC_MAX_TSK_MESH;
    if (!name) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk mesh name is null.\n");
        return i;
    }

    for (i = 0; i < LDC_MAX_TSK_MESH; i++) {
        if (strcmp(tsk_mesh[i].Name, name) == 0 && tsk_mesh[i].paddr /*&& tsk_mesh[i].vaddr*/) {
            break;
        }
    }
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "i in ldc_get_valid_tsk_mesh_by_name2 = %d\n", i);

    return i;
}

void mesh_gen_get_1st_size(SIZE_S in_size, u32 *mesh_1st_size)
{
    if (!mesh_1st_size)
        return;

    u32 ori_src_width = in_size.u32Width;
    u32 ori_src_height = in_size.u32Height;

    // In LDC Processing, width & height aligned to TILESIZE
    u32 src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
    u32 src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

    // modify frame size
    u32 dst_height_s1 = src_height_s1;
    u32 dst_width_s1 = src_width_s1;
    u32 num_tilex_s1 = dst_width_s1 / TILESIZE;
    u32 num_tiley_s1 = dst_height_s1 / TILESIZE;

    // 4 = 4 knots in a mesh
    *mesh_1st_size = sizeof(bm_coord2d_int_hw) * MESH_NUM_ATILE * MESH_NUM_ATILE * num_tilex_s1 * num_tiley_s1 * 4;
}

void mesh_gen_get_2nd_size(SIZE_S in_size, u32 *mesh_2nd_size)
{
    if (!mesh_2nd_size)
        return;

    u32 ori_src_width = in_size.u32Width;
    u32 ori_src_height = in_size.u32Height;

    // In LDC Processing, width & height aligned to TILESIZE
    u32 src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
    u32 src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

    // modify frame size
    u32 dst_height_s1 = src_height_s1;
    u32 dst_width_s1 = src_width_s1;
    u32 src_height_s2 = dst_width_s1;
    u32 src_width_s2 = dst_height_s1;
    u32 dst_height_s2 = src_height_s2;
    u32 dst_width_s2 = src_width_s2;
    u32 num_tilex_s2 = dst_width_s2 / TILESIZE;
    u32 num_tiley_s2 = dst_height_s2 / TILESIZE;

    // 4 = 4 knots in a mesh
    *mesh_2nd_size = sizeof(bm_coord2d_int_hw) * MESH_NUM_ATILE * MESH_NUM_ATILE * num_tilex_s2 * num_tiley_s2 * 4;
}

void mesh_gen_get_size(SIZE_S in_size,
                       SIZE_S out_size,
                       u32 *mesh_1st_size,
                       u32 *mesh_2nd_size)
{
    if (!mesh_1st_size || !mesh_2nd_size)
        return;

    (void)out_size;

    mesh_gen_get_1st_size(in_size, mesh_1st_size);
    mesh_gen_get_2nd_size(in_size, mesh_2nd_size);
}

static void _ldc_attr_map_cv182x(const LDC_ATTR_S *pstLDCAttr,
                                 bm_ldc_attr *cfg,
                                 bm_ldc_rgn_attr *rgn_attr,
                                 double x0, double y0, double r, int mesh_horcnt, int mesh_vercnt)
{
    // Global Initialization
    cfg->Enable = 1;
    cfg->OutW_disp = (int)x0 * 2; // -200;
    cfg->OutH_disp = (int)y0 * 2; // -200;
    cfg->InCenterX = (int)x0; // front-end set.
    cfg->InCenterY = (int)y0; // front-end set.
    cfg->InRadius = (int)r; // front-end set.

    cfg->SliceX_Num = 2;
    cfg->SliceY_Num = 2;

    cfg->RgnNum = 1;
    rgn_attr[0].RegionValid = 1;
    rgn_attr[0].ZoomV = pstLDCAttr->bAspect;
    rgn_attr[0].Pan = pstLDCAttr->s32XYRatio;
    rgn_attr[0].PanEnd = pstLDCAttr->s32DistortionRatio;
    rgn_attr[0].OutW = cfg->OutW_disp;
    rgn_attr[0].OutH = cfg->OutH_disp;
    rgn_attr[0].OutX = 0;
    rgn_attr[0].OutY = 0;
    rgn_attr[0].InRadius = pstLDCAttr->s32CenterXOffset;
    rgn_attr[0].OutRadius = pstLDCAttr->s32CenterYOffset;

    if (pstLDCAttr->stGridInfoAttr.Enable) {
        rgn_attr[0].MeshVer = mesh_vercnt;
        rgn_attr[0].MeshHor = mesh_horcnt;
    } else {
        rgn_attr[0].MeshVer = DEFAULT_WIDTH_MESH_NUM;
        rgn_attr[0].MeshHor = DEFAULT_HEIGHT_MESH_NUM;
    }
    rgn_attr[0].ThetaX = pstLDCAttr->s32XRatio;
    rgn_attr[0].ThetaZ = pstLDCAttr->s32YRatio;
}

static bm_status_t _get_region_dst_mesh_list(bm_ldc_rgn_attr *rgn_attr,
                                         int view_w,
                                         int view_h,
                                         int mesh_horcnt,
                                         int mesh_vercnt,
                                         int rgn_idx)
{
    if (rgn_attr[rgn_idx].RegionValid != 1)
       return BM_ERR_PARAM;

    // 1st loop: to find mesh infos. on source ( backward projection )
    // hit index for buffer

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "view_w = %d,\n", view_w);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "view_w = %d,\n", view_w);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_horcnt = %d,\n", mesh_horcnt);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_vercnt = %d,\n", mesh_vercnt);

    int mesh_xstep = ((1 << PHASEBIT) * (view_w) / mesh_horcnt); //.2
    int mesh_ystep = ((1 << PHASEBIT) * (view_h) / mesh_vercnt); //.2

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_xstep = %d,\n", mesh_xstep);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_ystep = %d,\n", mesh_ystep);

    // give 3 types
    // 1. normal grid mesh geeration.
    // 2. center high resoution.
    // 3. boundary high resolution.
    // To extend to 3x size to cover miss hit area.

    for (int j = 0; j < mesh_vercnt; j++) {
        for (int i = 0; i < mesh_horcnt; i++) {
            int meshidx = j * mesh_horcnt + i;

            if (meshidx >= MAX_FRAME_MESH_NUM) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING, "[%d][%d] meshidx = %d >= MAX_FRAME_MESH_NUM(%d)\n",
                        j, i, meshidx, MAX_FRAME_MESH_NUM);
                return BM_ERR_PARAM;
            }

            int xcor = (mesh_xstep * (i + 0)) >> PHASEBIT;
            int xcor_r = (mesh_xstep * (i + 1)) >> PHASEBIT;
            int ycor = (mesh_ystep * (j + 0)) >> PHASEBIT;
            int ycor_d = (mesh_ystep * (j + 1)) >> PHASEBIT;

            xcor_r = MIN(view_w - 1, xcor_r);
            ycor_d = MIN(view_h - 1, ycor_d);

            rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].xcor = xcor;
            rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].ycor = ycor;
            rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].xcor = xcor_r;
            rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].ycor = ycor;
            rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].xcor = xcor_r;
            rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].ycor = ycor_d;
            rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].xcor = xcor;
            rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].ycor = ycor_d;
        }

        if (j && 0 != (rgn_attr[rgn_idx].DstRgnMeshInfo[j * mesh_horcnt].knot[0].ycor -
                       rgn_attr[rgn_idx].DstRgnMeshInfo[(j - 1) * mesh_horcnt].knot[3].ycor))
        {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING, "WARNING!!!!!!! Mesh are not tightly connected to each other!!!\n");
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING, "Check The Position:\n");
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING, "[%d] DstRgnMeshInfo[%d * %d].knot[0].ycor = %10f\n",
                      rgn_idx, j, mesh_horcnt,
                      rgn_attr[rgn_idx].DstRgnMeshInfo[j * mesh_horcnt].knot[0].ycor);
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING, "[%d] DstRgnMeshInfo[(%d - 1) * %d].knot[3] = %10f\n",
                      rgn_idx, j, mesh_horcnt,
                      rgn_attr[rgn_idx].DstRgnMeshInfo[j * mesh_horcnt].knot[3].ycor);
            return BM_ERR_PARAM;
        }
    }
    // Third part ----------------------------------------------------------
    // Extend Destination Mesh Grid to 3x3 large.
    // extend destination mesh here, off line work!!
    // Why need extend ?
    // 1st stage de-warp of pincushion case:
    //    our mapping is grid mesh from final destination to source.
    //    distorion of vertical area has no mesh covered on it.
    //    we need to create mesh mapping for these area, so to extend 3x3
    //    final destination meshes.
    // ---------------------------------------------------------------------
    for (int reorder_meshidx = 0; reorder_meshidx < 9 * (mesh_horcnt * mesh_vercnt); reorder_meshidx++) {
        int mesh_horcntExt = (mesh_horcnt * 3);

        int reorder_idy = (reorder_meshidx / mesh_horcntExt);
        int reorder_idx = reorder_meshidx - (reorder_idy * mesh_horcntExt);

        int ori_idx = reorder_idx % mesh_horcnt;
        int ori_idy = reorder_idy % mesh_vercnt;

        int ext_idx = (reorder_idx / mesh_horcnt) - 1; // -1, 0, +1
        int ext_idy = (reorder_idy / mesh_vercnt) - 1; // -1, 0, +1

        for (int knotidx = 0; knotidx < 4; knotidx++) {
            // view "-1": ex: ori: 1079, upside = 179 - 1079 = 0
            rgn_attr[0].DstRgnMeshInfoExt[reorder_meshidx].knot[knotidx].xcor =
                rgn_attr[0].DstRgnMeshInfo[ori_idy * mesh_horcnt +
                ori_idx].knot[knotidx].xcor + ext_idx * (view_w - 1);

            rgn_attr[0].DstRgnMeshInfoExt[reorder_meshidx].knot[knotidx].ycor =
                rgn_attr[0].DstRgnMeshInfo[ori_idy * mesh_horcnt +
                ori_idx].knot[knotidx].ycor + ext_idy * (view_h - 1);
        }
    }
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "PASS2!!!!!!\n\r");
    return BM_SUCCESS;
}

static void ldc_get_region_src_mesh_list(bm_ldc_rgn_attr *rgn_attr,
                                         int rgn_idx, double x0, double y0,
                                         bm_meshdata_all* meshdata,
                                         const LDC_ATTR_S *pstLDCAttr)
{
    int view_w = rgn_attr[rgn_idx].OutW;
    int view_h = rgn_attr[rgn_idx].OutH;
    int mesh_horcnt = rgn_attr[rgn_idx].MeshHor;
    int mesh_vercnt = rgn_attr[rgn_idx].MeshVer;

    // register:
    bool bAspect        = (bool)rgn_attr[0].ZoomV;
    int XYRatio         = minmax(rgn_attr[0].Pan, 0, 100);
    int XRatio          = minmax(rgn_attr[0].ThetaX, 0, 100);
    int YRatio          = minmax(rgn_attr[0].ThetaZ, 0, 100);
    int CenterXOffset   = minmax(rgn_attr[0].InRadius, -511, 511);
    int CenterYOffset   = minmax(rgn_attr[0].OutRadius, -511, 511);
    int DistortionRatio = minmax(rgn_attr[0].PanEnd, -300, 500);

    double norm = sqrt((view_w / 2) * (view_w / 2) + (view_h / 2) * (view_h / 2));

    // internal link to register:
    double k = (double)DistortionRatio / 1000.0;
    double ud_gain = (1 + k * (((double)view_h / 2) * (double)view_h / 2) / norm / norm);
    double lr_gain = (1 + k * (((double)view_w / 2) * (double)view_w / 2) / norm / norm);

    double Aspect_gainX = MAX(ud_gain, lr_gain);
    double Aspect_gainY = MAX(ud_gain, lr_gain);
    bm_vector2D dist2d;

    if (pstLDCAttr->stGridInfoAttr.Enable) {
        int str_idx = *(meshdata->pmesh_dst) / meshdata->mesh_w;
        int str_idy = *(meshdata->pmesh_dst + 1) / meshdata->mesh_h;
        int end_idx = str_idx + meshdata->mesh_horcnt - 1;
        int end_idy = str_idy + meshdata->mesh_vercnt - 1;

        int ext_str_idx = str_idx + mesh_horcnt;
        int ext_end_idx = end_idx + mesh_horcnt;
        int ext_str_idy = str_idy + mesh_vercnt;
        int ext_end_idy = end_idy + mesh_vercnt;

        for (int i = 0; i < 9 * (mesh_horcnt * mesh_vercnt); i++)
        {
            int idy = i / (mesh_horcnt * 3);
            int idx = i % (mesh_horcnt * 3);

            for (int j = 0; j < 4; j++)
            {
                if (ext_str_idx <= idx && idx <= ext_end_idx && ext_str_idy <= idy && idy <= ext_end_idy)
                {
                    int roi_idx = idx - ext_str_idx;
                    int roi_idy = idy - ext_str_idy;
                    rgn_attr[rgn_idx].SrcRgnMeshInfoExt[i].knot[j].xcor = *(meshdata->pmesh_src + 8 * roi_idx + meshdata->mesh_horcnt * 8 * roi_idy + 2 * j);
                    rgn_attr[rgn_idx].SrcRgnMeshInfoExt[i].knot[j].ycor = *(meshdata->pmesh_src + 8 * roi_idx + meshdata->mesh_horcnt * 8 * roi_idy + 2 * j + 1);
                }
                else
                {
                    rgn_attr[rgn_idx].SrcRgnMeshInfoExt[i].knot[j].xcor = 0;
                    rgn_attr[rgn_idx].SrcRgnMeshInfoExt[i].knot[j].ycor = 0;
                }
            }
        }
    } else {
        for (int i = 0; i < 9 * (mesh_horcnt * mesh_vercnt); i++) {
            // Do LDC mapping in Center Mesh Grid only
            for (int knotidx = 0; knotidx < 4; knotidx++) {
                double x = rgn_attr[rgn_idx].DstRgnMeshInfoExt[i].knot[knotidx].xcor -
                        rgn_attr[rgn_idx].OutX - rgn_attr[rgn_idx].OutW / 2;
                double y = rgn_attr[rgn_idx].DstRgnMeshInfoExt[i].knot[knotidx].ycor -
                        rgn_attr[rgn_idx].OutY - rgn_attr[rgn_idx].OutH / 2;

                x = x - CenterXOffset; // -view_w / 2 + 1, view_w / 2 - 1);
                y = y - CenterYOffset; // -view_h / 2 + 1, view_h / 2 - 1);

                x = x / Aspect_gainX;
                y = y / Aspect_gainY;

                if (bAspect == true) {
                    x = x * (1 - 0.333 * (100 - XYRatio) / 100);
                    y = y * (1 - 0.333 * (100 - XYRatio) / 100);
                } else {
                    x = x * (1 - 0.333 * (100 - XRatio) / 100);
                    y = y * (1 - 0.333 * (100 - YRatio) / 100);
                }

                double rd = MIN(norm, sqrt(x * x + y * y));

                dist2d.x = x * (1 + k * ((rd / norm) * (rd / norm)));
                dist2d.y = y * (1 + k * ((rd / norm) * (rd / norm)));

                // update source mesh-info here
                rgn_attr[rgn_idx].SrcRgnMeshInfoExt[i].knot[knotidx].xcor = dist2d.x + x0 + CenterXOffset;
                rgn_attr[rgn_idx].SrcRgnMeshInfoExt[i].knot[knotidx].ycor = dist2d.y + y0 + CenterYOffset;
            }
        }
    }
}

static void _get_frame_mesh_list(bm_ldc_attr *cfg, bm_ldc_rgn_attr *rgn_attr)
{
    // pack all regions' mesh info, including src & dst.
    int rgnNum = cfg->RgnNum;
    int meshNumRgn[MAX_REGION_NUM];
    int frameMeshIdx = 0;

    for (int i = 0; i < rgnNum; i++) {
        if (rgn_attr[i].RegionValid == 1) {
            meshNumRgn[i] = (rgn_attr[i].MeshHor * rgn_attr[i].MeshVer);

            // go through each region loop
            for (int meshidx = 0; meshidx < 9 * meshNumRgn[i]; meshidx++) {
                // each mesh has 4 knots
                for (int knotidx = 0; knotidx < 4; knotidx++) {
                    cfg->DstRgnMeshInfoExt[frameMeshIdx].knot[knotidx].xcor =
                        rgn_attr[i].DstRgnMeshInfoExt[meshidx].knot[knotidx].xcor;
                    cfg->DstRgnMeshInfoExt[frameMeshIdx].knot[knotidx].ycor =
                        rgn_attr[i].DstRgnMeshInfoExt[meshidx].knot[knotidx].ycor;

                    cfg->SrcRgnMeshInfoExt[frameMeshIdx].knot[knotidx].xcor =
                        rgn_attr[i].SrcRgnMeshInfoExt[meshidx].knot[knotidx].xcor;
                    cfg->SrcRgnMeshInfoExt[frameMeshIdx].knot[knotidx].ycor =
                        rgn_attr[i].SrcRgnMeshInfoExt[meshidx].knot[knotidx].ycor;
                }
                frameMeshIdx += 1;
            }
        }
    }

    // update mesh index number
    cfg->TotalMeshNum = frameMeshIdx;
}

static bm_coord2d _find_knot_map2src(bm_coord2d_int cur_dst_mesh,
                                  bm_ldc_attr *cfg,
                                  int swmesh_hit_index, int stageID)
{
    bm_coord2d knot_source;
    bm_mesh_struct cur_dst_sw_mesh;
    bm_mesh_struct cur_src_sw_mesh;
    bm_mesh_struct *dst_mesh = cfg->DstRgnMeshInfoExt;
    bm_mesh_struct *src_mesh = cfg->SrcRgnMeshInfoExt;
    bm_mesh_struct *dst_mesh_2nd = cfg->DstRgnMeshInfoExt2ND;
    bm_mesh_struct *src_mesh_2nd = cfg->SrcRgnMeshInfoExt2ND;

    //======================================================================
    // input current destination grid mesh
    // input cur_dst_mesh hit stage-destination mesh index
    // load cur stage-destination mesh by given mesh index
    // calculate vector coefficients a & b
    // By coefficients a & b, mapping onto the cooridnate on source frame
    //======================================================================

    // check which triangle cur-pxl locates in,
    for (int knotidx = 0; knotidx < 4; knotidx++) {
        if (stageID == 0) {
            cur_dst_sw_mesh.knot[knotidx].xcor = dst_mesh[swmesh_hit_index].knot[knotidx].xcor;
            cur_dst_sw_mesh.knot[knotidx].ycor = src_mesh[swmesh_hit_index].knot[knotidx].ycor;

            cur_src_sw_mesh.knot[knotidx].xcor = src_mesh[swmesh_hit_index].knot[knotidx].xcor;
            cur_src_sw_mesh.knot[knotidx].ycor = src_mesh[swmesh_hit_index].knot[knotidx].ycor;
        } else {
            cur_dst_sw_mesh.knot[knotidx].xcor = dst_mesh_2nd[swmesh_hit_index].knot[knotidx].xcor;
            cur_dst_sw_mesh.knot[knotidx].ycor = dst_mesh_2nd[swmesh_hit_index].knot[knotidx].ycor;

            cur_src_sw_mesh.knot[knotidx].xcor = src_mesh_2nd[swmesh_hit_index].knot[knotidx].xcor;
            cur_src_sw_mesh.knot[knotidx].ycor = dst_mesh_2nd[swmesh_hit_index].knot[knotidx].ycor;
        }
    }

    // fixed in side 01 & side 23
    double x01_a = (double)cur_dst_mesh.xcor - cur_dst_sw_mesh.knot[(0 + stageID) % 4].xcor;
    double x01_b = cur_dst_sw_mesh.knot[(1 + stageID) % 4].xcor - (double)cur_dst_mesh.xcor;

    bm_coord2d inter_01, inter_32;

    inter_01.xcor = cur_dst_sw_mesh.knot[(0 + stageID) % 4].xcor +
            (cur_dst_sw_mesh.knot[(1 + stageID) % 4].xcor -
            cur_dst_sw_mesh.knot[(0 + stageID) % 4].xcor) * (x01_a) / (x01_a + x01_b);
    inter_01.ycor = cur_dst_sw_mesh.knot[(0 + stageID) % 4].ycor +
            (cur_dst_sw_mesh.knot[(1 + stageID) % 4].ycor -
            cur_dst_sw_mesh.knot[(0 + stageID) % 4].ycor) * (x01_a) / (x01_a + x01_b);
    inter_32.xcor = cur_dst_sw_mesh.knot[(3 + stageID) % 4].xcor +
            (cur_dst_sw_mesh.knot[(2 + stageID) % 4].xcor -
            cur_dst_sw_mesh.knot[(3 + stageID) % 4].xcor) * (x01_a) / (x01_a + x01_b);
    inter_32.ycor = cur_dst_sw_mesh.knot[(3 + stageID) % 4].ycor +
            (cur_dst_sw_mesh.knot[(2 + stageID) % 4].ycor -
            cur_dst_sw_mesh.knot[(3 + stageID) % 4].ycor) * (x01_a) / (x01_a + x01_b);

    double y01_a = (double)cur_dst_mesh.ycor - inter_01.ycor;
    double y01_b = inter_32.ycor - (double)cur_dst_mesh.ycor;

    bm_coord2d inter_01_src, inter_32_src;

    inter_01_src.xcor = cur_src_sw_mesh.knot[(0 + stageID) % 4].xcor +
                (cur_src_sw_mesh.knot[(1 + stageID) % 4].xcor -
                cur_src_sw_mesh.knot[(0 + stageID) % 4].xcor) * (x01_a) / (x01_a + x01_b);
    inter_32_src.xcor = cur_src_sw_mesh.knot[(3 + stageID) % 4].xcor +
                (cur_src_sw_mesh.knot[(2 + stageID) % 4].xcor -
                cur_src_sw_mesh.knot[(3 + stageID) % 4].xcor) * (x01_a) / (x01_a + x01_b);

    knot_source.ycor = (double)cur_dst_mesh.ycor;
    knot_source.xcor = inter_01_src.xcor + (inter_32_src.xcor - inter_01_src.xcor) * (y01_a) / (y01_a + y01_b);

    return knot_source;
}

static int _double2Int_s13_10(double value)
{
#ifdef TWOS_COMPLEMENT
    //double abs_value = abs(value);
    //double dtmp2cpl = abs_value * 1024;
    int tmp2cpl = (int)(abs((int)(value * (1 << QFORMAT_M))));

    // get 24 bit
    tmp2cpl = tmp2cpl & 0xFFFFFF;

    int rtn_value = tmp2cpl;

    if (value < 0)
        rtn_value = (~tmp2cpl) + 1;
    else
        rtn_value = rtn_value;

    rtn_value = rtn_value & 0xFFFFFF;

#else
    int sbit = (value < 0) ? 1 : 0;
    int tmp2cpl = (int)(abs(value) * (1 << QFORMAT_M));
    int rtn_value = sbit << (13 + QFORMAT_M) | tmp2cpl; // s13.QFORMAT_M
#endif
    return rtn_value;
}

static int _chk_location_to_line(bm_coord2d *meshcorA, bm_coord2d *meshcorB, int x, int y, int inc_zero)
{
    int onright = 0;

    // along vector from A -> B
    // A(x1, y1) to B(x2, y2), P(x0�Ay0) outsize the line
    // determine which side the point resides
    // a = (x2 - x1, y2 - y1)
    // b = (x0 - x1, y0 - y1)

    double x1 = meshcorA->xcor;
    double y1 = meshcorA->ycor;
    double x2 = meshcorB->xcor;
    double y2 = meshcorB->ycor;

    double tmp = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);

    // has optional inclusion of zero as in mesh.
    onright = inc_zero ? (tmp >= 0) : (tmp > 0);

    return onright;
}

static int _chk_in_mesh(int x, int y, bm_mesh_struct cur_sw_mesh)
{
    int inthismesh_hit = 0;
    int onright_01 = _chk_location_to_line(&cur_sw_mesh.knot[0], &cur_sw_mesh.knot[1], x, y, 1);
    int onright_12 = _chk_location_to_line(&cur_sw_mesh.knot[1], &cur_sw_mesh.knot[2], x, y, 0);
    int onright_23 = _chk_location_to_line(&cur_sw_mesh.knot[2], &cur_sw_mesh.knot[3], x, y, 0);
    int onright_30 = _chk_location_to_line(&cur_sw_mesh.knot[3], &cur_sw_mesh.knot[0], x, y, 1);

    // in mesh case: clockwise: 0 -> 1 -> 2 -> 3
    //     & counter clockwise: 3 -> 2 -> 1 -> 0
    // onright_01, onright_12, onright_23, onright_30, = ( 1,  1,  1,  1  )
    // onright_01, onright_12, onright_23, onright_30, = ( 0, 0, 0, 0 )

    // inside the mesh
    if ((onright_01 == onright_12) && (onright_12 == onright_23) && (onright_23 == onright_30)) {
        inthismesh_hit = 1;
    } else {
        inthismesh_hit = 0;
    }
    return inthismesh_hit;
}

static bm_status_t _get_1st_src_midxy(int num_tilex_s1,
                                      bm_ldc_attr *cfg,
                                      bm_coord2d_int_hw *src_1st_list,
                                      int tidx, int tidy, int mesh_horcnt, int mesh_vercnt,
                                      const LDC_ATTR_S *pstLDCAttr)
{
    u32 max_mesh_num = 9 * (mesh_vercnt * mesh_horcnt);
    // CVI_U32 max_mesh_num = 9 * (MAX_HEIGHT_MESH_NUM * MAX_WIDTH_MESH_NUM);

    // go check all meshes inside.
    for (int midy = 0; midy < MESH_NUM_ATILE; midy++) {
        for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
            // each grid mesh from destination
            // find the mapping coordinate onto the source.
            bm_coord2d_int cur_dst_mesh[4];

            memset(cur_dst_mesh, 0, sizeof(cur_dst_mesh));

            cur_dst_mesh[0].xcor = (tidx * TILESIZE) + (midx + 0) * HW_MESH_SIZE;
            cur_dst_mesh[1].xcor = (tidx * TILESIZE) + (midx + 1) * HW_MESH_SIZE;
            cur_dst_mesh[2].xcor = (tidx * TILESIZE) + (midx + 1) * HW_MESH_SIZE;
            cur_dst_mesh[3].xcor = (tidx * TILESIZE) + (midx + 0) * HW_MESH_SIZE;

            cur_dst_mesh[0].ycor = (tidy * TILESIZE) + (midy + 0) * HW_MESH_SIZE;
            cur_dst_mesh[1].ycor = (tidy * TILESIZE) + (midy + 0) * HW_MESH_SIZE;
            cur_dst_mesh[2].ycor = (tidy * TILESIZE) + (midy + 1) * HW_MESH_SIZE;
            cur_dst_mesh[3].ycor = (tidy * TILESIZE) + (midy + 1) * HW_MESH_SIZE;

            // for each knot of grid cur_dst_mesh, check all sw mesh
            // "on source" to estimate approximation.
            // swmesh_hit_index[N] is to record which sw-mesh index
            // cur knot-N hit.
            uint32_t swmesh_hit_index[4];

            for (int i = 0; i < 4; i++)
                swmesh_hit_index[i] = 0xFFFFFFFF;

            if (pstLDCAttr->stGridInfoAttr.Enable) {
                // go through al hwmesh knots
                int use_count = 0;
                for (int knotidx = 0; knotidx < 4; knotidx++) {
                    for (uint32_t meshidx = 0; meshidx < max_mesh_num; meshidx++) {
                        bm_mesh_struct cur_sw_mesh;
                        bm_mesh_struct *dst = &cfg->DstRgnMeshInfoExt[meshidx];
                        bm_mesh_struct *src = &cfg->SrcRgnMeshInfoExt[meshidx];

                        // load cur_sw_mesh to check if hwmesh's knot is in it?
                        for (int swknotidx = 0; swknotidx < 4; swknotidx++) {
                            // 1st stage, SW destination( x, y ) = ( int ,float )
                            // 1st stage, SW source( x,y ) = ( float, float )
                            cur_sw_mesh.knot[swknotidx].xcor =
                                dst->knot[swknotidx].xcor;
                            cur_sw_mesh.knot[swknotidx].ycor =
                                src->knot[swknotidx].ycor;
                        }

                        // check if cur-pixel in this mesh
                        int sw_mesh_hit = _chk_in_mesh(cur_dst_mesh[knotidx].xcor,
                                cur_dst_mesh[knotidx].ycor, cur_sw_mesh);
                        if (sw_mesh_hit) {
                            swmesh_hit_index[knotidx] = meshidx;
                            break;
                        }
                    }

                    // each knot has been
                    // assigned a swmesh index
                    // to estimate approximation.
                    // do estimation here:
                    bm_coord2d map_src_knot;

                    memset(&map_src_knot, 0, sizeof(map_src_knot));

                    if (swmesh_hit_index[knotidx] != 0xFFFFFFFF)  use_count++;
                }

                for (int knotidx = 0; knotidx < 4; knotidx++) {
                    bm_coord2d map_src_knot;
                    int xidx = tidx * MESH_NUM_ATILE + midx;
                    int yidx = tidy * MESH_NUM_ATILE + midy;
                    uint32_t xcor;
                    uint32_t offset;

                    if (use_count == 4) {
                        map_src_knot = _find_knot_map2src(cur_dst_mesh[knotidx], cfg, swmesh_hit_index[knotidx], 0); //0 = 1st stage
                        xcor = (uint32_t)_double2Int_s13_10(map_src_knot.xcor);
                    }
                    else {
                        xcor = 0;
                    }
                    offset = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + knotidx;

                    src_1st_list[offset].xcor[0] = xcor & 0xff;
                    src_1st_list[offset].xcor[1] = (xcor >> 8) & 0xff;
                    src_1st_list[offset].xcor[2] = (xcor >> 16) & 0xff;
                }
            } else {
                // go through al hwmesh knots
                for (int knotidx = 0; knotidx < 4; knotidx++) {
                    for (uint32_t meshidx = 0; meshidx < max_mesh_num; meshidx++) {
                        bm_mesh_struct cur_sw_mesh;
                        bm_mesh_struct *dst = &cfg->DstRgnMeshInfoExt[meshidx];
                        bm_mesh_struct *src = &cfg->SrcRgnMeshInfoExt[meshidx];

                        // load cur_sw_mesh to check if hwmesh's knot is in it?
                        for (int swknotidx = 0; swknotidx < 4; swknotidx++) {
                            // 1st stage, SW destination( x, y ) = ( int ,float )
                            // 1st stage, SW source( x,y ) = ( float, float )
                            cur_sw_mesh.knot[swknotidx].xcor =
                                dst->knot[swknotidx].xcor;
                            cur_sw_mesh.knot[swknotidx].ycor =
                                src->knot[swknotidx].ycor;
                        }

                        // check if cur-pixel in this mesh
                        int sw_mesh_hit = _chk_in_mesh(cur_dst_mesh[knotidx].xcor,
                                cur_dst_mesh[knotidx].ycor, cur_sw_mesh);
                        if (sw_mesh_hit) {
                            swmesh_hit_index[knotidx] = meshidx;
                            break;
                        }
                    }

                    // each knot has been
                    // assigned a swmesh index
                    // to estimate approximation.
                    // do estimation here:
                    bm_coord2d map_src_knot;

                    memset(&map_src_knot, 0, sizeof(map_src_knot));

                    if (swmesh_hit_index[knotidx] == 0xFFFFFFFF) {
                        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "knotidx = %d tidx = %d tidy = %d,\n", knotidx, tidx, tidy);
                        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "midx = %d, midy = %d,\n", midx, midy);
                        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "cur_dst_mesh[%d] = (%d, %d)\n", knotidx, cur_dst_mesh[knotidx].xcor, cur_dst_mesh[knotidx].ycor);
                        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "DEBUG STASRT!!\n");
                        return BM_ERR_PARAM;
                    }

                    map_src_knot = _find_knot_map2src(cur_dst_mesh[knotidx], cfg,
                                swmesh_hit_index[knotidx], 0); //0 = 1st stage

                    int xidx = tidx * MESH_NUM_ATILE + midx;
                    int yidx = tidy * MESH_NUM_ATILE + midy;
                    uint32_t xcor = (uint32_t)_double2Int_s13_10(map_src_knot.xcor);
                    uint32_t offset =
                        (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 +
                        knotidx;

                    src_1st_list[offset].xcor[0] = xcor & 0xff;
                    src_1st_list[offset].xcor[1] = (xcor >> 8) & 0xff;
                    src_1st_list[offset].xcor[2] = (xcor >> 16) & 0xff;
                }
            }
        }
    }

    return BM_SUCCESS;
}


static bm_status_t _offline_get_1st_src_mesh_table(int num_tiley_s1, int num_tilex_s1,
                                                   bm_ldc_attr *cfg,
                                                   bm_coord2d_int_hw *src_1st_list,
                                                   int mesh_horcnt, int mesh_vercnt,
                                                   const LDC_ATTR_S *pstLDCAttr)
{
    bm_status_t ret = BM_SUCCESS;

    // 1st stage buffer data.
    for (int tidy = 0; tidy < num_tiley_s1; tidy++) {
        for (int tidx = 0; tidx < num_tilex_s1; tidx++) {
            ret = _get_1st_src_midxy(num_tilex_s1, cfg, src_1st_list, tidx, tidy, mesh_horcnt, mesh_vercnt, pstLDCAttr);
            if (ret != BM_SUCCESS)
                return ret;
        }
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "OFFLINE tidy = %d\n", tidy);
    }

    return BM_SUCCESS;
}

static int _fill_src_2nd_list(int num_tilex_s2, bm_ldc_attr *cfg,
                                  bm_coord2d_int_hw *src_2nd_list, int tidx, int tidy, int midx, int midy,
                                  bm_mesh_struct *dst_mesh, bm_coord2d_int *cur_dst_mesh,
                                  uint32_t *swmesh_hit_index, int mesh_horcnt, int mesh_vercnt)
{
    u32 max_mesh_num = 9 * (mesh_vercnt * mesh_horcnt);
    // CVI_U32 max_mesh_num = 9 * (MAX_HEIGHT_MESH_NUM * MAX_WIDTH_MESH_NUM);

    // go through all hwmesh knots
    for (int knotidx = 0; knotidx < 4; knotidx++) {
        for (u32 meshidx = 0; meshidx < max_mesh_num; meshidx++) {
            bm_mesh_struct cur_sw_mesh;

            // load cur_sw_mesh to check if hwmesh's knot is in it?
            for (int swknotidx = 0; swknotidx < 4; swknotidx++) {
                // 1st stage, SW destination( x, y ) = ( int ,float )
                // 1st stage, SW source( x,y ) = ( float, float )
                cur_sw_mesh.knot[swknotidx].xcor =
                    dst_mesh[meshidx].knot[swknotidx].xcor;
                cur_sw_mesh.knot[swknotidx].ycor =
                    dst_mesh[meshidx].knot[swknotidx].ycor;
            }

            // check if cur-pixel in this mesh
            int sw_mesh_hit = _chk_in_mesh(cur_dst_mesh[knotidx].xcor,
                cur_dst_mesh[knotidx].ycor, cur_sw_mesh);

            if (sw_mesh_hit) {
                swmesh_hit_index[knotidx] = meshidx;
                continue;
            }
        }

        // each knot has been assigned a swmesh index to estimate approximation.
        // do estimation here:
        // int knotMissingFlag = 0;
        bm_coord2d map_src_knot;

        memset(&map_src_knot, 0, sizeof(map_src_knot));
        //bm_coord2d_int map_src_knot_32bit;
        if (swmesh_hit_index[knotidx] == 0xFFFFFFFF) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "!!!! 2ndSTAG----ERROR !!!\n");
            return BM_ERR_PARAM;
        }

        map_src_knot = _find_knot_map2src(cur_dst_mesh[knotidx], cfg,
                swmesh_hit_index[knotidx], 1); // 1: 2nd stage

        // update to buffer order should align to interpolation index:
        int xidx = tidx * MESH_NUM_ATILE + midx;
        int yidx = tidy * MESH_NUM_ATILE + midy;

        uint32_t xcor = (uint32_t)_double2Int_s13_10(map_src_knot.xcor);
        uint32_t idx  = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + knotidx;

        src_2nd_list[idx].xcor[0] = xcor & 0xff;
        src_2nd_list[idx].xcor[1] = (xcor >> 8) & 0xff;
        src_2nd_list[idx].xcor[2] = (xcor >> 16) & 0xff;
    }

    return BM_SUCCESS;
}

static int _offline_get_2nd_src_mesh_table(int stage2_rotate_type, int num_tiley_s2, int num_tilex_s2,
                                           bm_ldc_attr *cfg, bm_coord2d_int_hw *src_2nd_list, int src_width_s1, int src_height_s1,
                                           u32 mesh_2nd_size, int mesh_horcnt, int mesh_vercnt)
{
    // stage2_rotate_type = 0:  +90 degrees
    // stage2_rotate_type = 1:  -90 degrees

    u32 max_mesh_num = 9 * (mesh_vercnt * mesh_horcnt);
    // CVI_U32 max_mesh_num = 9 * (MAX_HEIGHT_MESH_NUM * MAX_WIDTH_MESH_NUM);

    int RMATRIX[2][2];
    bm_mesh_struct *dst_mesh = cfg->DstRgnMeshInfoExt2ND;
    bm_mesh_struct *src_mesh = cfg->SrcRgnMeshInfoExt2ND;

    int ret;

    if (stage2_rotate_type) {
        //-90 degrees
        RMATRIX[0][0] = 0;
        RMATRIX[0][1] = -1;
        RMATRIX[1][0] = 1;
        RMATRIX[1][1] = 0;
    } else {
        // + 90 degrees
        RMATRIX[0][0] = 0;
        RMATRIX[0][1] = 1;
        RMATRIX[1][0] = -1;
        RMATRIX[1][1] = 0;
    }

    for (u32 meshidx = 0; meshidx < max_mesh_num; meshidx++) {
        for (int knotidx = 0; knotidx < 4; knotidx++) {
            dst_mesh[meshidx].knot[knotidx].xcor =
                RMATRIX[0][0] * (cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].xcor) +
                RMATRIX[0][1] * (cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].ycor);

            dst_mesh[meshidx].knot[knotidx].ycor =
                RMATRIX[1][0] *	(cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].xcor) +
                RMATRIX[1][1] * (cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].ycor);

            src_mesh[meshidx].knot[knotidx].xcor =
                RMATRIX[0][0] * (cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].xcor) +
                RMATRIX[0][1] * (cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].ycor);

            src_mesh[meshidx].knot[knotidx].ycor =
                RMATRIX[1][0] * (cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].xcor) +
                RMATRIX[1][1] * (cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].ycor);

            if (RMATRIX[0][1] == 1) {
                dst_mesh[meshidx].knot[knotidx].xcor += 0;
                dst_mesh[meshidx].knot[knotidx].ycor += (src_width_s1 - 1);
                src_mesh[meshidx].knot[knotidx].xcor += 0;
                src_mesh[meshidx].knot[knotidx].ycor += (src_width_s1 - 1);
            } else {
                // +90
                dst_mesh[meshidx].knot[knotidx].xcor += (src_height_s1 - 1);
                dst_mesh[meshidx].knot[knotidx].ycor += 0;
                src_mesh[meshidx].knot[knotidx].xcor += (src_height_s1 - 1);
                src_mesh[meshidx].knot[knotidx].ycor += 0;
            }
        }
    }
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "rotate mesh table by 90 degree done!!\n");

    // 1st stage buffer data.
    for (int tidy = 0; tidy < num_tiley_s2; tidy++) {
        for (int tidx = 0; tidx < num_tilex_s2; tidx++) {
            // tmp buffer for dump data
            // for each tile
            // go check all meshes inside.
            for (int midy = 0; midy < MESH_NUM_ATILE; midy++) {
                for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
                    // each grid mesh from destination
                    // find the mapping coordinate onto the source.
                    bm_coord2d_int cur_dst_mesh[4];

                    memset(cur_dst_mesh, 0, sizeof(cur_dst_mesh));

                    cur_dst_mesh[0].xcor = (tidx * TILESIZE) + (midx + 0) * HW_MESH_SIZE;
                    cur_dst_mesh[1].xcor = (tidx * TILESIZE) + (midx + 1) * HW_MESH_SIZE;
                    cur_dst_mesh[2].xcor = (tidx * TILESIZE) + (midx + 1) * HW_MESH_SIZE;
                    cur_dst_mesh[3].xcor = (tidx * TILESIZE) + (midx + 0) * HW_MESH_SIZE;

                    cur_dst_mesh[0].ycor = (tidy * TILESIZE) + (midy + 0) * HW_MESH_SIZE;
                    cur_dst_mesh[1].ycor = (tidy * TILESIZE) + (midy + 0) * HW_MESH_SIZE;
                    cur_dst_mesh[2].ycor = (tidy * TILESIZE) + (midy + 1) * HW_MESH_SIZE;
                    cur_dst_mesh[3].ycor = (tidy * TILESIZE) + (midy + 1) * HW_MESH_SIZE;

                    // for each knot of grid cur_dst_mesh, check all sw mesh "on source" to
                    // estimate approximation.
                    // swmesh_hit_index[N] is to record which sw-mesh index cur knot-N hit.
                    uint32_t swmesh_hit_index[4];

                    for (int i = 0; i < 4; i++)
                        swmesh_hit_index[i] = 0xFFFFFFFF;

                    ret = _fill_src_2nd_list(num_tilex_s2, cfg, src_2nd_list,
                                             tidx, tidy, midx, midy,
                                             dst_mesh, cur_dst_mesh, swmesh_hit_index, mesh_horcnt, mesh_vercnt);
                    if (ret != BM_SUCCESS)
                        return ret;
                }
            }
        }
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "2nd Stage OFFLINE tidy = %d\n", tidy);
    }

#ifdef SAVE_MESH_TBL_FILE
    save_2nd_src_mesh_table(num_tiley_s2, num_tilex_s2, src_2nd_list, mesh_2nd_size);
#else
    (void)mesh_2nd_size;
#endif

    return BM_SUCCESS;
}

static int _convert_1st_src_midxy(int num_tilex_s1, u32 mesh_1st_size,
                                  bm_coord2d_int_hw *src_1st_list, int tidx, int tidy, uint8_t *ptr,
                                  uint32_t *offset_p)
{
    uint32_t offset = *offset_p;
    u32 idx;

    for (int midy = 0; midy < MESH_NUM_ATILE; midy++) {
        for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
            int xidx = tidx * MESH_NUM_ATILE + midx;
            int yidx = tidy * MESH_NUM_ATILE + midy;

            if (offset >= (mesh_1st_size - 2))
                return BM_ERR_PARAM;

            idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 0;
            ptr[offset++] = src_1st_list[idx].xcor[0];
            ptr[offset++] = src_1st_list[idx].xcor[1];
            ptr[offset++] = src_1st_list[idx].xcor[2];

            if (midx == MESH_NUM_ATILE - 1) {
                if (offset >= (mesh_1st_size - 2))
                    return BM_ERR_PARAM;

                idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 1;
                ptr[offset++] = src_1st_list[idx].xcor[0];
                ptr[offset++] = src_1st_list[idx].xcor[1];
                ptr[offset++] = src_1st_list[idx].xcor[2];
            }
        }
        if (midy == MESH_NUM_ATILE - 1) {
            for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
                int xidx = tidx * MESH_NUM_ATILE + midx;
                int yidx = tidy * MESH_NUM_ATILE + midy;

                if (offset >= (mesh_1st_size - 2))
                    return BM_ERR_PARAM;

                idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 3;
                ptr[offset++] = src_1st_list[idx].xcor[0];
                ptr[offset++] = src_1st_list[idx].xcor[1];
                ptr[offset++] = src_1st_list[idx].xcor[2];

                if (midx == MESH_NUM_ATILE - 1) {
                    if (offset >= (mesh_1st_size - 2)) {
                        return BM_ERR_PARAM;
                    }

                    idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 2;
                    ptr[offset++] = src_1st_list[idx].xcor[0];
                    ptr[offset++] = src_1st_list[idx].xcor[1];
                    ptr[offset++] = src_1st_list[idx].xcor[2];
                }
            }
        }
    }

    *offset_p = offset;

    return BM_SUCCESS;
}

static int convert_1st_src_mesh_table(int num_tiley_s1, int num_tilex_s1,
                                      bm_coord2d_int_hw *src_1st_list, bm_coord2d_int_hw *src_1st_list_1d,
                                      u32 mesh_1st_size)
{
    // packed data in dram for HW
    uint8_t *ptr = (uint8_t *)src_1st_list_1d;
    uint32_t offset = 0;
    int ret;

    for (int tidy = 0; tidy < num_tiley_s1; tidy++) {
        for (int tidx = 0; tidx < num_tilex_s1; tidx++) {
            ret = _convert_1st_src_midxy(num_tilex_s1, mesh_1st_size,
                                         src_1st_list, tidx, tidy, ptr,
                                         &offset);
            if (ret != BM_SUCCESS)
                return ret;

            // # 256 bytes per tile
            for (uint32_t i = 0; i < 13; i++)
                ptr[offset++] = 0xab;
        }
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "offset=%d, mesh_1st_size=%d\n", offset, mesh_1st_size);

#ifdef SAVE_MESH_TBL_FILE
    FILE *fp1x_bin = fopen("srcx_1st_mesh.bin", "wb");

    if (fp1x_bin) {
        size_t wr_size = fwrite(src_1st_list_1d, mesh_1st_size, 1, fp1x_bin);

        if (wr_size != mesh_1st_size)
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "1st src mesh, fwrite %d, only %d succeed\n", mesh_1st_size, wr_size);
        fclose(fp1x_bin);
    }
#endif

    return BM_SUCCESS;
}

static void _convert_2nd_src_midxy(int num_tilex_s2, bm_coord2d_int_hw *src_2nd_list,
                                   int tidx, int tidy, uint8_t *ptr, uint32_t *offset_ptr)
{
    uint32_t offset = *offset_ptr;
    u32 idx;

    for (int midy = 0; midy < MESH_NUM_ATILE; midy++) {
        for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
            int xidx = tidx * MESH_NUM_ATILE + midx;
            int yidx = tidy * MESH_NUM_ATILE + midy;

            idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 0;
            ptr[offset++] = src_2nd_list[idx].xcor[0];
            ptr[offset++] = src_2nd_list[idx].xcor[1];
            ptr[offset++] = src_2nd_list[idx].xcor[2];

            if (midx == MESH_NUM_ATILE - 1) {
                idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 1;
                ptr[offset++] = src_2nd_list[idx].xcor[0];
                ptr[offset++] = src_2nd_list[idx].xcor[1];
                ptr[offset++] = src_2nd_list[idx].xcor[2];
            }
        }

        if (midy == MESH_NUM_ATILE - 1) {
            for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
                int xidx = tidx * MESH_NUM_ATILE + midx;
                int yidx = tidy * MESH_NUM_ATILE + midy;

                idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 3;
                ptr[offset++] = src_2nd_list[idx].xcor[0];
                ptr[offset++] = src_2nd_list[idx].xcor[1];
                ptr[offset++] = src_2nd_list[idx].xcor[2];

                if (midx == MESH_NUM_ATILE - 1) {
                    idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 2;
                    ptr[offset++] = src_2nd_list[idx].xcor[0];
                    ptr[offset++] = src_2nd_list[idx].xcor[1];
                    ptr[offset++] = src_2nd_list[idx].xcor[2];
                }
            }
        }
    }
    *offset_ptr = offset;
}

static void convert_2nd_src_mesh_table(int num_tiley_s2, int num_tilex_s2,
                                      bm_coord2d_int_hw *src_2nd_list,
                                      bm_coord2d_int_hw *src_2nd_list_1d,
                                      u32 mesh_2nd_size)
{
    // packed data in dram for HW
    uint8_t *ptr = (uint8_t *)src_2nd_list_1d;
    uint32_t offset = 0;
    uint32_t map_file_size = num_tiley_s2 * num_tilex_s2 * 256;

    if (mesh_2nd_size < map_file_size) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "error, mesh_2nd_size(%d) < map_file_size(%d)\n", mesh_2nd_size, map_file_size);
        return;
    }

    for (int tidy = 0; tidy < num_tiley_s2; tidy++) {
        for (int tidx = 0; tidx < num_tilex_s2; tidx++) {
            _convert_2nd_src_midxy(num_tilex_s2, src_2nd_list, tidx,
                            tidy, ptr, &offset);

            // # 256 bytes per tile
            // fout.write(bytearray([0xab]*13))
            for (uint32_t i = 0; i < 13; i++)
                ptr[offset++] = 0xab;
        }
    }

#ifdef SAVE_MESH_TBL_FILE
    FILE *fp2x_bin = fopen("srcx_2nd_mesh.bin", "wb");
    size_t wr_size = fwrite(src_2nd_list_1d, mesh_2nd_size, 1, fp2x_bin);

    if (wr_size != mesh_2nd_size)
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "2nd src mesh, fwrite %d, only %d succeed\n", mesh_2nd_size, wr_size);
    fclose(fp2x_bin);
#endif
}

int load_meshdata(char *grid, bm_mesh_data_all_s *pmeshdata, const char *bindName)
{
    int info[100] = {0};

    memcpy(info, grid, 100 * sizeof(int));

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "head %d %d %d %d %d\n", info[0], info[1], info[2], info[3], info[4]);
    pmeshdata->mesh_horcnt = info[0]; // num of mesh in roi
    pmeshdata->mesh_vercnt = info[1]; // num of mesh in roi
    pmeshdata->num_pairs = info[2];
    pmeshdata->imgw = info[3];
    pmeshdata->imgh = info[4];
    //
    pmeshdata->mesh_w = info[5];   // unit: pixel
    pmeshdata->mesh_h = info[6];   // unit: pixel
    pmeshdata->unit_rx = info[7];  // unit: mesh_w
    pmeshdata->unit_ry = info[8];  // unit: mesh_h
    pmeshdata->_nbr_mesh_x = info[9];   // total meshes in horizontal
    pmeshdata->_nbr_mesh_y = info[10];  // total meshes in vertical
    memcpy(pmeshdata->corners, info + 11, sizeof(int) * 8);

    int _nbr_mesh_y = pmeshdata->mesh_vercnt; // for roi, not for whole image
    int _nbr_mesh_x = pmeshdata->mesh_horcnt;
    int count_grid = pmeshdata->num_pairs;
    pmeshdata->node_index = (_nbr_mesh_x + 1)*(_nbr_mesh_y + 1);

    strcpy(pmeshdata->grid_name, bindName);

    pmeshdata->pgrid_src = (int *)calloc(count_grid * 2, sizeof(int));
    pmeshdata->pgrid_dst = (int *)calloc(count_grid * 2, sizeof(int));
    pmeshdata->pmesh_src = (int *)calloc(count_grid * 8, sizeof(int));
    pmeshdata->pmesh_dst = (int *)calloc(count_grid * 8, sizeof(int));
    pmeshdata->pnode_src = (int *)calloc(pmeshdata->node_index*2, sizeof(int));
    pmeshdata->pnode_dst = (int *)calloc(pmeshdata->node_index*2, sizeof(int));

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_horcnt,mesh_vercnt,_nbr_mesh_x, _nbr_mesh_y, count_grid, num_nodes: %d %d %d %d %d %d \n",
              pmeshdata->mesh_horcnt, pmeshdata->mesh_vercnt, _nbr_mesh_x, _nbr_mesh_y, count_grid, pmeshdata->node_index);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "imgw, imgh, mesh_w, mesh_h ,unit_rx,unit_ry: %d %d %d %d %d %d \n",
              pmeshdata->imgw, pmeshdata->imgh, pmeshdata->mesh_w, pmeshdata->mesh_h, pmeshdata->unit_rx, pmeshdata->unit_ry);

    memcpy(pmeshdata->pgrid_src, grid + 100 * sizeof(int), count_grid * 2 * sizeof(int));
    memcpy(pmeshdata->pgrid_dst, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int), count_grid * 2 * sizeof(int));
    memcpy(pmeshdata->pmesh_src, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int) * 2, count_grid * 2 * 4 * sizeof(int));
    memcpy(pmeshdata->pmesh_dst, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int) * 2 + count_grid * 2 * 4 * sizeof(int), count_grid * 2 * 4 * sizeof(int));
    memcpy(pmeshdata->pnode_src, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int) * 2 + count_grid * 2 * 4 * sizeof(int) * 2, pmeshdata->node_index * 2 * sizeof(int));
    memcpy(pmeshdata->pnode_dst, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int) * 2 + count_grid * 2 * 4 * sizeof(int) * 2 + pmeshdata->node_index * 2 * sizeof(int), pmeshdata->node_index * 2 * sizeof(int));

    pmeshdata->balloc = true;
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "read success!\n");

    return 0;
}

bm_status_t mesh_gen_ldc(SIZE_S in_size,
                         SIZE_S out_size,
                         const LDC_ATTR_S *pstLDCAttr,
                         uint64_t mesh_phy_addr,
                         void *mesh_vir_addr,
                         bmcv_rot_mode rot,
                         char *grid)
{
    bm_status_t ret = BM_SUCCESS;
    bm_reg_ldc reg;
    bm_coord2d_int_hw *src_1st_list_1d = NULL, *src_2nd_list_1d = NULL;
    bm_coord2d_int_hw *src_1st_list = NULL, *src_2nd_list = NULL;
    bm_ldc_attr *cfg = NULL;
    bm_ldc_rgn_attr *rgn_attr = NULL;

    (void)mesh_phy_addr;
    (void)rot;

    u32 mesh_1st_size = 0, mesh_2nd_size = 0;

    mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);
    src_1st_list_1d = (bm_coord2d_int_hw *)mesh_vir_addr;
    src_2nd_list_1d = (bm_coord2d_int_hw *)(mesh_vir_addr + mesh_1st_size);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "1st src_1st_list=%p(%d), src_2nd_list=%p(%d)\n",src_1st_list_1d, mesh_1st_size, src_2nd_list_1d, mesh_2nd_size);

    src_1st_list = (bm_coord2d_int_hw *)malloc(mesh_1st_size);
    src_2nd_list = (bm_coord2d_int_hw *)malloc(mesh_2nd_size);
    cfg = (bm_ldc_attr *)calloc(1, sizeof(*cfg));
    rgn_attr = (bm_ldc_rgn_attr *)calloc(1, sizeof(*rgn_attr) * MAX_REGION_NUM);
    if (!src_1st_list || !src_2nd_list || !cfg || !rgn_attr) {
        free(src_1st_list);
        free(src_2nd_list);
        free(cfg);
        free(rgn_attr);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "fail to alloc mesh\n");
        return BM_ERR_NOMEM;
    }

    int mesh_horcnt = DEFAULT_WIDTH_MESH_NUM, mesh_vercnt = DEFAULT_HEIGHT_MESH_NUM;

    if (pstLDCAttr->stGridInfoAttr.Enable) {
        load_meshdata(grid, &ldc_meshdata, pstLDCAttr->stGridInfoAttr.gridBindName);
        mesh_horcnt = ldc_meshdata.imgw  / ldc_meshdata.mesh_w;
        mesh_vercnt = ldc_meshdata.imgh / ldc_meshdata.mesh_h;
    }

    // al registers
    reg.ldc_en = 1;
    reg.stage2_rotate_type = 0;

    int bgc_pack = 0x00217E;

    reg.bg_color_y_r = (bgc_pack & 0xFF0000) >> 16;
    reg.bg_color_u_g = (bgc_pack & 0x00FF00) >> 8;
    reg.bg_color_v_b = (bgc_pack & 0x0000FF) >> 0;

    int ori_src_width, ori_src_height;
    if (pstLDCAttr->stGridInfoAttr.Enable) {
        ori_src_width = ldc_meshdata.imgw;
        ori_src_height = ldc_meshdata.imgh;
    }
    else{
        ori_src_width = in_size.u32Width;
        ori_src_height = in_size.u32Height;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "ori_src_width = %d,\n", ori_src_width);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "ori_src_height = %d,\n", ori_src_height);

    // In LDC Processing, width & height  aligned to TILESIZE **
    int src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
    int src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "src_width_s1 = %d,\n", src_width_s1);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "src_height_s1 = %d,\n", src_height_s1);

    // auto assign correct range:
    reg.src_xstr_s1 = 0;
    reg.src_xend_s1 = ori_src_width - 1;
    reg.src_xstr_s2 = reg.stage2_rotate_type ? (src_height_s1 - ori_src_height) : 0;
    reg.src_xend_s2 = reg.stage2_rotate_type ? (src_height_s1 - 1) : (ori_src_height - 1);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "reg.stage2_rotate_type = %d,\n", reg.stage2_rotate_type);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "reg.bg_color_y_r = %d,\n", reg.bg_color_y_r);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "reg.bg_color_u_g = %d,\n", reg.bg_color_u_g);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "reg.bg_color_v_b = %d,\n", reg.bg_color_v_b);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "reg.src_xstart_s1 = %d,\n", reg.src_xstr_s1);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "reg.src_xend_s1 = %d,\n", reg.src_xend_s1);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "reg.src_xstart_s2 = %d,\n", reg.src_xstr_s2);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "reg.src_xend_s2 = %d,\n", reg.src_xend_s2);

    int x0, y0, r;

    x0 = ori_src_width / 2;
    y0 = ori_src_height / 2;
    r = MIN(x0, y0);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "cfg size %d\n", (int)sizeof(bm_ldc_attr));

    // update parameters
    _ldc_attr_map_cv182x(pstLDCAttr, cfg, rgn_attr, x0, y0, r, mesh_horcnt, mesh_vercnt);

    // In Each Mode, for Every Region:
    for (int rgn_idx = 0; rgn_idx < cfg->RgnNum; rgn_idx++) {
        // check region valid first
        if (!rgn_attr[rgn_idx].RegionValid)
            return BM_ERR_PARAM;

        // Destination Mesh-Info Allocation
        int view_w = rgn_attr[rgn_idx].OutW;
        int view_h = rgn_attr[rgn_idx].OutH;
        // int mesh_horcnt = rgn_attr[rgn_idx].MeshHor;
        // int mesh_vercnt = rgn_attr[rgn_idx].MeshVer;

        // get & store region mesh info.
        ret |= _get_region_dst_mesh_list(rgn_attr, view_w, view_h, mesh_horcnt, mesh_vercnt, rgn_idx);

        // Get Source Mesh-Info Projected from Destination by Differet ViewModw.
        ldc_get_region_src_mesh_list(rgn_attr, rgn_idx, x0, y0, &ldc_meshdata, pstLDCAttr); //, mat0);

        // debug msg
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "REGION %d: done.\n", rgn_idx);
    }

    //combine all region meshs - mesh projection done.
    _get_frame_mesh_list(cfg, rgn_attr);

    // modify frame size
    int dst_height_s1 = src_height_s1;
    int dst_width_s1  = src_width_s1;
    int src_height_s2 = dst_width_s1;
    int src_width_s2  = dst_height_s1;
    int dst_height_s2 = src_height_s2;
    int dst_width_s2  = src_width_s2;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "dst_height_s1 = %d,\n", dst_height_s1);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "dst_width_s1 = %d,\n", dst_width_s1);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "src_height_s2 = %d,\n", src_height_s2);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "src_width_s2 = %d,\n", src_width_s2);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "dst_height_s2 = %d,\n", dst_height_s2);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "dst_width_s2 = %d,\n", dst_width_s2);

    // 1st-stage, in(1984, 1088 ) -> out(1984, 1088)
    // 2nd-stage, in(1088, 1984 ) -> out(1088, 1984)

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "1st-stage, in(%d, %d ) -> out(%d, %d)\n",
                src_width_s1, src_height_s1, dst_width_s1, dst_height_s1);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "2nd-stage, in(%d, %d ) -> out(%d, %d)\n",
                src_width_s2, src_height_s2, dst_width_s2, dst_height_s2);

    // tileNum: 1st stage =( 31 X 17 )   2nd stage = ( 17 X 31 )

    int num_tilex_s1 = dst_width_s1 / TILESIZE;
    int num_tiley_s1 = dst_height_s1 / TILESIZE;
    int num_tilex_s2 = dst_width_s2 / TILESIZE;
    int num_tiley_s2 = dst_height_s2 / TILESIZE;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tileNum: 1st stage =( %d X %d )   2nd stage = ( %d X %d )\n",
                                                num_tilex_s1, num_tiley_s1, num_tilex_s2, num_tiley_s2);

    // to calculate each stage source mesh x-data.
    ret |= _offline_get_1st_src_mesh_table(num_tiley_s1, num_tilex_s1, cfg, src_1st_list, mesh_horcnt, mesh_vercnt, pstLDCAttr);

    ret |= _offline_get_2nd_src_mesh_table(reg.stage2_rotate_type, num_tiley_s2, num_tilex_s2, cfg,
                                          src_2nd_list, src_width_s1, src_height_s1, mesh_2nd_size, mesh_horcnt, mesh_vercnt);

    convert_1st_src_mesh_table(num_tiley_s1, num_tilex_s1, src_1st_list, src_1st_list_1d, mesh_1st_size);

    convert_2nd_src_mesh_table(num_tiley_s2, num_tilex_s2, src_2nd_list, src_2nd_list_1d, mesh_2nd_size);

    free(src_1st_list);
    free(src_2nd_list);
    free(cfg);
    free(rgn_attr);

    return ret;
}

static u8 ldc_get_idle_tsk_mesh(void)
{
    u8 i = LDC_MAX_TSK_MESH;

    for (i = 0; i < LDC_MAX_TSK_MESH; i++) {
        if (strcmp(tsk_mesh[i].Name, "") == 0 && !tsk_mesh[i].paddr && !tsk_mesh[i].vaddr)
            break;
    }
    return i;
}

bm_status_t bm_ldc_gen_gdc_mesh(bm_handle_t handle,
                                int width,
                                int height,
                                const LDC_ATTR_S *pstLDCAttr,
                                const char *name,
                                uint64_t *pu64PhyAddr,
                                void **ppVirAddr,
                                char *grid)
{
    bm_status_t ret = BM_SUCCESS;
    uint64_t paddr = 0;
    // void *vaddr;
    SIZE_S in_size, out_size;
    unsigned int mesh_1st_size = 0, mesh_2nd_size = 0, mesh_size = 0;
    bmcv_rot_mode enRotation = BMCV_ROTATION_0;

    in_size.u32Width  = (width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    in_size.u32Height = (height  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    out_size.u32Width = in_size.u32Width;
    out_size.u32Height = in_size.u32Height;

    u8 idx = ldc_get_valid_tsk_mesh_by_name(name);      // 32
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "idx in bm_ldc_gen_gdc_mesh = %d\n", idx);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "name in bm_ldc_gen_gdc_mesh = %s\n", name);

    bm_device_mem_t dmem;
    if (idx >= LDC_MAX_TSK_MESH)
    {
        mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);
        mesh_size = mesh_1st_size + mesh_2nd_size;

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "W = %d, H = %d, mesh_size = %d\n", in_size.u32Width, in_size.u32Height, mesh_size);

        ret = bm_malloc_device_byte(handle, &dmem, mesh_size);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_malloc_device_byte failed: %s\n", strerror(errno));
            return BM_ERR_FAILURE;
        }

        unsigned char *buffer = (unsigned char *)malloc(mesh_size);
        if (buffer == NULL) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "malloc buffer for mesh failed!\n");
            return BM_ERR_NOMEM;
        }
        memset(buffer, 0, mesh_size);

        ret = mesh_gen_ldc(in_size, out_size, pstLDCAttr, paddr, (void *)buffer, enRotation, grid);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Can't generate ldc mesh.\n");
            free(buffer);
            return BM_ERR_PARAM;
        }

        ret = bm_memcpy_s2d(handle, dmem, (void*)buffer);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_memcpy_s2d failed!\n");
            free(buffer);
            return BM_ERR_NOMEM;
        }
        free(buffer);
#if GDC_DUMP_MESH
        char mesh_name[128];
        snprintf(mesh_name, 128, "./mesh_%d_%d_%d_%d_%d_%d_%d.bin"
            , in_size.u32Width, in_size.u32Height, pstLDCAttr->s32XRatio, pstLDCAttr->s32YRatio
            , pstLDCAttr->s32CenterXOffset, pstLDCAttr->s32CenterYOffset, pstLDCAttr->s32DistortionRatio);

        FILE *fp = fopen(mesh_name, "wb");
        if (!fp) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "open file[%s] failed.\n", mesh_name);
            goto dump_fail;
        }
        if (fwrite((void *)ALIGN((CVI_U64)vaddr, DEFAULT_ALIGN), mesh_size, 1, fp) != 1)
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "fwrite data error\n");
        fclose(fp);
dump_fail:
#endif

        idx = ldc_get_idle_tsk_mesh();
        if (idx >= LDC_MAX_TSK_MESH) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "tsk mesh count(%d) is out of range(%d)\n", idx + 1, LDC_MAX_TSK_MESH);
            return BM_ERR_FAILURE;
        }

        strcpy(tsk_mesh[idx].Name, name);
        tsk_mesh[idx].paddr = (uint64_t)dmem.u.device.device_addr;
        tsk_mesh[idx].vaddr = NULL;

        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "idx in bm_ldc_gen_gdc_mesh for loop = %d\n", idx);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].Name in bm_ldc_gen_gdc_mesh for loop = %s\n", idx, tsk_mesh[idx].Name);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].paddr in bm_ldc_gen_gdc_mesh for loop = %#"PRIx64"\n", idx, (uint64_t)tsk_mesh[idx].paddr);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].vaddr in bm_ldc_gen_gdc_mesh for loop = %p\n", idx, tsk_mesh[idx].vaddr);

    }
    *pu64PhyAddr = (uint64_t)dmem.u.device.device_addr;
    *ppVirAddr = tsk_mesh[idx].vaddr;
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "In bm_ldc_gen_gdc_mesh, pu64PhyAddr = %#"PRIx64", ppVirAddr = %p.\n", *pu64PhyAddr, *ppVirAddr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk mesh(%#"PRIx64")\n", *pu64PhyAddr);

    return BM_SUCCESS;
}

bm_status_t bm_ldc_save_gdc_mesh(bm_handle_t handle,
                                int width,
                                int height,
                                const LDC_ATTR_S *pstLDCAttr,
                                bm_device_mem_t *dmem,
                                const char *name,
                                uint64_t *pu64PhyAddr,
                                void **ppVirAddr)
{
    bm_status_t ret = BM_SUCCESS;
    uint64_t paddr = 0;
    SIZE_S in_size, out_size;
    unsigned int mesh_1st_size = 0, mesh_2nd_size = 0, mesh_size = 0;
    bmcv_rot_mode enRotation = BMCV_ROTATION_0;

    in_size.u32Width  = ALIGN(width, DEFAULT_ALIGN);
    in_size.u32Height = ALIGN(height, DEFAULT_ALIGN);
    out_size.u32Width = in_size.u32Width;
    out_size.u32Height = in_size.u32Height;

    u8 idx = ldc_get_valid_tsk_mesh_by_name(name);      // 32

    if (idx >= LDC_MAX_TSK_MESH)
    {
        mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);
        mesh_size = mesh_1st_size + mesh_2nd_size;

        unsigned char *buffer = (unsigned char *)malloc(mesh_size);
        if (buffer == NULL) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "malloc buffer for mesh failed!\n");
            return BM_ERR_NOMEM;
        }
        memset(buffer, 0, mesh_size);

        ret = mesh_gen_ldc(in_size, out_size, pstLDCAttr, paddr, (void *)buffer, enRotation, NULL);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Can't generate ldc mesh.\n");
            free(buffer);
            return BM_ERR_PARAM;
        }

        ret = bm_memcpy_s2d(handle, *dmem, (void*)buffer);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_memcpy_s2d failed!\n");
            free(buffer);
            return BM_ERR_NOMEM;
        }
        free(buffer);

        idx = ldc_get_idle_tsk_mesh();
        if (idx >= LDC_MAX_TSK_MESH) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "tsk mesh count(%d) is out of range(%d)\n", idx + 1, LDC_MAX_TSK_MESH);
            return BM_ERR_FAILURE;
        }

        strcpy(tsk_mesh[idx].Name, name);
        tsk_mesh[idx].paddr = (uint64_t)(*dmem).u.device.device_addr;
        tsk_mesh[idx].vaddr = NULL;

        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "idx in bm_ldc_save_gdc_mesh = %d\n", idx);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].Name in bm_ldc_save_gdc_mesh for = %s\n", idx, tsk_mesh[idx].Name);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].paddr in bm_ldc_save_gdc_mesh for = %#"PRIx64"\n", idx, (uint64_t)tsk_mesh[idx].paddr);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].vaddr in bm_ldc_save_gdc_mesh for = %p\n", idx, tsk_mesh[idx].vaddr);

    }
    *pu64PhyAddr = (uint64_t)(*dmem).u.device.device_addr;
    *ppVirAddr = tsk_mesh[idx].vaddr;
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "In bm_ldc_save_gdc_mesh, pu64PhyAddr = %#"PRIx64", ppVirAddr = %p.\n", *pu64PhyAddr, *ppVirAddr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk mesh(%#"PRIx64")\n", *pu64PhyAddr);

    return BM_SUCCESS;
}

bm_status_t bm_ldc_load_gdc_mesh(bm_handle_t handle,
                                 int width,
                                 int height,
                                 bm_device_mem_t *dmem,
                                 const char *tskName,
                                 uint64_t *pu64PhyAddr,
                                 void **ppVirAddr)
{
    void *vaddr = 0;

    if (!tskName) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Please asign task name for LDC.\n");
        return BM_ERR_PARAM;
    }

    u8 idx = ldc_get_valid_tsk_mesh_by_name(tskName);      // 32
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "idx in bm_ldc_load_gdc_mesh = %d\n", idx);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "name in bm_ldc_load_gdc_mesh = %s\n", tskName);

    if (idx >= LDC_MAX_TSK_MESH) {

        idx = ldc_get_idle_tsk_mesh();      // 0
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "idx = %d \n", idx + 1);
        if (idx >= LDC_MAX_TSK_MESH) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk mesh count(%d) is out of range(%d)\n", idx + 1, LDC_MAX_TSK_MESH);
            return BM_ERR_FAILURE;
        }
        strcpy(tsk_mesh[idx].Name, tskName);
        tsk_mesh[idx].paddr = (uint64_t)(*dmem).u.device.device_addr;
        tsk_mesh[idx].vaddr = (void*)vaddr;

        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "idx in bm_ldc_load_gdc_mesh for = %d\n", idx);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].Name in bm_ldc_load_gdc_mesh for = %s\n", idx, tsk_mesh[idx].Name);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].paddr in bm_ldc_load_gdc_mesh for = %#"PRIx64"\n", idx, (uint64_t)tsk_mesh[idx].paddr);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].paddr in bm_ldc_load_gdc_mesh for = %p\n", idx, tsk_mesh[idx].vaddr);
    }
    *pu64PhyAddr = (uint64_t)(*dmem).u.device.device_addr;
    *ppVirAddr = tsk_mesh[idx].vaddr;
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "In bm_ldc_load_gdc_mesh, pu64PhyAddr = %#"PRIx64", ppVirAddr = %p.\n", *pu64PhyAddr, *ppVirAddr);

    return BM_SUCCESS;
}

bm_status_t bm_ldc_end_job(int fd, GDC_HANDLE hHandle)
{
    bm_status_t ret = BM_SUCCESS;
    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_end_job failed, null hHandle for test param.\n");
        return BM_ERR_PARAM;
    }

    struct gdc_handle_data cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.handle = hHandle;

    ret = (bm_status_t)ioctl(fd, CVI_LDC_END_JOB, &cfg);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_end_job failed!\n");
        ret = BM_ERR_FAILURE;
    }
    return ret;
}

bm_status_t bm_image_format_to_cvi_format(bm_image_format_ext fmt, bm_image_data_format_ext datatype, PIXEL_FORMAT_E * cvi_fmt) {
    if(datatype != DATA_TYPE_EXT_1N_BYTE) {
        switch(datatype) {
            case DATA_TYPE_EXT_1N_BYTE_SIGNED:
                *cvi_fmt = PIXEL_FORMAT_INT8_C3_PLANAR;
                break;
            case DATA_TYPE_EXT_FLOAT32:
                *cvi_fmt = PIXEL_FORMAT_FP32_C3_PLANAR;
                break;
            case DATA_TYPE_EXT_FP16:
                *cvi_fmt = PIXEL_FORMAT_FP16_C3_PLANAR;
                break;
            case DATA_TYPE_EXT_BF16:
                *cvi_fmt = PIXEL_FORMAT_BF16_C3_PLANAR;
                break;
            default:
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "datatype(%d) not supported, %s: %s: %d\n", datatype, filename(__FILE__), __func__, __LINE__);
                return BM_NOT_SUPPORTED;
        }
        return BM_SUCCESS;
    }
    switch(fmt) {
        case FORMAT_YUV420P:
        case FORMAT_COMPRESSED:
            *cvi_fmt = PIXEL_FORMAT_YUV_PLANAR_420;
            break;
        case FORMAT_YUV422P:
            *cvi_fmt = PIXEL_FORMAT_YUV_PLANAR_422;
            break;
        case FORMAT_YUV444P:
            *cvi_fmt = PIXEL_FORMAT_YUV_PLANAR_444;
            break;
        case FORMAT_NV12:
            *cvi_fmt = PIXEL_FORMAT_NV12;
            break;
        case FORMAT_NV21:
            *cvi_fmt = PIXEL_FORMAT_NV21;
            break;
        case FORMAT_NV16:
            *cvi_fmt = PIXEL_FORMAT_NV16;
            break;
        case FORMAT_NV61:
            *cvi_fmt = PIXEL_FORMAT_NV61;
            break;
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_RGB_PLANAR:
            *cvi_fmt = PIXEL_FORMAT_RGB_888_PLANAR;
            break;
        case FORMAT_BGRP_SEPARATE:
        case FORMAT_BGR_PLANAR:
            *cvi_fmt = PIXEL_FORMAT_BGR_888_PLANAR;
            break;
        case FORMAT_RGB_PACKED:
            *cvi_fmt = PIXEL_FORMAT_RGB_888;
            break;
        case FORMAT_BGR_PACKED:
            *cvi_fmt = PIXEL_FORMAT_BGR_888;
            break;
        case FORMAT_HSV_PLANAR:
            *cvi_fmt = PIXEL_FORMAT_HSV_888_PLANAR;
            break;
        case FORMAT_ARGB_PACKED:
            *cvi_fmt = PIXEL_FORMAT_ARGB_8888;
            break;
        case FORMAT_YUV444_PACKED:
            *cvi_fmt = PIXEL_FORMAT_RGB_888;
            break;
        case FORMAT_GRAY:
            *cvi_fmt = PIXEL_FORMAT_YUV_400;
            break;
        case FORMAT_YUV422_YUYV:
            *cvi_fmt = PIXEL_FORMAT_YUYV;
            break;
        case FORMAT_YUV422_YVYU:
            *cvi_fmt = PIXEL_FORMAT_YVYU;
            break;
        case FORMAT_YUV422_UYVY:
            *cvi_fmt = PIXEL_FORMAT_UYVY;
            break;
        case FORMAT_YUV422_VYUY:
            *cvi_fmt = PIXEL_FORMAT_VYUY;
            break;
        default:
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "fmt(%d) not supported, %s: %s: %d\n", fmt, filename(__FILE__), __func__, __LINE__);
            return BM_NOT_SUPPORTED;
    }
    return BM_SUCCESS;
}

bm_status_t bm_image_to_cvi_frame(bm_image *image,
                                  PIXEL_FORMAT_E enPixelFormat,
                                  VIDEO_FRAME_INFO_S *stVideoFrame)
{
    stVideoFrame->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    stVideoFrame->stVFrame.enPixelFormat = enPixelFormat;
    stVideoFrame->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stVideoFrame->stVFrame.enColorGamut = COLOR_GAMUT_BT709;
    stVideoFrame->stVFrame.u32Width = image->width;
    stVideoFrame->stVFrame.u32Height = image->height;
    stVideoFrame->stVFrame.u32TimeRef = 0;
    stVideoFrame->stVFrame.u64PTS = 0;
    stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;

    stVideoFrame->u32PoolId = 0;
    for (int i = 0; i < image->image_private->plane_num; ++i) {
        stVideoFrame->stVFrame.u32Stride[i] =  image->image_private->memory_layout[i].pitch_stride;
        stVideoFrame->stVFrame.u32Length[i] = image->image_private->memory_layout[i].size;
        stVideoFrame->stVFrame.u64PhyAddr[i] = image->image_private->data[i].u.device.device_addr;
        stVideoFrame->stVFrame.pu8VirAddr[i] = (unsigned char*)image->image_private->data[i].u.system.system_addr;
    }
    if (image->image_private->plane_num >= 3) {
        stVideoFrame->stVFrame.u32Length[2] = image->image_private->memory_layout[2].size;
        stVideoFrame->stVFrame.u64PhyAddr[2] = image->image_private->data[2].u.device.device_addr;
    }
    if (image->image_format == FORMAT_COMPRESSED) {
        stVideoFrame->stVFrame.u64ExtPhyAddr = image->image_private->data[3].u.device.device_addr;
        stVideoFrame->stVFrame.enCompressMode = COMPRESS_MODE_FRAME;
    }

    return BM_SUCCESS;
}

static bm_status_t ldc_free_cur_tsk_mesh(char* meshName)
{
    bm_status_t ret = BM_SUCCESS;
    u8 i = LDC_MAX_TSK_MESH;

    i = ldc_get_valid_tsk_mesh_by_name2(meshName);
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "i in ldc_free_cur_tsk_mesh = %d\n", i);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].paddr = %#"PRIx64"\n", i, (uint64_t)tsk_mesh[i].paddr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "tsk_mesh[%d].vaddr = %p\n", i, tsk_mesh[i].vaddr);

    if (i < LDC_MAX_TSK_MESH && tsk_mesh[i].paddr /*&& tsk_mesh[i].vaddr*/)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "start free tsk mesh[%s]\n", meshName);
        tsk_mesh[i].paddr = 0;
        tsk_mesh[i].vaddr = 0;
        memset(tsk_mesh[i].Name, 0, sizeof(tsk_mesh[i].Name));
    }
    return ret;
}

bm_status_t bm_ldc_free_cur_task_mesh(char *tskName)
{
    return ldc_free_cur_tsk_mesh(tskName);
}

bm_status_t bm_ldc_free_all_tsk_mesh(void)
{
    bm_status_t ret = BM_SUCCESS;
    for (u8 i = 0; i < LDC_MAX_TSK_MESH; i++)
    {
        if (tsk_mesh[i].paddr && tsk_mesh[i].vaddr)
        {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "start free tsk mesh[%s]\n", tsk_mesh[i].Name);
            tsk_mesh[i].paddr = 0;
            tsk_mesh[i].vaddr = 0;
            memset(tsk_mesh[i].Name, 0, sizeof(tsk_mesh[i].Name));
        } else {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "Failed to free %d-th tsk mesh[%s]\n", i, tsk_mesh[i].Name);
            ret = BM_ERR_PARAM;
        }
    }
    return ret;
}

bm_status_t bm_ldc_send_frame(int fd,
                              bm_image *input_img,
                              bm_image *output_img,
                              bm_ldc_basic_param *param)
{
    bm_status_t ret = BM_SUCCESS;
    PIXEL_FORMAT_E pixel_format = 0;

    memset(&param->stVideoFrameIn, 0, sizeof(param->stVideoFrameIn));
    memset(&param->stVideoFrameOut, 0, sizeof(param->stVideoFrameOut));
    bm_image_format_to_cvi_format(input_img->image_format, input_img->data_type, &pixel_format);
    // bm_image_format_to_cvi(output_img->image_format, output_img->data_type, &pixel_format);
    bm_image_to_cvi_frame(input_img, pixel_format, &param->stVideoFrameIn);
    bm_image_to_cvi_frame(output_img, pixel_format, &param->stVideoFrameOut);

    memcpy(&param->stTask.stImgIn, &param->stVideoFrameIn, sizeof(param->stVideoFrameIn));
    memcpy(&param->stTask.stImgOut, &param->stVideoFrameOut, sizeof(param->stVideoFrameOut));

    return ret;
}

bm_status_t bm_ldc_get_frame(int fd, bm_image *output_img, bm_ldc_basic_param *param)
{
    bm_status_t ret = BM_SUCCESS;
    PIXEL_FORMAT_E pixel_format = 0;

    bm_image_format_to_cvi_format(output_img->image_format, output_img->data_type, &pixel_format);
    bm_image_to_cvi_frame(output_img, pixel_format, &param->stVideoFrameOut);

    memcpy(&param->stVideoFrameOut, &param->stTask.stImgOut, sizeof(param->stTask.stImgOut));

    for(int i = 0; i < output_img->image_private->plane_num; i++) {
        output_img->image_private->data[i].u.device.device_addr = param->stVideoFrameOut.stVFrame.u64PhyAddr[i];
    }

    return ret;
}

bm_status_t bm_ldc_cancel_job(int fd, GDC_HANDLE hHandle)
{
    bm_status_t ret = BM_SUCCESS;
    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_cancel_job failed, null hHandle for test param.\n");
        return BM_ERR_PARAM;
    }

    struct gdc_handle_data cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.handle = hHandle;
    ret = (bm_status_t)ioctl(fd, CVI_LDC_CANCEL_JOB, &cfg);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_cancel_job failed!\n");
        ret = BM_ERR_FAILURE;
    }
    return ret;
}

bm_status_t bm_ldc_init(int fd)
{
    bm_status_t ret;
    ret = (bm_status_t)ioctl(fd, CVI_LDC_INIT);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Init LDC failed!\n");
        return ret;
    }
    return BM_SUCCESS;
}

bm_status_t bm_ldc_deinit(int fd)
{
    bm_status_t ret;
    ret = (bm_status_t)ioctl(fd, CVI_LDC_DEINIT);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Deinit LDC failed!\n");
        return ret;
    }
    ret = bm_ldc_free_all_tsk_mesh();
    return BM_SUCCESS;
}

bm_status_t bm_ldc_begin_job(int fd, GDC_HANDLE *phHandle)
{
    bm_status_t ret;

    struct gdc_handle_data cfg;
    memset(&cfg, 0, sizeof(cfg));

    ret = (bm_status_t)ioctl(fd, CVI_LDC_BEGIN_JOB, &cfg);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "LDC begin_job failed!\n");
        return ret;
    }
    *phHandle = cfg.handle;
    return BM_SUCCESS;
}

bm_status_t bm_ldc_set_job_identity(int fd, GDC_HANDLE hHandle, GDC_IDENTITY_ATTR_S *identity_attr)
{
    bm_status_t ret = BM_SUCCESS;
    struct gdc_identity_attr cfg = {0};
    cfg.handle = hHandle;
    memcpy(&cfg.attr, identity_attr, sizeof(*identity_attr));

    ret = (bm_status_t)ioctl(fd, CVI_LDC_SET_JOB_IDENTITY, &cfg);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "LDC set_job_identity failed!\n");
        return ret;
    }
    return BM_SUCCESS;
}

bm_status_t bm_ldc_add_rotation_task(int fd, GDC_HANDLE hHandle, GDC_TASK_ATTR_S *pstTask, ROTATION_E enRotation)
{
    bm_status_t ret;
    /* TODO:Add MOD_CHECK_NULL_PTR and CHECK_LDC_FORMAT */
    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_add_rotation_task failed, null hHandle for test param.\n");
        return BM_ERR_PARAM;
    }

    if (enRotation != ROTATION_0 && enRotation != ROTATION_90 && enRotation != ROTATION_270)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_add_rotation_task failed, do not support this rotation.\n");
        return BM_NOT_SUPPORTED;
    }

    struct gdc_task_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;

    memcpy(&attr.stImgIn, &pstTask->stImgIn, sizeof(attr.stImgIn));
    memcpy(&attr.stImgOut, &pstTask->stImgOut, sizeof(attr.stImgOut));
    attr.enRotation = enRotation;

    ret = (bm_status_t)ioctl(fd, CVI_LDC_ADD_ROT_TASK, &attr);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "LDC ldc_add_rotation_task failed!\n");
        return ret;
    }
    return BM_SUCCESS;
}

bm_status_t bm_ldc_add_gdc_task(bm_handle_t handle,
                                int fd,
                                GDC_HANDLE hHandle,
                                GDC_TASK_ATTR_S *pstTask,
                                const LDC_ATTR_S *pstLDCAttr,
                                bmcv_rot_mode enRotation,
                                bm_device_mem_t dmem)
{
    /*
        TODO:
        add MOD_CHECK_NULL_PTR and CHECK_GDC_FORMAT
    */
    bm_status_t ret = BM_SUCCESS;
    bmcv_rot_mode rot[2];
    if (enRotation == 1)
    {
        rot[0] = BMCV_ROTATION_90;
        rot[1] = BMCV_ROTATION_0;
    } else if (enRotation == 2)
    {
        rot[0] = BMCV_ROTATION_90;
        rot[1] = BMCV_ROTATION_90;
    } else if (enRotation == 3)
    {
        rot[0] = BMCV_ROTATION_270;
        rot[1] = BMCV_ROTATION_0;
    } else {
        rot[0] = BMCV_ROTATION_90;
        rot[1] = BMCV_ROTATION_270;
    }

    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_add_gdc_task failed, null hHandle for test param.\n");
        return BM_ERR_PARAM;
    }
    if (enRotation > BMCV_ROTATION_0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Do not support rotation (%d) when use ldc.\n", enRotation);
        return BM_NOT_SUPPORTED;
    }

    if (pstLDCAttr->stGridInfoAttr.Enable) {
        rot[0] = BMCV_ROTATION_270;
        rot[1] = BMCV_ROTATION_90;
    }

    struct gdc_task_attr attr;
    SIZE_S stSizeTmp;
    PIXEL_FORMAT_E enPixelFormatTmp = pstTask->stImgIn.stVFrame.enPixelFormat;
    VIDEO_FRAME_INFO_S stVideoFrameTmp;
    u32 mesh_1st_size;

    stSizeTmp.u32Width  = ALIGN(pstTask->stImgIn.stVFrame.u32Height, LDC_ALIGN);
    stSizeTmp.u32Height = ALIGN(pstTask->stImgIn.stVFrame.u32Width, LDC_ALIGN);

    ret = bm_ldc_comm_cfg_frame(handle, dmem, &stSizeTmp, enPixelFormatTmp, &stVideoFrameTmp);
    if (ret != BM_SUCCESS)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_comm_cfg_frame failed!\n");
        return ret;
    }

    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.stImgIn, &pstTask->stImgIn, sizeof(attr.stImgIn));
    memcpy(&attr.stImgOut, &stVideoFrameTmp, sizeof(attr.stImgOut));
    attr.au64privateData[0] = pstTask->au64privateData[0];
    attr.reserved = pstTask->reserved;
    attr.enRotation = rot[0];
    ret = (bm_status_t)ioctl(fd, CVI_LDC_ADD_LDC_TASK, &attr);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "LDC ldc_add_gdc_task stage-1 failed!\n");
        return ret;
    }

    mesh_gen_get_1st_size(stSizeTmp, &mesh_1st_size);
    memcpy(&attr.stImgIn, &stVideoFrameTmp, sizeof(attr.stImgIn));
    memcpy(&attr.stImgOut, &pstTask->stImgOut, sizeof(attr.stImgOut));
    attr.au64privateData[0] = pstTask->au64privateData[0] + mesh_1st_size;
    attr.enRotation = rot[1];
    ret = (bm_status_t)ioctl(fd, CVI_LDC_ADD_LDC_TASK, &attr);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "LDC ldc_add_gdc_task stage-2 failed!\n");
        return ret;
    }

    return ret;
}

static bm_status_t bm_ldc_basic_add_tsk(bm_handle_t handle, int fd, bm_ldc_basic_param *param, void *ptr, bm_device_mem_t mid_dmem)
{
    ROTATION_E enRotation;
    uint64_t u64PhyAddr;
    void *pVirAddr;
    bm_device_mem_t *a2dem;
    bm_gen_mesh_param *gen_mesh_param;
    bm_gdc_attr_and_grid_info *gdc_attr_and_grid;

    bm_status_t ret = BM_SUCCESS;
    if (!param) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_basic_add_tsk failed, null ptr for test param.\n");
        return BM_ERR_PARAM;
    }

    switch (param->op) {
        case BM_LDC_ROT:
            enRotation = *(ROTATION_E *)ptr;
            ret = bm_ldc_add_rotation_task(fd, param->hHandle, &param->stTask, enRotation);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_add_rotation_task failed!\n");
                ret = BM_ERR_FAILURE;
            }
            break;
        case BM_LDC_GDC:
            gdc_attr_and_grid = (bm_gdc_attr_and_grid_info *)ptr;
            ret = bm_ldc_gen_gdc_mesh(handle, param->size_in.u32Width, param->size_in.u32Height,
                                      &(gdc_attr_and_grid->ldc_attr), param->stTask.name, &u64PhyAddr, &pVirAddr, gdc_attr_and_grid->grid);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_gen_gdc_mesh(%s) fail\n", param->stTask.name);
                ret = BM_ERR_FAILURE;
            }
            param->stTask.au64privateData[0] = u64PhyAddr;
            ret = bm_ldc_add_gdc_task(handle, fd, param->hHandle, &param->stTask, &(gdc_attr_and_grid->ldc_attr), param->enRotation, mid_dmem);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_add_gdc_task in BM_LDC_GDC failed!\n");
                ret = BM_ERR_FAILURE;
            }
            break;
        case BM_LDC_GDC_GEN_MESH:
            gen_mesh_param = (bm_gen_mesh_param *)ptr;
            ret = bm_ldc_save_gdc_mesh(handle, param->size_in.u32Width, param->size_in.u32Height,
                                      &(gen_mesh_param->ldc_attr), &(gen_mesh_param->dmem), param->stTask.name, &u64PhyAddr, &pVirAddr);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_save_gdc_mesh(%s) fail\n", param->stTask.name);
                ret = BM_ERR_FAILURE;
            }
            param->stTask.au64privateData[0] = u64PhyAddr;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "In bm_ldc_basic_add_tsk, param->stTask.au64privateData[0] = %#"PRIx64"\n", param->stTask.au64privateData[0]);
            break;
        case BM_LDC_GDC_LOAD_MESH:
            a2dem = (bm_device_mem_t *)ptr;
            LDC_ATTR_S stLDCAttr = {0};
            ret = bm_ldc_load_gdc_mesh(handle, param->size_out.u32Width, param->size_out.u32Height,
                                      a2dem, param->stTask.name, &u64PhyAddr, &pVirAddr);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_load_gdc_mesh(%s) fail\n", param->stTask.name);
                ret = BM_ERR_FAILURE;
            }
            param->stTask.au64privateData[0] = u64PhyAddr;
            ret = bm_ldc_add_gdc_task(handle, fd, param->hHandle, &param->stTask, &stLDCAttr, BMCV_ROTATION_0, mid_dmem);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_add_gdc_task in BM_LDC_GDC_LOAD_MESH failed!\n");
                ret = BM_ERR_FAILURE;
            }
            break;
        default:
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not allow this op(%d) failed!\n", param->op);
            break;
    }
    return ret;
}

bm_status_t bm_ldc_basic(bm_handle_t handle,
                         bm_image in_image,
                         bm_image out_image,
                         bm_ldc_basic_param *param,
                         void *ptr)
{
    bm_status_t ret = BM_SUCCESS;
    int fd = 0;
    ret = bm_get_ldc_fd(&fd);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "get ldc fd failed!\n");
        return BM_ERR_DEVNOTREADY;
    }

    memset(&param->stVideoFrameIn, 0, sizeof(param->stVideoFrameIn));
    memset(&param->stVideoFrameOut, 0, sizeof(param->stVideoFrameOut));

    ret = bm_ldc_send_frame(fd, &in_image, &out_image, param);
    if (ret != BM_SUCCESS)
       return ret;

    // Init LDC
    ret = bm_ldc_init(fd);
    if (ret != BM_SUCCESS)
       return ret;

    param->hHandle = 0;

    ret = bm_ldc_begin_job(fd, &param->hHandle);
    if (ret != BM_SUCCESS)
       goto fail0;

    ret = bm_ldc_set_job_identity(fd, param->hHandle, &param->identity);
    if (ret != BM_SUCCESS)
       goto fail0;

    bm_device_mem_t mid_mem;
    unsigned int dem_size = 0;
    if (param->op == BM_LDC_GDC || param->op == BM_LDC_GDC_LOAD_MESH)
    {
        if (param->enPixelFormat == PIXEL_FORMAT_NV21 || param->enPixelFormat == PIXEL_FORMAT_NV12) {
            dem_size = param->size_out.u32Width * param->size_out.u32Height * 3 / 2;
        } else {
            dem_size = param->size_out.u32Width * param->size_out.u32Height;
        }
        ret = bm_malloc_device_byte(handle, &mid_mem, dem_size);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_malloc_device_byte failed: %s\n", strerror(errno));
            goto fail1;
        }
    }

    ret = bm_ldc_basic_add_tsk(handle, fd, param, ptr, mid_mem);
    if (ret != BM_SUCCESS)
        goto fail2;

    if (param->op == BM_LDC_GDC_GEN_MESH) {
        goto fail2;
    }

    ret = bm_ldc_end_job(fd, param->hHandle);
    if (ret != BM_SUCCESS)
        goto fail2;

    if (param->op == BM_LDC_ROT || param->op == BM_LDC_GDC || param->op == BM_LDC_GDC_LOAD_MESH) {
        ret = bm_ldc_get_frame(fd, &out_image, param);
        if (ret != BM_SUCCESS)
        goto fail2;
    }

fail2:
    ret = bm_ldc_free_cur_task_mesh(param->stTask.name);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "name in bm_ldc_free_cur_task_mesh = %s\n", param->stTask.name);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_free_cur_task_mesh failed!\n");
    }
fail1:
    if (param->op == BM_LDC_GDC || param->op == BM_LDC_GDC_LOAD_MESH)
    {
        bm_free_device(handle, mid_mem);
    }

    // ret |= bm_ldc_deinit(fd);
    // if (ret != BM_SUCCESS) {
    //     bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_deinit failed!\n");
    //     return ret;
    // }
fail0:
    if (ret) {
        if (param->hHandle) {
            ret |= bm_ldc_cancel_job(fd, param->hHandle);
        }
    }
    return ret;
}

bm_status_t check_bm_ldc_image_param(bm_image *input_img, bm_image *output_img) {
    bm_status_t ret = BM_SUCCESS;

    if((input_img->data_type != DATA_TYPE_EXT_1N_BYTE)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Data type only support DATA_TYPE_EXT_1N_BYTE!\n");
        return BM_ERR_FAILURE;
    }

    if((output_img->data_type != DATA_TYPE_EXT_1N_BYTE)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Data type not support!\n");
        return BM_ERR_FAILURE;
    }

    if((input_img->image_format != FORMAT_GRAY) && (input_img->image_format != FORMAT_NV12) && (input_img->image_format != FORMAT_NV21)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Img format only supported FORMAT_GRAY/FORMAT_NV12/FORMAT_NV21!\n");
        return BM_ERR_FAILURE;
    }

    if((input_img->width < 64) || (input_img->width > 4608) || (input_img->height < 64) || (input_img->height > 4608)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "The width and height of img error!\n");
        return BM_ERR_FAILURE;
    }

    if(input_img->width % 64 != 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Error! The img width should 64 align!\n");
        return BM_ERR_FAILURE;
    }

    return ret;
}

bm_status_t check_ldc_attr_param(LDC_ATTR_S *ldc_attr) {
    if (ldc_attr->bAspect == false) {
        if (ldc_attr->s32XRatio < 0 || ldc_attr->s32XRatio > 100) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_attr->s32XRatio should be in [0, 100]!\n");
            return BM_ERR_FAILURE;
        }
        if (ldc_attr->s32YRatio < 0 || ldc_attr->s32YRatio > 100) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_attr->s32YRatio should be in [0, 100]!\n");
            return BM_ERR_FAILURE;
        }
    } else {
        if (ldc_attr->s32XYRatio < 0 || ldc_attr->s32YRatio > 100) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_attr->s32YRatio should be in [0, 100]!\n");
            return BM_ERR_FAILURE;
        }
    }
    if (ldc_attr->s32CenterXOffset < -511 || ldc_attr->s32CenterXOffset > 511) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_attr->s32CenterXOffset should be in [-511, 511]!\n");
        return BM_ERR_FAILURE;
    }
    if (ldc_attr->s32CenterYOffset < -511 || ldc_attr->s32CenterYOffset > 511) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_attr->s32CenterYOffset should be in [-511, 511]!\n");
        return BM_ERR_FAILURE;
    }
    if (ldc_attr->s32DistortionRatio < -300 || ldc_attr->s32DistortionRatio > 500) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ldc_attr->s32DistortionRatio should be in [-300, 500]!\n");
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t bm_ldc_rot_internal(bm_handle_t          handle,
                                bm_image             in_image,
                                bm_image             out_image,
                                bmcv_rot_mode        rot_mode)
{
    bm_status_t ret = BM_SUCCESS;
    bm_ldc_basic_param param = {0};
    ROTATION_E rotation_mode = (ROTATION_E)rot_mode;
    if((rotation_mode != ROTATION_0) && (rotation_mode != ROTATION_90) && (rotation_mode != ROTATION_270)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ERROR! LDC output mode not supported!\n");
        return BM_ERR_FAILURE;
    }

    ret = check_bm_ldc_image_param(&in_image, &out_image);
    if(ret != BM_SUCCESS) {
        return ret;
    }
    param.size_in.u32Width   = in_image.width;
    param.size_in.u32Height  = in_image.height;
    param.size_out.u32Width  = (out_image.width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    param.size_out.u32Height = (out_image.height + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    bm_image_format_to_cvi_format(in_image.image_format, in_image.data_type, &param.enPixelFormat);

    snprintf(param.stTask.name, sizeof(param.stTask.name), "tsk_rot");
    param.identity.enModId = CVI_ID_USER;
    param.identity.u32ID = 0;
    // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_rot");
    param.identity.syncIo = true;

    param.op = BM_LDC_ROT;

    ret = bm_ldc_basic(handle, in_image, out_image, &param, (void *)&rotation_mode);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_basic in bm_ldc_rot_internal failed!\n");
        return ret;
    }
    return ret;
}

bm_status_t bm_ldc_gdc_internal(bm_handle_t          handle,
                                bm_image             in_image,
                                bm_image             out_image,
                                bmcv_gdc_attr        ldc_attr)
{
    bm_status_t ret = BM_SUCCESS;
    LDC_ATTR_S ldc_param = {0};
    bm_ldc_basic_param param = {0};
    memset(&param, 0, sizeof(param));
    memset(&ldc_param, 0, sizeof(ldc_param));

    param.size_in.u32Width   = in_image.width;
    param.size_in.u32Height  = in_image.height;
    param.size_out.u32Width  = (out_image.width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    param.size_out.u32Height = (out_image.height + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    bm_image_format_to_cvi_format(in_image.image_format, in_image.data_type, &param.enPixelFormat);

    param.op = BM_LDC_GDC;
    param.identity.enModId = CVI_ID_USER;
    param.identity.u32ID = 0;
    param.identity.syncIo = true;

    bm_gdc_attr_and_grid_info gdc_with_grid;
    memset(&gdc_with_grid, 0, sizeof(gdc_with_grid));

    if (ldc_attr.grid_info.size == 0) {
        snprintf(param.stTask.name, sizeof(param.stTask.name), "tsk_gdc");      // mesh name
        // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_gdc");

        gdc_with_grid.ldc_attr.bAspect = ldc_attr.bAspect;
        gdc_with_grid.ldc_attr.s32CenterXOffset = ldc_attr.s32CenterXOffset;
        gdc_with_grid.ldc_attr.s32CenterYOffset = ldc_attr.s32CenterYOffset;
        gdc_with_grid.ldc_attr.s32DistortionRatio = ldc_attr.s32DistortionRatio;
        gdc_with_grid.ldc_attr.s32XRatio = ldc_attr.s32XRatio;
        gdc_with_grid.ldc_attr.s32XYRatio = ldc_attr.s32XYRatio;
        gdc_with_grid.ldc_attr.s32YRatio = ldc_attr.s32YRatio;
        gdc_with_grid.ldc_attr.stGridInfoAttr.Enable = false;
        gdc_with_grid.grid = NULL;
    } else {
        snprintf(param.stTask.name, sizeof(param.stTask.name), "tsk_gdc_grid_0");
        // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_gdc_grid_0");

        gdc_with_grid.ldc_attr.stGridInfoAttr.Enable = true;
        gdc_with_grid.grid = (char *)ldc_attr.grid_info.u.system.system_addr;
        strcpy(gdc_with_grid.ldc_attr.stGridInfoAttr.gridFileName, "grid_info_79_44_3476_80_45_1280x720.dat");
        strcpy(gdc_with_grid.ldc_attr.stGridInfoAttr.gridBindName, param.stTask.name);
    }

    ret = bm_ldc_basic(handle, in_image, out_image, &param, (void *)&gdc_with_grid);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_basic in bm_ldc_gdc_internal failed!\n");
        return ret;
    }
    return ret;
}

bm_status_t bm_ldc_gdc_gen_mesh_internal(bm_handle_t          handle,
                                         bm_image             in_image,
                                         bm_image             out_image,
                                         bmcv_gdc_attr        ldc_attr,
                                         bm_device_mem_t      dmem)
{
    bm_status_t ret = BM_SUCCESS;
    bm_ldc_basic_param param = {0};

    param.size_in.u32Width   = in_image.width;
    param.size_in.u32Height  = in_image.height;
    param.size_out.u32Width  = (out_image.width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    param.size_out.u32Height = (out_image.height + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    bm_image_format_to_cvi_format(in_image.image_format, in_image.data_type, &param.enPixelFormat);

    snprintf(param.stTask.name, sizeof(param.stTask.name), "tsk_gdc");      // mesh name
    param.identity.enModId = CVI_ID_USER;
    param.identity.u32ID = 0;
    param.identity.syncIo = true;

    param.op = BM_LDC_GDC_GEN_MESH;

    bm_gen_mesh_param gen_param;

    gen_param.ldc_attr.bAspect = ldc_attr.bAspect;
    gen_param.ldc_attr.s32CenterXOffset = ldc_attr.s32CenterXOffset;
    gen_param.ldc_attr.s32CenterYOffset = ldc_attr.s32CenterYOffset;
    gen_param.ldc_attr.s32DistortionRatio = ldc_attr.s32DistortionRatio;
    gen_param.ldc_attr.s32XRatio = ldc_attr.s32XRatio;
    gen_param.ldc_attr.s32XYRatio = ldc_attr.s32XYRatio;
    gen_param.ldc_attr.s32YRatio = ldc_attr.s32YRatio;
    gen_param.ldc_attr.stGridInfoAttr.Enable = false;

    gen_param.dmem = dmem;

    ret = check_ldc_attr_param(&gen_param.ldc_attr);
    if(ret != BM_SUCCESS) {
        return ret;
    }

    ret = bm_ldc_basic(handle, in_image, out_image, &param, (void *)&gen_param);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_basic in bm_ldc_gdc_gen_mesh_internal failed!\n");
        return ret;
    }
    dmem.u.device.device_addr = param.stTask.au64privateData[0];
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "In bm_ldc_gdc_gen_mesh_internal, dmem->u.device.device_addr = %#"PRIx64"\n", (uint64_t)dmem.u.device.device_addr);
    return ret;
}

bm_status_t bm_ldc_gdc_load_mesh_internal(bm_handle_t          handle,
                                          bm_image             in_image,
                                          bm_image             out_image,
                                          bm_device_mem_t      dmem)
{
    bm_status_t ret = BM_SUCCESS;
    bm_ldc_basic_param param = {0};

    param.size_in.u32Width   = in_image.width;
    param.size_in.u32Height  = in_image.height;
    param.size_out.u32Width  = (out_image.width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    param.size_out.u32Height = (out_image.height + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    bm_image_format_to_cvi_format(in_image.image_format, in_image.data_type, &param.enPixelFormat);

    snprintf(param.stTask.name, sizeof(param.stTask.name), "tsk_gdc");
    param.identity.enModId = CVI_ID_USER;
    param.identity.u32ID = 0;
    // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_gdc");
    param.identity.syncIo = true;

    param.op = BM_LDC_GDC_LOAD_MESH;

    ret = bm_ldc_basic(handle, in_image, out_image, &param, (void *)&dmem);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_ldc_basic in bm_ldc_gdc_load_mesh_internal failed!\n");
        return ret;
    }
    return ret;
}
