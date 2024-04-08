#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <unistd.h>
#include "bmcv_internal.h"
#include "bmcv_a2_dwa_internal.h"
#ifdef __linux__
#include <sys/ioctl.h>
#endif

extern bm_status_t bm_image_format_to_cvi(bm_image_format_ext fmt, bm_image_data_format_ext datatype,
                                          PIXEL_FORMAT_E * cvi_fmt);

unsigned short uncommonly_used_idx = DWA_MAX_TSK_MESH - 1;
static int meshHor = MESH_HOR_DEFAULT;
static int meshVer = MESH_HOR_DEFAULT;
static int MESH_EDGE[4][2] = {
    { 0, 1 },
    { 1, 3 },
    { 2, 0 },
    { 3, 2 }
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
BM_TSK_MESH_ATTR_S dwa_tskMesh[DWA_MAX_TSK_MESH];

static s32 dwa_init(s32 fd)
{
    return ioctl(fd, CVI_DWA_INIT);
}

s32 dwa_deinit(s32 fd)
{
    return ioctl(fd, CVI_DWA_DEINIT);
}

static s32 dwa_begin_job(s32 fd, struct dwa_handle_data *cfg)
{
    return ioctl(fd, CVI_DWA_BEGIN_JOB, cfg);
}

static s32 dwa_end_job(s32 fd, struct dwa_handle_data *cfg)
{
    return ioctl(fd, CVI_DWA_END_JOB, cfg);
}

s32 dwa_cancel_job(s32 fd, struct dwa_handle_data *cfg)
{
    return ioctl(fd, CVI_DWA_CANCEL_JOB, cfg);
}

static s32 dwa_add_rotation_task(s32 fd, struct dwa_task_attr *attr)
{
    return ioctl(fd, CVI_DWA_ADD_ROT_TASK, attr);
}

static s32 dwa_add_ldc_task(s32 fd, struct dwa_task_attr *attr)
{
    return ioctl(fd, CVI_DWA_ADD_LDC_TASK, attr);
}

static s32 dwa_add_warp_task(s32 fd, struct dwa_task_attr *attr)
{
    return ioctl(fd, CVI_DWA_ADD_WAR_TASK, attr);
}

static s32 dwa_add_correction_task(s32 fd, struct dwa_task_attr *attr)
{
    return ioctl(fd, CVI_DWA_ADD_COR_TASK, attr);
}

static s32 dwa_add_affine_task(s32 fd, struct dwa_task_attr *attr)
{
    return ioctl(fd, CVI_DWA_ADD_AFF_TASK, attr);
}

static s32 dwa_set_job_identity(s32 fd, struct dwa_identity_attr *indentity)
{
    return ioctl(fd, CVI_DWA_SET_JOB_IDENTITY, indentity);
}

s32 dwa_get_work_job(s32 fd, struct dwa_handle_data *cfg)
{
    return ioctl(fd, CVI_DWA_GET_WORK_JOB, cfg);
}

s32 dwa_get_chn_frm(s32 fd, struct dwa_chn_frm_cfg *cfg)
{
    return ioctl(fd, CVI_DWA_GET_CHN_FRM, cfg);
}

static bool is_rect_overlap(POINT_S l1, POINT_S r1, POINT_S l2, POINT_S r2)
{
    // If one rectangle is on left side of other
    if (l1.s32X > r2.s32X || l2.s32X > r1.s32X)
        return false;

    // If one rectangle is above other
    if (l1.s32Y > r2.s32Y || l2.s32Y > r1.s32Y)
        return false;

    return true;
}

static void BM_LOAD_FRAME_CONFIG(BM_FISHEYE_ATTR *FISHEYE_CONFIG)
{
    switch (FISHEYE_CONFIG->UsageMode)
    {
    case MODE_PANORAMA_360:
        FISHEYE_CONFIG->RgnNum = 1;
        break;
    case MODE_PANORAMA_180:
        FISHEYE_CONFIG->RgnNum = 1;
        break;
    case MODE_01_1O:
        FISHEYE_CONFIG->RgnNum = 1;
        break;
    case MODE_02_1O4R:
        FISHEYE_CONFIG->RgnNum = 4; // 1O should be handled in scaler block.
        break;
    case MODE_03_4R:
        FISHEYE_CONFIG->RgnNum = 4;
        break;
    case MODE_04_1P2R:
        FISHEYE_CONFIG->RgnNum = 3;
        break;
    case MODE_05_1P2R:
        FISHEYE_CONFIG->RgnNum = 3;
        break;
    case MODE_06_1P:
        FISHEYE_CONFIG->RgnNum = 1;
        break;
    case MODE_07_2P:
        FISHEYE_CONFIG->RgnNum = 2;
        break;
    case MODE_STEREO_FIT:
        FISHEYE_CONFIG->RgnNum = 1;
        break;
    default:
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "UsageMode Error!\n");
        //system("pause");
        break;
    }
}

static void bm_generate_mesh_id(struct mesh_param *param, POINT_S dst_mesh_tbl[][4])
{
    const int NUM_X_LINE_A_SLICE = DIV_UP(param->width, param->slice_num_w);
    const int NUM_Y_LINE_A_SLICE = ALIGN(DIV_UP(param->height, param->slice_num_h), NUMBER_Y_LINE_A_SUBTILE);
    u16 *mesh_id = param->mesh_id_addr;
    u16 slice_idx_x, slice_idx_y;
    u32 id_idx = 0;

    mesh_id[id_idx++] = MESH_ID_FST;
    for (slice_idx_y = 0; slice_idx_y < param->slice_num_h; slice_idx_y++) {
        int src_y = CLIP((slice_idx_y * NUM_Y_LINE_A_SLICE), 0, param->height);
        int dst_y = CLIP((src_y + NUM_Y_LINE_A_SLICE), 0, param->height);
        for (slice_idx_x = 0; slice_idx_x < param->slice_num_w; slice_idx_x++) {
            int src_x = CLIP((slice_idx_x * NUM_X_LINE_A_SLICE), 0, param->width);
            int dst_x = CLIP((src_x + NUM_X_LINE_A_SLICE), 0, param->width);

            mesh_id[id_idx++] = MESH_ID_FSS;
            mesh_id[id_idx++] = param->mesh_tbl_phy_addr & 0x0fff;
            mesh_id[id_idx++] = (param->mesh_tbl_phy_addr >> 12) & 0x0fff;
            mesh_id[id_idx++] = (param->mesh_tbl_phy_addr >> 24) & 0x0fff;
            mesh_id[id_idx++] = (param->mesh_tbl_phy_addr >> 36) & 0x000f;
            mesh_id[id_idx++] = src_x;
            mesh_id[id_idx++] = (dst_x - src_x);
            mesh_id[id_idx++] = src_y;
            mesh_id[id_idx++] = (dst_y - src_y);

            for (int y = src_y; y < dst_y; y += NUMBER_Y_LINE_A_SUBTILE) {
                POINT_S l1, r1, l2, r2;

                l1.s32X = src_x;
                l1.s32Y = y;
                r1.s32X = dst_x;
                r1.s32Y = y + NUMBER_Y_LINE_A_SUBTILE;
                mesh_id[id_idx++] = MESH_ID_FSP;
                for (int m = 0; m < param->mesh_num; ++m) {
                    // To reduce time consumption
                    // assumption: dst mesh is ordered (left->right, up->down)
                    if (dst_mesh_tbl[m][3].s32Y < l1.s32Y) continue;
                    if (dst_mesh_tbl[m][0].s32Y > r1.s32Y) break;

                    l2.s32X = dst_mesh_tbl[m][0].s32X;
                    l2.s32Y = dst_mesh_tbl[m][0].s32Y;
                    r2.s32X = dst_mesh_tbl[m][3].s32X;
                    r2.s32Y = dst_mesh_tbl[m][3].s32Y;
                    if (is_rect_overlap(l1, r1, l2, r2))
                        mesh_id[id_idx++] = m;
                }
                mesh_id[id_idx++] = MESH_ID_FTE;
            }
            mesh_id[id_idx++] = MESH_ID_FSE;
        }
    }
    mesh_id[id_idx++] = MESH_ID_FED;

    for (int i = 0; i < 32; ++i)
        mesh_id[id_idx++] = 0x00;
}

static void BM_LOAD_REGION_CONFIG(BM_FISHEYE_ATTR* FISHEYE_CONFIG, BM_FISHEYE_REGION_ATTR* FISHEYE_REGION)
{
    // to make sure parameters aligned to frame ratio
    double width_sec = FISHEYE_CONFIG->OutW_disp / 40;
    double height_sec = FISHEYE_CONFIG->OutH_disp / 40;

    // load default settings
    if (FISHEYE_CONFIG->UsageMode == MODE_02_1O4R )
    {
        FISHEYE_REGION[0].RegionValid = 1;
        FISHEYE_REGION[0].MeshVer = 16;
        FISHEYE_REGION[0].MeshHor = 16;
        FISHEYE_REGION[0].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[0].ThetaX = 0.33 * M_PI;
        FISHEYE_REGION[0].ThetaZ = 0;
        FISHEYE_REGION[0].ThetaY = 0;
        FISHEYE_REGION[0].ZoomH = 2048;                 // zooming factor for horizontal:   0 ~ 4095 to control view
        FISHEYE_REGION[0].ZoomV = 3072;                 // zooming factor for vertical:     0 ~ 4095 to control view
        FISHEYE_REGION[0].Pan  = UI_CTRL_VALUE_CENTER;  // center = 180, value = 0 ~ 360    => value = +- 180 degrees
        FISHEYE_REGION[0].Tilt = UI_CTRL_VALUE_CENTER;  // center = 180, value = 0 ~ 360    => value = +-30 degrees
        FISHEYE_REGION[0].OutW = (width_sec * 15);
        FISHEYE_REGION[0].OutH = (height_sec * 20);
        FISHEYE_REGION[0].OutX = 0;
        FISHEYE_REGION[0].OutY = 0;

        FISHEYE_REGION[1].RegionValid = 1;
        FISHEYE_REGION[1].MeshVer = 16;
        FISHEYE_REGION[1].MeshHor = 16;
        FISHEYE_REGION[1].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[1].ThetaX = 0.33 * M_PI;
        FISHEYE_REGION[1].ThetaZ = 0.5 * M_PI;
        FISHEYE_REGION[1].ThetaY = 0;
        FISHEYE_REGION[1].ZoomH = 2048;
        FISHEYE_REGION[1].ZoomV = 3072;
        FISHEYE_REGION[1].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[1].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[1].OutW = (width_sec * 15);
        FISHEYE_REGION[1].OutH = (height_sec * 20);
        FISHEYE_REGION[1].OutX = (width_sec * 15);
        FISHEYE_REGION[1].OutY = (height_sec * 0);

        FISHEYE_REGION[2].RegionValid = 1;
        FISHEYE_REGION[2].MeshVer = 16;
        FISHEYE_REGION[2].MeshHor = 16;
        FISHEYE_REGION[2].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[2].ThetaX = 0.33 * M_PI;
        FISHEYE_REGION[2].ThetaZ = M_PI;
        FISHEYE_REGION[2].ThetaY = 0;
        FISHEYE_REGION[2].ZoomH = 2048;
        FISHEYE_REGION[2].ZoomV = 3072;
        FISHEYE_REGION[2].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[2].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[2].OutW = (width_sec * 15);
        FISHEYE_REGION[2].OutH = (height_sec * 20);
        FISHEYE_REGION[2].OutX = (width_sec * 0);
        FISHEYE_REGION[2].OutY = (height_sec * 20);

        FISHEYE_REGION[3].RegionValid = 1;
        FISHEYE_REGION[3].MeshVer = 16;
        FISHEYE_REGION[3].MeshHor = 16;
        FISHEYE_REGION[3].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[3].ThetaX = 0.33 * M_PI;
        FISHEYE_REGION[3].ThetaZ = 1.5 * M_PI;
        FISHEYE_REGION[3].ThetaY = 0;
        FISHEYE_REGION[3].ZoomH = 2048;
        FISHEYE_REGION[3].ZoomV = 3072;
        FISHEYE_REGION[3].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[3].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[3].OutW = (width_sec * 15);
        FISHEYE_REGION[3].OutH = (height_sec * 20);
        FISHEYE_REGION[3].OutX = (width_sec * 15);
        FISHEYE_REGION[3].OutY = (height_sec * 20);

        FISHEYE_REGION[4].RegionValid = 0;
    }
    else if (FISHEYE_CONFIG->UsageMode == MODE_03_4R)
    {
        FISHEYE_REGION[0].RegionValid = 1;
        FISHEYE_REGION[0].MeshVer = 16;
        FISHEYE_REGION[0].MeshHor = 16;
        FISHEYE_REGION[0].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[0].ThetaX = 0.33 * M_PI;
        FISHEYE_REGION[0].ThetaZ = 0;
        FISHEYE_REGION[0].ThetaY = 0;
        FISHEYE_REGION[0].ZoomH = 1024;                 // zooming factor for horizontal:0 ~ 4095 to control view
        FISHEYE_REGION[0].ZoomV = 3072;                 // zooming factor for vertical:0 ~ 4095 to control view
        FISHEYE_REGION[0].Pan = UI_CTRL_VALUE_CENTER;   // center = 180, value = 0 ~ 360 => value = +- 180 degrees
        FISHEYE_REGION[0].Tilt = UI_CTRL_VALUE_CENTER;  // center = 180, value = 0 ~ 360 => value = +-30 degrees
        FISHEYE_REGION[0].OutW = (width_sec * 20);
        FISHEYE_REGION[0].OutH = (height_sec * 20);
        FISHEYE_REGION[0].OutX = 0;//(width_sec * 2);
        FISHEYE_REGION[0].OutY = 0;//(height_sec * 2);

        FISHEYE_REGION[1].RegionValid = 1;
        FISHEYE_REGION[1].MeshVer = 16;
        FISHEYE_REGION[1].MeshHor = 16;
        FISHEYE_REGION[1].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[1].ThetaX = 0.33 * M_PI;
        FISHEYE_REGION[1].ThetaZ = 0.5 * M_PI;
        FISHEYE_REGION[1].ThetaY = 0;
        FISHEYE_REGION[1].ZoomH = 1024;
        FISHEYE_REGION[1].ZoomV = 3072;
        FISHEYE_REGION[1].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[1].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[1].OutW = (width_sec * 20);
        FISHEYE_REGION[1].OutH = (height_sec * 20);
        FISHEYE_REGION[1].OutX = (width_sec * 20);
        FISHEYE_REGION[1].OutY = (height_sec * 0);

        FISHEYE_REGION[2].RegionValid = 1;
        FISHEYE_REGION[2].MeshVer = 16;
        FISHEYE_REGION[2].MeshHor = 16;
        FISHEYE_REGION[2].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[2].ThetaX = 0.33 * M_PI;
        FISHEYE_REGION[2].ThetaZ = M_PI;
        FISHEYE_REGION[2].ThetaY = 0;
        FISHEYE_REGION[2].ZoomH = 1024;
        FISHEYE_REGION[2].ZoomV = 3072;
        FISHEYE_REGION[2].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[2].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[2].OutW = (width_sec * 20);
        FISHEYE_REGION[2].OutH = (height_sec * 20);
        FISHEYE_REGION[2].OutX = (width_sec * 0);
        FISHEYE_REGION[2].OutY = (height_sec * 20);

        FISHEYE_REGION[3].RegionValid = 1;
        FISHEYE_REGION[3].MeshVer = 16;
        FISHEYE_REGION[3].MeshHor = 16;
        FISHEYE_REGION[3].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[3].ThetaX = 0.33 * M_PI;
        FISHEYE_REGION[3].ThetaZ = 1.5 * M_PI;
        FISHEYE_REGION[3].ThetaY = 0;
        FISHEYE_REGION[3].ZoomH = 1024;
        FISHEYE_REGION[3].ZoomV = 3072;
        FISHEYE_REGION[3].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[3].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[3].OutW = (width_sec * 20);
        FISHEYE_REGION[3].OutH = (height_sec * 20);
        FISHEYE_REGION[3].OutX = (width_sec * 20);
        FISHEYE_REGION[3].OutY = (height_sec * 20);

        FISHEYE_REGION[4].RegionValid = 0;
    }
    else if (FISHEYE_CONFIG->UsageMode == MODE_04_1P2R)
    {
        // Region #1 => Panorama 180
        FISHEYE_REGION[0].RegionValid = 1;
        FISHEYE_REGION[0].MeshVer = 16;
        FISHEYE_REGION[0].MeshHor = 16;
        FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_180;
        //FISHEYE_REGION[0].ThetaX = 0;
        //FISHEYE_REGION[0].ThetaY = 0;
        //FISHEYE_REGION[0].ThetaZ = 0;
        FISHEYE_REGION[0].ZoomH = 4096;                 // value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
        FISHEYE_REGION[0].ZoomV = 1920;                 // value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90
        FISHEYE_REGION[0].Pan   = UI_CTRL_VALUE_CENTER;         // value range = 0 ~ 360, => -180 ~ 0 ~ +180
        FISHEYE_REGION[0].Tilt  = UI_CTRL_VALUE_CENTER;         // value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
        FISHEYE_REGION[0].OutW  = (width_sec * 40);
        FISHEYE_REGION[0].OutH  = (height_sec * 22);
        FISHEYE_REGION[0].OutX  = 0;                    //(width_sec * 1);
        FISHEYE_REGION[0].OutY  = 0;                    //height_sec * 1);
        //FISHEYE_REGION[0].InRadius = 50;              // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //FISHEYE_REGION[0].OutRadius = 450;            // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //FISHEYE_REGION[0].PanEnd = 180;

        FISHEYE_REGION[1].RegionValid = 1;
        FISHEYE_REGION[1].MeshVer = 8;
        FISHEYE_REGION[1].MeshHor = 8;
        FISHEYE_REGION[1].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[1].ThetaX = M_PI / 4;
        FISHEYE_REGION[1].ThetaZ = M_PI / 2;
        FISHEYE_REGION[1].ThetaY = 0;
        FISHEYE_REGION[1].ZoomH = 2048;
        FISHEYE_REGION[1].ZoomV = 2048;
        FISHEYE_REGION[1].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[1].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[1].OutW = (width_sec * 20);
        FISHEYE_REGION[1].OutH = (height_sec * 18);
        FISHEYE_REGION[1].OutX = (width_sec * 0);
        FISHEYE_REGION[1].OutY = (height_sec * 22);

        FISHEYE_REGION[2].RegionValid = 1;
        FISHEYE_REGION[2].MeshVer = 8;
        FISHEYE_REGION[2].MeshHor = 8;
        FISHEYE_REGION[2].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[2].ThetaX = M_PI / 4;
        FISHEYE_REGION[2].ThetaZ = M_PI;
        FISHEYE_REGION[2].ThetaY = 0;
        FISHEYE_REGION[2].ZoomH = 2048;
        FISHEYE_REGION[2].ZoomV = 2048;
        FISHEYE_REGION[2].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[2].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[2].OutW = (width_sec * 20);
        FISHEYE_REGION[2].OutH = (height_sec * 18);
        FISHEYE_REGION[2].OutX = (width_sec * 20);
        FISHEYE_REGION[2].OutY = (height_sec * 22);

        FISHEYE_REGION[3].RegionValid = 0;
        FISHEYE_REGION[4].RegionValid = 0;
    }
    else if (FISHEYE_CONFIG->UsageMode == MODE_05_1P2R)
    {
        // Region #1 => Panorama 180
        FISHEYE_REGION[0].RegionValid = 1;
        FISHEYE_REGION[0].MeshVer = 16;
        FISHEYE_REGION[0].MeshHor = 16;
        FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_180;
        //FISHEYE_REGION[0].ThetaX = 0;
        //FISHEYE_REGION[0].ThetaY = 0;
        //FISHEYE_REGION[0].ThetaZ = 0;
        FISHEYE_REGION[0].ZoomH = 3000;                 // value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
        FISHEYE_REGION[0].ZoomV = 2048;                 // value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90
        FISHEYE_REGION[0].Pan  = UI_CTRL_VALUE_CENTER;          // value range = 0 ~ 360, => -180 ~ 0 ~ +180
        FISHEYE_REGION[0].Tilt = UI_CTRL_VALUE_CENTER;          // value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
        FISHEYE_REGION[0].OutW = (width_sec * 27 );
        FISHEYE_REGION[0].OutH = (height_sec * 40);
        FISHEYE_REGION[0].OutX = 0;                 //(width_sec * 1);
        FISHEYE_REGION[0].OutY = 0;                 //height_sec * 1);
        //FISHEYE_REGION[0].InRadius = 50;          // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //FISHEYE_REGION[0].OutRadius = 450;        // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //FISHEYE_REGION[0].PanEnd = 180;

        FISHEYE_REGION[1].RegionValid = 1;
        FISHEYE_REGION[1].MeshVer = 8;
        FISHEYE_REGION[1].MeshHor = 8;
        FISHEYE_REGION[1].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[1].ThetaX = M_PI / 4;
        FISHEYE_REGION[1].ThetaZ = M_PI / 2;
        FISHEYE_REGION[1].ThetaY = 0;
        FISHEYE_REGION[1].ZoomH = 2048;
        FISHEYE_REGION[1].ZoomV = 2048;
        FISHEYE_REGION[1].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[1].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[1].OutW = (width_sec * 13);
        FISHEYE_REGION[1].OutH = (height_sec * 20);
        FISHEYE_REGION[1].OutX = (width_sec * 27);
        FISHEYE_REGION[1].OutY = (height_sec * 0);

        FISHEYE_REGION[2].RegionValid = 1;
        FISHEYE_REGION[2].MeshVer = 8;
        FISHEYE_REGION[2].MeshHor = 8;
        FISHEYE_REGION[2].ViewMode = PROJECTION_REGION;
        FISHEYE_REGION[2].ThetaX = M_PI / 4;
        FISHEYE_REGION[2].ThetaZ = M_PI;
        FISHEYE_REGION[2].ThetaY = 0;
        FISHEYE_REGION[2].ZoomH = 2048;
        FISHEYE_REGION[2].ZoomV = 2048;
        FISHEYE_REGION[2].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        FISHEYE_REGION[2].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        FISHEYE_REGION[2].OutW = (width_sec * 13);
        FISHEYE_REGION[2].OutH = (height_sec * 20);
        FISHEYE_REGION[2].OutX = (width_sec * 27);
        FISHEYE_REGION[2].OutY = (height_sec * 20);

        FISHEYE_REGION[3].RegionValid = 0;
        FISHEYE_REGION[4].RegionValid = 0;


    }
    else if (FISHEYE_CONFIG->UsageMode == MODE_06_1P)
    {
        // Region #1 => Panorama 180
        FISHEYE_REGION[0].RegionValid = 1;
        FISHEYE_REGION[0].MeshVer = 30;
        FISHEYE_REGION[0].MeshHor = 30;
        FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_180;
        //FISHEYE_REGION[0].ThetaX = 0;
        //FISHEYE_REGION[0].ThetaY = 0;
        //FISHEYE_REGION[0].ThetaZ = 0;
        FISHEYE_REGION[0].ZoomH = 4096; // value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
        FISHEYE_REGION[0].ZoomV = 2800; // value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90
        FISHEYE_REGION[0].Pan  = UI_CTRL_VALUE_CENTER;  // value range = 0 ~ 360, => -180 ~ 0 ~ +180
        FISHEYE_REGION[0].Tilt = UI_CTRL_VALUE_CENTER;  // value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
        FISHEYE_REGION[0].OutW = (width_sec * 40 );
        FISHEYE_REGION[0].OutH = (height_sec * 40);
        FISHEYE_REGION[0].OutX = 0; //(width_sec * 1);
        FISHEYE_REGION[0].OutY = 0; //height_sec * 1);
        //FISHEYE_REGION[0].InRadius = 50;      // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //FISHEYE_REGION[0].OutRadius = 450;    // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //FISHEYE_REGION[0].PanEnd = 180;

        FISHEYE_REGION[1].RegionValid = 0;
        FISHEYE_REGION[2].RegionValid = 0;
        FISHEYE_REGION[3].RegionValid = 0;
        FISHEYE_REGION[4].RegionValid = 0;
    } else if (FISHEYE_CONFIG->UsageMode == MODE_07_2P ) {
        //_Panorama360View2;
        FISHEYE_REGION[0].RegionValid = 1;
        FISHEYE_REGION[0].MeshVer = 16;
        FISHEYE_REGION[0].MeshHor = 16;
        FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_360;
        FISHEYE_REGION[0].ThetaX = M_PI / 4;
        FISHEYE_REGION[0].ThetaZ = 0;
        FISHEYE_REGION[0].ThetaY = 0;
        //FISHEYE_REGION[0].ZoomH = 4095;               // Not Used in Panorama 360 Mode.
        FISHEYE_REGION[0].ZoomV = 4095;                 // To ZoomIn OutRadius
        FISHEYE_REGION[0].Pan = 40;                     // for panorama 360 => Pan is the label start position angle ( in degrees
        FISHEYE_REGION[0].Tilt = 200;                   // to add shift offset vertical angle.
        FISHEYE_REGION[0].OutW = (width_sec * 40);
        FISHEYE_REGION[0].OutH = (height_sec * 20);
        FISHEYE_REGION[0].OutX = (width_sec * 0);
        FISHEYE_REGION[0].OutY = (height_sec * 0);
        FISHEYE_REGION[0].InRadius = 300;               // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        FISHEYE_REGION[0].OutRadius = 3712;             // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        FISHEYE_REGION[0].PanEnd = 220;

        FISHEYE_REGION[1].RegionValid = 1;
        FISHEYE_REGION[1].MeshVer = 16;
        FISHEYE_REGION[1].MeshHor = 16;
        FISHEYE_REGION[1].ViewMode = PROJECTION_PANORAMA_360;
        FISHEYE_REGION[1].ThetaX = M_PI / 4;
        FISHEYE_REGION[1].ThetaZ = 0;
        FISHEYE_REGION[1].ThetaY = 0;
        //FISHEYE_REGION[1].ZoomH = 4095;               // Not Used in Panorama 360 Mode.
        FISHEYE_REGION[1].ZoomV = 4095;                 // To ZoomIn OutRadius
        FISHEYE_REGION[1].Pan  = 220;                   // for panorama 360 => Pan is the label start position angle ( in degrees
        FISHEYE_REGION[1].Tilt = 200;                   // to add shift offset vertical angle.
        FISHEYE_REGION[1].OutW = (width_sec * 40);
        FISHEYE_REGION[1].OutH = (height_sec * 20);
        FISHEYE_REGION[1].OutX = (width_sec * 0);
        FISHEYE_REGION[1].OutY = (height_sec * 20);
        FISHEYE_REGION[1].InRadius = 300;               // a ratio to represent OutRadius length. 1 = full origina redius.  value/512 is the value.
        FISHEYE_REGION[1].OutRadius = 3712;             // a ratio to represent OutRadius length. 1 = full origina redius.  value/512 is the value.
        FISHEYE_REGION[1].PanEnd = 40;                  //

        FISHEYE_REGION[2].RegionValid = 0;
        FISHEYE_REGION[3].RegionValid = 0;
        FISHEYE_REGION[4].RegionValid = 0;
    } else if (FISHEYE_CONFIG->UsageMode == MODE_PANORAMA_180) {
        FISHEYE_REGION[0].RegionValid = 1;
        FISHEYE_REGION[0].MeshVer = 16;
        FISHEYE_REGION[0].MeshHor = 16;
        FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_180;
        //FISHEYE_REGION[0].ThetaX = M_PI / 4;
        //FISHEYE_REGION[0].ThetaZ = 0;
        //FISHEYE_REGION[0].ThetaY = 0;
        FISHEYE_REGION[0].ZoomH = 4096;                 // Not Used in Panorama 360 Mode.
        FISHEYE_REGION[0].ZoomV = 4096;                 // To ZoomIn OutRadius
        FISHEYE_REGION[0].Pan = 180;                    // for panorama 360 => Pan is the label start position angle ( in degrees
        FISHEYE_REGION[0].Tilt = 180;                   // to add shift offset vertical angle.
        FISHEYE_REGION[0].OutW = (width_sec * 40);
        FISHEYE_REGION[0].OutH = (height_sec * 40);
        FISHEYE_REGION[0].OutX = 0;                 //(width_sec * 1);
        FISHEYE_REGION[0].OutY = 0;                 //height_sec * 1);
        //FISHEYE_REGION[0].InRadius = 50;          // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //FISHEYE_REGION[0].OutRadius = 450;        // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //FISHEYE_REGION[0].PanEnd = 180;

        FISHEYE_REGION[1].RegionValid = 0;
        FISHEYE_REGION[2].RegionValid = 0;
        FISHEYE_REGION[3].RegionValid = 0;
        FISHEYE_REGION[4].RegionValid = 0;
    } else if (FISHEYE_CONFIG->UsageMode == MODE_PANORAMA_360) {
        //_Panorama360View2;
        FISHEYE_REGION[0].RegionValid = 1;
        FISHEYE_REGION[0].MeshVer = 8;
        FISHEYE_REGION[0].MeshHor = 8;
        FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_360;
        FISHEYE_REGION[0].ThetaX = M_PI / 4;
        FISHEYE_REGION[0].ThetaZ = 0;
        FISHEYE_REGION[0].ThetaY = 0;
        //FISHEYE_REGION[0].ZoomH = 4095;               // Not Used in Panorama 360 Mode.
        FISHEYE_REGION[0].ZoomV = 4095;                 // To ZoomIn OutRadius
        FISHEYE_REGION[0].Pan = 0;                      // for panorama 360 => Pan is the label start position angle ( in degrees
        FISHEYE_REGION[0].Tilt = 180;                   // to add shift offset vertical angle.
        FISHEYE_REGION[0].OutW = (width_sec * 40);
        FISHEYE_REGION[0].OutH = (height_sec * 40) * 4 / 6;
        FISHEYE_REGION[0].OutX = 0;                     //(width_sec * 1);
        FISHEYE_REGION[0].OutY = (height_sec * 40) / 6;         //(height_sec * 1);
        FISHEYE_REGION[0].InRadius = 800;               // a ratio to represent OutRadius length. 1 => full origina redius. value = 0 ~ 4095,
        FISHEYE_REGION[0].OutRadius = 4096;             // a ratio to represent OutRadius length. 1 => full origina redius. value = 0 ~ 4095
        FISHEYE_REGION[0].PanEnd = 359;

        FISHEYE_REGION[1].RegionValid = 0;
        FISHEYE_REGION[2].RegionValid = 0;
        FISHEYE_REGION[3].RegionValid = 0;
        FISHEYE_REGION[4].RegionValid = 0;
    } else if (FISHEYE_CONFIG->UsageMode == MODE_STEREO_FIT) {
        FISHEYE_REGION[0].RegionValid   = 1;
        FISHEYE_REGION[0].MeshVer      = 128;
        FISHEYE_REGION[0].MeshHor      = 128;
        FISHEYE_REGION[0].ViewMode      = PROJECTION_STEREO_FIT;
        FISHEYE_REGION[0].ThetaX      = M_PI / 4;
        FISHEYE_REGION[0].ThetaZ      = 0;
        FISHEYE_REGION[0].ThetaY      = 0;
        //FISHEYE_REGION[0].ZoomH = 4095;            // Not Used in Panorama 360 Mode.
        FISHEYE_REGION[0].ZoomV         = 4095;               // To ZoomIn OutRadius
        FISHEYE_REGION[0].Pan         = 0;                  // for panorama 360 => Pan is the label start position angle ( in degrees
        FISHEYE_REGION[0].Tilt         = 180;               // to add shift offset vertical angle.
        FISHEYE_REGION[0].OutW         = FISHEYE_CONFIG->OutW_disp;
        FISHEYE_REGION[0].OutH         = FISHEYE_CONFIG->OutH_disp;
        FISHEYE_REGION[0].OutX         = 0;                  //(width_sec * 1);
        FISHEYE_REGION[0].OutY         = 0;                  //(height_sec * 1);
        FISHEYE_REGION[0].InRadius      = 0;            // a ratio to represent OutRadius lenght. 1 => full origina redius.   value = 0 ~ 4095,
        FISHEYE_REGION[0].OutRadius      = 4095;            // a ratio to represent OutRadius lenght. 1 => full origina redius.   value = 0 ~ 4095
        FISHEYE_REGION[0].PanEnd      = 359;
        //
        FISHEYE_REGION[1].RegionValid = 0;
        FISHEYE_REGION[2].RegionValid = 0;
        FISHEYE_REGION[3].RegionValid = 0;
        FISHEYE_REGION[4].RegionValid = 0;
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Not done yet!\n");
        //system("pause");
    }
}

static void bm_image_to_cvi_frame(bm_image *image, PIXEL_FORMAT_E enPixelFormat, VIDEO_FRAME_INFO_S *stVideoFrame)
{
    stVideoFrame->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    stVideoFrame->stVFrame.enPixelFormat = enPixelFormat;
    stVideoFrame->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stVideoFrame->stVFrame.enColorGamut = COLOR_GAMUT_BT709;
    stVideoFrame->stVFrame.u32Width = image->width;
    stVideoFrame->stVFrame.u32Height = image->height;
    for(int i = 0; i < image->image_private->plane_num; i++){
        stVideoFrame->stVFrame.u32Stride[i] =  image->image_private->memory_layout[i].pitch_stride;
        stVideoFrame->stVFrame.u32Length[i] = image->image_private->memory_layout[i].size;
        stVideoFrame->stVFrame.u64PhyAddr[i] = image->image_private->data[i].u.device.device_addr;
    }
    stVideoFrame->stVFrame.u32TimeRef = 0;
    stVideoFrame->stVFrame.u64PTS = 0;
    stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;
    stVideoFrame->u32PoolId = 0;
}

static void dwa_mesh_gen_get_size(SIZE_S in_size, SIZE_S out_size, u32 *mesh_id_size, u32 *mesh_tbl_size)
{
    if (!mesh_id_size || !mesh_tbl_size)
        return;

    (void)in_size;
    (void)out_size;

    // CVI_GDC_MESH_SIZE_FISHEYE
    *mesh_tbl_size = 0x60000;
    *mesh_id_size = 0x50000;
}

static CVI_U8 get_idle_tsk_mesh(bm_handle_t handle)
{
    CVI_U8 i = DWA_MAX_TSK_MESH;

    for (i = 0; i < DWA_MAX_TSK_MESH; i++) {
        if (strcmp(dwa_tskMesh[i].Name, "") == 0 && !dwa_tskMesh[i].paddr && !dwa_tskMesh[i].vaddr)
            break;
    }

    if(i == DWA_MAX_TSK_MESH){
        bm_device_mem_t mem;
        uncommonly_used_idx++;
        uncommonly_used_idx = uncommonly_used_idx % DWA_MAX_TSK_MESH;
        mem.u.device.device_addr = dwa_tskMesh[uncommonly_used_idx].paddr;
        bm_free_device(handle, mem);
        return uncommonly_used_idx;
    }
    return i;
}

static void bm_ldc_attr_map(const LDC_ATTR_S *pstLDCAttr, SIZE_S out_size, BM_FISHEYE_ATTR *FISHEYE_CONFIG
    , BM_FISHEYE_REGION_ATTR *FISHEYE_REGION)
{
    FISHEYE_CONFIG->MntMode = 0;
    FISHEYE_CONFIG->OutW_disp = out_size.u32Width;
    FISHEYE_CONFIG->OutH_disp = out_size.u32Height;
    FISHEYE_CONFIG->InCenterX = out_size.u32Width >> 1;
    FISHEYE_CONFIG->InCenterY = out_size.u32Height >> 1;
    // TODO: how to handl radius
    FISHEYE_CONFIG->InRadius = MIN2(FISHEYE_CONFIG->InCenterX, FISHEYE_CONFIG->InCenterY);
    FISHEYE_CONFIG->FStrength = 0;
    FISHEYE_CONFIG->TCoef  = 0;

    FISHEYE_CONFIG->RgnNum = 1;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "OutW_disp(%d) OutH_disp(%d)\n"
        , FISHEYE_CONFIG->OutW_disp, FISHEYE_CONFIG->OutH_disp);

    for (int i = 0; i < MAX_REGION_NUM; ++i)
        FISHEYE_REGION[i].RegionValid = 0;

    for (int i = 0; i < FISHEYE_CONFIG->RgnNum; ++i) {
        FISHEYE_REGION[i].RegionValid = 1;

        //FISHEYE_REGION[i].ZoomH = pstLDCAttr->bAspect;
        FISHEYE_REGION[i].ZoomV = pstLDCAttr->bAspect;
        FISHEYE_REGION[i].Pan = pstLDCAttr->s32XYRatio;
        FISHEYE_REGION[i].PanEnd = pstLDCAttr->s32DistortionRatio;
        FISHEYE_REGION[i].Tilt = 180;
        FISHEYE_REGION[i].OutW = FISHEYE_CONFIG->OutW_disp;
        FISHEYE_REGION[i].OutH = FISHEYE_CONFIG->OutH_disp;
        FISHEYE_REGION[i].OutX = 0;
        FISHEYE_REGION[i].OutY = 0;
        FISHEYE_REGION[i].InRadius = pstLDCAttr->s32CenterXOffset;
        FISHEYE_REGION[i].OutRadius = pstLDCAttr->s32CenterYOffset;
        FISHEYE_REGION[i].MeshVer = 16;
        FISHEYE_REGION[i].MeshHor = 16;
        if (pstLDCAttr->stGridInfoAttr.Enable) //use new ldc c models
            FISHEYE_REGION[i].ViewMode = PROJECTION_STEREO_FIT;
        else
            FISHEYE_REGION[i].ViewMode = PROJECTION_LDC;
        FISHEYE_REGION[i].ThetaX = pstLDCAttr->s32XRatio;
        FISHEYE_REGION[i].ThetaZ = pstLDCAttr->s32YRatio;
        FISHEYE_REGION[i].ThetaY = 0;

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "Region(%d) ViewMode(%d) MeshVer(%d) MeshHor(%d)\n"
            , i, FISHEYE_REGION[i].ViewMode, FISHEYE_REGION[i].MeshVer, FISHEYE_REGION[i].MeshHor);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "bAspect(%d) XYRatio(%d) DistortionRatio(%d)\n"
            , (bool)FISHEYE_REGION[i].ZoomV, FISHEYE_REGION[i].Pan, FISHEYE_REGION[i].PanEnd);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "XRatio(%d) XYRatio(%d)\n"
            , (int)FISHEYE_REGION[i].ThetaX, (int)FISHEYE_REGION[i].ThetaZ);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "CenterXOffset(%lf) CenterYOffset(%lf) Rect(%d %d %d %d)\n"
            , FISHEYE_REGION[i].InRadius, FISHEYE_REGION[i].OutRadius
            , FISHEYE_REGION[i].OutX, FISHEYE_REGION[i].OutY
            , FISHEYE_REGION[i].OutW, FISHEYE_REGION[i].OutH);
    }
}

static int dwa_get_mesh_size(int *p_mesh_hor, int *p_mesh_ver)
{
    MOD_CHECK_NULL_PTR(BM_ID_GDC, p_mesh_hor);
    MOD_CHECK_NULL_PTR(BM_ID_GDC, p_mesh_ver);
    *p_mesh_hor = meshHor;
    *p_mesh_ver = meshVer;

    return BM_SUCCESS;
}

long get_diff_in_us(struct timespec t1, struct timespec t2)
{
    struct timespec diff;

    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec * 1000000.0 + diff.tv_nsec / 1000.0);
}

int mesh_tbl_reorder_and_parse_3(uint16_t *mesh_scan_tile_mesh_id_list, int mesh_id_list_entry_num,
                                 int isrc_x_mesh_tbl[][4], int isrc_y_mesh_tbl[][4], int idst_x_mesh_tbl[][4],
                                 int idst_y_mesh_tbl[][4], int X_TILE_NUMBER,
                                 int Y_TILE_NUMBER, int Y_SUBTILE_NUMBER, int **reorder_mesh_tbl,
                                 int *reorder_mesh_tbl_entry_num, uint16_t *reorder_mesh_id_list,
                                 int *reorder_mesh_id_list_entry_num, uint64_t mesh_tbl_phy_addr, int imgw, int imgh)
{
    int mesh = -1;
    int reorder_mesh;
    int mesh_id_idx = 0;
    int mesh_idx = 0;
    int max_mesh_cnt = floor(imgw / 16) * floor(imgh / 16);
    (void)(max_mesh_cnt);
    //int *reorder_id_map = (int *)malloc(sizeof(int) * max_mesh_cnt);
    int *reorder_id_map = (int *)malloc(sizeof(int) * 128*128);
    int *reorder_mesh_slice_tbl = reorder_mesh_tbl[0]; // initial mesh_tbl to

    int i = 0;
    //int ext_mem_addr_alignment = 32;

    (void) mesh_id_list_entry_num;
    (void)Y_SUBTILE_NUMBER;

    // frame start ID
    reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++];

    for (int k = 0; k < Y_TILE_NUMBER; k++) {
        for (int j = 0; j < X_TILE_NUMBER; j++) {
            reorder_mesh_tbl[j + k * X_TILE_NUMBER] = reorder_mesh_slice_tbl + mesh_idx;

            reorder_mesh = 0;
#if 1 // (JAMMY) clear to -1
            //for (int l = 0; l < max_mesh_cnt; l++)
            for (int l = 0; l < 128 *128; l++) {

                reorder_id_map[l] = -1;
            }
#else
            memset(reorder_id_map, 0xff, sizeof(sizeof(int) * 128 * 128));
#endif

            // slice start ID
            mesh = mesh_scan_tile_mesh_id_list[i++];
            reorder_mesh_id_list[mesh_id_idx++] = mesh;

#if 0 // (JAMMY) replace with phy-addr later.
            // reorder mesh table address -> reorder mesh id list header
            uintptr_t addr = (uintptr_t)reorder_mesh_tbl[j + k * X_TILE_NUMBER];

            reorder_mesh_id_list[mesh_id_idx++] = addr & 0x0000000fff;
            reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0000fff000) >> 12;
            reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0fff000000) >> 24;
            reorder_mesh_id_list[mesh_id_idx++] = (addr & 0xf000000000) >> 36;
#else
            u64 addr = mesh_tbl_phy_addr
                + ((uintptr_t)reorder_mesh_tbl[j + k * X_TILE_NUMBER]
                    - (uintptr_t)reorder_mesh_tbl[0]);

            reorder_mesh_id_list[mesh_id_idx++] = addr & 0x0000000fff;
            reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0000fff000) >> 12;
            reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0fff000000) >> 24;
            reorder_mesh_id_list[mesh_id_idx++] = (addr & 0xf000000000) >> 36;
#endif

            // slice src and width -> reorder mesh id list header
            reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile x src
            reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile width
            reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile y src
            reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile height

            while (mesh_scan_tile_mesh_id_list[i] != MESH_ID_FSE) {
                mesh = mesh_scan_tile_mesh_id_list[i++];
                if (mesh == MESH_ID_FST || mesh == MESH_ID_FSS) {
                    continue;
                } else if (mesh == MESH_ID_FSP || /* mesh == MESH_ID_FED */ mesh == MESH_ID_FTE) {
                    reorder_mesh_id_list[mesh_id_idx++] = mesh; // meta-data header
                } else /* if (mesh != MESH_ID_FED) */ {
                    if (reorder_id_map[mesh] == -1) {
                        reorder_id_map[mesh] = reorder_mesh;
                        reorder_mesh++;

                        for (int l = 0; l < 4; l++) {
                            reorder_mesh_slice_tbl[mesh_idx++] = isrc_x_mesh_tbl[mesh][l];
                            reorder_mesh_slice_tbl[mesh_idx++] = isrc_y_mesh_tbl[mesh][l];
                            reorder_mesh_slice_tbl[mesh_idx++] = idst_x_mesh_tbl[mesh][l];
                            reorder_mesh_slice_tbl[mesh_idx++] = idst_y_mesh_tbl[mesh][l];
                        }
                    }

                    reorder_mesh_id_list[mesh_id_idx++] = reorder_id_map[mesh];
                }
            }

            // slice end ID
            reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++];
        }
    }

    // frame end ID
    reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++];

    if (reorder_mesh_id_list[mesh_id_idx -1] != MESH_ID_FED) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "invalid param, frame end id is not %#x\n", MESH_ID_FED);
    }

    /* dwa on DDR each time 256bit(32 byte)@burst8,
    ** must make sure that the frame_end tag not be allowed
    ** to fall on the 8th 32byte
    */
    int frm_end_tag = reorder_mesh_id_list[mesh_id_idx - 1];
    int k = 0;
    int mesh_id_idx1 = mesh_id_idx - 1;
    int mesh_id_pos_lines = mesh_id_idx1 << 1;

    for (k = 0xe0; k <= 0xff; k++) {
        //CVI_TRACE_DWA(CVI_DBG_DEBUG, "mesh_id_pos_lines:%#x, %#x, k:%#x\n" , mesh_id_pos_lines, mesh_id_pos_lines & 0xff, k);
        if ((mesh_id_pos_lines & 0xff) == k) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "frm_end_tag in [%#x] pos, will put to next 2 lines(32byte)\n", mesh_id_pos_lines);
            for (int h = 0; h < 16; h++) {
                reorder_mesh_id_list[mesh_id_idx1++] = 0x0000;
            }
            break;
        }
    }
    if (k <= 0xff)
        reorder_mesh_id_list[mesh_id_idx1++] = frm_end_tag;

    *reorder_mesh_tbl_entry_num = mesh_idx;
    *reorder_mesh_id_list_entry_num = mesh_id_idx;

    free(reorder_id_map);

    return 0;
}


int mesh_coordinate_float2fixed(float src_x_mesh_tbl[][4], float src_y_mesh_tbl[][4], float dst_x_mesh_tbl[][4],
                float dst_y_mesh_tbl[][4], int mesh_tbl_num, int isrc_x_mesh_tbl[][4],
                int isrc_y_mesh_tbl[][4], int idst_x_mesh_tbl[][4], int idst_y_mesh_tbl[][4])
{
    int64_t MAX_VAL;
    int64_t MIN_VAL;
    double tmp_val;
    int64_t val;

    MAX_VAL = 1;
    MAX_VAL <<= (DEWARP_COORD_MBITS + DEWARP_COORD_NBITS);
    MAX_VAL -= 1;
    MIN_VAL = -1 * MAX_VAL;

    for (int i = 0; i < mesh_tbl_num; i++) {
        for (int j = 0; j < 4; j++) {
            tmp_val = (src_x_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
            val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
            isrc_x_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);

            tmp_val = (src_y_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
            val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
            isrc_y_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);

            tmp_val = (dst_x_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
            val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
            idst_x_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);

            tmp_val = (dst_y_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
            val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
            idst_y_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);
        }
    }

    return 0;
}

int mesh_scan_preproc_3(int dst_width, int dst_height
    , const float dst_x_mesh_tbl[][4], const float dst_y_mesh_tbl[][4]
    , int mesh_tbl_num, uint16_t *mesh_scan_id_order, int X_TILE_NUMBER
    , int NUM_Y_LINE_A_SUBTILE, int Y_TILE_NUMBER, int MAX_MESH_NUM_A_TILE)
{
    int tile_idx_x, tile_idx_y;
    int current_dst_mesh_y_line_intersection_cnt, mesh_num_cnt = 0;

    int id_idx = 0;
    int tmp_mesh_cnt = MAX_MESH_NUM_A_TILE * 4;
    int *tmp_mesh_scan_id_order = (int *)malloc(sizeof(int) * (MAX_MESH_NUM_A_TILE * 4));
    int NUM_X_LINE_A_TILE = ceil((float)dst_width / (float)X_TILE_NUMBER / 2.0) * 2;
    int NUM_Y_LINE_A_TILE =
        (int)(ceil(ceil((float)dst_height / (float)Y_TILE_NUMBER) / (float)NUMBER_Y_LINE_A_SUBTILE)) *
        NUMBER_Y_LINE_A_SUBTILE;

    mesh_scan_id_order[id_idx++] = MESH_ID_FST;

    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "X_TILE_NUMBER(%d) Y_TILE_NUMBER(%d)\n", X_TILE_NUMBER, Y_TILE_NUMBER);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "NUM_X_LINE_A_TILE(%d) NUM_Y_LINE_A_TILE(%d)\n", NUM_X_LINE_A_TILE, NUM_Y_LINE_A_TILE);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "NUM_Y_LINE_A_SUBTILE(%d)\n", NUM_Y_LINE_A_SUBTILE);

    for (tile_idx_y = 0; tile_idx_y < Y_TILE_NUMBER; tile_idx_y++) {
        // src_y = (1st line of this tile )
        // dst_y = (1st line of next tile )
        int src_y = CLIP((tile_idx_y * NUM_Y_LINE_A_TILE), 0, dst_height);
        int dst_y = CLIP((src_y + NUM_Y_LINE_A_TILE), 0, dst_height);

        for (tile_idx_x = 0; tile_idx_x < X_TILE_NUMBER; tile_idx_x++) {
            mesh_scan_id_order[id_idx++] = MESH_ID_FSS;

            int src_x = CLIP((tile_idx_x * NUM_X_LINE_A_TILE), 0, dst_width);
            int dst_x = CLIP((src_x + NUM_X_LINE_A_TILE), 0, dst_width);

            // label starting position (slice) and its size ( x & y )
            mesh_scan_id_order[id_idx++] = src_x;
            mesh_scan_id_order[id_idx++] = dst_x - src_x;
            mesh_scan_id_order[id_idx++] = src_y;
            mesh_scan_id_order[id_idx++] = dst_y - src_y;

            for (int y = src_y; y < dst_y; y++) {
                // if first line of a tile, then initialization
                if (y % NUM_Y_LINE_A_SUBTILE == 0) {
                    mesh_num_cnt = 0;
                    for (int i = 0; i < tmp_mesh_cnt; i++)
                        tmp_mesh_scan_id_order[i] = -1;

                    // frame separator ID insertion
                    mesh_scan_id_order[id_idx++] = MESH_ID_FSP;
                }

                for (int m = 0; m < mesh_tbl_num; m++) {
                    current_dst_mesh_y_line_intersection_cnt = 0;
                    int is_mesh_incorp_yline = 0;
                    int min_x = MIN(MIN(dst_x_mesh_tbl[m][0], dst_x_mesh_tbl[m][1]),
                            MIN(dst_x_mesh_tbl[m][2], dst_x_mesh_tbl[m][3]));
                    int max_x = MAX(MAX(dst_x_mesh_tbl[m][0], dst_x_mesh_tbl[m][1]),
                            MAX(dst_x_mesh_tbl[m][2], dst_x_mesh_tbl[m][3]));
                    int min_y = MIN(MIN(dst_y_mesh_tbl[m][0], dst_y_mesh_tbl[m][1]),
                            MIN(dst_y_mesh_tbl[m][2], dst_y_mesh_tbl[m][3]));
                    int max_y = MAX(MAX(dst_y_mesh_tbl[m][0], dst_y_mesh_tbl[m][1]),
                            MAX(dst_y_mesh_tbl[m][2], dst_y_mesh_tbl[m][3]));
                    if (min_y <= y && y <= max_y && min_x <= src_x && dst_x <= max_x)
                        is_mesh_incorp_yline = 1;

                    if (!is_mesh_incorp_yline) {
                        for (int k = 0; k < 4; k++) {
                            float knot_dst_a_y = dst_y_mesh_tbl[m][MESH_EDGE[k][0]];
                            float knot_dst_b_y = dst_y_mesh_tbl[m][MESH_EDGE[k][1]];
                            float knot_dst_a_x = dst_x_mesh_tbl[m][MESH_EDGE[k][0]];
                            float knot_dst_b_x = dst_x_mesh_tbl[m][MESH_EDGE[k][1]];
                            float delta_a_y = (float)y - knot_dst_a_y;
                            float delta_b_y = (float)y - knot_dst_b_y;
                            int intersect_x = 0;

                            if ((src_x <= knot_dst_a_x) && (knot_dst_a_x <= dst_x))
                                intersect_x = 1;
                            if ((src_x <= knot_dst_b_x) && (knot_dst_b_x <= dst_x))
                                intersect_x = 1;

                            // check whether if vertex connection
                            if ((delta_a_y == 0.f) && (intersect_x == 1)) {
                                current_dst_mesh_y_line_intersection_cnt += 2;
                            }
                            // check whether if edge connection
                            else if ((delta_a_y * delta_b_y < 0) && (intersect_x == 1)) {
                                current_dst_mesh_y_line_intersection_cnt += 2;
                            }
                            // otherwise no connection
                        } // finish check in a mesh
                    }

                    if ((current_dst_mesh_y_line_intersection_cnt > 0) || (is_mesh_incorp_yline == 1)) {
                        // check the mesh in list or not
                        int isInList = 0;

                        for (int i = 0; i < mesh_num_cnt; i++) {
                            if (m == tmp_mesh_scan_id_order[i]) {
                                isInList = 1;
                                break;
                            }
                        }
                        // not in the list, then add the mesh to list
                        if (!isInList) {
                            tmp_mesh_scan_id_order[mesh_num_cnt] = m;
                            mesh_num_cnt++;
                        }
                    }
                }

                // x direction reorder
                if (((y % NUM_Y_LINE_A_SUBTILE) == (NUM_Y_LINE_A_SUBTILE - 1)) || (y == dst_height - 1)) {
                    for (int i = 0; i < mesh_num_cnt - 1; i++) {
                        for (int j = 0; j < mesh_num_cnt - 1 - i; j++) {
                            int m0 = tmp_mesh_scan_id_order[j];
                            int m1 = tmp_mesh_scan_id_order[j + 1];
                            float knot_m0_x0 = dst_x_mesh_tbl[m0][0];
                            float knot_m0_x1 = dst_x_mesh_tbl[m0][1];
                            float knot_m0_x2 = dst_x_mesh_tbl[m0][2];
                            float knot_m0_x3 = dst_x_mesh_tbl[m0][3];
                            float knot_m1_x0 = dst_x_mesh_tbl[m1][0];
                            float knot_m1_x1 = dst_x_mesh_tbl[m1][1];
                            float knot_m1_x2 = dst_x_mesh_tbl[m1][2];
                            float knot_m1_x3 = dst_x_mesh_tbl[m1][3];

                            int m0_min_x = MIN(MIN(knot_m0_x0, knot_m0_x1),
                                        MIN(knot_m0_x2, knot_m0_x3));
                            int m1_min_x = MIN(MIN(knot_m1_x0, knot_m1_x1),
                                        MIN(knot_m1_x2, knot_m1_x3));

                            if (m0_min_x > m1_min_x) {
                                int tmp = tmp_mesh_scan_id_order[j];

                                tmp_mesh_scan_id_order[j] =
                                    tmp_mesh_scan_id_order[j + 1];
                                tmp_mesh_scan_id_order[j + 1] = tmp;
                            }
                        }
                    }

                    // mesh ID insertion
                    for (int i = 0; i < mesh_num_cnt; i++) {
                        mesh_scan_id_order[id_idx++] = tmp_mesh_scan_id_order[i];
                    }

                    // tile end ID insertion
                    mesh_scan_id_order[id_idx++] = MESH_ID_FTE;
                }
            }

            // frame slice end ID insertion
            mesh_scan_id_order[id_idx++] = MESH_ID_FSE;
        }
    }

    // frame end ID insertion
    mesh_scan_id_order[id_idx++] = MESH_ID_FED;

    free(tmp_mesh_scan_id_order);

    return id_idx;
}

void bm_get_frame_mesh_list(BM_FISHEYE_ATTR* FISHEYE_CONFIG, BM_FISHEYE_REGION_ATTR* FISHEYE_REGION)
{
    // pack all regions' mesh info, including src & dst.
    int rgnNum = FISHEYE_CONFIG->RgnNum;
    int meshNumRgn;
    int frameMeshIdx = 0;
    int rotate_index = FISHEYE_CONFIG->rotate_index;
    int view_w = FISHEYE_CONFIG->OutW_disp;
    int view_h = FISHEYE_CONFIG->OutH_disp;

    for (int i = 0; i < rgnNum; i++) {
        if (FISHEYE_REGION[i].RegionValid == 1) {
            meshNumRgn = (FISHEYE_REGION[i].MeshHor * FISHEYE_REGION[i].MeshVer);

            // go through each region loop
            for (int meshidx = 0; meshidx < meshNumRgn; meshidx++) {
                // each mesh has 4 knots
                for (int knotidx = 0; knotidx < 4; knotidx++) {
                    // do rotaion:
                    double x_src = FISHEYE_REGION[i].SrcRgnMeshInfo[meshidx].knot[knotidx].xcor;
                    double y_src = FISHEYE_REGION[i].SrcRgnMeshInfo[meshidx].knot[knotidx].ycor;
                    double x_dst = FISHEYE_REGION[i].DstRgnMeshInfo[meshidx].knot[knotidx].xcor;
                    double y_dst = FISHEYE_REGION[i].DstRgnMeshInfo[meshidx].knot[knotidx].ycor;

                    double x_dst_out, y_dst_out;
                    _do_mesh_rotate(rotate_index, view_h, view_w, x_dst, y_dst, &x_dst_out, &y_dst_out);

                    FISHEYE_CONFIG->DstRgnMeshInfo[frameMeshIdx].knot[knotidx].xcor = x_dst_out; //FISHEYE_REGION[i].DstRgnMeshInfo[meshidx].knot[knotidx].xcor;
                    FISHEYE_CONFIG->DstRgnMeshInfo[frameMeshIdx].knot[knotidx].ycor = y_dst_out; //FISHEYE_REGION[i].DstRgnMeshInfo[meshidx].knot[knotidx].ycor;
                    FISHEYE_CONFIG->SrcRgnMeshInfo[frameMeshIdx].knot[knotidx].xcor = x_src; //FISHEYE_REGION[i].SrcRgnMeshInfo[meshidx].knot[knotidx].xcor;
                    FISHEYE_CONFIG->SrcRgnMeshInfo[frameMeshIdx].knot[knotidx].ycor = y_src; //FISHEYE_REGION[i].SrcRgnMeshInfo[meshidx].knot[knotidx].ycor;
                }
                frameMeshIdx += 1;
            }
        }
    }

    // update mesh index number
    FISHEYE_CONFIG->TotalMeshNum = frameMeshIdx;
}

void _do_mesh_rotate(int rotate_index, int view_h, int view_w, double xin, double yin, double *xout, double *yout)
{
    double RMATRIX[2][2];

    if (rotate_index == 0)              //00 degrees
    {
        RMATRIX[0][0] = 1;
        RMATRIX[0][1] = 0;
        RMATRIX[1][0] = 0;
        RMATRIX[1][1] = 1;
    }
    else if (rotate_index == 1)         //-90 degrees
    {
        RMATRIX[0][0] = 0;
        RMATRIX[0][1] = 1;
        RMATRIX[1][0] = -1;
        RMATRIX[1][1] = 0;
    }
    else if (rotate_index == 2)         // + 90 degrees
    {
        RMATRIX[0][0] = -1;
        RMATRIX[0][1] = 0;
        RMATRIX[1][0] = 0;
        RMATRIX[1][1] = -1;
    }
    else
    {
        RMATRIX[0][0] = 0;
        RMATRIX[0][1] = -1;
        RMATRIX[1][0] = 1;
        RMATRIX[1][1] = 0;
    }

    *xout = RMATRIX[0][0] * xin + RMATRIX[1][0] * yin;
    *yout = RMATRIX[0][1] * xin + RMATRIX[1][1] * yin;


#if 0
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "view_h = %d\n", view_h);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "view_w = %d\n", view_w);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, " RMATRIX[0][0] = %f\n", RMATRIX[0][0]);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, " RMATRIX[0][1] = %f\n", RMATRIX[0][1]);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, " RMATRIX[1][0] = %f\n", RMATRIX[1][0]);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, " RMATRIX[1][1] = %f\n", RMATRIX[1][1]);


    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "xin = %f\n", xin);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "yin = %f\n", yin);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "xout = %f\n", xout);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "yout = %f\n", yout);
    system("pause");
#endif

    if (rotate_index == 0) {
        *xout = *xout;
        *yout = *yout;
    } else if (rotate_index == 1) {
        *xout = *xout + (view_h - 1);
        *yout = *yout;
    } else if (rotate_index == 2) {
        *xout = *xout + (view_w - 1);
        *yout = *yout + (view_h - 1);
    } else {
        *xout = *xout + 0;
        *yout = *yout + (view_w - 1);
    }
}

static void _LDC_View(BM_FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, double x0, double y0)
{
    if (FISHEYE_REGION[rgn_idx].ViewMode != PROJECTION_LDC)
        return;

    int view_w = FISHEYE_REGION[rgn_idx].OutW;
    int view_h = FISHEYE_REGION[rgn_idx].OutH;
    int mesh_horcnt = FISHEYE_REGION[rgn_idx].MeshHor;
    int mesh_vercnt = FISHEYE_REGION[rgn_idx].MeshVer;


    // register:
    bool bAspect    = (bool)FISHEYE_REGION[0].ZoomV;
    int XYRatio     = minmax(FISHEYE_REGION[0].Pan, 0, 100);
    int XRatio      = minmax(FISHEYE_REGION[0].ThetaX, 0, 100);
    int YRatio      = minmax(FISHEYE_REGION[0].ThetaZ, 0, 100);
    int CenterXOffset   = minmax(FISHEYE_REGION[0].InRadius, -511, 511);
    int CenterYOffset   = minmax(FISHEYE_REGION[0].OutRadius, -511, 511);
    int DistortionRatio = minmax(FISHEYE_REGION[0].PanEnd, -300, 500);

    double norm = sqrt((view_w / 2) * (view_w / 2) + (view_h / 2) * (view_h / 2));

    // internal link to register:
    double k = (double)DistortionRatio / 1000.0;
    double ud_gain = (1 + k * (((double)view_h / 2) * (double)view_h / 2) / norm / norm);
    double lr_gain = (1 + k * (((double)view_w / 2) * (double)view_w / 2) / norm / norm);

#if OFFSET_SCALE
    double new_x0 = x0;
    double new_y0 = y0;
    int CenterXOffsetScale = (CenterXOffset / OFFSET_SCALE);
    int CenterYOffsetScale = (CenterYOffset / OFFSET_SCALE);
    new_x0 +=  (CenterXOffset/OFFSET_SCALE);
    new_y0 +=  (CenterYOffset/OFFSET_SCALE);
#endif

    double Aspect_gainX = MAX2(ud_gain, lr_gain);
    double Aspect_gainY = MAX2(ud_gain, lr_gain);


    Vector2D dist2d;

    // go through all meshes in thus regions
    for (int i = 0; i < (mesh_horcnt * mesh_vercnt); i++) {
        // each mesh has 4 knots
        for (int knotidx = 0; knotidx < 4; knotidx++) {
            // display window center locate @ (0,0), mesh info shife back to center O.
            // for each region, rollback center is (0,0)
            double x = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor
                - FISHEYE_REGION[rgn_idx].OutX - FISHEYE_REGION[rgn_idx].OutW / 2;
            double y = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor
                - FISHEYE_REGION[rgn_idx].OutY - FISHEYE_REGION[rgn_idx].OutH / 2;

#if OFFSET_SCALE // new method
            x = minmax(x, (-view_w+ CenterXOffsetScale) / 2 + 1, (view_w+ CenterXOffsetScale) / 2 - 1);
            y = minmax(y, (-view_h+ CenterYOffsetScale) / 2 + 1, (view_h+ CenterYOffsetScale) / 2 - 1);
#else
            x = minmax(x - CenterXOffset, -view_w / 2 + 1, view_w / 2 - 1);
            y = minmax(y - CenterYOffset, -view_h / 2 + 1, view_h / 2 - 1);
#endif

            x = x / Aspect_gainX;
            y = y / Aspect_gainY;

            if (bAspect == true) {
                x = x * (1 - 0.333 * (100 - XYRatio)/100);
                y = y * (1 - 0.333 * (100 - XYRatio)/100);
            } else {
                x = x * (1 - 0.333 * (100 - XRatio)/100);
                y = y * (1 - 0.333 * (100 - YRatio)/100);
            }

            double rd = sqrt(x * x + y * y);

            // Zooming In/Out Control by ZoomH & ZoomV
            // 1.0 stands for +- 50% of zoomH ratio. => set as
            dist2d.x = x * (1 + k * ((rd / norm) * (rd / norm)));
            dist2d.y = y * (1 + k * ((rd / norm) * (rd / norm)));

            // update source mesh-info here
#if OFFSET_SCALE
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + new_x0;
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + new_y0;
#else
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0 + CenterXOffset;
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0 + CenterYOffset;
#endif
        }
    }
}


static void GenRotMatrix3D_YXZ(RotMatrix3D* mat, BM_FISHEYE_REGION_ATTR* FISHEYE_REGION_IDX)
{
    // This Rotation Matrix Order = R = Rz*Rx*Ry
    // rotation order = θy -> θx -> θz
    //_LOAD_REGION_CONFIG;
    // initital position
    double tmp_phy_x = FISHEYE_REGION_IDX->ThetaX;  //phy_x;
    double tmp_phy_y = FISHEYE_REGION_IDX->ThetaY;  //phy_y;    // Not Used for Now.
    double tmp_phy_z = FISHEYE_REGION_IDX->ThetaZ;  //phy_z;

    //_LOAD_REGION_CONFIG(FISHEYE_CONFIG, FISHEYE_REGION);
    // UI Control
    double ctrl_tilt, ctrl_pan;// = minmax(FISHEYE_REGION_IDX[rgn_idx].Tilt - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);
    //double ctrl_pan  = minmax(FISHEYE_REGION_IDX[rgn_idx].Pan - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);

    if (FISHEYE_REGION_IDX->ViewMode == PROJECTION_REGION)
    {
        ctrl_tilt = minmax(FISHEYE_REGION_IDX->Tilt - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);
        ctrl_pan  = minmax(FISHEYE_REGION_IDX->Pan  - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);
    }
    else
    {
        // not used in panorama case
        ctrl_pan  = 0;
        ctrl_tilt = 0;
    }

    tmp_phy_x += (ctrl_tilt * M_PI / 2)/(2* UI_CTRL_VALUE_CENTER);
    tmp_phy_y += 0;
    tmp_phy_z += (ctrl_pan * M_PI)/(2* UI_CTRL_VALUE_CENTER);

    mat->coef[0][0] = cos(tmp_phy_y) * cos(tmp_phy_z) - sin(tmp_phy_x) * sin(tmp_phy_y) * sin(tmp_phy_z);
    mat->coef[0][1] = -cos(tmp_phy_x) * sin(tmp_phy_z);
    mat->coef[0][2] = sin(tmp_phy_y) * cos(tmp_phy_z) + sin(tmp_phy_x) * cos(tmp_phy_y) * sin(tmp_phy_z);
    mat->coef[0][3] = 0;
    mat->coef[1][0] = cos(tmp_phy_y) * sin(tmp_phy_z) + sin(tmp_phy_x) * sin(tmp_phy_y) * cos(tmp_phy_z);
    mat->coef[1][1] = cos(tmp_phy_x) * cos(tmp_phy_z);
    mat->coef[1][2] = sin(tmp_phy_y) * sin(tmp_phy_z) - sin(tmp_phy_x) * cos(tmp_phy_y) * cos(tmp_phy_z);
    mat->coef[1][3] = 0;
    mat->coef[2][0] = -cos(tmp_phy_x) * sin(tmp_phy_y);
    mat->coef[2][1] = sin(tmp_phy_x);
    mat->coef[2][2] = cos(tmp_phy_x) * cos(tmp_phy_y);
    mat->coef[2][3] = 0;
    mat->coef[3][0] = 0;
    mat->coef[3][1] = 0;
    mat->coef[3][2] = 0;
    mat->coef[3][3] = 1;
}


static void Normalize3D(Vector3D* v)
{
    double x3d, y3d, z3d, d;

    x3d = v->x;
    y3d = v->y;
    z3d = v->z;
    d = sqrt(x3d * x3d + y3d * y3d + z3d * z3d);
    v->x = x3d / d;
    v->y = y3d / d;
    v->z = z3d / d;
}


static void Equidistant(Vector3D* v3d, Vector2D* v2d, double f)
{
    double rd, ri, theta, phy;

    Normalize3D(v3d);

    // calculate theta from the angle between (0,0,1) and (x3d, y3d, z3d)
    theta = acos(v3d->z);

    // calculate ri = f tan(theta)
    ri = f * tan(theta);
    if (theta == 0.0) {
        ri += 1e-64;
        theta += 1e-64;
    }

    // calculate rd = f * theta
    rd = f * (theta / (M_PI / 2));
    phy = atan(rd / f);

    // calculate new distorted vector (x,y,z) = (0,0,1) * (1-rd/ri) + (x3d, y3d, z3d) * (rd/ri)
    Vector3D v3d_p;

    v3d_p.x = v3d->x * phy / theta;
    v3d_p.y = v3d->y * phy / theta;
    v3d_p.z = v3d->z * phy / theta + (1 - phy / theta);

    // calculate the target point (xi, yi) on image at z = f
    v2d->x = (f / v3d_p.z) * v3d_p.x;
    v2d->y = (f / v3d_p.z) * v3d_p.y;
}

static void Rotate3D(Vector3D* v3d, RotMatrix3D* mat)
{
    //int i, j;
    Vector3D v1;

    v1.x = mat->coef[0][0] * v3d->x + mat->coef[0][1] * v3d->y +
        mat->coef[0][2] * v3d->z + mat->coef[0][3];
    v1.y = mat->coef[1][0] * v3d->x + mat->coef[1][1] * v3d->y +
        mat->coef[1][2] * v3d->z + mat->coef[1][3];
    v1.z = mat->coef[2][0] * v3d->x + mat->coef[2][1] * v3d->y +
        mat->coef[2][2] * v3d->z + mat->coef[2][3];

    v3d->x = v1.x;
    v3d->y = v1.y;
    v3d->z = v1.z;
}

static void _Panorama180View2(BM_FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, FISHEYE_MOUNT_MODE_E MOUNT, double x0, double y0, double r)
{
    if (FISHEYE_REGION[rgn_idx].ViewMode != PROJECTION_PANORAMA_180)
        return;
    if (MOUNT != FISHEYE_WALL_MOUNT) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "mount_mode(%d) not supported in Panorama180\n", MOUNT);
        return;
    }

    int view_w = FISHEYE_REGION[rgn_idx].OutW;
    int view_h = FISHEYE_REGION[rgn_idx].OutH;
    int tot_mesh_cnt = FISHEYE_REGION[rgn_idx].MeshHor * FISHEYE_REGION[rgn_idx].MeshVer;

    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "view_w = %d, view_h = %d, \n", view_w, view_h);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_horcnt = %d,\n", mesh_horcnt);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_vercnt = %d,\n", mesh_vercnt);

    // UI PARAMETERS
    int _UI_horViewOffset = FISHEYE_REGION[rgn_idx].Pan;        // value range = 0 ~ 360, => -180 ~ 0 ~ +180
    int _UI_verViewOffset = FISHEYE_REGION[rgn_idx].Tilt;       // value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
    int _UI_horViewRange  = FISHEYE_REGION[rgn_idx].ZoomH;      // value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
    int _UI_verViewRange  = FISHEYE_REGION[rgn_idx].ZoomV;      // value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90

    _UI_verViewRange = (_UI_verViewRange == 4095) ? 4096 : _UI_verViewRange;
    _UI_horViewRange = (_UI_horViewRange == 4095) ? 4096 : _UI_horViewRange;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "_UI_horViewOffset = %d,\n", _UI_horViewOffset);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "_UI_verViewOffset = %d,\n", _UI_verViewOffset);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "_UI_horViewRange = %d,\n", _UI_horViewRange);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "_UI_verViewRange = %d,\n", _UI_verViewRange);

    // calculate default view range:
    double view_range_ver_degs = (double)_UI_verViewRange * 90 / 4096;
    double view_range_hor_degs = (double)_UI_horViewRange * 90 / 4096;
    double va_ver_degs = view_range_ver_degs;
    double va_hor_degs = view_range_hor_degs;

    // calculate offsets
    double va_hor_offset = ((double)_UI_horViewOffset - 180) * 90 / 360;
    double va_ver_offset = ((double)_UI_verViewOffset - 180) * 90 / 360;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "va_hor_offset = %f,\n", va_hor_offset);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "va_ver_offset = %f,\n", va_ver_offset);

    // Offset to shift view angle
    double va_ver_start = minmax(90 - va_ver_degs + va_ver_offset,  0,  90);
    double va_ver_end   = minmax(90 + va_ver_degs + va_ver_offset, 90, 180);
    double va_hor_start = minmax(90 - va_hor_degs + va_hor_offset,  0,  90);
    double va_hor_end   = minmax(90 + va_hor_degs + va_hor_offset, 90, 180);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "va_ver_start = %f,\n", va_ver_start);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "va_ver_end = %f,\n", va_ver_end);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "va_hor_start = %f,\n", va_hor_start);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "va_hor_end = %f,\n", va_hor_end);

    //system("pause");

    RotMatrix3D mat0;

    Vector3D pix3d;
    Vector2D dist2d;
    int X_offset = FISHEYE_REGION[rgn_idx].OutX + FISHEYE_REGION[rgn_idx].OutW / 2;
    int Y_offset = FISHEYE_REGION[rgn_idx].OutY + FISHEYE_REGION[rgn_idx].OutH / 2;

    for (int i = 0; i < tot_mesh_cnt; i++)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "i = %d,", i);
        // each mesh has 4 knots
        for (int knotidx = 0; knotidx < 4; knotidx++)
        {
            double x = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor - X_offset;
            double y = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor - Y_offset;
            //double z = r;

            // initial pseudo plane cooridinates as vp3d
            //double theta_hor_cur = va_hor_start + ((2 * va_hor_degs * (view_w / 2 - x)) / (double)view_w);
            double theta_hor_cur = va_hor_start + (((va_hor_end - va_hor_start) * (view_w / 2 - x)) / (double)view_w);
            double theta_hor_cur_rad = M_PI / 2 - (theta_hor_cur * M_PI) / 180;     //θx

            double theta_ver_cur = va_ver_start + (((va_ver_end - va_ver_start) * (view_h / 2 - y)) / (double)view_h);
            double theta_ver_cur_rad = M_PI / 2 - (theta_ver_cur * M_PI) / 180;     //θy

            // if (knotidx == 0)
            // {
            //     bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "(x,y)=(%f,%f) FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor = %f, ", x, y, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor);
            //     bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "hor_deg = %f, ver_deg = %f,\n\r",theta_hor_cur, theta_ver_cur);
            //     bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "hor_rad = %f,  ver_rad = %f, \n\r", theta_hor_cur_rad, theta_ver_cur_rad);
            // }

            double theta_x = theta_hor_cur_rad;
            double theta_y = theta_ver_cur_rad;
            double theta_z = (M_PI / 2);

            FISHEYE_REGION[rgn_idx].ThetaX = theta_x;
            FISHEYE_REGION[rgn_idx].ThetaY = theta_y;
            FISHEYE_REGION[rgn_idx].ThetaZ = theta_z;

            GenRotMatrix3D_YXZ(&mat0, &FISHEYE_REGION[rgn_idx]);

            pix3d.x = 0;
            pix3d.y = 0;
            pix3d.z = 1;

            Rotate3D(&pix3d, &mat0);

            // generate new 3D vector thru rotated pixel
            //Normalize3D(&pix3d);

            // generate 2D location on distorted image
            //Equidistant_Panorama(&pix3d, &dist2d, r);
            Equidistant(&pix3d, &dist2d, r);

            // update source mesh-info here
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0;
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0;

            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "(x2d)(%f,%f),\n\r",dist2d.x + x0, dist2d.y + y0);

        }
    }
}

static void Equidistant_Panorama(Vector3D* v3d, Vector2D* v2d, double f)
{
    double rd, theta;

    // calculate theta from the angle between (0,0,1) and (x3d, y3d, z3d)
    //theta = acos(v3d->z); //bastian

    theta = v3d->z;

    if (theta == 0.0) {
        theta += 1e-64;
    }

#if 0
    // calculate rd = f * theta
    rd = f * (theta / (M_PI / 2));
    phy = atan(rd / f);

    // calculate new distorted vector (x,y,z) = (0,0,1) * (1-rd/ri) + (x3d, y3d, z3d) * (rd/ri)
    Vector3D v3d_p;
    v3d_p.x = v3d->x * phy / theta;
    v3d_p.y = v3d->y * phy / theta;
    v3d_p.z = v3d->z * phy / theta + (1 - phy / theta);

    // calculate the target point (xi, yi) on image at z = f
    v2d->x = (f / v3d_p.z) * v3d_p.x;
    v2d->y = (f / v3d_p.z) * v3d_p.y;

#else
    // calculate rd = f * theta
    // assume: fisheye image 2D' radius rd
    // if it's equidistant: r = focal*incident_angle
    // max incident angle = PI/2
    rd = f * (theta / (M_PI / 2));

    // ( 0,0 ) & (v3d->x, v3d->y): find equation y = ax + b
    // a = v3d.y/v3d.x
    // if incident angle from space ( x, y ),
    double L_a = v3d->y / v3d->x;

    v2d->x = (v3d->x >= 0) ? (-sqrt((rd * rd) / (L_a * L_a + 1))) : sqrt((rd * rd) / (L_a * L_a + 1));
    v2d->y = L_a * v2d->x;

#endif
}

static void _Panorama360View2(BM_FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, FISHEYE_MOUNT_MODE_E MOUNT, double x0, double y0, double r)
{
    if (FISHEYE_REGION[rgn_idx].ViewMode != PROJECTION_PANORAMA_360)
        return;
    if ((MOUNT != FISHEYE_CEILING_MOUNT) && (MOUNT != FISHEYE_DESKTOP_MOUNT)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "mount_mode(%d) not supported in Panorama360\n", MOUNT);
        return;
    }

    int view_w = FISHEYE_REGION[rgn_idx].OutW;
    int view_h = FISHEYE_REGION[rgn_idx].OutH;
    int tot_mesh_cnt = FISHEYE_REGION[rgn_idx].MeshHor * FISHEYE_REGION[rgn_idx].MeshVer;

    //Vector2D vp2d;
    //Vector3D vp3d;
    //RotMatrix3D mat0;

    Vector3D pix3d;
    Vector2D dist2d;

    // UI PARAMETERS
    //float _UI_start_angle     = FISHEYE_REGION[rgn_idx].Pan;              // value from 0=360, 0 ~ 356 is the range of adjustment.
    float _UI_view_offset       = FISHEYE_REGION[rgn_idx].Tilt;             // value = 0 ~ 360, center = 180 ( original ), OR create offset to shift OutRadius & InRadius ( = adjust theta angle in our implementation )
    float _UI_inradius      = FISHEYE_REGION[rgn_idx].InRadius;             // value = 0 ~ 4095
    float _UI_outradius     = FISHEYE_REGION[rgn_idx].OutRadius;            // value = 0 ~ 4095
    float _UI_zoom_outradius    = FISHEYE_REGION[rgn_idx].ZoomV;            // a ratio to zoom OutRadius length.

    _UI_inradius  = (_UI_inradius >= 4095)  ? 4096 : _UI_inradius;
    _UI_outradius = (_UI_outradius >= 4095) ? 4096 : _UI_outradius;
    _UI_zoom_outradius = (_UI_zoom_outradius >= 4095) ? 4096 : _UI_zoom_outradius;

    int start_angle_degrees  = FISHEYE_REGION[rgn_idx].Pan;
    int end_angle__degrees   = FISHEYE_REGION[rgn_idx].PanEnd;


    float raw_outradius_pxl = (_UI_outradius * r )/ 4096;
    float raw_inradius_pxl  = (_UI_inradius * r )/ 4096;
    float radiusOffset      = (raw_outradius_pxl * (_UI_view_offset - 180)) / 360;

    float inradius_pxl_final  = MIN(r, MAX(0, raw_inradius_pxl + radiusOffset));
    float outradius_pxl_final = MIN(r, MAX(inradius_pxl_final, raw_inradius_pxl + (MAX(0, (raw_outradius_pxl - raw_inradius_pxl)) * _UI_zoom_outradius) / 4096 + radiusOffset));

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "r = %f,\n", r);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "raw_outradius_pxl = %f,\n", raw_outradius_pxl);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "raw_inradius_pxl = %f,\n", raw_inradius_pxl);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "radiusOffset = %f,\n", radiusOffset);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "inradius_pxl_final = %f,\n", inradius_pxl_final);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "outradius_pxl_final = %f,\n", outradius_pxl_final);


    float va_ver_end_rads   = inradius_pxl_final * M_PI / (2 * r);//_UI_inradius * M_PI / 1024;     // for Equidistant =>rd = f*theta, rd_max = f*PI/2 = r ( in code), f = 2r/PI. => theta = rd/f = rd*PI/2r
    float va_ver_start_rads = outradius_pxl_final * M_PI / (2 * r);//_UI_outradius * M_PI / 1024;
    float va_ver_rads = MIN( M_PI/2, va_ver_start_rads - va_ver_end_rads);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "va_ver_end_rads = %f,\n", va_ver_end_rads);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "va_ver_start_rads = %f,\n", va_ver_start_rads);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "va_ver_rads = %f,\n", va_ver_rads);

    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "( %d, %d, %d, %f, %f, %d,) \n", rgn_idx, _UI_start_angle, _UI_view_offset, _UI_inradius, _UI_outradius, _UI_zoom_outradius);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "start_angle_degrees = %d, end_angle__degrees = %d, va_ver_degs = %f, va_ver_start_degs = %f, \n\r", start_angle_degrees, end_angle__degrees, va_ver_rads, va_ver_start_rads);
    // system("pause");

    int total_angle = (360 + (end_angle__degrees - start_angle_degrees)) % 360;
    int half_w = view_w / 2;
    int half_h = view_h / 2;
    int X_offset = FISHEYE_REGION[rgn_idx].OutX + FISHEYE_REGION[rgn_idx].OutW / 2;
    int Y_offset = FISHEYE_REGION[rgn_idx].OutY + FISHEYE_REGION[rgn_idx].OutH / 2;

    for (int i = 0; i < tot_mesh_cnt; i++)
    {
        // each mesh has 4 knots
        for (int knotidx = 0; knotidx < 4; knotidx++)
        {
            double phi_degrees, phi_rad, theta_rv;

            double x = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor - X_offset;
            double y = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor - Y_offset;

            if(MOUNT == FISHEYE_DESKTOP_MOUNT) {
                phi_degrees = (total_angle * (half_w - x )) / (double)view_w;
                phi_rad = (((start_angle_degrees + phi_degrees) * M_PI)) / 180;
                //theta_rv = (((va_ver_degs * M_PI) / 180) * (y + (view_h / 2))) / (double)(view_h);
                theta_rv = ( va_ver_rads * (y + half_h)) / (double)(view_h);
            } else if (MOUNT == FISHEYE_CEILING_MOUNT) {
                phi_degrees = (total_angle * (half_w + x)) / (double)view_w;
                phi_rad = (((start_angle_degrees + phi_degrees) * M_PI)) / 180;
                //theta_rv = (((va_ver_degs * M_PI) / 180) * ((view_h / 2) - y)) / (double)(view_h);
                theta_rv = ( va_ver_rads * (half_h - y)) / (double)(view_h);
            }


            // 2D plane cooridnate to cylinder
            // rotation initial @ [x, y, z] = [1, 0, 0];
            double xc =  1 * cos(-phi_rad) + 0 * sin(-phi_rad);
            double yc = -1 * sin(-phi_rad) + 0 * cos(-phi_rad);
            double zc = theta_rv + va_ver_end_rads;

            //Rotate3D(&pix3d, &mat0);
            pix3d.x = xc;
            pix3d.y = yc;
            pix3d.z = zc;
            //Equidistant(&pix3d, &dist2d, r);

            Equidistant_Panorama(&pix3d, &dist2d, r);

            // update source mesh-info here
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0;
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0;
        }
    }

}

static void _RegionView2(BM_FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, FISHEYE_MOUNT_MODE_E MOUNT, double x0, double y0, double r)
{
    if (FISHEYE_REGION[rgn_idx].ViewMode != PROJECTION_REGION)
        return;

    //int view_w = FISHEYE_REGION[rgn_idx].OutW;
    //int view_h = FISHEYE_REGION[rgn_idx].OutH;
    int tot_mesh_cnt = FISHEYE_REGION[rgn_idx].MeshHor * FISHEYE_REGION[rgn_idx].MeshVer;

    // rotation matrix to point out view center of this region.
    RotMatrix3D mat0;
    GenRotMatrix3D_YXZ(&mat0, &FISHEYE_REGION[rgn_idx]);

    Vector3D pix3d;
    Vector2D dist2d;
    int X_offset = FISHEYE_REGION[rgn_idx].OutX + FISHEYE_REGION[rgn_idx].OutW / 2;
    int Y_offset = FISHEYE_REGION[rgn_idx].OutY + FISHEYE_REGION[rgn_idx].OutH / 2;
    double w_ratio = 1.0 * (FISHEYE_REGION[rgn_idx].ZoomH - 2048) / 2048;
    double h_ratio = 1.0 * (FISHEYE_REGION[rgn_idx].ZoomV - 2048) / 2048;

    // go through all meshes in thus regions
    // mat0 is decided by view angle defined in re gion config.
    for (int i = 0; i < tot_mesh_cnt; i++)
    {
        // each mesh has 4 knots
        for (int knotidx = 0; knotidx < 4; knotidx++)
        {
            // display window center locate @ (0,0), mesh info shife back to center O.
            // for each region, rollback center is (0,0)
            double x = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor - X_offset;
            double y = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor - Y_offset;
            double z = r;


            if (MOUNT == FISHEYE_DESKTOP_MOUNT) {
                x = -x;
                y = -y;
            }


            // Zooming In/Out Control by ZoomH & ZoomV
            // 1.0 stands for +- 50% of zoomH ratio. => set as
            x = x *(1 + w_ratio);
            y = y *(1 + h_ratio);

            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "(x,y)=(%f,%f)\n\r", x, y);

            pix3d.x = x;
            pix3d.y = y;
            pix3d.z = z;

            // Region Porjection Model
            Rotate3D(&pix3d, &mat0);
            Equidistant(&pix3d, &dist2d, r);

            // update source mesh-info here
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0;
            FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "rgn(%d) mesh(%d) node(%d) (%lf %lf)\n", rgn_idx, i, knotidx, FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor, FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor);
        }
    }
}

int dwa_load_meshdata(char *grid, BM_MESH_DATA_ALL_S *pmeshdata, const char *bindName)
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

static int generate_mesh_on_fisheye(
    const GRID_INFO_ATTR_S* pstGridInfoAttr,
    BM_FISHEYE_ATTR* FISHEYE_CONFIG,
    BM_FISHEYE_REGION_ATTR* FISHEYE_REGION,
    int X_TILE_NUMBER,
    int Y_TILE_NUMBER,
    uint16_t *reorder_mesh_id_list,
    int **reorder_mesh_tbl,
    uint64_t mesh_tbl_phy_addr,
    void *ptr)
{
    int mesh_tbl_num;   // get number of meshes
    double x0, y0, r;   // infos of src_img, (x0,y0) = center of image,  r = radius of image.

    struct timespec start, end;
    BM_MESH_DATA_ALL_S g_MeshData[MESH_DATA_MAX_NUM];

    clock_gettime(CLOCK_MONOTONIC, &start);

    x0 = FISHEYE_CONFIG->InCenterX;
    y0 = FISHEYE_CONFIG->InCenterY;
    r = FISHEYE_CONFIG->InRadius;
    // In Each Mode, for Every Region:
    for (int rgn_idx = 0; rgn_idx < FISHEYE_CONFIG->RgnNum; rgn_idx++) {
        // check region valid first
        if (!FISHEYE_REGION[rgn_idx].RegionValid) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,"Region invalid.\n");
            return -1;
        }
        if (FISHEYE_REGION[rgn_idx].ViewMode == PROJECTION_STEREO_FIT) {
            char *grid = (char *)ptr;
            if (dwa_load_meshdata(grid, &g_MeshData[rgn_idx], pstGridInfoAttr->gridBindName)) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dwa_load_meshdata failed.\n");
                return -1;
            }
            if (bm_get_region_mesh_list(FISHEYE_REGION, rgn_idx, &g_MeshData[rgn_idx])) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "invalid param, FISHEYE_REGION is NULL.\n");
                return -1;
            }
            SAFE_FREE_POINTER(g_MeshData[rgn_idx].pgrid_src);
            SAFE_FREE_POINTER(g_MeshData[rgn_idx].pgrid_dst);
            SAFE_FREE_POINTER(g_MeshData[rgn_idx].pmesh_src);
            SAFE_FREE_POINTER(g_MeshData[rgn_idx].pmesh_dst);
            SAFE_FREE_POINTER(g_MeshData[rgn_idx].pnode_src);
            SAFE_FREE_POINTER(g_MeshData[rgn_idx].pnode_dst);
            //SAFE_FREE_POINTER(pMeshData->_pmapx);
            //SAFE_FREE_POINTER(pMeshData->_pmapy);
            g_MeshData[rgn_idx].balloc = false;
        } else {
            UNUSED(pstGridInfoAttr);
            // get & store region mesh info.
            bm_get_region_dst_mesh_list(FISHEYE_REGION, rgn_idx);

            // Get Source Mesh-Info Projected from Destination by Differet ViewModw.
            bm_get_region_src_mesh_list(FISHEYE_CONFIG->MntMode, FISHEYE_REGION, rgn_idx, x0, y0, r);
        }
    }
    //combine all region meshs - mesh projection done.
    bm_get_frame_mesh_list(FISHEYE_CONFIG, FISHEYE_REGION);

    mesh_tbl_num = FISHEYE_CONFIG->TotalMeshNum;
    float src_x_mesh_tbl[mesh_tbl_num][4];
    float src_y_mesh_tbl[mesh_tbl_num][4];
    float dst_x_mesh_tbl[mesh_tbl_num][4];
    float dst_y_mesh_tbl[mesh_tbl_num][4];
    for (int mesh_idx = 0; mesh_idx < FISHEYE_CONFIG->TotalMeshNum; mesh_idx++) {
        for (int knotidx = 0; knotidx < 4; knotidx++) {
            src_x_mesh_tbl[mesh_idx][knotidx] = FISHEYE_CONFIG->SrcRgnMeshInfo[mesh_idx].knot[knotidx].xcor;
            src_y_mesh_tbl[mesh_idx][knotidx] = FISHEYE_CONFIG->SrcRgnMeshInfo[mesh_idx].knot[knotidx].ycor;
            dst_x_mesh_tbl[mesh_idx][knotidx] = FISHEYE_CONFIG->DstRgnMeshInfo[mesh_idx].knot[knotidx].xcor;
            dst_y_mesh_tbl[mesh_idx][knotidx] = FISHEYE_CONFIG->DstRgnMeshInfo[mesh_idx].knot[knotidx].ycor;
        }
    }

    int dst_height, dst_width;
    if (FISHEYE_CONFIG->rotate_index == 1 || FISHEYE_CONFIG->rotate_index == 3) {
        dst_height = FISHEYE_CONFIG->OutW_disp;
        dst_width  = FISHEYE_CONFIG->OutH_disp;
    } else {
        dst_height = FISHEYE_CONFIG->OutH_disp;
        dst_width  = FISHEYE_CONFIG->OutW_disp;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_tbl_num = %d\n", mesh_tbl_num);      // for debug

    /////////////////////////////////////////////////////////////////////////////////////
    // mesh scan order preprocessing
    /////////////////////////////////////////////////////////////////////////////////////
    int Y_SUBTILE_NUMBER = ceil((float)dst_height / (float)NUMBER_Y_LINE_A_SUBTILE);
    int MAX_MESH_NUM_A_TILE = ((64 * 2) / X_TILE_NUMBER) * (1 + ceil((4*128)/(float)dst_height)); // (maximum horizontal meshes number x 2)/horizontal tiles number
    uint16_t mesh_scan_tile_mesh_id_list[MAX_MESH_NUM_A_TILE * Y_SUBTILE_NUMBER * X_TILE_NUMBER];

    int mesh_id_list_entry_num =
        mesh_scan_preproc_3(dst_width, dst_height,
                    dst_x_mesh_tbl, dst_y_mesh_tbl, mesh_tbl_num, mesh_scan_tile_mesh_id_list,
                    X_TILE_NUMBER, NUMBER_Y_LINE_A_SUBTILE,
                    Y_TILE_NUMBER, MAX_MESH_NUM_A_TILE);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_id_list_entry_num = %d\n", mesh_id_list_entry_num);      // for debug

    int isrc_x_mesh_tbl[mesh_tbl_num][4];
    int isrc_y_mesh_tbl[mesh_tbl_num][4];
    int idst_x_mesh_tbl[mesh_tbl_num][4];
    int idst_y_mesh_tbl[mesh_tbl_num][4];

    mesh_coordinate_float2fixed(src_x_mesh_tbl, src_y_mesh_tbl, dst_x_mesh_tbl, dst_y_mesh_tbl, mesh_tbl_num,
                    isrc_x_mesh_tbl, isrc_y_mesh_tbl, idst_x_mesh_tbl, idst_y_mesh_tbl);

    int reorder_mesh_tbl_entry_num, reorder_mesh_id_list_entry_num;

    mesh_tbl_reorder_and_parse_3(mesh_scan_tile_mesh_id_list, mesh_id_list_entry_num,
                        isrc_x_mesh_tbl, isrc_y_mesh_tbl, idst_x_mesh_tbl, idst_y_mesh_tbl,
                        X_TILE_NUMBER, Y_TILE_NUMBER, Y_SUBTILE_NUMBER, reorder_mesh_tbl,
                        &reorder_mesh_tbl_entry_num, reorder_mesh_id_list,
                        &reorder_mesh_id_list_entry_num, mesh_tbl_phy_addr, dst_width, dst_height);
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh table size (bytes) = %d\n", (reorder_mesh_tbl_entry_num * 4));
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh id list size (bytes) = %d\n", (reorder_mesh_id_list_entry_num * 2));

    clock_gettime(CLOCK_MONOTONIC, &end);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "time consumed: %fms\n",(FLOAT)get_diff_in_us(start, end) / 1000);     // for debug
    return 0;
}

void bm_get_region_src_mesh_list(FISHEYE_MOUNT_MODE_E MOUNT, BM_FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, double x0, double y0, double r)
{
    // ViewModeType to decide mapping & UI contrl parameters.
    PROJECTION_MODE ViewModeType = FISHEYE_REGION[rgn_idx].ViewMode;

    if (ViewModeType == PROJECTION_REGION)
        _RegionView2( FISHEYE_REGION, rgn_idx, MOUNT, x0, y0, r);
    else if (ViewModeType == PROJECTION_PANORAMA_360)
        _Panorama360View2( FISHEYE_REGION, rgn_idx, MOUNT, x0, y0, r);
    else if (ViewModeType == PROJECTION_PANORAMA_180)
        _Panorama180View2(FISHEYE_REGION, rgn_idx, MOUNT, x0, y0, r);
    else if (ViewModeType == PROJECTION_LDC)
        _LDC_View(FISHEYE_REGION, rgn_idx, x0, y0);
    else
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ERROR!!! THIS CASE SHOULDNOTHAPPEN!!!!!!\n\r");
}

void bm_get_region_dst_mesh_list(BM_FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx)
{
    if (FISHEYE_REGION[rgn_idx].RegionValid != 1)
        return;

    // Destination Mesh-Info Allocation
    int view_w = FISHEYE_REGION[rgn_idx].OutW;
    int view_h = FISHEYE_REGION[rgn_idx].OutH;
    int mesh_horcnt = FISHEYE_REGION[rgn_idx].MeshHor;
    int mesh_vercnt = FISHEYE_REGION[rgn_idx].MeshVer;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "view_w(%d) view_h(%d) mesh_horcnt(%d) mesh_vercnt(%d)\n", view_w, view_h, mesh_horcnt, mesh_vercnt);
    int knot_horcnt = mesh_horcnt + 1;
    int knot_vercnt = mesh_vercnt + 1;
    // tmp internal buffer
    // maximum knot number = 1024 (pre-set)
    COORDINATE2D *meshknot_hit_buf = malloc(sizeof(COORDINATE2D) * (knot_horcnt * knot_vercnt + 1));

    // 1st loop: to find mesh infos. on source ( backward projection )
    // hit index for buffer
    int knotcnt = 0;
    int mesh_w = (view_w / mesh_horcnt);
    int mesh_h = (view_h / mesh_vercnt);
    int half_w = view_w / 2;
    int half_h = view_h / 2;
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_w(%d) mesh_h(%d)\n", mesh_w, mesh_h);
#if 0
    for (int y = -half_h; y < half_h; ++y) {
        bool yknot_hit = ((y + half_h) % mesh_h) == 0;
        bool LastMeshFixY = ((y + half_h) == (mesh_h * mesh_vercnt)) && (view_h != mesh_h * mesh_vercnt);

        for (int x = -half_w; x < half_w; x++) {
            bool xknot_hit = ((x + half_w) % mesh_w) == 0;
            bool hitknot = ((xknot_hit && yknot_hit)
                    || (((x + 1) == (half_w)) && yknot_hit)
                    || (xknot_hit && ((y + 1) == (half_h)))
                    || (((x + 1) == (half_w)) && ((y + 1) == (half_h))));

            // LastMeshFix is to fix unequal mesh block counts.
            bool LastMeshFixX = ((x + half_w) == (mesh_w * mesh_horcnt)) && (view_w != mesh_w * mesh_horcnt);

            hitknot = hitknot && !(LastMeshFixX || LastMeshFixY);

            if (hitknot) {
                meshknot_hit_buf[knotcnt].xcor = FISHEYE_REGION[rgn_idx].OutX + (x + half_w);
                meshknot_hit_buf[knotcnt].ycor = FISHEYE_REGION[rgn_idx].OutY + (y + half_h);
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "%d(%lf %lf)\n", knotcnt, meshknot_hit_buf[knotcnt].xcor, meshknot_hit_buf[knotcnt].ycor);
                knotcnt += 1;
            }
        }
    }
#else
    int y = -half_h;
    for (int j = knot_horcnt; j > 0; --j) {
        int x = -half_w;
        for (int i = knot_horcnt; i > 0; --i) {
            meshknot_hit_buf[knotcnt].xcor = FISHEYE_REGION[rgn_idx].OutX + (x + half_w);
            meshknot_hit_buf[knotcnt].ycor = FISHEYE_REGION[rgn_idx].OutY + (y + half_h);
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "%d(%lf %lf)\n", knotcnt, meshknot_hit_buf[knotcnt].xcor, meshknot_hit_buf[knotcnt].ycor);
            knotcnt += 1;
            x += mesh_w;
            if (i == 2)
                x = half_w;
        }
        y += mesh_h;
        if (j == 2)
            y = half_h;
    }
#endif

    meshknot_hit_buf[knotcnt].xcor = 0xFFFFFFFF;    //End of Knot List.
    meshknot_hit_buf[knotcnt].ycor = 0xFFFFFFFF;    //End of Knot List
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "knotcnt(%d)\n", knotcnt);       // for debug
    // tmp for debug

    for (int j = 0; j < mesh_vercnt; j++) {
        for (int i = 0; i < mesh_horcnt; i++) {
            int meshidx = j * mesh_horcnt + i;
            int knotidx = j * (mesh_horcnt + 1) + i;    // knot num = mesh num +1 ( @ horizon )

            FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].xcor = meshknot_hit_buf[knotidx].xcor;
            FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].ycor = meshknot_hit_buf[knotidx].ycor;
            FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].xcor = meshknot_hit_buf[knotidx + 1].xcor;
            FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].ycor = meshknot_hit_buf[knotidx + 1].ycor;
            FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].xcor = meshknot_hit_buf[knotidx + (mesh_horcnt + 1)].xcor;
            FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].ycor = meshknot_hit_buf[knotidx + (mesh_horcnt + 1)].ycor;
            FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].xcor = meshknot_hit_buf[knotidx + 1 + (mesh_horcnt + 1)].xcor;
            FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].ycor = meshknot_hit_buf[knotidx + 1 + (mesh_horcnt + 1)].ycor;
        }
    }

    free(meshknot_hit_buf);
}

int bm_get_region_all_mesh_data_memory(BM_FISHEYE_REGION_ATTR *FISHEYE_REGION
    , int view_w, int view_h, int rgn, BM_MESH_DATA_ALL_S *pmeshinfo)
{
    if (!FISHEYE_REGION[rgn].RegionValid) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "region[%d] is not invalid\n", rgn);
        return -1;
    }
    // int mesh_horcnt, mesh_vercnt;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "points to grids, hw mesh\n");
    FISHEYE_REGION[rgn].MeshHor = pmeshinfo->mesh_horcnt;// grid_info[0];//mesh_horcnt;
    FISHEYE_REGION[rgn].MeshVer = pmeshinfo->mesh_vercnt;//mesh_vercnt;

    // mesh_horcnt = pmeshinfo->mesh_horcnt;
    // mesh_vercnt = pmeshinfo->mesh_vercnt;

    //int view_w = FISHEYE_REGION[rgn].OutW;
    //int view_h = FISHEYE_REGION[rgn].OutH;
    //int mesh_horcnt = FISHEYE_REGION[rgn].MeshHor;
    //int mesh_vercnt = FISHEYE_REGION[rgn].MeshVer;

    // register:
#if !WITHOUT_BIAS
#if !DPU_MODE
    bool bAspect = (bool)FISHEYE_REGION[rgn].ZoomV;
    int XYRatio = minmax(FISHEYE_REGION[rgn].Pan, 0, 100);
    int XRatio, YRatio;

    if (bAspect) {
        XRatio = YRatio = XYRatio;
    } else {
        XRatio = minmax(FISHEYE_REGION[rgn].ThetaX, 0, 100);
        YRatio = minmax(FISHEYE_REGION[rgn].ThetaZ, 0, 100);
    }

    int CenterXOffset = minmax(FISHEYE_REGION[rgn].InRadius, -511, 511);
    int CenterYOffset = minmax(FISHEYE_REGION[rgn].OutRadius, -511, 511);
    CenterXOffset = CenterYOffset = 0;
#endif
#endif
    //int DistortionRatio = minmax(FISHEYE_REGION[rgn].PanEnd, -300, 500);
    //double norm = sqrt((view_w / 2) * (view_w / 2) + (view_h / 2) * (view_h / 2));

    // 1st loop: to find mesh infos. on source ( backward projection )
    // hit index for buffer

    // for debug
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "view_h = %d src,\n", view_h);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "view_w = %d,\n", view_w);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_horcnt = %d,\n", mesh_horcnt);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_vercnt = %d,\n", mesh_vercnt);
    int meshidx = pmeshinfo->num_pairs;
    //int offsetX = 50 * 2;
    //float scale_value = 1.0;

    for (int i = 0; i < meshidx; i++) {
        FISHEYE_REGION[rgn].SrcRgnMeshInfo[i].knot[0].xcor = pmeshinfo->pmesh_src[8 * i];
        FISHEYE_REGION[rgn].SrcRgnMeshInfo[i].knot[0].ycor = pmeshinfo->pmesh_src[8 * i + 1];
        FISHEYE_REGION[rgn].SrcRgnMeshInfo[i].knot[1].xcor = pmeshinfo->pmesh_src[8 * i + 2];
        FISHEYE_REGION[rgn].SrcRgnMeshInfo[i].knot[1].ycor = pmeshinfo->pmesh_src[8 * i + 3];
        FISHEYE_REGION[rgn].SrcRgnMeshInfo[i].knot[2].xcor = pmeshinfo->pmesh_src[8 * i + 4];
        FISHEYE_REGION[rgn].SrcRgnMeshInfo[i].knot[2].ycor = pmeshinfo->pmesh_src[8 * i + 5];
        FISHEYE_REGION[rgn].SrcRgnMeshInfo[i].knot[3].xcor = pmeshinfo->pmesh_src[8 * i + 6];
        FISHEYE_REGION[rgn].SrcRgnMeshInfo[i].knot[3].ycor = pmeshinfo->pmesh_src[8 * i + 7];
#if WITHOUT_BIAS
        FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[0].xcor = pmeshinfo->pmesh_dst[8 * i];
        FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[0].ycor = pmeshinfo->pmesh_dst[8 * i + 1];
        FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[1].xcor = pmeshinfo->pmesh_dst[8 * i + 2];
        FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[1].ycor = pmeshinfo->pmesh_dst[8 * i + 3];
        FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[2].xcor = pmeshinfo->pmesh_dst[8 * i + 4];
        FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[2].ycor = pmeshinfo->pmesh_dst[8 * i + 5];
        FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[3].xcor = pmeshinfo->pmesh_dst[8 * i + 6];
        FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[3].ycor = pmeshinfo->pmesh_dst[8 * i + 7];
#else
        int x, y;

        for (int j = 0; j < 4; j++) {
            x = pmeshinfo->pmesh_dst[2 * j];
            y = pmeshinfo->pmesh_dst[2 * j + 1];
#if !DPU_MODE
            x = x - CenterXOffset;
            y = y - CenterYOffset;

            if (bAspect) {
                x = x * (1 - 0.333 * (50 - XYRatio) / 100);
                y = y * (1 - 0.333 * (50 - XYRatio) / 100);
            } else {
                x = x * (1 - 0.333 * (50 - XRatio) / 100);
                y = y * (1 - 0.333 * (50 - YRatio) / 100);
            }
#endif
            FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[j].xcor = x;
            FISHEYE_REGION[rgn].DstRgnMeshInfo[i].knot[j].ycor = y;
        }
#endif
    }

    // update Node structure
    // Construct node tied to each other.
    int node_index = pmeshinfo->node_index;
    for (int i = 0; i < node_index; i++) {
        FISHEYE_REGION[rgn].SrcRgnNodeInfo[i].node.xcor = pmeshinfo->pnode_src[2 * i];
        FISHEYE_REGION[rgn].SrcRgnNodeInfo[i].node.ycor = pmeshinfo->pnode_src[2 * i + 1];
        FISHEYE_REGION[rgn].SrcRgnNodeInfo[i].node.xbias = 0;
        FISHEYE_REGION[rgn].SrcRgnNodeInfo[i].node.ybias = 0;
        FISHEYE_REGION[rgn].SrcRgnNodeInfo[i].valid = true;

        FISHEYE_REGION[rgn].DstRgnNodeInfo[i].node.xbias = 0;
        FISHEYE_REGION[rgn].DstRgnNodeInfo[i].node.ybias = 0;
        FISHEYE_REGION[rgn].DstRgnNodeInfo[i].valid = true;
#if WITHOUT_BIAS
        FISHEYE_REGION[rgn].DstRgnNodeInfo[i].node.xcor = pmeshinfo->pnode_dst[2 * i];
        FISHEYE_REGION[rgn].DstRgnNodeInfo[i].node.ycor = pmeshinfo->pnode_dst[2 * i + 1];
#else
        int x = pmeshinfo->pnode_dst[2 * i];
        int y = pmeshinfo->pnode_dst[2 * i + 1];
#if !DPU_MODE
        x = x - CenterXOffset;
        y = y - CenterYOffset;

        if (bAspect) {
            x = x * (1 - 0.333 * (50 - XYRatio) / 100);
            y = y * (1 - 0.333 * (50 - XYRatio) / 100);
        } else {
            x = x * (1 - 0.333 * (50 - XRatio) / 100);
            y = y * (1 - 0.333 * (50 - YRatio) / 100);
        }
#endif

        FISHEYE_REGION[rgn].DstRgnNodeInfo[i].node.xcor = x;
        FISHEYE_REGION[rgn].DstRgnNodeInfo[i].node.ycor = y;
#endif
    }

    //update node structure to mesh => all meshes are surrounded by points
    bm_dwa_node_to_mesh(FISHEYE_REGION, rgn);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "PASS load mesh!!!!\n\r");    // for debug

    return 0;
}

int bm_get_region_mesh_list(BM_FISHEYE_REGION_ATTR *FISHEYE_REGION, int rgn_idx, BM_MESH_DATA_ALL_S *pMeshData)
{
    if (!FISHEYE_REGION) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "invalid param, FISHEYE_REGION is NULL.\n");
        return -1;
    }
    if (!pMeshData) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "invalid param, pMeshData is NULL.\n");
        return -2;
    }
    int tw = pMeshData->imgw;
    int th = pMeshData->imgh;
    int max_mesh_cnt = floor(tw / 16) * floor(th / 16);

    if (pMeshData->balloc) {
        if (pMeshData->_nbr_mesh_x * pMeshData->_nbr_mesh_y > max_mesh_cnt) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "mesh_x, mesh_y is too large.\n");
            return -1;
        }
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "g_MeshData is not alloc.\n");
        return -1;
    }

    // load output region from meshdata
    if (bm_load_region_config(FISHEYE_REGION, rgn_idx, pMeshData)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "load region cfg fail from meshdata.\n");
        return -1;
    }

    if (bm_get_region_all_mesh_data_memory(FISHEYE_REGION, tw, th, 0, pMeshData)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "get region meshdata fail.\n");
        return -1;
    }

    return 0;
}

void bm_dwa_node_to_mesh(BM_FISHEYE_REGION_ATTR *FISHEYE_REGION, int rgn)
{
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "node_to_mesh\n");
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "rgn(%d)(%d)(%d)\n",rgn,FISHEYE_REGION[0].MeshHor,FISHEYE_REGION[0].MeshVer);
    int maxWMeshNum  = FISHEYE_REGION[0].MeshHor;
    int maxHMeshNum = FISHEYE_REGION[0].MeshVer;
    //Node done, load to Mesh
    for (int mesh_yidx = 0; mesh_yidx < maxHMeshNum; mesh_yidx++) {
        for (int mesh_xidx = 0; mesh_xidx < maxWMeshNum; mesh_xidx++) {
            // destination node to mesh
            FISHEYE_REGION[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[0].xcor =
                FISHEYE_REGION[rgn].DstRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx].node.xcor;
            FISHEYE_REGION[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[0].ycor =
                FISHEYE_REGION[rgn].DstRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx].node.ycor;

            FISHEYE_REGION[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[1].xcor =
                FISHEYE_REGION[rgn].DstRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx + 1].node.xcor;
            FISHEYE_REGION[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[1].ycor =
                FISHEYE_REGION[rgn].DstRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx + 1].node.ycor;

            FISHEYE_REGION[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[3].xcor =
                FISHEYE_REGION[rgn].DstRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx + 1].node.xcor;
            FISHEYE_REGION[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[3].ycor =
                FISHEYE_REGION[rgn].DstRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx + 1].node.ycor;

            FISHEYE_REGION[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[2].xcor =
                FISHEYE_REGION[rgn].DstRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx].node.xcor;
            FISHEYE_REGION[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[2].ycor =
                FISHEYE_REGION[rgn].DstRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx].node.ycor;

            // source node to mesh
            FISHEYE_REGION[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[0].xcor =
                FISHEYE_REGION[rgn].SrcRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx].node.xcor;
            FISHEYE_REGION[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[0].ycor =
                FISHEYE_REGION[rgn].SrcRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx].node.ycor;

            FISHEYE_REGION[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[1].xcor =
                FISHEYE_REGION[rgn].SrcRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx + 1].node.xcor;
            FISHEYE_REGION[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[1].ycor =
                FISHEYE_REGION[rgn].SrcRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx + 1].node.ycor;

            FISHEYE_REGION[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[3].xcor =
                FISHEYE_REGION[rgn].SrcRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx + 1].node.xcor;
            FISHEYE_REGION[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[3].ycor =
                FISHEYE_REGION[rgn].SrcRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx + 1].node.ycor;

            FISHEYE_REGION[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[2].xcor =
                FISHEYE_REGION[rgn].SrcRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx].node.xcor;
            FISHEYE_REGION[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[2].ycor =
                FISHEYE_REGION[rgn].SrcRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx].node.ycor;
        }
    }
}

int bm_load_region_config(BM_FISHEYE_REGION_ATTR *FISHEYE_REGION, int rgn, BM_MESH_DATA_ALL_S *meshd)
{
    if (meshd == NULL || !FISHEYE_REGION) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "meshdata  or FISHEYE_REGION is NULL.\n");
        return -1;
    }
    FISHEYE_REGION[rgn].RegionValid = true;
    //FISHEYE_REGION[rgn].ViewMode = PROJECTION_STEREO_FIT;

    // update region info from memory
    int horcnt = meshd->mesh_horcnt;
    int vercnt = meshd->mesh_vercnt;
    int meshw = meshd->mesh_w;
    int meshh = meshd->mesh_h;
    int rx = meshd->unit_rx;
    int ry = meshd->unit_ry;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "horcnt, vercnt, meshw, meshh, rx, ry: %d %d %d %d %d %d\n"
        , horcnt, vercnt, meshw, meshh, rx, ry);
    FISHEYE_REGION[rgn].MeshVer = vercnt;
    FISHEYE_REGION[rgn].MeshHor = horcnt;
    FISHEYE_REGION[rgn].OutW = meshw * horcnt;  // FISHEYE_CONFIG->OutW_disp; // Note: !! It's roi width
    FISHEYE_REGION[rgn].OutH = meshh * vercnt;  // FISHEYE_CONFIG->OutH_disp;
    FISHEYE_REGION[rgn].OutX = meshw * rx;      // (width_sec * 1);
    FISHEYE_REGION[rgn].OutY = meshh * ry;      // (height_sec * 1);

    return 0;
}

static bm_status_t bm_dwa_rotation_check_size(ROTATION_E enRotation, const GDC_TASK_ATTR_S *pstTask)
{
    if (enRotation >= ROTATION_MAX) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "invalid rotation(%d).\n", enRotation);
        return BM_ERR_FAILURE;
    }

    if (enRotation == ROTATION_90 || enRotation == ROTATION_270 || enRotation == ROTATION_XY_FLIP) {
        if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Height) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "rotation(%d) invalid: 'output width(%d) < input height(%d)'\n",
                        enRotation, pstTask->stImgOut.stVFrame.u32Width,
                        pstTask->stImgIn.stVFrame.u32Height);
            return BM_ERR_FAILURE;
        }
        if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Width) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "rotation(%d) invalid: 'output height(%d) < input width(%d)'\n",
                        enRotation, pstTask->stImgOut.stVFrame.u32Height,
                        pstTask->stImgIn.stVFrame.u32Width);
            return BM_ERR_FAILURE;
        }
    } else {
        if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Width) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "rotation(%d) invalid: 'output width(%d) < input width(%d)'\n",
                        enRotation, pstTask->stImgOut.stVFrame.u32Width,
                        pstTask->stImgIn.stVFrame.u32Width);
            return BM_ERR_FAILURE;
        }
        if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Height) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "rotation(%d) invalid: 'output height(%d) < input height(%d)'\n",
                        enRotation, pstTask->stImgOut.stVFrame.u32Height,
                        pstTask->stImgIn.stVFrame.u32Height);
            return BM_ERR_FAILURE;
        }
    }

    return BM_SUCCESS;
}

static bm_status_t bm_get_valid_tsk_mesh_by_name(bm_handle_t handle, const CHAR *name, u8 *idx)
{
    u8 i = DWA_MAX_TSK_MESH;

    if (!name) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "tsk mesh name is null\n");
        *idx = i;
        return BM_ERR_FAILURE;
    }
    for (i = 0; i < DWA_MAX_TSK_MESH; i++) {
        if (strcmp(dwa_tskMesh[i].Name, name) == 0 /*&& dwa_tskMesh[i].paddr*/) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "got remain tsk mesh[%d-%s-%llx]\n", i, dwa_tskMesh[i].Name, dwa_tskMesh[i].paddr);       // for debug
            *idx = i;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "idx = (%d)\n", *idx);       // for debug
            return BM_SUCCESS;
        }
    }
    *idx = get_idle_tsk_mesh(handle);
    if (*idx >= DWA_MAX_TSK_MESH) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "tsk mesh count(%d) is out of range(%d)\n", *idx + 1, DWA_MAX_TSK_MESH);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "idx = (%d)\n", *idx);
        return BM_ERR_FAILURE;
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "start alloc new tsk mesh-[%d]-[%s]\n", *idx, name);       // for debug
        strcpy(dwa_tskMesh[*idx].Name, name);
        return BM_ERR_FAILURE;
    }
}

static void bm_dwa_mesh_gen_rotation(SIZE_S in_size, SIZE_S out_size, ROTATION_E rot
    , uint64_t mesh_phy_addr, void *mesh_vir_addr)
{
    BM_FISHEYE_ATTR *FISHEYE_CONFIG;
    LDC_ATTR_S stLDCAttr = { .bAspect = true, .s32XYRatio = 100,
        .s32CenterXOffset = 0, .s32CenterYOffset = 0, .s32DistortionRatio = 0 };
    BM_FISHEYE_REGION_ATTR *FISHEYE_REGION;
    int nMeshhor, nMeshVer;

    FISHEYE_CONFIG = (BM_FISHEYE_ATTR*)calloc(1, sizeof(*FISHEYE_CONFIG));
    if (!FISHEYE_CONFIG) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye config\n");
        free(FISHEYE_CONFIG);
        return;
    }
    FISHEYE_REGION = (BM_FISHEYE_REGION_ATTR*)calloc(1, sizeof(*FISHEYE_REGION) * MAX_REGION_NUM);
    if (!FISHEYE_REGION) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye region config\n");
        free(FISHEYE_CONFIG);
        free(FISHEYE_REGION);
        return;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "in_size(%d %d) out_size(%d %d)\n"
        , in_size.u32Width, in_size.u32Height
        , out_size.u32Width, out_size.u32Height);

    bm_ldc_attr_map(&stLDCAttr, in_size, FISHEYE_CONFIG, FISHEYE_REGION);
    dwa_get_mesh_size(&nMeshhor, &nMeshVer);
    FISHEYE_REGION[0].MeshHor = nMeshhor;
    FISHEYE_REGION[0].MeshVer = nMeshVer;

    FISHEYE_CONFIG->rotate_index = rot;

    int X_TILE_NUMBER, Y_TILE_NUMBER;
    u32 mesh_id_size, mesh_tbl_size;
    u64 mesh_id_phy_addr, mesh_tbl_phy_addr;

    X_TILE_NUMBER = DIV_UP(in_size.u32Width, DWA_MESH_SLICE_BASE_W);
    Y_TILE_NUMBER = DIV_UP(in_size.u32Height, DWA_MESH_SLICE_BASE_H);

    // calculate mesh_id/mesh_tbl's size in bytes.
    mesh_tbl_size = 0x20000;
    mesh_id_size = 0x40000;

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

    // Provide virtual address to write mesh.
    reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
    generate_mesh_on_fisheye(NULL, FISHEYE_CONFIG, FISHEYE_REGION, X_TILE_NUMBER, Y_TILE_NUMBER
        , (uint16_t *)mesh_vir_addr
        , reorder_mesh_tbl, mesh_tbl_phy_addr, (void *)NULL);

    free(FISHEYE_CONFIG);
    free(FISHEYE_REGION);
}

static void bm_fisheye_attr_map(const FISHEYE_ATTR_S *pstFisheyeAttr, SIZE_S out_size, BM_FISHEYE_ATTR *FISHEYE_CONFIG, BM_FISHEYE_REGION_ATTR *FISHEYE_REGION)
{
    FISHEYE_CONFIG->MntMode = pstFisheyeAttr->enMountMode;
    FISHEYE_CONFIG->OutW_disp = out_size.u32Width;
    FISHEYE_CONFIG->OutH_disp = out_size.u32Height;
    FISHEYE_CONFIG->InCenterX = pstFisheyeAttr->s32HorOffset;
    FISHEYE_CONFIG->InCenterY = pstFisheyeAttr->s32VerOffset;
    // TODO: how to handl radius
    FISHEYE_CONFIG->InRadius = MIN2(FISHEYE_CONFIG->InCenterX, FISHEYE_CONFIG->InCenterY);
    FISHEYE_CONFIG->FStrength = pstFisheyeAttr->s32FanStrength;
    FISHEYE_CONFIG->TCoef  = pstFisheyeAttr->u32TrapezoidCoef;

    FISHEYE_CONFIG->RgnNum = pstFisheyeAttr->u32RegionNum;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "OutW_disp(%d) OutH_disp(%d)\n", FISHEYE_CONFIG->OutW_disp, FISHEYE_CONFIG->OutH_disp);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "InCenterX(%d) InCenterY(%d)\n", FISHEYE_CONFIG->InCenterX, FISHEYE_CONFIG->InCenterY);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "FStrength(%lf) TCoef(%lf) RgnNum(%d)\n", FISHEYE_CONFIG->FStrength, FISHEYE_CONFIG->TCoef, FISHEYE_CONFIG->RgnNum);

    for (int i = 0; i < MAX_REGION_NUM; ++i)
        FISHEYE_REGION[i].RegionValid = 0;

    for (int i = 0; i < FISHEYE_CONFIG->RgnNum; ++i) {
        FISHEYE_REGION[i].RegionValid = 1;

        FISHEYE_REGION[i].ZoomH = pstFisheyeAttr->astFishEyeRegionAttr[i].u32HorZoom;
        FISHEYE_REGION[i].ZoomV = pstFisheyeAttr->astFishEyeRegionAttr[i].u32VerZoom;
        FISHEYE_REGION[i].Pan = pstFisheyeAttr->astFishEyeRegionAttr[i].u32Pan;
        FISHEYE_REGION[i].Tilt = pstFisheyeAttr->astFishEyeRegionAttr[i].u32Tilt;
        FISHEYE_REGION[i].OutW = pstFisheyeAttr->astFishEyeRegionAttr[i].stOutRect.u32Width;
        FISHEYE_REGION[i].OutH = pstFisheyeAttr->astFishEyeRegionAttr[i].stOutRect.u32Height;
        FISHEYE_REGION[i].OutX = pstFisheyeAttr->astFishEyeRegionAttr[i].stOutRect.s32X;
        FISHEYE_REGION[i].OutY = pstFisheyeAttr->astFishEyeRegionAttr[i].stOutRect.s32Y;
        FISHEYE_REGION[i].InRadius = pstFisheyeAttr->astFishEyeRegionAttr[i].u32InRadius;
        FISHEYE_REGION[i].OutRadius = pstFisheyeAttr->astFishEyeRegionAttr[i].u32OutRadius;
        if (pstFisheyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_NORMAL) {
            FISHEYE_REGION[i].MeshVer = 16;
            FISHEYE_REGION[i].MeshHor = 16;
            FISHEYE_REGION[i].ViewMode = PROJECTION_REGION;
            FISHEYE_REGION[i].ThetaX = 0.4*M_PI;
            FISHEYE_REGION[i].ThetaZ = 0;
            FISHEYE_REGION[i].ThetaY = 0;
        } else if (pstFisheyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA) {
            FISHEYE_REGION[i].MeshVer = 32;
            FISHEYE_REGION[i].MeshHor = 32;
            FISHEYE_REGION[i].ViewMode = PROJECTION_PANORAMA_180;
        } else if (pstFisheyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_360_PANORAMA) {
            FISHEYE_REGION[i].MeshVer = (FISHEYE_CONFIG->RgnNum == 1) ? 64 : 32;
            FISHEYE_REGION[i].MeshHor = (FISHEYE_CONFIG->RgnNum == 1) ? 64 : 32;
            FISHEYE_REGION[i].ViewMode = PROJECTION_PANORAMA_360;
            FISHEYE_REGION[i].ThetaX = M_PI / 4;
            FISHEYE_REGION[i].ThetaZ = 0;
            FISHEYE_REGION[i].ThetaY = 0;
            FISHEYE_REGION[i].PanEnd = FISHEYE_REGION[i].Pan
                + 360 * pstFisheyeAttr->astFishEyeRegionAttr[i].u32HorZoom / 4096;
        }
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "Region(%d) ViewMode(%d) MeshVer(%d) MeshHor(%d)\n"
            , i, FISHEYE_REGION[i].ViewMode, FISHEYE_REGION[i].MeshVer, FISHEYE_REGION[i].MeshHor);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "ZoomH(%lf) ZoomV(%lf) Pan(%d) Tilt(%d) PanEnd(%d)\n"
            , FISHEYE_REGION[i].ZoomH, FISHEYE_REGION[i].ZoomV
            , FISHEYE_REGION[i].Pan, FISHEYE_REGION[i].Tilt, FISHEYE_REGION[i].PanEnd);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "InRadius(%lf) OutRadius(%lf) Rect(%d %d %d %d)\n"
            , FISHEYE_REGION[i].InRadius, FISHEYE_REGION[i].OutRadius
            , FISHEYE_REGION[i].OutX, FISHEYE_REGION[i].OutY
            , FISHEYE_REGION[i].OutW, FISHEYE_REGION[i].OutH);
    }
}

static bm_status_t dwa_mesh_gen_fisheye(SIZE_S in_size, SIZE_S out_size, const FISHEYE_ATTR_S *pstFisheyeAttr
    , uint64_t mesh_phy_addr, void *mesh_vir_addr, ROTATION_E rot, char *grid)
{
    BM_FISHEYE_ATTR *FISHEYE_CONFIG;
    BM_FISHEYE_REGION_ATTR *FISHEYE_REGION;
    bmcv_usage_mode UseMode;

    FISHEYE_CONFIG = (BM_FISHEYE_ATTR *)calloc(1, sizeof(*FISHEYE_CONFIG));
    if (!FISHEYE_CONFIG) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye config\n");
        free(FISHEYE_CONFIG);
        return BM_ERR_NOMEM;
    }
    FISHEYE_REGION = (BM_FISHEYE_REGION_ATTR *)calloc(1, sizeof(*FISHEYE_REGION) * MAX_REGION_NUM);
    if (!FISHEYE_REGION) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye region config\n");
        free(FISHEYE_CONFIG);
        free(FISHEYE_REGION);
        return BM_ERR_NOMEM;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "in_size(%d %d) out_size(%d %d)\n"
        , in_size.u32Width, in_size.u32Height
        , out_size.u32Width, out_size.u32Height);

    if (pstFisheyeAttr->stGridInfoAttr.Enable)
    {
        UseMode = MODE_STEREO_FIT;
    } else {
        UseMode = pstFisheyeAttr->enUseMode;
    }

    if (UseMode > 0) {
        double x0, y0, r;   // infos of src_img, (x0,y0) = center of image,  r = radius of image.
        x0 = pstFisheyeAttr->s32HorOffset;
        y0 = pstFisheyeAttr->s32VerOffset;
        r = MIN2(x0, y0);

        FISHEYE_CONFIG->Enable = true;
        FISHEYE_CONFIG->BgEnable = true;
        FISHEYE_CONFIG->MntMode = pstFisheyeAttr->enMountMode;
        FISHEYE_CONFIG->UsageMode = UseMode;
        FISHEYE_CONFIG->OutW_disp = out_size.u32Width;
        FISHEYE_CONFIG->OutH_disp = out_size.u32Height;
        FISHEYE_CONFIG->BgColor.R = 0;
        FISHEYE_CONFIG->BgColor.G = 0;
        FISHEYE_CONFIG->BgColor.B = 0;
        FISHEYE_CONFIG->InCenterX = x0; // front-end set.
        FISHEYE_CONFIG->InCenterY = y0; // front-end set.
        FISHEYE_CONFIG->InRadius = r;   // front-end set.
        FISHEYE_CONFIG->FStrength = pstFisheyeAttr->s32FanStrength;

        BM_LOAD_FRAME_CONFIG(FISHEYE_CONFIG);
        BM_LOAD_REGION_CONFIG(FISHEYE_CONFIG, FISHEYE_REGION);
    } else
        bm_fisheye_attr_map(pstFisheyeAttr, out_size, FISHEYE_CONFIG, FISHEYE_REGION);

    FISHEYE_CONFIG->rotate_index = rot;

    int X_TILE_NUMBER, Y_TILE_NUMBER;
    CVI_U32 mesh_id_size, mesh_tbl_size;
    CVI_U64 mesh_id_phy_addr, mesh_tbl_phy_addr;

    X_TILE_NUMBER = DIV_UP(in_size.u32Width, DWA_MESH_SLICE_BASE_W);
    Y_TILE_NUMBER = DIV_UP(in_size.u32Height, DWA_MESH_SLICE_BASE_H);

    // calculate mesh_id/mesh_tbl's size in bytes.
    mesh_tbl_size = 0x60000;
    mesh_id_size = 0x30000;

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

    // Provide virtual address to write mesh.
    reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
    if (pstFisheyeAttr->stGridInfoAttr.Enable == true)
        generate_mesh_on_fisheye(&(pstFisheyeAttr->stGridInfoAttr), FISHEYE_CONFIG, FISHEYE_REGION, X_TILE_NUMBER, Y_TILE_NUMBER
        , (uint16_t *)mesh_vir_addr
        , reorder_mesh_tbl, mesh_tbl_phy_addr, grid);
    else
        generate_mesh_on_fisheye(NULL, FISHEYE_CONFIG, FISHEYE_REGION, X_TILE_NUMBER, Y_TILE_NUMBER
        , (uint16_t *)mesh_vir_addr
        , reorder_mesh_tbl, mesh_tbl_phy_addr, (void *)NULL);
    free(FISHEYE_CONFIG);
    free(FISHEYE_REGION);

    return BM_SUCCESS;
}

static void generate_mesh_on_faces(struct mesh_param *param, const AFFINE_ATTR_S *pstAttr)
{
    const u8 w_knot_num = param->width/pstAttr->stDestSize.u32Width + 1;
    const u8 h_knot_num = param->height/pstAttr->stDestSize.u32Height + 1;
    // Limit slice's width/height to avoid unnecessary DRAM write(by bg-color)
    u16 width_slice = 0, height_slice = 0;
    u16 i, j, x, y, knot_idx;
    POINT_S dst_knot_tbl[w_knot_num * h_knot_num];
    POINT_S dst_mesh_tbl[param->mesh_num][4];
    u16 tbl_idx = 0;
    u32 *mesh_tbl = param->mesh_tbl_addr;

    // generate node
    for (j = 0; j < h_knot_num; ++j) {
        y = j * pstAttr->stDestSize.u32Height;
        for (i = 0; i < w_knot_num; ++i) {
            knot_idx = j * w_knot_num + i;
            x = i * pstAttr->stDestSize.u32Width;
            dst_knot_tbl[knot_idx].s32X = x;
            dst_knot_tbl[knot_idx].s32Y = y;
        }
    }

    // map node to each mesh
    for (i = 0; i < param->mesh_num; ++i) {
        knot_idx = i + (i / (param->width/pstAttr->stDestSize.u32Width));    // there is 1 more node than mesh each row

        dst_mesh_tbl[i][0] =  dst_knot_tbl[knot_idx];
        dst_mesh_tbl[i][1] =  dst_knot_tbl[knot_idx + 1];
        dst_mesh_tbl[i][2] =  dst_knot_tbl[knot_idx + w_knot_num];
        dst_mesh_tbl[i][3] =  dst_knot_tbl[knot_idx + w_knot_num + 1];

        if (dst_mesh_tbl[i][1].s32X > width_slice) width_slice = dst_mesh_tbl[i][1].s32X;
        if (dst_mesh_tbl[i][2].s32Y > height_slice) height_slice = dst_mesh_tbl[i][2].s32Y;

        for (j = 0; j < 4; ++j) {
            mesh_tbl[tbl_idx++] = pstAttr->astRegionAttr[i][j].x * (double)(1 << DEWARP_COORD_NBITS);
            mesh_tbl[tbl_idx++] = pstAttr->astRegionAttr[i][j].y * (double)(1 << DEWARP_COORD_NBITS);
            mesh_tbl[tbl_idx++] = dst_mesh_tbl[i][j].s32X * (double)(1 << DEWARP_COORD_NBITS);
            mesh_tbl[tbl_idx++] = dst_mesh_tbl[i][j].s32Y * (double)(1 << DEWARP_COORD_NBITS);
        }
    }

    param->width = width_slice;
    param->height = height_slice;
    bm_generate_mesh_id(param, dst_mesh_tbl);
}

static void dwa_mesh_gen_affine(SIZE_S in_size, SIZE_S out_size, const AFFINE_ATTR_S *pstAffineAttr
    , uint64_t mesh_phy_addr, void *mesh_vir_addr)
{
    const int MAX_MESH_NUM_A_TILE = 4;
    int X_TILE_NUMBER, Y_TILE_NUMBER;
    int owidth, oheight;
    int Y_SUBTILE_NUMBER;
    u32 mesh_id_size, mesh_tbl_size;
    u64 mesh_id_phy_addr, mesh_tbl_phy_addr;

    owidth = out_size.u32Width;
    oheight = out_size.u32Height;
    X_TILE_NUMBER = (DIV_UP(in_size.u32Width, 1024) > 1) ? DIV_UP(in_size.u32Width, 1024) << 1
        : 2;
    Y_TILE_NUMBER = (DIV_UP(in_size.u32Height, 1024) > 1) ? DIV_UP(in_size.u32Height, 1024) << 1
        : 2;

    // calculate mesh_id/mesh_tbl's size in bytes.
    Y_SUBTILE_NUMBER = ceil((float)out_size.u32Height / (float)NUMBER_Y_LINE_A_SUBTILE);
    mesh_tbl_size = X_TILE_NUMBER * Y_TILE_NUMBER * 16 * sizeof(u32);
    mesh_id_size = MAX_MESH_NUM_A_TILE * Y_SUBTILE_NUMBER * X_TILE_NUMBER * sizeof(u16);

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    struct mesh_param param;

    param.width = owidth;
    param.height = oheight;
    param.mesh_id_addr = mesh_vir_addr;
    param.mesh_tbl_addr = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);;
    param.mesh_tbl_phy_addr = mesh_tbl_phy_addr;
    param.slice_num_w = 1;
    param.slice_num_h = 1;
    param.mesh_num = pstAffineAttr->u32RegionNum;
    generate_mesh_on_faces(&param, pstAffineAttr);
}

static bm_status_t dwa_mesh_gen_ldc(SIZE_S in_size, SIZE_S out_size, const LDC_ATTR_S *pstLDCAttr,
            uint64_t mesh_phy_addr, void *mesh_vir_addr, ROTATION_E rot, char *grid)
{
    BM_FISHEYE_ATTR *FISHEYE_CONFIG;
    BM_FISHEYE_REGION_ATTR *FISHEYE_REGION;

    FISHEYE_CONFIG = (BM_FISHEYE_ATTR*)calloc(1, sizeof(*FISHEYE_CONFIG));
    if (!FISHEYE_CONFIG) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye config\n");
        free(FISHEYE_CONFIG);
        return BM_ERR_FAILURE;
    }
    FISHEYE_REGION = (BM_FISHEYE_REGION_ATTR*)calloc(1, sizeof(*FISHEYE_REGION) * MAX_REGION_NUM);
    if (!FISHEYE_REGION) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye region config\n");
        free(FISHEYE_CONFIG);
        free(FISHEYE_REGION);
        return BM_ERR_FAILURE;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "in_size(%d %d) out_size(%d %d)\n"
        , in_size.u32Width, in_size.u32Height
        , out_size.u32Width, out_size.u32Height);

    bm_ldc_attr_map(pstLDCAttr, out_size, FISHEYE_CONFIG, FISHEYE_REGION);
    FISHEYE_CONFIG->rotate_index = rot;

    int X_TILE_NUMBER, Y_TILE_NUMBER;
    u32 mesh_id_size;
    u32 mesh_tbl_size;
    u64 mesh_id_phy_addr, mesh_tbl_phy_addr;

    X_TILE_NUMBER = DIV_UP(in_size.u32Width, DWA_MESH_SLICE_BASE_W);
    Y_TILE_NUMBER = DIV_UP(in_size.u32Height, DWA_MESH_SLICE_BASE_H);

    // calculate mesh_id/mesh_tbl's size in bytes.
    mesh_tbl_size = 0x60000;
    mesh_id_size = 0x50000;

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

    // Provide virtual address to write mesh.
    reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
    if (pstLDCAttr->stGridInfoAttr.Enable == true)
        generate_mesh_on_fisheye(&(pstLDCAttr->stGridInfoAttr), FISHEYE_CONFIG, FISHEYE_REGION, X_TILE_NUMBER, Y_TILE_NUMBER
            , (uint16_t *)mesh_vir_addr, reorder_mesh_tbl, mesh_tbl_phy_addr, grid);
    else
        generate_mesh_on_fisheye(&(pstLDCAttr->stGridInfoAttr), FISHEYE_CONFIG, FISHEYE_REGION, X_TILE_NUMBER, Y_TILE_NUMBER
            , (uint16_t *)mesh_vir_addr, reorder_mesh_tbl, mesh_tbl_phy_addr, (void *)NULL);

    free(FISHEYE_CONFIG);
    free(FISHEYE_REGION);

    return BM_SUCCESS;
}

static bm_status_t dwa_mesh_gen_warp(SIZE_S in_size, SIZE_S out_size, const WARP_ATTR_S *pstWarpAttr,
            uint64_t mesh_phy_addr, void *mesh_vir_addr, char *grid)
{
    BM_FISHEYE_ATTR *FISHEYE_CONFIG;
    BM_FISHEYE_REGION_ATTR *FISHEYE_REGION;

    FISHEYE_CONFIG = (BM_FISHEYE_ATTR*)calloc(1, sizeof(*FISHEYE_CONFIG));
    if (!FISHEYE_CONFIG) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye config\n");
        free(FISHEYE_CONFIG);
        return BM_ERR_FAILURE;
    }
    FISHEYE_REGION = (BM_FISHEYE_REGION_ATTR*)calloc(1, sizeof(*FISHEYE_REGION) * MAX_REGION_NUM);
    if (!FISHEYE_REGION) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye region config\n");
        free(FISHEYE_CONFIG);
        free(FISHEYE_REGION);
        return BM_ERR_FAILURE;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "in_size(%d %d) out_size(%d %d)\n"
        , in_size.u32Width, in_size.u32Height
        , out_size.u32Width, out_size.u32Height);

    FISHEYE_CONFIG->MntMode = 0;
    FISHEYE_CONFIG->OutW_disp = out_size.u32Width;
    FISHEYE_CONFIG->OutH_disp = out_size.u32Height;
    FISHEYE_CONFIG->FStrength = 0;
    FISHEYE_CONFIG->TCoef = 0;

    FISHEYE_CONFIG->RgnNum = 1;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "OutW_disp(%d) OutH_disp(%d)\n", FISHEYE_CONFIG->OutW_disp, FISHEYE_CONFIG->OutH_disp);

    FISHEYE_REGION[0].RegionValid = 1;
    FISHEYE_REGION[0].ViewMode = PROJECTION_STEREO_FIT;

    int X_TILE_NUMBER, Y_TILE_NUMBER;
    u32 mesh_id_size;
    u32 mesh_tbl_size;
    u64 mesh_id_phy_addr, mesh_tbl_phy_addr;

    X_TILE_NUMBER = DIV_UP(in_size.u32Width, DWA_MESH_SLICE_BASE_W);
    Y_TILE_NUMBER = DIV_UP(in_size.u32Height, DWA_MESH_SLICE_BASE_H);

    // calculate mesh_id/mesh_tbl's size in bytes.
    mesh_tbl_size = 0x60000;
    mesh_id_size = 0x30000;

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

    // Provide virtual address to write mesh.
    reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
    generate_mesh_on_fisheye(&(pstWarpAttr->stGridInfoAttr), FISHEYE_CONFIG, FISHEYE_REGION, X_TILE_NUMBER, Y_TILE_NUMBER
        , (uint16_t *)mesh_vir_addr, reorder_mesh_tbl, mesh_tbl_phy_addr, grid);
    free(FISHEYE_CONFIG);
    free(FISHEYE_REGION);

    return BM_SUCCESS;
}

static bm_status_t bm_dwa_add_ldc_task(bm_handle_t handle, int fd, GDC_HANDLE hHandle, GDC_TASK_ATTR_S *pstTask
    , const LDC_ATTR_S *pstLDCAttr, ROTATION_E enRotation, char *grid)
{
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstTask);
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstLDCAttr);
    CHECK_DWA_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_FAILURE;
    }

    if (enRotation != ROTATION_0) {
        if (bm_dwa_rotation_check_size(enRotation, pstTask) != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dwa_rotation_check_size fail\n");
            return BM_ERR_FAILURE;
        }
    }

    struct dwa_task_attr attr;
    u8 idx;
    pthread_mutex_lock(&mutex);
    ret = bm_get_valid_tsk_mesh_by_name(handle, pstTask->name, &idx);

    if (ret != BM_SUCCESS) {
        SIZE_S in_size, out_size;
        u32 mesh_1st_size, mesh_2nd_size;
        u64 paddr;
        bm_device_mem_t pmem;

        in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
        in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
        out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
        out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;
        dwa_mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);

        bm_malloc_device_byte(handle, &pmem, mesh_1st_size + mesh_2nd_size);
        paddr = pmem.u.device.device_addr;
        char *buffer = (char *)malloc(mesh_1st_size + mesh_2nd_size);
        if (buffer == NULL) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "malloc buffer for mesh failed!\n");
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        memset(buffer, 0, mesh_1st_size + mesh_2nd_size);
        dwa_mesh_gen_ldc(in_size, out_size, pstLDCAttr, paddr, (void*)buffer, enRotation, grid);

        ret = bm_memcpy_s2d(handle, pmem, (void*)buffer);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_memcpy_s2d failed!\n");
            free(buffer);
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        free(buffer);

        dwa_tskMesh[idx].paddr = paddr;
        dwa_tskMesh[idx].vaddr = 0;
    }
    pthread_mutex_unlock(&mutex);

    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.stImgIn, &pstTask->stImgIn, sizeof(attr.stImgIn));
    memcpy(&attr.stImgOut, &pstTask->stImgOut, sizeof(attr.stImgOut));
    //memcpy(attr.au64privateData, pstTask->au64privateData, sizeof(attr.au64privateData));
    memcpy(&attr.stLdcAttr, pstLDCAttr, sizeof(*pstLDCAttr));
    attr.reserved = pstTask->reserved;
    attr.au64privateData[0] = dwa_tskMesh[idx].paddr;

    pstTask->au64privateData[0] = dwa_tskMesh[idx].paddr;
    pstTask->au64privateData[1] = (u64)((uintptr_t)dwa_tskMesh[idx].vaddr);
    return dwa_add_ldc_task(fd, &attr);
}

static bm_status_t bm_dwa_add_rotation_task(bm_handle_t handle, int fd, DWA_HANDLE hHandle, GDC_TASK_ATTR_S *pstTask, ROTATION_E enRotation)
{
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstTask);
    CHECK_DWA_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

    if (!hHandle){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_FAILURE;
    }

    if (enRotation < ROTATION_0 || enRotation >= ROTATION_MAX) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dwa(%d) param invalid\n", enRotation);
        return BM_ERR_FAILURE;
    }

    if (bm_dwa_rotation_check_size(enRotation, pstTask) != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dwa_rotation_check_size fail\n");
        return BM_ERR_FAILURE;
    }

    struct dwa_task_attr attr;
    u64 paddr;
    bm_device_mem_t pmem;
    SIZE_S in_size, out_size;

    in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
    in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
    out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
    out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;

    u8 idx;
    pthread_mutex_lock(&mutex);
    ret = bm_get_valid_tsk_mesh_by_name(handle, pstTask->name, &idx);
    if (ret != BM_SUCCESS) {
        bm_malloc_device_byte(handle, &pmem, DWA_MESH_SIZE_ROT);
        paddr = pmem.u.device.device_addr;
        char *buffer = (char *)malloc(DWA_MESH_SIZE_ROT);
        if (buffer == NULL) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "malloc buffer for mesh failed!\n");
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        memset(buffer, 0, DWA_MESH_SIZE_ROT);
        bm_dwa_mesh_gen_rotation(in_size, out_size, enRotation, paddr, (void*)buffer);

        ret = bm_memcpy_s2d(handle, pmem, (void*)buffer);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_memcpy_s2d failed!\n");
            free(buffer);
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        free(buffer);

        dwa_tskMesh[idx].paddr = paddr;
        dwa_tskMesh[idx].vaddr = 0;
    }
    pthread_mutex_unlock(&mutex);
    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.stImgIn, &pstTask->stImgIn, sizeof(attr.stImgIn));
    memcpy(&attr.stImgOut, &pstTask->stImgOut, sizeof(attr.stImgOut));
    //memcpy(attr.au64privateData, pstTask->au64privateData, sizeof(attr.au64privateData));
    attr.reserved = pstTask->reserved;
    attr.enRotation = enRotation;
    attr.au64privateData[0] = dwa_tskMesh[idx].paddr;

    pstTask->au64privateData[0] = dwa_tskMesh[idx].paddr;
    pstTask->au64privateData[1] = (u64)((uintptr_t)dwa_tskMesh[idx].vaddr);
    return dwa_add_rotation_task(fd, &attr);
}

static bm_status_t bm_dwa_add_correction_task(bm_handle_t handle, int fd, GDC_HANDLE hHandle, GDC_TASK_ATTR_S *pstTask,
                const FISHEYE_ATTR_S *pstFishEyeAttr, char *grid){
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstTask);
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstFishEyeAttr);
    CHECK_DWA_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_FAILURE;
    }

    if (pstFishEyeAttr->bEnable) {
        if(!pstFishEyeAttr->stGridInfoAttr.Enable) {
            if (pstFishEyeAttr->u32RegionNum == 0) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "RegionNum(%d) can't be 0 if enable fisheye.\n",
                            pstFishEyeAttr->u32RegionNum);
                return BM_ERR_FAILURE;
            }
            if (pstFishEyeAttr->enUseMode == MODE_01_1O || pstFishEyeAttr->enUseMode == MODE_STEREO_FIT) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "FISHEYE not support MODE_01_1O and MODE_STEREO_FIT.\n");
                return BM_ERR_PARAM;
            }
            if (((u32)pstFishEyeAttr->s32HorOffset > pstTask->stImgIn.stVFrame.u32Width) ||
                ((u32)pstFishEyeAttr->s32VerOffset > pstTask->stImgIn.stVFrame.u32Height)) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "center pos(%d %d) out of frame size(%d %d).\n",
                            pstFishEyeAttr->s32HorOffset, pstFishEyeAttr->s32VerOffset,
                            pstTask->stImgIn.stVFrame.u32Width, pstTask->stImgIn.stVFrame.u32Height);
                return BM_ERR_PARAM;
            }
            for (u32 i = 0; i < pstFishEyeAttr->u32RegionNum; ++i) {
                if ((pstFishEyeAttr->enMountMode == FISHEYE_WALL_MOUNT) &&
                    (pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_360_PANORAMA)) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Rgn(%d): WALL_MOUNT not support Panorama_360.\n", i);
                    return BM_ERR_PARAM;
                }
                if ((pstFishEyeAttr->enMountMode == FISHEYE_CEILING_MOUNT) &&
                    (pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA)) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Rgn(%d): CEILING_MOUNT not support Panorama_180.\n", i);
                    return BM_ERR_PARAM;
                }
                if ((pstFishEyeAttr->enMountMode == FISHEYE_DESKTOP_MOUNT) &&
                    (pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA)) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Rgn(%d): DESKTOP_MOUNT not support Panorama_180.\n", i);
                    return BM_ERR_PARAM;
                }
            }
        }
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "FishEyeAttr is not be enabled.\n");
        return BM_ERR_FAILURE;
    }

    struct dwa_task_attr attr;

    u8 idx;
    pthread_mutex_lock(&mutex);
    ret = bm_get_valid_tsk_mesh_by_name(handle, pstTask->name, &idx);
    if (ret != BM_SUCCESS) {
        SIZE_S in_size, out_size;
        u64 paddr;
        bm_device_mem_t pmem;

        in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
        in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
        out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
        out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;

        bm_malloc_device_byte(handle, &pmem, DWA_MESH_SIZE_FISHEYE);
        paddr = pmem.u.device.device_addr;
        char *buffer = (char *)malloc(DWA_MESH_SIZE_FISHEYE);
        if (buffer == NULL) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "malloc buffer for mesh failed!\n");
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        memset(buffer, 0, DWA_MESH_SIZE_FISHEYE);
        dwa_mesh_gen_fisheye(in_size, out_size, pstFishEyeAttr, paddr, (void*)buffer, ROTATION_0, grid);

        ret = bm_memcpy_s2d(handle, pmem, (void*)buffer);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_memcpy_s2d failed!\n");
            free(buffer);
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        free(buffer);

        dwa_tskMesh[idx].paddr = paddr;
        dwa_tskMesh[idx].vaddr = 0;
    }
    pthread_mutex_unlock(&mutex);

    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.stImgIn, &pstTask->stImgIn, sizeof(attr.stImgIn));
    memcpy(&attr.stImgOut, &pstTask->stImgOut, sizeof(attr.stImgOut));
    //memcpy(attr.au64privateData, pstTask->au64privateData, sizeof(attr.au64privateData));
    memcpy(&attr.stFishEyeAttr, pstFishEyeAttr, sizeof(*pstFishEyeAttr));
    attr.reserved = pstTask->reserved;
    attr.au64privateData[0] = dwa_tskMesh[idx].paddr;

    pstTask->au64privateData[0] = dwa_tskMesh[idx].paddr;
    pstTask->au64privateData[1] = (u64)((uintptr_t)dwa_tskMesh[idx].vaddr);
    return dwa_add_correction_task(fd, &attr);

}

static bm_status_t bm_dwa_add_affine_task(bm_handle_t handle, int fd, GDC_HANDLE hHandle, GDC_TASK_ATTR_S *pstTask, const AFFINE_ATTR_S *pstAffineAttr)
{
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstTask);
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstAffineAttr);
    CHECK_DWA_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_FAILURE;
    }

    if (pstAffineAttr->u32RegionNum == 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "u32RegionNum(%d) can't be zero.\n", pstAffineAttr->u32RegionNum);
        return BM_ERR_FAILURE;
    }

    if (pstAffineAttr->stDestSize.u32Width > pstTask->stImgOut.stVFrame.u32Width) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dest's width(%d) can't be larger than frame's width(%d)\n",
                    pstAffineAttr->stDestSize.u32Width, pstTask->stImgOut.stVFrame.u32Width);
        return BM_ERR_FAILURE;
    }
    for (u32 i = 0; i < pstAffineAttr->u32RegionNum; ++i) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_INFO, "u32RegionNum(%d) (%f, %f) (%f, %f) (%f, %f) (%f, %f)\n", i,
                    pstAffineAttr->astRegionAttr[i][0].x, pstAffineAttr->astRegionAttr[i][0].y,
                    pstAffineAttr->astRegionAttr[i][1].x, pstAffineAttr->astRegionAttr[i][1].y,
                    pstAffineAttr->astRegionAttr[i][2].x, pstAffineAttr->astRegionAttr[i][2].y,
                    pstAffineAttr->astRegionAttr[i][3].x, pstAffineAttr->astRegionAttr[i][3].y);
        if ((pstAffineAttr->astRegionAttr[i][0].x < 0) || (pstAffineAttr->astRegionAttr[i][0].y < 0) ||
            (pstAffineAttr->astRegionAttr[i][1].x < 0) || (pstAffineAttr->astRegionAttr[i][1].y < 0) ||
            (pstAffineAttr->astRegionAttr[i][2].x < 0) || (pstAffineAttr->astRegionAttr[i][2].y < 0) ||
            (pstAffineAttr->astRegionAttr[i][3].x < 0) || (pstAffineAttr->astRegionAttr[i][3].y < 0)) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "u32RegionNum(%d) affine point can't be negative\n", i);
            return BM_ERR_FAILURE;
        }
        if ((pstAffineAttr->astRegionAttr[i][1].x < pstAffineAttr->astRegionAttr[i][0].x) ||
            (pstAffineAttr->astRegionAttr[i][3].x < pstAffineAttr->astRegionAttr[i][2].x)) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "u32RegionNum(%d) point1/3's x should be bigger thant 0/2's\n", i);
            return BM_ERR_FAILURE;
        }
        if ((pstAffineAttr->astRegionAttr[i][2].y < pstAffineAttr->astRegionAttr[i][0].y) ||
            (pstAffineAttr->astRegionAttr[i][3].y < pstAffineAttr->astRegionAttr[i][1].y)) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "u32RegionNum(%d) point2/3's y should be bigger thant 0/1's\n", i);
            return BM_ERR_FAILURE;
        }
    }

    struct dwa_task_attr attr;

    u8 idx;
    pthread_mutex_lock(&mutex);
    ret = bm_get_valid_tsk_mesh_by_name(handle, pstTask->name, &idx);
    if (ret != BM_SUCCESS) {
        SIZE_S in_size, out_size;
        u64 paddr;
        bm_device_mem_t pmem;

        in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
        in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
        out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
        out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;

        bm_malloc_device_byte(handle, &pmem, DWA_MESH_SIZE_AFFINE);
        paddr = pmem.u.device.device_addr;
        char *buffer = (char *)malloc(DWA_MESH_SIZE_AFFINE);
        if (buffer == NULL) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "malloc buffer for mesh failed!\n");
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        memset(buffer, 0, DWA_MESH_SIZE_AFFINE);
        dwa_mesh_gen_affine(in_size, out_size, pstAffineAttr, paddr, (void*)buffer);

        ret = bm_memcpy_s2d(handle, pmem, (void*)buffer);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_memcpy_s2d failed!\n");
            free(buffer);
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        free(buffer);

        dwa_tskMesh[idx].paddr = paddr;
        dwa_tskMesh[idx].vaddr = 0;
    }
    pthread_mutex_unlock(&mutex);

    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.stImgIn, &pstTask->stImgIn, sizeof(attr.stImgIn));
    memcpy(&attr.stImgOut, &pstTask->stImgOut, sizeof(attr.stImgOut));
    //memcpy(attr.au64privateData, pstTask->au64privateData, sizeof(attr.au64privateData));
    memcpy(&attr.stAffineAttr, pstAffineAttr, sizeof(*pstAffineAttr));
    attr.reserved = pstTask->reserved;
    attr.au64privateData[0] = dwa_tskMesh[idx].paddr;

    pstTask->au64privateData[0] = dwa_tskMesh[idx].paddr;
    pstTask->au64privateData[1] = (u64)((uintptr_t)dwa_tskMesh[idx].vaddr);
    return dwa_add_affine_task(fd, &attr);
}

static bm_status_t bm_dwa_add_dewarp_task(bm_handle_t handle, int fd, GDC_HANDLE hHandle, GDC_TASK_ATTR_S *pstTask
    , const WARP_ATTR_S *pstWarpAttr, char *grid)
{
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstTask);
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstWarpAttr);
    CHECK_DWA_FORMAT(pstTask->stImgIn, pstTask->stImgOut);

    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle.\n");
        return BM_ERR_FAILURE;
    }

    if (!pstWarpAttr->bEnable)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "WarpAttr is not be enabled.\n");
        return BM_ERR_FAILURE;
    }

    if (!pstWarpAttr->stGridInfoAttr.Enable)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "GridInfoAttr is not be enabled.\n");
        return BM_ERR_FAILURE;
    }

    struct dwa_task_attr attr;

    u8 idx;
    pthread_mutex_lock(&mutex);
    ret = bm_get_valid_tsk_mesh_by_name(handle, pstTask->name, &idx);
    if (ret != BM_SUCCESS) {
        SIZE_S in_size, out_size;
        u64 paddr;
        bm_device_mem_t pmem;

        in_size.u32Width = pstTask->stImgIn.stVFrame.u32Width;
        in_size.u32Height = pstTask->stImgIn.stVFrame.u32Height;
        out_size.u32Width = pstTask->stImgOut.stVFrame.u32Width;
        out_size.u32Height = pstTask->stImgOut.stVFrame.u32Height;

        bm_malloc_device_byte(handle, &pmem, DWA_MESH_SIZE_FISHEYE);
        paddr = pmem.u.device.device_addr;
        char *buffer = (char *)malloc(DWA_MESH_SIZE_FISHEYE);
        if (buffer == NULL) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "malloc buffer for mesh failed!\n");
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        memset(buffer, 0, DWA_MESH_SIZE_FISHEYE);
        dwa_mesh_gen_warp(in_size, out_size, pstWarpAttr, paddr, (void*)buffer, grid);

        ret = bm_memcpy_s2d(handle, pmem, (void*)buffer);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_memcpy_s2d failed!\n");
            free(buffer);
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        free(buffer);

        dwa_tskMesh[idx].paddr = paddr;
        dwa_tskMesh[idx].vaddr = 0;
    }
    pthread_mutex_unlock(&mutex);

    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.stImgIn, &pstTask->stImgIn, sizeof(attr.stImgIn));
    memcpy(&attr.stImgOut, &pstTask->stImgOut, sizeof(attr.stImgOut));
    //memcpy(attr.au64privateData, pstTask->au64privateData, sizeof(attr.au64privateData));
    memcpy(&attr.stWarpAttr, pstWarpAttr, sizeof(*pstWarpAttr));
    attr.reserved = pstTask->reserved;
    attr.au64privateData[0] = dwa_tskMesh[idx].paddr;

    pstTask->au64privateData[0] = dwa_tskMesh[idx].paddr;
    pstTask->au64privateData[1] = (u64)((uintptr_t)dwa_tskMesh[idx].vaddr);
    return dwa_add_warp_task(fd, &attr);
}

static bm_status_t bm_dwa_add_task(bm_handle_t handle, int fd, bm_dwa_basic_param *param, void *ptr)
{
    ROTATION_E enRotation;
    AFFINE_ATTR_S *stAffineAttr;
    BM_DEWARP_ATTR_AND_GRID *dewarp_attr_and_grid;
    BM_GDC_ATTR_AND_GRID *gdc_attr_and_grid;
    BM_FISHEYE_ATTR_AND_GRID *fisheye_attr_and_grid;

    s32 ret = BM_ERR_FAILURE;

    if (!param) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dwa_basic fail, null ptr for test param\n");
        return BM_ERR_FAILURE;
    }

    switch(param->op){
        case DWA_TEST_FISHEYE:
            fisheye_attr_and_grid = (BM_FISHEYE_ATTR_AND_GRID *)ptr;

            ret = bm_dwa_add_correction_task(handle, fd, param->hHandle, &param->stTask, &(fisheye_attr_and_grid->fisheye_attr), fisheye_attr_and_grid->grid);
            if(ret) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_add_correction_task failed\n");
            }
            break;
        case DWA_TEST_ROT:
            enRotation = *(ROTATION_E *)ptr;

            ret = bm_dwa_add_rotation_task(handle, fd, param->hHandle, &param->stTask, enRotation);
            if(ret){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_add_rotation_task failed\n");
            }
            break;
        case DWA_TEST_LDC:
            enRotation = (ROTATION_E)param->stTask.reserved;
            gdc_attr_and_grid = (BM_GDC_ATTR_AND_GRID *)ptr;

            ret = bm_dwa_add_ldc_task(handle, fd, param->hHandle, &param->stTask, &(gdc_attr_and_grid->ldc_attr), enRotation, gdc_attr_and_grid->grid);
            if (ret){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_add_ldc_task failed\n");
            }
            break;
        case DWA_TEST_AFFINE:
            stAffineAttr = (AFFINE_ATTR_S *)ptr;

            ret = bm_dwa_add_affine_task(handle, fd, param->hHandle, &param->stTask, stAffineAttr);
            if(ret){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_add_affine_task failed\n");
            }
            break;
        case DWA_TEST_DEWARP:
            dewarp_attr_and_grid = (BM_DEWARP_ATTR_AND_GRID *)ptr;

            ret = bm_dwa_add_dewarp_task(handle, fd, param->hHandle, &param->stTask, &(dewarp_attr_and_grid->dewarp_attr), dewarp_attr_and_grid->grid);
            if(ret){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_add_dewarp_task failed\n");
            }
            break;
        default:
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not allow this op(%d) fail\n", param->op);
            break;
    }
    return ret;
}

static bm_status_t bm_dwa_init(int fd)
{
    bm_status_t ret = BM_SUCCESS;
    if (bm_get_dwa_fd(&fd) != BM_SUCCESS)
        return BM_ERR_FAILURE;

    ret = (bm_status_t)dwa_init(fd);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "init fail\n");
        return ret;
    }

    return ret;
}

static bm_status_t bm_dwa_begin_job(int fd, DWA_HANDLE *phHandle)
{
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, phHandle);

    struct dwa_handle_data cfg;

    memset(&cfg, 0, sizeof(cfg));

    if (dwa_begin_job(fd, &cfg))
        return BM_ERR_FAILURE;

    *phHandle = cfg.handle;
    return BM_SUCCESS;
}

static bm_status_t bm_dwa_set_job_identity(int fd, DWA_HANDLE hHandle, DWA_IDENTITY_ATTR_S *identity_attr)
{
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, identity_attr);

    if(!hHandle){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_FAILURE;
    }

    struct dwa_identity_attr cfg = {0};
    cfg.handle = hHandle;
    memcpy(&cfg.attr, identity_attr, sizeof(*identity_attr));

    return dwa_set_job_identity(fd, &cfg);
}

static bm_status_t bm_dwa_send_frame(int fd,
                                bm_image *input_image,
                                bm_image *output_image,
                                bm_dwa_basic_param *param)
{
    bm_status_t ret = BM_SUCCESS;
    PIXEL_FORMAT_E pixel_format = 0;

    memset(&param->stVideoFrameIn,0,sizeof(param->stVideoFrameIn));
    memset(&param->stVideoFrameOut,0,sizeof(param->stVideoFrameOut));

    bm_image_format_to_cvi(input_image->image_format, input_image->data_type, &pixel_format);
    bm_image_format_to_cvi(output_image->image_format, output_image->data_type, &pixel_format);
    bm_image_to_cvi_frame(input_image, pixel_format, &param->stVideoFrameIn);
    bm_image_to_cvi_frame(output_image, pixel_format, &param->stVideoFrameOut);

    memcpy(&param->stTask.stImgIn, &param->stVideoFrameIn, sizeof(param->stVideoFrameIn));
    memcpy(&param->stTask.stImgOut, &param->stVideoFrameOut, sizeof(param->stVideoFrameOut));

    return ret;
}

static bm_status_t bm_dwa_end_job(int fd, GDC_HANDLE hHandle)
{
    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_PARAM;
    }

    struct dwa_handle_data cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.handle = hHandle;
    return dwa_end_job(fd, &cfg);
}

bm_status_t bm_dwa_get_frame(int fd,
                             bm_image *output_image,
                             bm_dwa_basic_param *param)
{
    bm_status_t ret = BM_SUCCESS;
    PIXEL_FORMAT_E pixel_format = 0;

    bm_image_format_to_cvi(output_image->image_format, output_image->data_type, &pixel_format);
    bm_image_to_cvi_frame(output_image, pixel_format, &param->stVideoFrameOut);

    memcpy(&param->stTask.stImgOut, &param->stVideoFrameOut, sizeof(param->stVideoFrameOut));

    for(int i = 0; i < output_image->image_private->plane_num; i++){
        output_image->image_private->data[i].u.device.device_addr = param->stVideoFrameOut.stVFrame.u64PhyAddr[i];
    }

    return ret;
}

bm_status_t bm_dwa_get_chn_frame(int fd, DWA_IDENTITY_ATTR_S *identity_attr, VIDEO_FRAME_INFO_S *pstFrameInfo, int s32MilliSec)
{
    bm_status_t ret;
    struct dwa_chn_frm_cfg cfg;

    MOD_CHECK_NULL_PTR(CVI_ID_DWA, identity_attr);
    MOD_CHECK_NULL_PTR(CVI_ID_DWA, pstFrameInfo);

    memset(&cfg, 0, sizeof(cfg));
    memcpy(&cfg.identity.attr, identity_attr, sizeof(*identity_attr));
    cfg.MilliSec = s32MilliSec;

    ret = dwa_get_chn_frm(fd, &cfg);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "identity[%s-%d-%d] get chn frame fail, Ret[%d]\n", identity_attr->Name , identity_attr->enModId, identity_attr->u32ID, ret);
        return BM_ERR_FAILURE;
    }
    memcpy(pstFrameInfo, &cfg.VideoFrame, sizeof(*pstFrameInfo));

    return ret;
}

bm_status_t bm_dwa_cancel_job(int fd, GDC_HANDLE hHandle)
{
    bm_status_t ret = BM_SUCCESS;
    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_cancel_job failed, null hHandle for test param.\n");
        return BM_ERR_PARAM;
    }

    struct dwa_handle_data cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.handle = hHandle;
    ret = (bm_status_t)ioctl(fd, CVI_DWA_CANCEL_JOB, &cfg);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dwa_cancel_job failed!\n");
        ret = BM_ERR_FAILURE;
    }
    return ret;
}

static bm_status_t bm_dwa_basic(bm_handle_t handle,
                                bm_image input_image,
                                bm_image output_image,
                                bm_dwa_basic_param *param,
                                void *ptr){

    s32 fd = 0;

    s32 ret = bm_get_dwa_fd(&fd);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_get_dwa_fd failed!\n");
        return BM_ERR_DEVNOTREADY;
    }

    ret = bm_dwa_send_frame(fd, &input_image, &output_image, param);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_send_frame failed!\n");
        return BM_ERR_FAILURE;
    }

    ret = bm_dwa_init(fd);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "BM_DWA_Init failed!\n");
        return BM_ERR_FAILURE;
    }

    ret = bm_dwa_begin_job(fd, &param->hHandle);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "BM_DWA_BeginJob failed!\n");
        ret = BM_ERR_FAILURE;
        goto fail;
    }

    ret = bm_dwa_set_job_identity(fd, param->hHandle, &param->identity);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "BM_DWA_SetJobIdentity failed!\n");
        ret = BM_ERR_FAILURE;
        goto fail;
    }

    ret = bm_dwa_add_task(handle, fd, param, ptr);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "BM_DWA_ADDTASK failed!\n");
        ret = BM_ERR_FAILURE;
        goto fail;
    }

    ret = bm_dwa_end_job(fd, param->hHandle);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "BM_DWA_EndJob failed!\n");
        ret = BM_ERR_FAILURE;
        goto fail;
    }

    ret = bm_dwa_get_chn_frame(fd, &param->identity, &param->stVideoFrameOut, 500);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_get_chn_frame failed!\n");
        ret = BM_ERR_FAILURE;
        goto fail;
    }

    ret = bm_dwa_get_frame(fd, &output_image, param);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_get_frame failed!\n");
        ret = BM_ERR_FAILURE;
        goto fail;
    }

fail:
    if (ret) {
        if (param->hHandle) {
            ret |= bm_dwa_cancel_job(fd, param->hHandle);
        }
    }
    return ret;
}

bm_status_t bmcv_dwa_rot_internel(bm_handle_t          handle,
                                  bm_image             input_image,
                                  bm_image             output_image,
                                  bmcv_rot_mode        rot_mode){
    bm_status_t ret = BM_SUCCESS;
    bm_dwa_basic_param param;
    ROTATION_E rotation_mode = (ROTATION_E)rot_mode;
    bm_rot_name rot_name;
    char md5_str[MD5_STRING_LENGTH + 1];
    memset(&rot_name, 0, sizeof(rot_name));
    memset(&param, 0, sizeof(param));

    param.size_in.u32Width = input_image.width;
    param.size_in.u32Height = input_image.height;
    param.size_out.u32Width = output_image.width;
    param.size_out.u32Height = output_image.height;

    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.enModId = CVI_ID_DWA;
    param.identity.u32ID = 0;

    param.identity.syncIo = false;
    param.op = DWA_TEST_ROT;

    rot_name.param = param;
    rot_name.rotation_mode = rotation_mode;
    md5_get((unsigned char *)&rot_name, sizeof(rot_name), md5_str, 0);

    snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, md5_str);
    // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_rot");
    ret = bm_dwa_basic(handle, input_image, output_image, &param, (void *)&rotation_mode);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa basic failed \n");
    }
    return ret;
}

bm_status_t bmcv_dwa_gdc_internel(bm_handle_t          handle,
                                  bm_image             input_image,
                                  bm_image             output_image,
                                  bmcv_gdc_attr        ldc_attr){
    bm_status_t ret = BM_SUCCESS;
    bm_dwa_basic_param param;
    bm_gdc_name gdc_name;
    BM_GDC_ATTR_AND_GRID gdc_with_grid;
    char md5_str[MD5_STRING_LENGTH + 1];
    memset(&gdc_name, 0, sizeof(gdc_name));
    memset(&param, 0, sizeof(param));
    memset(&gdc_with_grid, 0, sizeof(gdc_with_grid));

    param.size_in.u32Width = input_image.width;
    param.size_in.u32Height = input_image.height;
    param.size_out.u32Width = output_image.width;
    param.size_out.u32Height = output_image.height;

    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.enModId = CVI_ID_DWA;
    param.identity.u32ID = 0;

    param.identity.syncIo = false;
    param.op = DWA_TEST_LDC;

    if (ldc_attr.grid_info.size == 0) {
        gdc_with_grid.ldc_attr.bAspect = ldc_attr.bAspect;
        gdc_with_grid.ldc_attr.s32CenterXOffset = ldc_attr.s32CenterXOffset;
        gdc_with_grid.ldc_attr.s32CenterYOffset = ldc_attr.s32CenterYOffset;
        gdc_with_grid.ldc_attr.s32DistortionRatio = ldc_attr.s32DistortionRatio;
        gdc_with_grid.ldc_attr.s32XRatio = ldc_attr.s32XRatio;
        gdc_with_grid.ldc_attr.s32XYRatio = ldc_attr.s32XYRatio;
        gdc_with_grid.ldc_attr.s32YRatio = ldc_attr.s32YRatio;
        gdc_with_grid.ldc_attr.stGridInfoAttr.Enable = false;
        gdc_with_grid.grid = NULL;

        gdc_name.param = param;
        gdc_name.ldc_attr = gdc_with_grid.ldc_attr;

        md5_get((unsigned char *)&gdc_name, sizeof(gdc_name), md5_str, 0);
        snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, md5_str);
        // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_gdc");
    } else {
        gdc_with_grid.ldc_attr.stGridInfoAttr.Enable = true;
        gdc_with_grid.grid = (char *)ldc_attr.grid_info.u.system.system_addr;

        gdc_name.param = param;
        gdc_name.ldc_attr = gdc_with_grid.ldc_attr;

        char md5_grid_info[MD5_STRING_LENGTH + 1];
        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "ldc_attr.grid_size = [%d]\n", ldc_attr.grid_info.size);
        md5_get((unsigned char *)gdc_with_grid.grid, ldc_attr.grid_info.size, md5_grid_info, 0);
        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "ldc_attr.grid_info[0] = [%d]\n", gdc_with_grid.grid[0]);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "ldc_attr.grid_info[1] = [%d]\n", gdc_with_grid.grid[1]);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "ldc_attr.grid_info[2] = [%d]\n", gdc_with_grid.grid[2]);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "ldc_attr.grid_info[3] = [%d]\n", gdc_with_grid.grid[3]);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "ldc_attr.grid_info[4] = [%d]\n", gdc_with_grid.grid[4]);
        strcpy(gdc_with_grid.ldc_attr.stGridInfoAttr.gridBindName, md5_grid_info);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "grid_info md5 = [%s]\n", md5_grid_info);
        strcpy(gdc_name.grid, md5_grid_info);
        md5_get((unsigned char *)&gdc_name, sizeof(gdc_name), md5_str, 0);
        snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, md5_str);
        strcpy(gdc_with_grid.ldc_attr.stGridInfoAttr.gridFileName, "grid_info_79_43_3397_80_45_1280x720.dat");
    }

    ret = bm_dwa_basic(handle, input_image, output_image, &param, (void *)&gdc_with_grid);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa basic failed \n");
    }
    return ret;
}

bm_status_t bmcv_dwa_fisheye_internel(bm_handle_t          handle,
                                      bm_image             input_image,
                                      bm_image             output_image,
                                      bmcv_fisheye_attr_s  fisheye_attr) {
    bm_status_t ret = BM_SUCCESS;
    bm_dwa_basic_param param;
    FISHEYE_ATTR_S dwa_fisheye_attr;
    bm_fisheye_name fisheye_name;
    BM_FISHEYE_ATTR_AND_GRID fisheye_with_grid;
    char md5_str[MD5_STRING_LENGTH + 1];
    memset(&param, 0, sizeof(param));
    memset(&dwa_fisheye_attr, 0, sizeof(dwa_fisheye_attr));
    memset(&fisheye_with_grid, 0, sizeof(fisheye_with_grid));

    param.size_in.u32Width = input_image.width;
    param.size_in.u32Height = input_image.height;
    param.size_out.u32Width = output_image.width;
    param.size_out.u32Height = output_image.height;
    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.enModId = CVI_ID_DWA;
    param.identity.u32ID = 0;
    param.identity.syncIo = false;

    param.op = DWA_TEST_FISHEYE;

    if (fisheye_attr.grid_info.size == 0) {
        fisheye_with_grid.fisheye_attr.bEnable = fisheye_attr.bEnable;
        fisheye_with_grid.fisheye_attr.bBgColor = fisheye_attr.bBgColor;
        fisheye_with_grid.fisheye_attr.u32BgColor = fisheye_attr.u32BgColor;
        fisheye_with_grid.fisheye_attr.s32HorOffset = fisheye_attr.s32HorOffset;
        fisheye_with_grid.fisheye_attr.s32VerOffset = fisheye_attr.s32VerOffset;
        fisheye_with_grid.fisheye_attr.enMountMode = (FISHEYE_MOUNT_MODE_E)fisheye_attr.enMountMode;
        fisheye_with_grid.fisheye_attr.enUseMode = (USAGE_MODE)fisheye_attr.enUseMode;
        fisheye_with_grid.fisheye_attr.astFishEyeRegionAttr[0].enViewMode = (FISHEYE_VIEW_MODE_E)(fisheye_attr.enViewMode);
        fisheye_with_grid.fisheye_attr.u32RegionNum = fisheye_attr.u32RegionNum;
        fisheye_with_grid.fisheye_attr.stGridInfoAttr.Enable = false;
        fisheye_with_grid.grid = NULL;

        fisheye_name.param = param;
        fisheye_name.fisheye_attr = fisheye_with_grid.fisheye_attr;
        md5_get((unsigned char *)&fisheye_name, sizeof(fisheye_name), md5_str, 0);
        snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, md5_str);
        // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_fisheye");
    } else {
        fisheye_with_grid.fisheye_attr.bEnable = fisheye_attr.bEnable;
        fisheye_with_grid.fisheye_attr.stGridInfoAttr.Enable = true;
        fisheye_with_grid.grid = (char *)fisheye_attr.grid_info.u.system.system_addr;

        fisheye_name.param = param;
        fisheye_name.fisheye_attr = fisheye_with_grid.fisheye_attr;
        char md5_grid_info[MD5_STRING_LENGTH + 1];
        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "fisheye_attr.grid_size = [%d]\n", fisheye_attr.grid_info.size);
        md5_get((unsigned char *)fisheye_with_grid.grid, fisheye_attr.grid_info.size, md5_grid_info, 0);
        strcpy(fisheye_with_grid.fisheye_attr.stGridInfoAttr.gridBindName, md5_grid_info);
        strcpy(fisheye_name.grid, md5_grid_info);
        md5_get((unsigned char *)&fisheye_name, sizeof(fisheye_name), md5_str, 0);
        snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, md5_str);
        strcpy(fisheye_with_grid.fisheye_attr.stGridInfoAttr.gridFileName, "L_grid_info_68_68_4624_70_70_dst_2240x2240_src_2240x2240.dat");
    }

    ret = bm_dwa_basic(handle, input_image, output_image, &param, (void *)&fisheye_with_grid);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa basic failed \n");
    }
    return ret;
}

bm_status_t bmcv_dwa_affine_internel(bm_handle_t          handle,
                                     bm_image             input_image,
                                     bm_image             output_image,
                                     bmcv_affine_attr_s   affine_attr){
    bm_status_t ret = BM_SUCCESS;
    bm_dwa_basic_param param;
    AFFINE_ATTR_S dwa_affine_attr;
    bm_affine_name affine_name;
    char md5_str[MD5_STRING_LENGTH + 1];
    memset(&param, 0, sizeof(param));
    memset(&dwa_affine_attr, 0, sizeof(dwa_affine_attr));
    memset(&affine_name, 0, sizeof(affine_name));

    param.size_in.u32Width = input_image.width;
    param.size_in.u32Height = input_image.height;
    param.size_out.u32Width = output_image.width;
    param.size_out.u32Height = output_image.height;
    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.enModId = CVI_ID_DWA;
    param.identity.u32ID = 0;
    param.identity.syncIo = false;

    param.op = DWA_TEST_AFFINE;

    affine_name.param = param;
    affine_name.affine_attr = dwa_affine_attr;
    md5_get((unsigned char *)&affine_name, sizeof(affine_name), md5_str, 0);
    snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, md5_str);
    // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_affine");

    dwa_affine_attr.u32RegionNum = affine_attr.u32RegionNum;
    memcpy(dwa_affine_attr.astRegionAttr, affine_attr.astRegionAttr, sizeof(affine_attr.astRegionAttr));    // user input
    dwa_affine_attr.stDestSize.u32Width = affine_attr.stDestSize.u32Width;
    dwa_affine_attr.stDestSize.u32Height = affine_attr.stDestSize.u32Height;

    ret = bm_dwa_basic(handle, input_image, output_image, &param, (void *)&dwa_affine_attr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa basic failed \n");
    }
    return ret;
}

bm_status_t bmcv_dwa_dewarp_internel(bm_handle_t          handle,
                                     bm_image             input_image,
                                     bm_image             output_image,
                                     bm_device_mem_t      grid_info){
    bm_status_t ret = BM_SUCCESS;
    bm_dwa_basic_param param;
    bm_dewarp_name dewarp_name;
    BM_DEWARP_ATTR_AND_GRID dewarp_with_grid;
    memset(&param, 0, sizeof(param));
    memset(&dewarp_name, 0, sizeof(dewarp_name));
    memset(&dewarp_with_grid, 0, sizeof(dewarp_with_grid));

    char md5_str[MD5_STRING_LENGTH + 1];
    param.size_in.u32Width = input_image.width;
    param.size_in.u32Height = input_image.height;
    param.size_out.u32Width = output_image.width;
    param.size_out.u32Height = output_image.height;
    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.enModId = CVI_ID_DWA;
    param.identity.u32ID = 0;
    param.identity.syncIo = false;

    param.op = DWA_TEST_DEWARP;

    dewarp_with_grid.dewarp_attr.bEnable = true;
    dewarp_with_grid.dewarp_attr.stGridInfoAttr.Enable = true;
    dewarp_with_grid.grid = (char *)grid_info.u.system.system_addr;
    dewarp_name.param = param;
    dewarp_name.dewarp_attr = dewarp_with_grid.dewarp_attr;

    char md5_grid_info[MD5_STRING_LENGTH + 1];
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_DEBUG, "dewarp_attr.grid_size = [%d]\n", grid_info.size);
    md5_get((unsigned char *)dewarp_with_grid.grid, grid_info.size, md5_grid_info, 0);
    strcpy(dewarp_with_grid.dewarp_attr.stGridInfoAttr.gridBindName, md5_grid_info);
    strcpy(dewarp_name.grid, md5_grid_info);
    md5_get((unsigned char *)&dewarp_name, sizeof(dewarp_name), md5_str, 0);
    snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, md5_str);
    strcpy(dewarp_with_grid.dewarp_attr.stGridInfoAttr.gridFileName, "grid_info_79_43_3397_80_45_1280x720.dat");

    ret = bm_dwa_basic(handle, input_image, output_image, &param, (void *)&dewarp_with_grid);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa basic failed \n");
    }
    return ret;
}