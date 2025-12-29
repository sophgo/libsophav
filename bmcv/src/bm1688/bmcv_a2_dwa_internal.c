#ifndef BM_PCIE_MODE
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
                                          pixel_format_e * cvi_fmt);

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
bm_tsk_mesh_attr_s dwa_tskMesh[DWA_MAX_TSK_MESH];

// static s32 dwa_init(s32 fd)
// {
//     return ioctl(fd, LDC_INIT);
// }

s32 dwa_deinit(s32 fd)
{
    return ioctl(fd, LDC_DEINIT);
}

static s32 dwa_begin_job(s32 fd, struct gdc_handle_data *cfg)
{
    return ioctl(fd, LDC_BEGIN_JOB, cfg);
}

static s32 dwa_end_job(s32 fd, struct gdc_handle_data *cfg)
{
    return ioctl(fd, LDC_END_JOB, cfg);
}

s32 dwa_cancel_job(s32 fd, struct gdc_handle_data *cfg)
{
    return ioctl(fd, LDC_CANCEL_JOB, cfg);
}

static s32 dwa_add_rotation_task(s32 fd, struct gdc_task_attr *attr)
{
    return ioctl(fd, LDC_ADD_DWA_ROT_TASK, attr);
}

static s32 dwa_add_ldc_task(s32 fd, struct gdc_task_attr *attr)
{
    return ioctl(fd, LDC_ADD_LDC_TASK, attr);
}

static s32 dwa_add_warp_task(s32 fd, struct gdc_task_attr *attr)
{
    return ioctl(fd, LDC_ADD_WAR_TASK, attr);
}

static s32 dwa_add_correction_task(s32 fd, struct gdc_task_attr *attr)
{
    return ioctl(fd, LDC_ADD_COR_TASK, attr);
}

static s32 dwa_add_affine_task(s32 fd, struct gdc_task_attr *attr)
{
    return ioctl(fd, LDC_ADD_AFF_TASK, attr);
}

static s32 dwa_set_job_identity(s32 fd, struct gdc_identity_attr *indentity)
{
    return ioctl(fd, LDC_SET_JOB_IDENTITY, indentity);
}

s32 dwa_get_work_job(s32 fd, struct gdc_handle_data *cfg)
{
    return ioctl(fd, LDC_GET_WORK_JOB, cfg);
}

s32 dwa_get_chn_frm(s32 fd, struct gdc_chn_frm_cfg *cfg)
{
    return ioctl(fd, LDC_GET_CHN_FRM, cfg);
}

static bool is_rect_overlap(point_s l1, point_s r1, point_s l2, point_s r2)
{
    // If one rectangle is on left side of other
    if (l1.x > r2.x || l2.x > r1.x)
        return false;

    // If one rectangle is above other
    if (l1.y > r2.y || l2.y > r1.y)
        return false;

    return true;
}

static void bm_load_frame_config(bm_fisheye_attr *fisheye_config)
{
    switch (fisheye_config->UsageMode)
    {
    case MODE_PANORAMA_360:
        fisheye_config->RgnNum = 1;
        break;
    case MODE_PANORAMA_180:
        fisheye_config->RgnNum = 1;
        break;
    case MODE_01_1O:
        fisheye_config->RgnNum = 1;
        break;
    case MODE_02_1O4R:
        fisheye_config->RgnNum = 4; // 1O should be handled in scaler block.
        break;
    case MODE_03_4R:
        fisheye_config->RgnNum = 4;
        break;
    case MODE_04_1P2R:
        fisheye_config->RgnNum = 3;
        break;
    case MODE_05_1P2R:
        fisheye_config->RgnNum = 3;
        break;
    case MODE_06_1P:
        fisheye_config->RgnNum = 1;
        break;
    case MODE_07_2P:
        fisheye_config->RgnNum = 2;
        break;
    case MODE_STEREO_FIT:
        fisheye_config->RgnNum = 1;
        break;
    default:
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "UsageMode Error!\n");
        //system("pause");
        break;
    }
}

static void bm_generate_mesh_id(bm_mesh_param *param, point_s dst_mesh_tbl[][4])
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
                point_s l1, r1, l2, r2;

                l1.x = src_x;
                l1.y = y;
                r1.x = dst_x;
                r1.y = y + NUMBER_Y_LINE_A_SUBTILE;
                mesh_id[id_idx++] = MESH_ID_FSP;
                for (int m = 0; m < param->mesh_num; ++m) {
                    // To reduce time consumption
                    // assumption: dst mesh is ordered (left->right, up->down)
                    if (dst_mesh_tbl[m][3].y < l1.y) continue;
                    if (dst_mesh_tbl[m][0].y > r1.y) break;

                    l2.x = dst_mesh_tbl[m][0].x;
                    l2.y = dst_mesh_tbl[m][0].y;
                    r2.x = dst_mesh_tbl[m][3].x;
                    r2.y = dst_mesh_tbl[m][3].y;
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

static void bm_load_region_config(bm_fisheye_attr* fisheye_config, bm_fisheye_region_attr* fisheye_region)
{
    // to make sure parameters aligned to frame ratio
    double width_sec = fisheye_config->OutW_disp / 40;
    double height_sec = fisheye_config->OutH_disp / 40;

    // load default settings
    if (fisheye_config->UsageMode == MODE_02_1O4R )
    {
        fisheye_region[0].RegionValid = 1;
        fisheye_region[0].MeshVer = 16;
        fisheye_region[0].MeshHor = 16;
        fisheye_region[0].ViewMode = PROJECTION_REGION;
        fisheye_region[0].ThetaX = 0.33 * M_PI;
        fisheye_region[0].ThetaZ = 0;
        fisheye_region[0].ThetaY = 0;
        fisheye_region[0].ZoomH = 2048;                 // zooming factor for horizontal:   0 ~ 4095 to control view
        fisheye_region[0].ZoomV = 3072;                 // zooming factor for vertical:     0 ~ 4095 to control view
        fisheye_region[0].Pan  = UI_CTRL_VALUE_CENTER;  // center = 180, value = 0 ~ 360    => value = +- 180 degrees
        fisheye_region[0].Tilt = UI_CTRL_VALUE_CENTER;  // center = 180, value = 0 ~ 360    => value = +-30 degrees
        fisheye_region[0].OutW = (width_sec * 15);
        fisheye_region[0].OutH = (height_sec * 20);
        fisheye_region[0].OutX = 0;
        fisheye_region[0].OutY = 0;

        fisheye_region[1].RegionValid = 1;
        fisheye_region[1].MeshVer = 16;
        fisheye_region[1].MeshHor = 16;
        fisheye_region[1].ViewMode = PROJECTION_REGION;
        fisheye_region[1].ThetaX = 0.33 * M_PI;
        fisheye_region[1].ThetaZ = 0.5 * M_PI;
        fisheye_region[1].ThetaY = 0;
        fisheye_region[1].ZoomH = 2048;
        fisheye_region[1].ZoomV = 3072;
        fisheye_region[1].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[1].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[1].OutW = (width_sec * 15);
        fisheye_region[1].OutH = (height_sec * 20);
        fisheye_region[1].OutX = (width_sec * 15);
        fisheye_region[1].OutY = (height_sec * 0);

        fisheye_region[2].RegionValid = 1;
        fisheye_region[2].MeshVer = 16;
        fisheye_region[2].MeshHor = 16;
        fisheye_region[2].ViewMode = PROJECTION_REGION;
        fisheye_region[2].ThetaX = 0.33 * M_PI;
        fisheye_region[2].ThetaZ = M_PI;
        fisheye_region[2].ThetaY = 0;
        fisheye_region[2].ZoomH = 2048;
        fisheye_region[2].ZoomV = 3072;
        fisheye_region[2].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[2].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[2].OutW = (width_sec * 15);
        fisheye_region[2].OutH = (height_sec * 20);
        fisheye_region[2].OutX = (width_sec * 0);
        fisheye_region[2].OutY = (height_sec * 20);

        fisheye_region[3].RegionValid = 1;
        fisheye_region[3].MeshVer = 16;
        fisheye_region[3].MeshHor = 16;
        fisheye_region[3].ViewMode = PROJECTION_REGION;
        fisheye_region[3].ThetaX = 0.33 * M_PI;
        fisheye_region[3].ThetaZ = 1.5 * M_PI;
        fisheye_region[3].ThetaY = 0;
        fisheye_region[3].ZoomH = 2048;
        fisheye_region[3].ZoomV = 3072;
        fisheye_region[3].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[3].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[3].OutW = (width_sec * 15);
        fisheye_region[3].OutH = (height_sec * 20);
        fisheye_region[3].OutX = (width_sec * 15);
        fisheye_region[3].OutY = (height_sec * 20);

        fisheye_region[4].RegionValid = 0;
    }
    else if (fisheye_config->UsageMode == MODE_03_4R)
    {
        fisheye_region[0].RegionValid = 1;
        fisheye_region[0].MeshVer = 16;
        fisheye_region[0].MeshHor = 16;
        fisheye_region[0].ViewMode = PROJECTION_REGION;
        fisheye_region[0].ThetaX = 0.33 * M_PI;
        fisheye_region[0].ThetaZ = 0;
        fisheye_region[0].ThetaY = 0;
        fisheye_region[0].ZoomH = 1024;                 // zooming factor for horizontal:0 ~ 4095 to control view
        fisheye_region[0].ZoomV = 3072;                 // zooming factor for vertical:0 ~ 4095 to control view
        fisheye_region[0].Pan = UI_CTRL_VALUE_CENTER;   // center = 180, value = 0 ~ 360 => value = +- 180 degrees
        fisheye_region[0].Tilt = UI_CTRL_VALUE_CENTER;  // center = 180, value = 0 ~ 360 => value = +-30 degrees
        fisheye_region[0].OutW = (width_sec * 20);
        fisheye_region[0].OutH = (height_sec * 20);
        fisheye_region[0].OutX = 0;//(width_sec * 2);
        fisheye_region[0].OutY = 0;//(height_sec * 2);

        fisheye_region[1].RegionValid = 1;
        fisheye_region[1].MeshVer = 16;
        fisheye_region[1].MeshHor = 16;
        fisheye_region[1].ViewMode = PROJECTION_REGION;
        fisheye_region[1].ThetaX = 0.33 * M_PI;
        fisheye_region[1].ThetaZ = 0.5 * M_PI;
        fisheye_region[1].ThetaY = 0;
        fisheye_region[1].ZoomH = 1024;
        fisheye_region[1].ZoomV = 3072;
        fisheye_region[1].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[1].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[1].OutW = (width_sec * 20);
        fisheye_region[1].OutH = (height_sec * 20);
        fisheye_region[1].OutX = (width_sec * 20);
        fisheye_region[1].OutY = (height_sec * 0);

        fisheye_region[2].RegionValid = 1;
        fisheye_region[2].MeshVer = 16;
        fisheye_region[2].MeshHor = 16;
        fisheye_region[2].ViewMode = PROJECTION_REGION;
        fisheye_region[2].ThetaX = 0.33 * M_PI;
        fisheye_region[2].ThetaZ = M_PI;
        fisheye_region[2].ThetaY = 0;
        fisheye_region[2].ZoomH = 1024;
        fisheye_region[2].ZoomV = 3072;
        fisheye_region[2].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[2].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[2].OutW = (width_sec * 20);
        fisheye_region[2].OutH = (height_sec * 20);
        fisheye_region[2].OutX = (width_sec * 0);
        fisheye_region[2].OutY = (height_sec * 20);

        fisheye_region[3].RegionValid = 1;
        fisheye_region[3].MeshVer = 16;
        fisheye_region[3].MeshHor = 16;
        fisheye_region[3].ViewMode = PROJECTION_REGION;
        fisheye_region[3].ThetaX = 0.33 * M_PI;
        fisheye_region[3].ThetaZ = 1.5 * M_PI;
        fisheye_region[3].ThetaY = 0;
        fisheye_region[3].ZoomH = 1024;
        fisheye_region[3].ZoomV = 3072;
        fisheye_region[3].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[3].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[3].OutW = (width_sec * 20);
        fisheye_region[3].OutH = (height_sec * 20);
        fisheye_region[3].OutX = (width_sec * 20);
        fisheye_region[3].OutY = (height_sec * 20);

        fisheye_region[4].RegionValid = 0;
    }
    else if (fisheye_config->UsageMode == MODE_04_1P2R)
    {
        // Region #1 => Panorama 180
        fisheye_region[0].RegionValid = 1;
        fisheye_region[0].MeshVer = 16;
        fisheye_region[0].MeshHor = 16;
        fisheye_region[0].ViewMode = PROJECTION_PANORAMA_180;
        //fisheye_region[0].ThetaX = 0;
        //fisheye_region[0].ThetaY = 0;
        //fisheye_region[0].ThetaZ = 0;
        fisheye_region[0].ZoomH = 4096;                 // value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
        fisheye_region[0].ZoomV = 1920;                 // value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90
        fisheye_region[0].Pan   = UI_CTRL_VALUE_CENTER;         // value range = 0 ~ 360, => -180 ~ 0 ~ +180
        fisheye_region[0].Tilt  = UI_CTRL_VALUE_CENTER;         // value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
        fisheye_region[0].OutW  = (width_sec * 40);
        fisheye_region[0].OutH  = (height_sec * 22);
        fisheye_region[0].OutX  = 0;                    //(width_sec * 1);
        fisheye_region[0].OutY  = 0;                    //height_sec * 1);
        //fisheye_region[0].InRadius = 50;              // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //fisheye_region[0].OutRadius = 450;            // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //fisheye_region[0].PanEnd = 180;

        fisheye_region[1].RegionValid = 1;
        fisheye_region[1].MeshVer = 8;
        fisheye_region[1].MeshHor = 8;
        fisheye_region[1].ViewMode = PROJECTION_REGION;
        fisheye_region[1].ThetaX = M_PI / 4;
        fisheye_region[1].ThetaZ = M_PI / 2;
        fisheye_region[1].ThetaY = 0;
        fisheye_region[1].ZoomH = 2048;
        fisheye_region[1].ZoomV = 2048;
        fisheye_region[1].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[1].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[1].OutW = (width_sec * 20);
        fisheye_region[1].OutH = (height_sec * 18);
        fisheye_region[1].OutX = (width_sec * 0);
        fisheye_region[1].OutY = (height_sec * 22);

        fisheye_region[2].RegionValid = 1;
        fisheye_region[2].MeshVer = 8;
        fisheye_region[2].MeshHor = 8;
        fisheye_region[2].ViewMode = PROJECTION_REGION;
        fisheye_region[2].ThetaX = M_PI / 4;
        fisheye_region[2].ThetaZ = M_PI;
        fisheye_region[2].ThetaY = 0;
        fisheye_region[2].ZoomH = 2048;
        fisheye_region[2].ZoomV = 2048;
        fisheye_region[2].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[2].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[2].OutW = (width_sec * 20);
        fisheye_region[2].OutH = (height_sec * 18);
        fisheye_region[2].OutX = (width_sec * 20);
        fisheye_region[2].OutY = (height_sec * 22);

        fisheye_region[3].RegionValid = 0;
        fisheye_region[4].RegionValid = 0;
    }
    else if (fisheye_config->UsageMode == MODE_05_1P2R)
    {
        // Region #1 => Panorama 180
        fisheye_region[0].RegionValid = 1;
        fisheye_region[0].MeshVer = 16;
        fisheye_region[0].MeshHor = 16;
        fisheye_region[0].ViewMode = PROJECTION_PANORAMA_180;
        //fisheye_region[0].ThetaX = 0;
        //fisheye_region[0].ThetaY = 0;
        //fisheye_region[0].ThetaZ = 0;
        fisheye_region[0].ZoomH = 3000;                 // value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
        fisheye_region[0].ZoomV = 2048;                 // value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90
        fisheye_region[0].Pan  = UI_CTRL_VALUE_CENTER;          // value range = 0 ~ 360, => -180 ~ 0 ~ +180
        fisheye_region[0].Tilt = UI_CTRL_VALUE_CENTER;          // value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
        fisheye_region[0].OutW = (width_sec * 27 );
        fisheye_region[0].OutH = (height_sec * 40);
        fisheye_region[0].OutX = 0;                 //(width_sec * 1);
        fisheye_region[0].OutY = 0;                 //height_sec * 1);
        //fisheye_region[0].InRadius = 50;          // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //fisheye_region[0].OutRadius = 450;        // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //fisheye_region[0].PanEnd = 180;

        fisheye_region[1].RegionValid = 1;
        fisheye_region[1].MeshVer = 8;
        fisheye_region[1].MeshHor = 8;
        fisheye_region[1].ViewMode = PROJECTION_REGION;
        fisheye_region[1].ThetaX = M_PI / 4;
        fisheye_region[1].ThetaZ = M_PI / 2;
        fisheye_region[1].ThetaY = 0;
        fisheye_region[1].ZoomH = 2048;
        fisheye_region[1].ZoomV = 2048;
        fisheye_region[1].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[1].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[1].OutW = (width_sec * 13);
        fisheye_region[1].OutH = (height_sec * 20);
        fisheye_region[1].OutX = (width_sec * 27);
        fisheye_region[1].OutY = (height_sec * 0);

        fisheye_region[2].RegionValid = 1;
        fisheye_region[2].MeshVer = 8;
        fisheye_region[2].MeshHor = 8;
        fisheye_region[2].ViewMode = PROJECTION_REGION;
        fisheye_region[2].ThetaX = M_PI / 4;
        fisheye_region[2].ThetaZ = M_PI;
        fisheye_region[2].ThetaY = 0;
        fisheye_region[2].ZoomH = 2048;
        fisheye_region[2].ZoomV = 2048;
        fisheye_region[2].Pan = UI_CTRL_VALUE_CENTER;           // theta-X
        fisheye_region[2].Tilt = UI_CTRL_VALUE_CENTER;          // theta-Z
        fisheye_region[2].OutW = (width_sec * 13);
        fisheye_region[2].OutH = (height_sec * 20);
        fisheye_region[2].OutX = (width_sec * 27);
        fisheye_region[2].OutY = (height_sec * 20);

        fisheye_region[3].RegionValid = 0;
        fisheye_region[4].RegionValid = 0;


    }
    else if (fisheye_config->UsageMode == MODE_06_1P)
    {
        // Region #1 => Panorama 180
        fisheye_region[0].RegionValid = 1;
        fisheye_region[0].MeshVer = 30;
        fisheye_region[0].MeshHor = 30;
        fisheye_region[0].ViewMode = PROJECTION_PANORAMA_180;
        //fisheye_region[0].ThetaX = 0;
        //fisheye_region[0].ThetaY = 0;
        //fisheye_region[0].ThetaZ = 0;
        fisheye_region[0].ZoomH = 4096; // value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
        fisheye_region[0].ZoomV = 2800; // value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90
        fisheye_region[0].Pan  = UI_CTRL_VALUE_CENTER;  // value range = 0 ~ 360, => -180 ~ 0 ~ +180
        fisheye_region[0].Tilt = UI_CTRL_VALUE_CENTER;  // value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
        fisheye_region[0].OutW = (width_sec * 40 );
        fisheye_region[0].OutH = (height_sec * 40);
        fisheye_region[0].OutX = 0; //(width_sec * 1);
        fisheye_region[0].OutY = 0; //height_sec * 1);
        //fisheye_region[0].InRadius = 50;      // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //fisheye_region[0].OutRadius = 450;    // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //fisheye_region[0].PanEnd = 180;

        fisheye_region[1].RegionValid = 0;
        fisheye_region[2].RegionValid = 0;
        fisheye_region[3].RegionValid = 0;
        fisheye_region[4].RegionValid = 0;
    } else if (fisheye_config->UsageMode == MODE_07_2P ) {
        //_Panorama360View2;
        fisheye_region[0].RegionValid = 1;
        fisheye_region[0].MeshVer = 16;
        fisheye_region[0].MeshHor = 16;
        fisheye_region[0].ViewMode = PROJECTION_PANORAMA_360;
        fisheye_region[0].ThetaX = M_PI / 4;
        fisheye_region[0].ThetaZ = 0;
        fisheye_region[0].ThetaY = 0;
        //fisheye_region[0].ZoomH = 4095;               // Not Used in Panorama 360 Mode.
        fisheye_region[0].ZoomV = 4095;                 // To ZoomIn OutRadius
        fisheye_region[0].Pan = 40;                     // for panorama 360 => Pan is the label start position angle ( in degrees
        fisheye_region[0].Tilt = 200;                   // to add shift offset vertical angle.
        fisheye_region[0].OutW = (width_sec * 40);
        fisheye_region[0].OutH = (height_sec * 20);
        fisheye_region[0].OutX = (width_sec * 0);
        fisheye_region[0].OutY = (height_sec * 0);
        fisheye_region[0].InRadius = 300;               // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        fisheye_region[0].OutRadius = 3712;             // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        fisheye_region[0].PanEnd = 220;

        fisheye_region[1].RegionValid = 1;
        fisheye_region[1].MeshVer = 16;
        fisheye_region[1].MeshHor = 16;
        fisheye_region[1].ViewMode = PROJECTION_PANORAMA_360;
        fisheye_region[1].ThetaX = M_PI / 4;
        fisheye_region[1].ThetaZ = 0;
        fisheye_region[1].ThetaY = 0;
        //fisheye_region[1].ZoomH = 4095;               // Not Used in Panorama 360 Mode.
        fisheye_region[1].ZoomV = 4095;                 // To ZoomIn OutRadius
        fisheye_region[1].Pan  = 220;                   // for panorama 360 => Pan is the label start position angle ( in degrees
        fisheye_region[1].Tilt = 200;                   // to add shift offset vertical angle.
        fisheye_region[1].OutW = (width_sec * 40);
        fisheye_region[1].OutH = (height_sec * 20);
        fisheye_region[1].OutX = (width_sec * 0);
        fisheye_region[1].OutY = (height_sec * 20);
        fisheye_region[1].InRadius = 300;               // a ratio to represent OutRadius length. 1 = full origina redius.  value/512 is the value.
        fisheye_region[1].OutRadius = 3712;             // a ratio to represent OutRadius length. 1 = full origina redius.  value/512 is the value.
        fisheye_region[1].PanEnd = 40;                  //

        fisheye_region[2].RegionValid = 0;
        fisheye_region[3].RegionValid = 0;
        fisheye_region[4].RegionValid = 0;
    } else if (fisheye_config->UsageMode == MODE_PANORAMA_180) {
        fisheye_region[0].RegionValid = 1;
        fisheye_region[0].MeshVer = 16;
        fisheye_region[0].MeshHor = 16;
        fisheye_region[0].ViewMode = PROJECTION_PANORAMA_180;
        //fisheye_region[0].ThetaX = M_PI / 4;
        //fisheye_region[0].ThetaZ = 0;
        //fisheye_region[0].ThetaY = 0;
        fisheye_region[0].ZoomH = 4096;                 // Not Used in Panorama 360 Mode.
        fisheye_region[0].ZoomV = 4096;                 // To ZoomIn OutRadius
        fisheye_region[0].Pan = 180;                    // for panorama 360 => Pan is the label start position angle ( in degrees
        fisheye_region[0].Tilt = 180;                   // to add shift offset vertical angle.
        fisheye_region[0].OutW = (width_sec * 40);
        fisheye_region[0].OutH = (height_sec * 40);
        fisheye_region[0].OutX = 0;                 //(width_sec * 1);
        fisheye_region[0].OutY = 0;                 //height_sec * 1);
        //fisheye_region[0].InRadius = 50;          // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //fisheye_region[0].OutRadius = 450;        // a ratio to represent OutRadius length. 1 => full origina redius. value/512 is the value.
        //fisheye_region[0].PanEnd = 180;

        fisheye_region[1].RegionValid = 0;
        fisheye_region[2].RegionValid = 0;
        fisheye_region[3].RegionValid = 0;
        fisheye_region[4].RegionValid = 0;
    } else if (fisheye_config->UsageMode == MODE_PANORAMA_360) {
        //_Panorama360View2;
        fisheye_region[0].RegionValid = 1;
        fisheye_region[0].MeshVer = 8;
        fisheye_region[0].MeshHor = 8;
        fisheye_region[0].ViewMode = PROJECTION_PANORAMA_360;
        fisheye_region[0].ThetaX = M_PI / 4;
        fisheye_region[0].ThetaZ = 0;
        fisheye_region[0].ThetaY = 0;
        //fisheye_region[0].ZoomH = 4095;               // Not Used in Panorama 360 Mode.
        fisheye_region[0].ZoomV = 4095;                 // To ZoomIn OutRadius
        fisheye_region[0].Pan = 0;                      // for panorama 360 => Pan is the label start position angle ( in degrees
        fisheye_region[0].Tilt = 180;                   // to add shift offset vertical angle.
        fisheye_region[0].OutW = (width_sec * 40);
        fisheye_region[0].OutH = (height_sec * 40) * 4 / 6;
        fisheye_region[0].OutX = 0;                     //(width_sec * 1);
        fisheye_region[0].OutY = (height_sec * 40) / 6;         //(height_sec * 1);
        fisheye_region[0].InRadius = 800;               // a ratio to represent OutRadius length. 1 => full origina redius. value = 0 ~ 4095,
        fisheye_region[0].OutRadius = 4096;             // a ratio to represent OutRadius length. 1 => full origina redius. value = 0 ~ 4095
        fisheye_region[0].PanEnd = 359;

        fisheye_region[1].RegionValid = 0;
        fisheye_region[2].RegionValid = 0;
        fisheye_region[3].RegionValid = 0;
        fisheye_region[4].RegionValid = 0;
    } else if (fisheye_config->UsageMode == MODE_STEREO_FIT) {
        fisheye_region[0].RegionValid   = 1;
        fisheye_region[0].MeshVer      = 128;
        fisheye_region[0].MeshHor      = 128;
        fisheye_region[0].ViewMode      = PROJECTION_STEREO_FIT;
        fisheye_region[0].ThetaX      = M_PI / 4;
        fisheye_region[0].ThetaZ      = 0;
        fisheye_region[0].ThetaY      = 0;
        //fisheye_region[0].ZoomH = 4095;            // Not Used in Panorama 360 Mode.
        fisheye_region[0].ZoomV         = 4095;               // To ZoomIn OutRadius
        fisheye_region[0].Pan         = 0;                  // for panorama 360 => Pan is the label start position angle ( in degrees
        fisheye_region[0].Tilt         = 180;               // to add shift offset vertical angle.
        fisheye_region[0].OutW         = fisheye_config->OutW_disp;
        fisheye_region[0].OutH         = fisheye_config->OutH_disp;
        fisheye_region[0].OutX         = 0;                  //(width_sec * 1);
        fisheye_region[0].OutY         = 0;                  //(height_sec * 1);
        fisheye_region[0].InRadius      = 0;            // a ratio to represent OutRadius lenght. 1 => full origina redius.   value = 0 ~ 4095,
        fisheye_region[0].OutRadius      = 4095;            // a ratio to represent OutRadius lenght. 1 => full origina redius.   value = 0 ~ 4095
        fisheye_region[0].PanEnd      = 359;
        //
        fisheye_region[1].RegionValid = 0;
        fisheye_region[2].RegionValid = 0;
        fisheye_region[3].RegionValid = 0;
        fisheye_region[4].RegionValid = 0;
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Not done yet!\n");
        //system("pause");
    }
}

static void bm_image_to_cvi_frame(bm_image *image, pixel_format_e enPixelFormat, video_frame_info_s *stVideoFrame)
{
	int planar_to_seperate = 0;
	if((image->image_format == FORMAT_RGB_PLANAR || image->image_format == FORMAT_BGR_PLANAR))
		planar_to_seperate = 1;

	stVideoFrame->video_frame.compress_mode = COMPRESS_MODE_NONE;
	stVideoFrame->video_frame.pixel_format = enPixelFormat;
	stVideoFrame->video_frame.video_format = VIDEO_FORMAT_LINEAR;
	stVideoFrame->video_frame.width = image->width;
	stVideoFrame->video_frame.height = image->height;
	if((!is_full_image(image->image_format)) || image->image_format == FORMAT_COMPRESSED)
		stVideoFrame->video_frame.width = stVideoFrame->video_frame.width & (~0x1);
	if(is_yuv420_image(image->image_format) || image->image_format == FORMAT_COMPRESSED)
		stVideoFrame->video_frame.height = stVideoFrame->video_frame.height & (~0x1);
	for (int i = 0; i < image->image_private->plane_num; ++i) {
		if((i == 3) && (image->image_format == FORMAT_COMPRESSED)){
			stVideoFrame->video_frame.ext_phy_addr = image->image_private->data[i].u.device.device_addr;
			stVideoFrame->video_frame.ext_length = image->image_private->memory_layout[i].size;
			stVideoFrame->video_frame.ext_virt_addr = (unsigned char*)image->image_private->data[i].u.system.system_addr;
			stVideoFrame->video_frame.compress_mode = COMPRESS_MODE_FRAME;
		} else if((i > 0) && (image->image_format == FORMAT_COMPRESSED)){
			continue;
		} else {
			stVideoFrame->video_frame.stride[i] = image->image_private->memory_layout[i].pitch_stride;
			stVideoFrame->video_frame.length[i] = image->image_private->memory_layout[i].size;
			stVideoFrame->video_frame.phyaddr[i] = image->image_private->data[i].u.device.device_addr;
			stVideoFrame->video_frame.viraddr[i] = (unsigned char*)image->image_private->data[i].u.system.system_addr;
		}
	}

	if(planar_to_seperate){
		for (int i = 1; i < 3; ++i) {
			stVideoFrame->video_frame.stride[i] = image->image_private->memory_layout[0].pitch_stride;
			stVideoFrame->video_frame.length[i] = image->image_private->memory_layout[0].pitch_stride * image->height;
			stVideoFrame->video_frame.phyaddr[i] = image->image_private->data[0].u.device.device_addr + image->image_private->memory_layout[0].pitch_stride * image->height * i;
		}
	}

	if(image->image_format == FORMAT_COMPRESSED){
		for (int i = 1, j = 2; i < 3; ++i, --j) {
			stVideoFrame->video_frame.stride[i] = image->image_private->memory_layout[j].pitch_stride;
			stVideoFrame->video_frame.length[i] = image->image_private->memory_layout[j].size;
			stVideoFrame->video_frame.phyaddr[i] = image->image_private->data[j].u.device.device_addr;
			stVideoFrame->video_frame.viraddr[i] = (unsigned char*)image->image_private->data[j].u.system.system_addr;
		}
	}

	if((enPixelFormat == PIXEL_FORMAT_UINT8_C3_PLANAR || enPixelFormat == PIXEL_FORMAT_INT8_C3_PLANAR ||
		enPixelFormat == PIXEL_FORMAT_BF16_C3_PLANAR || enPixelFormat == PIXEL_FORMAT_FP16_C3_PLANAR || enPixelFormat == PIXEL_FORMAT_FP32_C3_PLANAR) &&
		(image->image_format == FORMAT_BGR_PLANAR || image->image_format == FORMAT_BGRP_SEPARATE)){
		stVideoFrame->video_frame.stride[0] = stVideoFrame->video_frame.stride[2];
		stVideoFrame->video_frame.length[0] = stVideoFrame->video_frame.length[2];
		stVideoFrame->video_frame.phyaddr[0] = stVideoFrame->video_frame.phyaddr[2];
		stVideoFrame->video_frame.stride[2] = image->image_private->memory_layout[0].pitch_stride;
		stVideoFrame->video_frame.length[2] = image->image_private->memory_layout[0].pitch_stride * image->height;
		stVideoFrame->video_frame.phyaddr[2] = image->image_private->data[0].u.device.device_addr;
	}

	stVideoFrame->video_frame.align = 1;
	stVideoFrame->video_frame.time_ref = 0;
	stVideoFrame->video_frame.pts = 0;
	stVideoFrame->video_frame.dynamic_range = DYNAMIC_RANGE_SDR8;
	stVideoFrame->pool_id = 0xffff;
}

static void dwa_mesh_gen_get_size(size_s in_size, size_s out_size, u32 *mesh_id_size, u32 *mesh_tbl_size)
{
    if (!mesh_id_size || !mesh_tbl_size)
        return;

    (void)in_size;
    (void)out_size;

    // CVI_GDC_MESH_SIZE_FISHEYE
    *mesh_tbl_size = 0x100000;
    *mesh_id_size = 0x50000;
}

static unsigned char get_idle_tsk_mesh(bm_handle_t handle)
{
    unsigned char i = DWA_MAX_TSK_MESH;

    for (i = 0; i < DWA_MAX_TSK_MESH; i++) {
        if (strcmp(dwa_tskMesh[i].Name, "") == 0 && !dwa_tskMesh[i].mem.u.device.device_addr && !dwa_tskMesh[i].mem.u.system.system_addr)
            break;
    }

    if(i == DWA_MAX_TSK_MESH){
        bm_device_mem_t mem;
        uncommonly_used_idx++;
        uncommonly_used_idx = uncommonly_used_idx % DWA_MAX_TSK_MESH;
        mem = dwa_tskMesh[uncommonly_used_idx].mem;
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "device_addr = %lx, dmabuf_fd = %d, size = %d\n", mem.u.device.device_addr, mem.u.device.dmabuf_fd, mem.size);
        bm_free_device(handle, mem);
        return uncommonly_used_idx;
    }
    return i;
}

static void bm_ldc_attr_map(const ldc_attr_s *pstLDCAttr,
                            size_s out_size,
                            bm_fisheye_attr *fisheye_config,
                            bm_fisheye_region_attr *fisheye_region)
{
    fisheye_config->MntMode = 0;
    fisheye_config->OutW_disp = out_size.width;
    fisheye_config->OutH_disp = out_size.height;
    fisheye_config->InCenterX = out_size.width >> 1;
    fisheye_config->InCenterY = out_size.height >> 1;
    // TODO: how to handl radius
    fisheye_config->InRadius = MIN(fisheye_config->InCenterX, fisheye_config->InCenterY);
    fisheye_config->FStrength = 0;
    fisheye_config->TCoef  = 0;

    fisheye_config->RgnNum = 1;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "OutW_disp(%d) OutH_disp(%d)\n"
        , fisheye_config->OutW_disp, fisheye_config->OutH_disp);

    for (int i = 0; i < MAX_REGION_NUM; ++i)
        fisheye_region[i].RegionValid = 0;

    for (int i = 0; i < fisheye_config->RgnNum; ++i) {
        fisheye_region[i].RegionValid = 1;

        //fisheye_region[i].ZoomH = pstLDCAttr->bAspect;
        fisheye_region[i].ZoomV = pstLDCAttr->aspect;
        fisheye_region[i].Pan = pstLDCAttr->xy_ratio;
        fisheye_region[i].PanEnd = pstLDCAttr->distortion_ratio;
        fisheye_region[i].Tilt = 180;
        fisheye_region[i].OutW = fisheye_config->OutW_disp;
        fisheye_region[i].OutH = fisheye_config->OutH_disp;
        fisheye_region[i].OutX = 0;
        fisheye_region[i].OutY = 0;
        fisheye_region[i].InRadius = pstLDCAttr->center_x_offset;
        fisheye_region[i].OutRadius = pstLDCAttr->center_y_offset;
        fisheye_region[i].MeshVer = 16;
        fisheye_region[i].MeshHor = 16;
        if (pstLDCAttr->grid_info_attr.enable) //use new ldc c models
            fisheye_region[i].ViewMode = PROJECTION_STEREO_FIT;
        else
            fisheye_region[i].ViewMode = PROJECTION_LDC;
        fisheye_region[i].ThetaX = pstLDCAttr->x_ratio;
        fisheye_region[i].ThetaZ = pstLDCAttr->y_ratio;
        fisheye_region[i].ThetaY = 0;

        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "Region(%d) ViewMode(%d) MeshVer(%d) MeshHor(%d)\n"
            , i, fisheye_region[i].ViewMode, fisheye_region[i].MeshVer, fisheye_region[i].MeshHor);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "aspect(%d) XYRatio(%d) DistortionRatio(%d)\n"
            , (bool)fisheye_region[i].ZoomV, fisheye_region[i].Pan, fisheye_region[i].PanEnd);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "XRatio(%d) XYRatio(%d)\n"
            , (int)fisheye_region[i].ThetaX, (int)fisheye_region[i].ThetaZ);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "CenterXOffset(%lf) CenterYOffset(%lf) Rect(%d %d %d %d)\n"
            , fisheye_region[i].InRadius, fisheye_region[i].OutRadius
            , fisheye_region[i].OutX, fisheye_region[i].OutY
            , fisheye_region[i].OutW, fisheye_region[i].OutH);
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
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "frm_end_tag in [%#x] pos, will put to next 2 lines(32byte)\n", mesh_id_pos_lines);
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
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "X_TILE_NUMBER(%d) Y_TILE_NUMBER(%d)\n", X_TILE_NUMBER, Y_TILE_NUMBER);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "NUM_X_LINE_A_TILE(%d) NUM_Y_LINE_A_TILE(%d)\n", NUM_X_LINE_A_TILE, NUM_Y_LINE_A_TILE);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "NUM_Y_LINE_A_SUBTILE(%d)\n", NUM_Y_LINE_A_SUBTILE);

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
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "view_h = %d\n", view_h);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "view_w = %d\n", view_w);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, " RMATRIX[0][0] = %f\n", RMATRIX[0][0]);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, " RMATRIX[0][1] = %f\n", RMATRIX[0][1]);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, " RMATRIX[1][0] = %f\n", RMATRIX[1][0]);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, " RMATRIX[1][1] = %f\n", RMATRIX[1][1]);


    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "xin = %f\n", xin);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "yin = %f\n", yin);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "xout = %f\n", xout);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "yout = %f\n", yout);
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

void bm_get_frame_mesh_list(bm_fisheye_attr* fisheye_config, bm_fisheye_region_attr* fisheye_region)
{
    // pack all regions' mesh info, including src & dst.
    int rgnNum = fisheye_config->RgnNum;
    int meshNumRgn;
    int frameMeshIdx = 0;
    int rotate_index = fisheye_config->rotate_index;
    int view_w = fisheye_config->OutW_disp;
    int view_h = fisheye_config->OutH_disp;

    for (int i = 0; i < rgnNum; i++) {
        if (fisheye_region[i].RegionValid == 1) {
            meshNumRgn = (fisheye_region[i].MeshHor * fisheye_region[i].MeshVer);

            // go through each region loop
            for (int meshidx = 0; meshidx < meshNumRgn; meshidx++) {
                // each mesh has 4 knots
                for (int knotidx = 0; knotidx < 4; knotidx++) {
                    // do rotaion:
                    double x_src = fisheye_region[i].SrcRgnMeshInfo[meshidx].knot[knotidx].xcor;
                    double y_src = fisheye_region[i].SrcRgnMeshInfo[meshidx].knot[knotidx].ycor;
                    double x_dst = fisheye_region[i].DstRgnMeshInfo[meshidx].knot[knotidx].xcor;
                    double y_dst = fisheye_region[i].DstRgnMeshInfo[meshidx].knot[knotidx].ycor;

                    double x_dst_out, y_dst_out;
                    _do_mesh_rotate(rotate_index, view_h, view_w, x_dst, y_dst, &x_dst_out, &y_dst_out);

                    fisheye_config->DstRgnMeshInfo[frameMeshIdx].knot[knotidx].xcor = x_dst_out; //fisheye_region[i].DstRgnMeshInfo[meshidx].knot[knotidx].xcor;
                    fisheye_config->DstRgnMeshInfo[frameMeshIdx].knot[knotidx].ycor = y_dst_out; //fisheye_region[i].DstRgnMeshInfo[meshidx].knot[knotidx].ycor;
                    fisheye_config->SrcRgnMeshInfo[frameMeshIdx].knot[knotidx].xcor = x_src; //fisheye_region[i].SrcRgnMeshInfo[meshidx].knot[knotidx].xcor;
                    fisheye_config->SrcRgnMeshInfo[frameMeshIdx].knot[knotidx].ycor = y_src; //fisheye_region[i].SrcRgnMeshInfo[meshidx].knot[knotidx].ycor;
                }
                frameMeshIdx += 1;
            }
        }
    }

    // update mesh index number
    fisheye_config->TotalMeshNum = frameMeshIdx;
}

static void _ldc_view(bm_fisheye_region_attr* fisheye_region, int rgn_idx, double x0, double y0)
{
    if (fisheye_region[rgn_idx].ViewMode != PROJECTION_LDC)
        return;

    int view_w = fisheye_region[rgn_idx].OutW;
    int view_h = fisheye_region[rgn_idx].OutH;
    int mesh_horcnt = fisheye_region[rgn_idx].MeshHor;
    int mesh_vercnt = fisheye_region[rgn_idx].MeshVer;


    // register:
    bool aspect    = (bool)fisheye_region[0].ZoomV;
    int XYRatio     = minmax(fisheye_region[0].Pan, 0, 100);
    int XRatio      = minmax(fisheye_region[0].ThetaX, 0, 100);
    int YRatio      = minmax(fisheye_region[0].ThetaZ, 0, 100);
    int CenterXOffset   = minmax(fisheye_region[0].InRadius, -511, 511);
    int CenterYOffset   = minmax(fisheye_region[0].OutRadius, -511, 511);
    int DistortionRatio = minmax(fisheye_region[0].PanEnd, -300, 500);

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

    double Aspect_gainX = MAX(ud_gain, lr_gain);
    double Aspect_gainY = MAX(ud_gain, lr_gain);


    bm_vector2d dist2d;

    // go through all meshes in thus regions
    for (int i = 0; i < (mesh_horcnt * mesh_vercnt); i++) {
        // each mesh has 4 knots
        for (int knotidx = 0; knotidx < 4; knotidx++) {
            // display window center locate @ (0,0), mesh info shife back to center O.
            // for each region, rollback center is (0,0)
            double x = fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor
                - fisheye_region[rgn_idx].OutX - fisheye_region[rgn_idx].OutW / 2;
            double y = fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor
                - fisheye_region[rgn_idx].OutY - fisheye_region[rgn_idx].OutH / 2;

#if OFFSET_SCALE // new method
            x = minmax(x, (-view_w+ CenterXOffsetScale) / 2 + 1, (view_w+ CenterXOffsetScale) / 2 - 1);
            y = minmax(y, (-view_h+ CenterYOffsetScale) / 2 + 1, (view_h+ CenterYOffsetScale) / 2 - 1);
#else
            x = minmax(x - CenterXOffset, -view_w / 2 + 1, view_w / 2 - 1);
            y = minmax(y - CenterYOffset, -view_h / 2 + 1, view_h / 2 - 1);
#endif

            x = x / Aspect_gainX;
            y = y / Aspect_gainY;

            if (aspect == true) {
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
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + new_x0;
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + new_y0;
#else
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0 + CenterXOffset;
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0 + CenterYOffset;
#endif
        }
    }
}


static void gen_rot_matrix3d_yxz(bm_rotMatrix3d* mat, bm_fisheye_region_attr* fisheye_region_idx)
{
    // This Rotation Matrix Order = R = Rz*Rx*Ry
    // rotation order = θy -> θx -> θz
    //_LOAD_REGION_CONFIG;
    // initital position
    double tmp_phy_x = fisheye_region_idx->ThetaX;  //phy_x;
    double tmp_phy_y = fisheye_region_idx->ThetaY;  //phy_y;    // Not Used for Now.
    double tmp_phy_z = fisheye_region_idx->ThetaZ;  //phy_z;

    //_LOAD_REGION_CONFIG(fisheye_config, fisheye_region);
    // UI Control
    double ctrl_tilt, ctrl_pan;// = minmax(fisheye_region_idx[rgn_idx].Tilt - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);
    //double ctrl_pan  = minmax(fisheye_region_idx[rgn_idx].Pan - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);

    if (fisheye_region_idx->ViewMode == PROJECTION_REGION)
    {
        ctrl_tilt = minmax(fisheye_region_idx->Tilt - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);
        ctrl_pan  = minmax(fisheye_region_idx->Pan  - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);
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


static void normalize_3d(bm_vector3d* v)
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


static void equidistant(bm_vector3d* v3d, bm_vector2d* v2d, double f)
{
    double rd, ri, theta, phy;

    normalize_3d(v3d);

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
    bm_vector3d v3d_p;

    v3d_p.x = v3d->x * phy / theta;
    v3d_p.y = v3d->y * phy / theta;
    v3d_p.z = v3d->z * phy / theta + (1 - phy / theta);

    // calculate the target point (xi, yi) on image at z = f
    v2d->x = (f / v3d_p.z) * v3d_p.x;
    v2d->y = (f / v3d_p.z) * v3d_p.y;
}

static void Rotate3D(bm_vector3d* v3d, bm_rotMatrix3d* mat)
{
    //int i, j;
    bm_vector3d v1;

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

static void _Panorama180View2(bm_fisheye_region_attr* fisheye_region, int rgn_idx, fisheye_mount_mode_e MOUNT, double x0, double y0, double r)
{
    if (fisheye_region[rgn_idx].ViewMode != PROJECTION_PANORAMA_180)
        return;
    if (MOUNT != FISHEYE_WALL_MOUNT) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "mount_mode(%d) not supported in Panorama180\n", MOUNT);
        return;
    }

    int view_w = fisheye_region[rgn_idx].OutW;
    int view_h = fisheye_region[rgn_idx].OutH;
    int tot_mesh_cnt = fisheye_region[rgn_idx].MeshHor * fisheye_region[rgn_idx].MeshVer;

    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "view_w = %d, view_h = %d, \n", view_w, view_h);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_horcnt = %d,\n", mesh_horcnt);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_vercnt = %d,\n", mesh_vercnt);

    // UI PARAMETERS
    int _UI_horViewOffset = fisheye_region[rgn_idx].Pan;        // value range = 0 ~ 360, => -180 ~ 0 ~ +180
    int _UI_verViewOffset = fisheye_region[rgn_idx].Tilt;       // value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
    int _UI_horViewRange  = fisheye_region[rgn_idx].ZoomH;      // value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
    int _UI_verViewRange  = fisheye_region[rgn_idx].ZoomV;      // value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90

    _UI_verViewRange = (_UI_verViewRange == 4095) ? 4096 : _UI_verViewRange;
    _UI_horViewRange = (_UI_horViewRange == 4095) ? 4096 : _UI_horViewRange;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "_UI_horViewOffset = %d,\n", _UI_horViewOffset);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "_UI_verViewOffset = %d,\n", _UI_verViewOffset);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "_UI_horViewRange = %d,\n", _UI_horViewRange);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "_UI_verViewRange = %d,\n", _UI_verViewRange);

    // calculate default view range:
    double view_range_ver_degs = (double)_UI_verViewRange * 90 / 4096;
    double view_range_hor_degs = (double)_UI_horViewRange * 90 / 4096;
    double va_ver_degs = view_range_ver_degs;
    double va_hor_degs = view_range_hor_degs;

    // calculate offsets
    double va_hor_offset = ((double)_UI_horViewOffset - 180) * 90 / 360;
    double va_ver_offset = ((double)_UI_verViewOffset - 180) * 90 / 360;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "va_hor_offset = %f,\n", va_hor_offset);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "va_ver_offset = %f,\n", va_ver_offset);

    // Offset to shift view angle
    double va_ver_start = minmax(90 - va_ver_degs + va_ver_offset,  0,  90);
    double va_ver_end   = minmax(90 + va_ver_degs + va_ver_offset, 90, 180);
    double va_hor_start = minmax(90 - va_hor_degs + va_hor_offset,  0,  90);
    double va_hor_end   = minmax(90 + va_hor_degs + va_hor_offset, 90, 180);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "va_ver_start = %f,\n", va_ver_start);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "va_ver_end = %f,\n", va_ver_end);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "va_hor_start = %f,\n", va_hor_start);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "va_hor_end = %f,\n", va_hor_end);

    //system("pause");

    bm_rotMatrix3d mat0;

    bm_vector3d pix3d;
    bm_vector2d dist2d;
    int X_offset = fisheye_region[rgn_idx].OutX + fisheye_region[rgn_idx].OutW / 2;
    int Y_offset = fisheye_region[rgn_idx].OutY + fisheye_region[rgn_idx].OutH / 2;

    for (int i = 0; i < tot_mesh_cnt; i++)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "i = %d,", i);
        // each mesh has 4 knots
        for (int knotidx = 0; knotidx < 4; knotidx++)
        {
            double x = fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor - X_offset;
            double y = fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor - Y_offset;
            //double z = r;

            // initial pseudo plane cooridinates as vp3d
            //double theta_hor_cur = va_hor_start + ((2 * va_hor_degs * (view_w / 2 - x)) / (double)view_w);
            double theta_hor_cur = va_hor_start + (((va_hor_end - va_hor_start) * (view_w / 2 - x)) / (double)view_w);
            double theta_hor_cur_rad = M_PI / 2 - (theta_hor_cur * M_PI) / 180;     //θx

            double theta_ver_cur = va_ver_start + (((va_ver_end - va_ver_start) * (view_h / 2 - y)) / (double)view_h);
            double theta_ver_cur_rad = M_PI / 2 - (theta_ver_cur * M_PI) / 180;     //θy

            // if (knotidx == 0)
            // {
            //     bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "(x,y)=(%f,%f) fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor = %f, ", x, y, fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor);
            //     bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "hor_deg = %f, ver_deg = %f,\n\r",theta_hor_cur, theta_ver_cur);
            //     bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "hor_rad = %f,  ver_rad = %f, \n\r", theta_hor_cur_rad, theta_ver_cur_rad);
            // }

            double theta_x = theta_hor_cur_rad;
            double theta_y = theta_ver_cur_rad;
            double theta_z = (M_PI / 2);

            fisheye_region[rgn_idx].ThetaX = theta_x;
            fisheye_region[rgn_idx].ThetaY = theta_y;
            fisheye_region[rgn_idx].ThetaZ = theta_z;

            gen_rot_matrix3d_yxz(&mat0, &fisheye_region[rgn_idx]);

            pix3d.x = 0;
            pix3d.y = 0;
            pix3d.z = 1;

            Rotate3D(&pix3d, &mat0);

            // generate new 3D vector thru rotated pixel
            // normalize_3d(&pix3d);

            // generate 2D location on distorted image
            // equidistant_panorama(&pix3d, &dist2d, r);
            equidistant(&pix3d, &dist2d, r);

            // update source mesh-info here
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0;
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0;

            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "(x2d)(%f,%f),\n\r",dist2d.x + x0, dist2d.y + y0);

        }
    }
}

static void equidistant_panorama(bm_vector3d* v3d, bm_vector2d* v2d, double f)
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
    bm_vector3d v3d_p;
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

static void _panorama360View2(bm_fisheye_region_attr* fisheye_region, int rgn_idx, fisheye_mount_mode_e MOUNT, double x0, double y0, double r)
{
    if (fisheye_region[rgn_idx].ViewMode != PROJECTION_PANORAMA_360)
        return;
    if ((MOUNT != FISHEYE_CEILING_MOUNT) && (MOUNT != FISHEYE_DESKTOP_MOUNT)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "mount_mode(%d) not supported in Panorama360\n", MOUNT);
        return;
    }

    int view_w = fisheye_region[rgn_idx].OutW;
    int view_h = fisheye_region[rgn_idx].OutH;
    int tot_mesh_cnt = fisheye_region[rgn_idx].MeshHor * fisheye_region[rgn_idx].MeshVer;

    //bm_vector2d vp2d;
    //bm_vector3d vp3d;
    //bm_rotMatrix3d mat0;

    bm_vector3d pix3d;
    bm_vector2d dist2d;

    // UI PARAMETERS
    //float _UI_start_angle     = fisheye_region[rgn_idx].Pan;              // value from 0=360, 0 ~ 356 is the range of adjustment.
    float _UI_view_offset       = fisheye_region[rgn_idx].Tilt;             // value = 0 ~ 360, center = 180 ( original ), OR create offset to shift OutRadius & InRadius ( = adjust theta angle in our implementation )
    float _UI_inradius      = fisheye_region[rgn_idx].InRadius;             // value = 0 ~ 4095
    float _UI_outradius     = fisheye_region[rgn_idx].OutRadius;            // value = 0 ~ 4095
    float _UI_zoom_outradius    = fisheye_region[rgn_idx].ZoomV;            // a ratio to zoom OutRadius length.

    _UI_inradius  = (_UI_inradius >= 4095)  ? 4096 : _UI_inradius;
    _UI_outradius = (_UI_outradius >= 4095) ? 4096 : _UI_outradius;
    _UI_zoom_outradius = (_UI_zoom_outradius >= 4095) ? 4096 : _UI_zoom_outradius;

    int start_angle_degrees  = fisheye_region[rgn_idx].Pan;
    int end_angle__degrees   = fisheye_region[rgn_idx].PanEnd;


    float raw_outradius_pxl = (_UI_outradius * r )/ 4096;
    float raw_inradius_pxl  = (_UI_inradius * r )/ 4096;
    float radiusOffset      = (raw_outradius_pxl * (_UI_view_offset - 180)) / 360;

    float inradius_pxl_final  = MIN(r, MAX(0, raw_inradius_pxl + radiusOffset));
    float outradius_pxl_final = MIN(r, MAX(inradius_pxl_final, raw_inradius_pxl + (MAX(0, (raw_outradius_pxl - raw_inradius_pxl)) * _UI_zoom_outradius) / 4096 + radiusOffset));

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "r = %f,\n", r);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "raw_outradius_pxl = %f,\n", raw_outradius_pxl);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "raw_inradius_pxl = %f,\n", raw_inradius_pxl);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "radiusOffset = %f,\n", radiusOffset);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "inradius_pxl_final = %f,\n", inradius_pxl_final);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "outradius_pxl_final = %f,\n", outradius_pxl_final);


    float va_ver_end_rads   = inradius_pxl_final * M_PI / (2 * r);//_UI_inradius * M_PI / 1024;     // for Equidistant =>rd = f*theta, rd_max = f*PI/2 = r ( in code), f = 2r/PI. => theta = rd/f = rd*PI/2r
    float va_ver_start_rads = outradius_pxl_final * M_PI / (2 * r);//_UI_outradius * M_PI / 1024;
    float va_ver_rads = MIN( M_PI/2, va_ver_start_rads - va_ver_end_rads);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "va_ver_end_rads = %f,\n", va_ver_end_rads);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "va_ver_start_rads = %f,\n", va_ver_start_rads);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "va_ver_rads = %f,\n", va_ver_rads);

    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "( %d, %d, %d, %f, %f, %d,) \n", rgn_idx, _UI_start_angle, _UI_view_offset, _UI_inradius, _UI_outradius, _UI_zoom_outradius);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "start_angle_degrees = %d, end_angle__degrees = %d, va_ver_degs = %f, va_ver_start_degs = %f, \n\r", start_angle_degrees, end_angle__degrees, va_ver_rads, va_ver_start_rads);
    // system("pause");

    int total_angle = (360 + (end_angle__degrees - start_angle_degrees)) % 360;
    int half_w = view_w / 2;
    int half_h = view_h / 2;
    int X_offset = fisheye_region[rgn_idx].OutX + fisheye_region[rgn_idx].OutW / 2;
    int Y_offset = fisheye_region[rgn_idx].OutY + fisheye_region[rgn_idx].OutH / 2;

    for (int i = 0; i < tot_mesh_cnt; i++)
    {
        // each mesh has 4 knots
        for (int knotidx = 0; knotidx < 4; knotidx++)
        {
            double phi_degrees, phi_rad, theta_rv;

            double x = fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor - X_offset;
            double y = fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor - Y_offset;

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
            //equidistant(&pix3d, &dist2d, r);

            equidistant_panorama(&pix3d, &dist2d, r);

            // update source mesh-info here
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0;
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0;
        }
    }

}

static void _regionView2(bm_fisheye_region_attr* fisheye_region, int rgn_idx, fisheye_mount_mode_e MOUNT, double x0, double y0, double r)
{
    if (fisheye_region[rgn_idx].ViewMode != PROJECTION_REGION)
        return;

    //int view_w = fisheye_region[rgn_idx].OutW;
    //int view_h = fisheye_region[rgn_idx].OutH;
    int tot_mesh_cnt = fisheye_region[rgn_idx].MeshHor * fisheye_region[rgn_idx].MeshVer;

    // rotation matrix to point out view center of this region.
    bm_rotMatrix3d mat0;
    gen_rot_matrix3d_yxz(&mat0, &fisheye_region[rgn_idx]);

    bm_vector3d pix3d;
    bm_vector2d dist2d;
    int X_offset = fisheye_region[rgn_idx].OutX + fisheye_region[rgn_idx].OutW / 2;
    int Y_offset = fisheye_region[rgn_idx].OutY + fisheye_region[rgn_idx].OutH / 2;
    double w_ratio = 1.0 * (fisheye_region[rgn_idx].ZoomH - 2048) / 2048;
    double h_ratio = 1.0 * (fisheye_region[rgn_idx].ZoomV - 2048) / 2048;

    // go through all meshes in thus regions
    // mat0 is decided by view angle defined in re gion config.
    for (int i = 0; i < tot_mesh_cnt; i++)
    {
        // each mesh has 4 knots
        for (int knotidx = 0; knotidx < 4; knotidx++)
        {
            // display window center locate @ (0,0), mesh info shife back to center O.
            // for each region, rollback center is (0,0)
            double x = fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor - X_offset;
            double y = fisheye_region[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor - Y_offset;
            double z = r;


            if (MOUNT == FISHEYE_DESKTOP_MOUNT) {
                x = -x;
                y = -y;
            }


            // Zooming In/Out Control by ZoomH & ZoomV
            // 1.0 stands for +- 50% of zoomH ratio. => set as
            x = x *(1 + w_ratio);
            y = y *(1 + h_ratio);

            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "(x,y)=(%f,%f)\n\r", x, y);

            pix3d.x = x;
            pix3d.y = y;
            pix3d.z = z;

            // Region Porjection Model
            Rotate3D(&pix3d, &mat0);
            equidistant(&pix3d, &dist2d, r);

            // update source mesh-info here
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0;
            fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "rgn(%d) mesh(%d) node(%d) (%lf %lf)\n", rgn_idx, i, knotidx, fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor, fisheye_region[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor);
        }
    }
}

int dwa_load_meshdata(char *grid, bm_mesh_data_all_s *pmeshdata, const char *bindName)
{
    int info[100] = {0};

    memcpy(info, grid, 100 * sizeof(int));

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "head %d %d %d %d %d\n", info[0], info[1], info[2], info[3], info[4]);
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
    memcpy(pmeshdata->corners, info + 11, sizeof(int) * 10);
    pmeshdata->grid_mode = (enum grid_info_mode)info[21]; //grid_info_mode

    pmeshdata->slice_info.magic = info[22];
    if (pmeshdata->slice_info.magic == SLICE_MAGIC) {
        pmeshdata->slice_info.slice_h_cnt = info[23];
        pmeshdata->slice_info.slice_v_cnt = info[24];
        pmeshdata->slice_info.cache_hit_cnt = info[25];
        pmeshdata->slice_info.cache_miss_cnt = info[26];
        pmeshdata->slice_info.cache_req_cnt = info[27];
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "slice_info: %d %d %d %d %d\n", info[23], info[24], info[25], info[26], info[27]);
    } else {
        memset(&pmeshdata->slice_info, 0, sizeof(pmeshdata->slice_info));
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "slice magic invalid, use default slice info\n");
    }

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

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_horcnt,mesh_vercnt,_nbr_mesh_x, _nbr_mesh_y, count_grid, num_nodes: %d %d %d %d %d %d \n",
              pmeshdata->mesh_horcnt, pmeshdata->mesh_vercnt, _nbr_mesh_x, _nbr_mesh_y, count_grid, pmeshdata->node_index);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "imgw, imgh, mesh_w, mesh_h ,unit_rx,unit_ry: %d %d %d %d %d %d \n",
              pmeshdata->imgw, pmeshdata->imgh, pmeshdata->mesh_w, pmeshdata->mesh_h, pmeshdata->unit_rx, pmeshdata->unit_ry);
    if (count_grid > 0)
    {
        memcpy(pmeshdata->pgrid_src, grid + 100 * sizeof(int), count_grid * 2 * sizeof(int));
        memcpy(pmeshdata->pgrid_dst, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int), count_grid * 2 * sizeof(int));
        memcpy(pmeshdata->pmesh_src, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int) * 2, count_grid * 2 * 4 * sizeof(int));
        memcpy(pmeshdata->pmesh_dst, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int) * 2 + count_grid * 2 * 4 * sizeof(int), count_grid * 2 * 4 * sizeof(int));
    }

    if (pmeshdata->node_index > 0)
    {
        memcpy(pmeshdata->pnode_src, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int) * 2 + count_grid * 2 * 4 * sizeof(int) * 2, pmeshdata->node_index * 2 * sizeof(int));
        memcpy(pmeshdata->pnode_dst, grid + 100 * sizeof(int) + count_grid * 2 * sizeof(int) * 2 + count_grid * 2 * 4 * sizeof(int) * 2 + pmeshdata->node_index * 2 * sizeof(int), pmeshdata->node_index * 2 * sizeof(int));
    }
    pmeshdata->balloc = true;
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "read success!\n");

    return 0;
}

int _bm_load_region_config(bm_fisheye_region_attr *fisheye_region, int rgn, bm_mesh_data_all_s *meshd)
{
    if (meshd == NULL || !fisheye_region) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "meshdata  or fisheye_region is NULL.\n");
        return -1;
    }
    fisheye_region[rgn].RegionValid = true;
    //fisheye_region[rgn].ViewMode = PROJECTION_STEREO_FIT;

    // update region info from memory
    int horcnt = meshd->mesh_horcnt;
    int vercnt = meshd->mesh_vercnt;
    int meshw = meshd->mesh_w;
    int meshh = meshd->mesh_h;
    int rx = meshd->unit_rx;
    int ry = meshd->unit_ry;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "horcnt, vercnt, meshw, meshh, rx, ry: %d %d %d %d %d %d\n"
        , horcnt, vercnt, meshw, meshh, rx, ry);
    fisheye_region[rgn].MeshVer = vercnt;
    fisheye_region[rgn].MeshHor = horcnt;
    fisheye_region[rgn].OutW = meshw * horcnt;  // fisheye_config->OutW_disp; // Note: !! It's roi width
    fisheye_region[rgn].OutH = meshh * vercnt;  // fisheye_config->OutH_disp;
    fisheye_region[rgn].OutX = meshw * rx;      // (width_sec * 1);
    fisheye_region[rgn].OutY = meshh * ry;      // (height_sec * 1);

    return 0;
}

void bm_dwa_node_to_mesh(bm_fisheye_region_attr *fisheye_region, int rgn)
{
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "node_to_mesh\n");
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "rgn(%d)(%d)(%d)\n",rgn,fisheye_region[0].MeshHor,fisheye_region[0].MeshVer);
    int maxWMeshNum  = fisheye_region[0].MeshHor;
    int maxHMeshNum = fisheye_region[0].MeshVer;
    //Node done, load to Mesh
    for (int mesh_yidx = 0; mesh_yidx < maxHMeshNum; mesh_yidx++) {
        for (int mesh_xidx = 0; mesh_xidx < maxWMeshNum; mesh_xidx++) {
            // destination node to mesh
            fisheye_region[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[0].xcor =
                fisheye_region[rgn].DstRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx].node.xcor;
            fisheye_region[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[0].ycor =
                fisheye_region[rgn].DstRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx].node.ycor;

            fisheye_region[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[1].xcor =
                fisheye_region[rgn].DstRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx + 1].node.xcor;
            fisheye_region[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[1].ycor =
                fisheye_region[rgn].DstRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx + 1].node.ycor;

            fisheye_region[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[3].xcor =
                fisheye_region[rgn].DstRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx + 1].node.xcor;
            fisheye_region[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[3].ycor =
                fisheye_region[rgn].DstRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx + 1].node.ycor;

            fisheye_region[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[2].xcor =
                fisheye_region[rgn].DstRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx].node.xcor;
            fisheye_region[rgn].DstRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[2].ycor =
                fisheye_region[rgn].DstRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx].node.ycor;

            // source node to mesh
            fisheye_region[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[0].xcor =
                fisheye_region[rgn].SrcRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx].node.xcor;
            fisheye_region[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[0].ycor =
                fisheye_region[rgn].SrcRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx].node.ycor;

            fisheye_region[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[1].xcor =
                fisheye_region[rgn].SrcRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx + 1].node.xcor;
            fisheye_region[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[1].ycor =
                fisheye_region[rgn].SrcRgnNodeInfo[mesh_yidx * (maxWMeshNum + 1) + mesh_xidx + 1].node.ycor;

            fisheye_region[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[3].xcor =
                fisheye_region[rgn].SrcRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx + 1].node.xcor;
            fisheye_region[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[3].ycor =
                fisheye_region[rgn].SrcRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx + 1].node.ycor;

            fisheye_region[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[2].xcor =
                fisheye_region[rgn].SrcRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx].node.xcor;
            fisheye_region[rgn].SrcRgnMeshInfo[mesh_yidx * maxWMeshNum + mesh_xidx].knot[2].ycor =
                fisheye_region[rgn].SrcRgnNodeInfo[(mesh_yidx + 1) * (maxWMeshNum + 1) + mesh_xidx].node.ycor;
        }
    }
}

int bm_get_region_all_mesh_data_memory(bm_fisheye_region_attr *fisheye_region, int view_w, int view_h, int rgn, bm_mesh_data_all_s *pmeshinfo)
{
    if (!fisheye_region[rgn].RegionValid) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "region[%d] is not invalid\n", rgn);
        return -1;
    }
    // int mesh_horcnt, mesh_vercnt;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "points to grids, hw mesh\n");
    fisheye_region[rgn].MeshHor = pmeshinfo->mesh_horcnt;// grid_info[0];//mesh_horcnt;
    fisheye_region[rgn].MeshVer = pmeshinfo->mesh_vercnt;//mesh_vercnt;

    // mesh_horcnt = pmeshinfo->mesh_horcnt;
    // mesh_vercnt = pmeshinfo->mesh_vercnt;

    //int view_w = fisheye_region[rgn].OutW;
    //int view_h = fisheye_region[rgn].OutH;
    //int mesh_horcnt = fisheye_region[rgn].MeshHor;
    //int mesh_vercnt = fisheye_region[rgn].MeshVer;

    // register:
#if !WITHOUT_BIAS
#if !DPU_MODE
    bool bAspect = (bool)fisheye_region[rgn].ZoomV;
    int XYRatio = minmax(fisheye_region[rgn].Pan, 0, 100);
    int XRatio, YRatio;

    if (bAspect) {
        XRatio = YRatio = XYRatio;
    } else {
        XRatio = minmax(fisheye_region[rgn].ThetaX, 0, 100);
        YRatio = minmax(fisheye_region[rgn].ThetaZ, 0, 100);
    }

    int CenterXOffset = minmax(fisheye_region[rgn].InRadius, -511, 511);
    int CenterYOffset = minmax(fisheye_region[rgn].OutRadius, -511, 511);
    CenterXOffset = CenterYOffset = 0;
#endif
#endif
    //int DistortionRatio = minmax(fisheye_region[rgn].PanEnd, -300, 500);
    //double norm = sqrt((view_w / 2) * (view_w / 2) + (view_h / 2) * (view_h / 2));

    // 1st loop: to find mesh infos. on source ( backward projection )
    // hit index for buffer

    // for debug
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "view_h = %d src,\n", view_h);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "view_w = %d,\n", view_w);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_horcnt = %d,\n", mesh_horcnt);
    // bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_vercnt = %d,\n", mesh_vercnt);
    int meshidx = pmeshinfo->num_pairs;
    //int offsetX = 50 * 2;
    //float scale_value = 1.0;

    for (int i = 0; i < meshidx; i++) {
        fisheye_region[rgn].SrcRgnMeshInfo[i].knot[0].xcor = pmeshinfo->pmesh_src[8 * i];
        fisheye_region[rgn].SrcRgnMeshInfo[i].knot[0].ycor = pmeshinfo->pmesh_src[8 * i + 1];
        fisheye_region[rgn].SrcRgnMeshInfo[i].knot[1].xcor = pmeshinfo->pmesh_src[8 * i + 2];
        fisheye_region[rgn].SrcRgnMeshInfo[i].knot[1].ycor = pmeshinfo->pmesh_src[8 * i + 3];
        fisheye_region[rgn].SrcRgnMeshInfo[i].knot[2].xcor = pmeshinfo->pmesh_src[8 * i + 4];
        fisheye_region[rgn].SrcRgnMeshInfo[i].knot[2].ycor = pmeshinfo->pmesh_src[8 * i + 5];
        fisheye_region[rgn].SrcRgnMeshInfo[i].knot[3].xcor = pmeshinfo->pmesh_src[8 * i + 6];
        fisheye_region[rgn].SrcRgnMeshInfo[i].knot[3].ycor = pmeshinfo->pmesh_src[8 * i + 7];
#if WITHOUT_BIAS
        fisheye_region[rgn].DstRgnMeshInfo[i].knot[0].xcor = pmeshinfo->pmesh_dst[8 * i];
        fisheye_region[rgn].DstRgnMeshInfo[i].knot[0].ycor = pmeshinfo->pmesh_dst[8 * i + 1];
        fisheye_region[rgn].DstRgnMeshInfo[i].knot[1].xcor = pmeshinfo->pmesh_dst[8 * i + 2];
        fisheye_region[rgn].DstRgnMeshInfo[i].knot[1].ycor = pmeshinfo->pmesh_dst[8 * i + 3];
        fisheye_region[rgn].DstRgnMeshInfo[i].knot[2].xcor = pmeshinfo->pmesh_dst[8 * i + 4];
        fisheye_region[rgn].DstRgnMeshInfo[i].knot[2].ycor = pmeshinfo->pmesh_dst[8 * i + 5];
        fisheye_region[rgn].DstRgnMeshInfo[i].knot[3].xcor = pmeshinfo->pmesh_dst[8 * i + 6];
        fisheye_region[rgn].DstRgnMeshInfo[i].knot[3].ycor = pmeshinfo->pmesh_dst[8 * i + 7];
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
            fisheye_region[rgn].DstRgnMeshInfo[i].knot[j].xcor = x;
            fisheye_region[rgn].DstRgnMeshInfo[i].knot[j].ycor = y;
        }
#endif
    }

    // update Node structure
    // Construct node tied to each other.
    int node_index = pmeshinfo->node_index;
    for (int i = 0; i < node_index; i++) {
        fisheye_region[rgn].SrcRgnNodeInfo[i].node.xcor = pmeshinfo->pnode_src[2 * i];
        fisheye_region[rgn].SrcRgnNodeInfo[i].node.ycor = pmeshinfo->pnode_src[2 * i + 1];
        fisheye_region[rgn].SrcRgnNodeInfo[i].node.xbias = 0;
        fisheye_region[rgn].SrcRgnNodeInfo[i].node.ybias = 0;
        fisheye_region[rgn].SrcRgnNodeInfo[i].valid = true;

        fisheye_region[rgn].DstRgnNodeInfo[i].node.xbias = 0;
        fisheye_region[rgn].DstRgnNodeInfo[i].node.ybias = 0;
        fisheye_region[rgn].DstRgnNodeInfo[i].valid = true;
#if WITHOUT_BIAS
        fisheye_region[rgn].DstRgnNodeInfo[i].node.xcor = pmeshinfo->pnode_dst[2 * i];
        fisheye_region[rgn].DstRgnNodeInfo[i].node.ycor = pmeshinfo->pnode_dst[2 * i + 1];
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

        fisheye_region[rgn].DstRgnNodeInfo[i].node.xcor = x;
        fisheye_region[rgn].DstRgnNodeInfo[i].node.ycor = y;
#endif
    }

    //update node structure to mesh => all meshes are surrounded by points
    bm_dwa_node_to_mesh(fisheye_region, rgn);

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "PASS load mesh!!!!\n\r");    // for debug

    return 0;
}

int bm_get_region_mesh_list(bm_fisheye_region_attr *fisheye_region, int rgn_idx, bm_mesh_data_all_s *p_mesh_data)
{
    if (!fisheye_region) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "invalid param, fisheye_region is NULL.\n");
        return -1;
    }
    if (!p_mesh_data) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "invalid param, pMeshData is NULL.\n");
        return -2;
    }
    int tw = p_mesh_data->imgw;
    int th = p_mesh_data->imgh;
    int max_mesh_cnt = floor(tw / 16) * floor(th / 16);

    if (p_mesh_data->balloc) {
        if (p_mesh_data->_nbr_mesh_x * p_mesh_data->_nbr_mesh_y > max_mesh_cnt) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "mesh_x, mesh_y is too large.\n");
            return -1;
        }
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "g_MeshData is not alloc.\n");
        return -1;
    }

    // load output region from meshdata
    if (_bm_load_region_config(fisheye_region, rgn_idx, p_mesh_data)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "load region cfg fail from meshdata.\n");
        return -1;
    }

    if (bm_get_region_all_mesh_data_memory(fisheye_region, tw, th, 0, p_mesh_data)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "get region meshdata fail.\n");
        return -1;
    }

    return 0;
}

void bm_get_region_src_mesh_list(fisheye_mount_mode_e MOUNT, bm_fisheye_region_attr* fisheye_region, int rgn_idx, double x0, double y0, double r)
{
    // ViewModeType to decide mapping & UI contrl parameters.
    bm_projection_mode ViewModeType = fisheye_region[rgn_idx].ViewMode;

    if (ViewModeType == PROJECTION_REGION)
        _regionView2(fisheye_region, rgn_idx, MOUNT, x0, y0, r);
    else if (ViewModeType == PROJECTION_PANORAMA_360)
        _panorama360View2(fisheye_region, rgn_idx, MOUNT, x0, y0, r);
    else if (ViewModeType == PROJECTION_PANORAMA_180)
        _Panorama180View2(fisheye_region, rgn_idx, MOUNT, x0, y0, r);
    else if (ViewModeType == PROJECTION_LDC)
        _ldc_view(fisheye_region, rgn_idx, x0, y0);
    else
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "ERROR!!! THIS CASE SHOULDNOTHAPPEN!!!!!!\n\r");
}

void bm_get_region_dst_mesh_list(bm_fisheye_region_attr* fisheye_region, int rgn_idx)
{
    if (fisheye_region[rgn_idx].RegionValid != 1)
        return;

    // Destination Mesh-Info Allocation
    int view_w = fisheye_region[rgn_idx].OutW;
    int view_h = fisheye_region[rgn_idx].OutH;
    int mesh_horcnt = fisheye_region[rgn_idx].MeshHor;
    int mesh_vercnt = fisheye_region[rgn_idx].MeshVer;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "view_w(%d) view_h(%d) mesh_horcnt(%d) mesh_vercnt(%d)\n", view_w, view_h, mesh_horcnt, mesh_vercnt);
    int knot_horcnt = mesh_horcnt + 1;
    int knot_vercnt = mesh_vercnt + 1;
    // tmp internal buffer
    // maximum knot number = 1024 (pre-set)
    bm_coordinate2d *meshknot_hit_buf = malloc(sizeof(bm_coordinate2d) * (knot_horcnt * knot_vercnt + 1));

    // 1st loop: to find mesh infos. on source ( backward projection )
    // hit index for buffer
    int knotcnt = 0;
    int mesh_w = (view_w / mesh_horcnt);
    int mesh_h = (view_h / mesh_vercnt);
    int half_w = view_w / 2;
    int half_h = view_h / 2;
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_w(%d) mesh_h(%d)\n", mesh_w, mesh_h);
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
                meshknot_hit_buf[knotcnt].xcor = fisheye_region[rgn_idx].OutX + (x + half_w);
                meshknot_hit_buf[knotcnt].ycor = fisheye_region[rgn_idx].OutY + (y + half_h);
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "%d(%lf %lf)\n", knotcnt, meshknot_hit_buf[knotcnt].xcor, meshknot_hit_buf[knotcnt].ycor);
                knotcnt += 1;
            }
        }
    }
#else
    int y = -half_h;
    for (int j = knot_horcnt; j > 0; --j) {
        int x = -half_w;
        for (int i = knot_horcnt; i > 0; --i) {
            meshknot_hit_buf[knotcnt].xcor = fisheye_region[rgn_idx].OutX + (x + half_w);
            meshknot_hit_buf[knotcnt].ycor = fisheye_region[rgn_idx].OutY + (y + half_h);
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "%d(%lf %lf)\n", knotcnt, meshknot_hit_buf[knotcnt].xcor, meshknot_hit_buf[knotcnt].ycor);
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
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "knotcnt(%d)\n", knotcnt);       // for debug
    // tmp for debug

    for (int j = 0; j < mesh_vercnt; j++) {
        for (int i = 0; i < mesh_horcnt; i++) {
            int meshidx = j * mesh_horcnt + i;
            int knotidx = j * (mesh_horcnt + 1) + i;    // knot num = mesh num +1 ( @ horizon )

            fisheye_region[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].xcor = meshknot_hit_buf[knotidx].xcor;
            fisheye_region[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].ycor = meshknot_hit_buf[knotidx].ycor;
            fisheye_region[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].xcor = meshknot_hit_buf[knotidx + 1].xcor;
            fisheye_region[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].ycor = meshknot_hit_buf[knotidx + 1].ycor;
            fisheye_region[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].xcor = meshknot_hit_buf[knotidx + (mesh_horcnt + 1)].xcor;
            fisheye_region[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].ycor = meshknot_hit_buf[knotidx + (mesh_horcnt + 1)].ycor;
            fisheye_region[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].xcor = meshknot_hit_buf[knotidx + 1 + (mesh_horcnt + 1)].xcor;
            fisheye_region[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].ycor = meshknot_hit_buf[knotidx + 1 + (mesh_horcnt + 1)].ycor;
        }
    }

    free(meshknot_hit_buf);
}

int get_free_meshdata(const char *bindName, bm_mesh_data_all_s *g_MeshData)
{
    if (!bindName) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bindName is NULL.\n");
        return -1;
    }

    for (int i = 0; i < MESH_DATA_MAX_NUM; i++) {
        if (!strlen(g_MeshData[i].grid_name)) {
            strcpy(g_MeshData[i].grid_name, bindName);
            return i;
        }
    }

    return -1;
}

static int generate_mesh_on_fisheye(const grid_info_attr_s* pstGridInfoAttr,
                                    bm_fisheye_attr* fisheye_config,
                                    bm_fisheye_region_attr* fisheye_region,
                                    int X_TILE_NUMBER,
                                    int Y_TILE_NUMBER,
                                    uint16_t *reorder_mesh_id_list,
                                    int **reorder_mesh_tbl,
                                    uint64_t mesh_tbl_phy_addr,
                                    void *ptr)
{
    int mesh_tbl_num;   // get number of meshes
    double x0, y0, r;   // infos of src_img, (x0,y0) = center of image,  r = radius of image.
    int grid_idx = -1;
    struct timespec start, end;
    bm_mesh_data_all_s g_mesh_data[MESH_DATA_MAX_NUM];

    clock_gettime(CLOCK_MONOTONIC, &start);

    x0 = fisheye_config->InCenterX;
    y0 = fisheye_config->InCenterY;
    r = fisheye_config->InRadius;
    // initialize
    for (int i = 0; i < MESH_DATA_MAX_NUM; i++)
        g_mesh_data[i].grid_mode = GRID_MODE_REGION_BASE;

    // grid_idx = -1;
    // In Each Mode, for Every Region:
    for (int rgn_idx = 0; rgn_idx < fisheye_config->RgnNum; rgn_idx++) {
        // check region valid first
        if (!fisheye_region[rgn_idx].RegionValid) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,"Region invalid.\n");
            return -1;
        }
        if (fisheye_region[rgn_idx].ViewMode == PROJECTION_STEREO_FIT) {
            grid_idx = get_free_meshdata(pstGridInfoAttr->grid_bind_name, g_mesh_data);
            if (grid_idx < 0 || grid_idx >= MESH_DATA_MAX_NUM) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "meshData buf full.\n");
                return -1;
            }

            char *grid = (char *)ptr;
            if (dwa_load_meshdata(grid, &g_mesh_data[grid_idx], pstGridInfoAttr->grid_bind_name)) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dwa_load_meshdata failed.\n");
                return -1;
            }
            if (g_mesh_data[grid_idx].slice_info.magic == SLICE_MAGIC) {
                X_TILE_NUMBER = g_mesh_data[grid_idx].slice_info.slice_h_cnt;
                Y_TILE_NUMBER = g_mesh_data[grid_idx].slice_info.slice_v_cnt;
            }

            if (bm_get_region_mesh_list(fisheye_region, rgn_idx, &g_mesh_data[grid_idx])) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "invalid param, fisheye_region is NULL.\n");
                return -1;
            }
        } else {
            UNUSED(pstGridInfoAttr);
            // get & store region mesh info.
            bm_get_region_dst_mesh_list(fisheye_region, rgn_idx);

            // Get Source Mesh-Info Projected from Destination by Differet ViewModw.
            bm_get_region_src_mesh_list(fisheye_config->MntMode, fisheye_region, rgn_idx, x0, y0, r);
        }
    }
    //combine all region meshs - mesh projection done.
    bm_get_frame_mesh_list(fisheye_config, fisheye_region);

    mesh_tbl_num = fisheye_config->TotalMeshNum;
    float src_x_mesh_tbl[mesh_tbl_num][4];
    float src_y_mesh_tbl[mesh_tbl_num][4];
    float dst_x_mesh_tbl[mesh_tbl_num][4];
    float dst_y_mesh_tbl[mesh_tbl_num][4];

    if (ptr == NULL) grid_idx = 0;

    switch (g_mesh_data[grid_idx].grid_mode) {
    case GRID_MODE_REGION_BASE:
        for (int mesh_idx = 0; mesh_idx < fisheye_config->TotalMeshNum; mesh_idx++) {
            for (int knotidx = 0; knotidx < 4; knotidx++) {
                src_x_mesh_tbl[mesh_idx][knotidx] = fisheye_config->SrcRgnMeshInfo[mesh_idx].knot[knotidx].xcor;
                src_y_mesh_tbl[mesh_idx][knotidx] = fisheye_config->SrcRgnMeshInfo[mesh_idx].knot[knotidx].ycor;
                dst_x_mesh_tbl[mesh_idx][knotidx] = fisheye_config->DstRgnMeshInfo[mesh_idx].knot[knotidx].xcor;
                dst_y_mesh_tbl[mesh_idx][knotidx] = fisheye_config->DstRgnMeshInfo[mesh_idx].knot[knotidx].ycor;
            }
        }
        break;
    case GRID_MODE_MESH_BASE:
        fisheye_config->TotalMeshNum = g_mesh_data[grid_idx].num_pairs;
        mesh_tbl_num = fisheye_config->TotalMeshNum;
        for (int mesh_idx = 0; mesh_idx < fisheye_config->TotalMeshNum; mesh_idx++) {
            for (int knotidx = 0; knotidx < 4; knotidx++) {
                dst_x_mesh_tbl[mesh_idx][knotidx] = (float)g_mesh_data[grid_idx].pmesh_dst[8 * mesh_idx + 2 * knotidx + 0];
                dst_y_mesh_tbl[mesh_idx][knotidx] = (float)g_mesh_data[grid_idx].pmesh_dst[8 * mesh_idx + 2 * knotidx + 1];
                src_x_mesh_tbl[mesh_idx][knotidx] = (float)g_mesh_data[grid_idx].pmesh_src[8 * mesh_idx + 2 * knotidx + 0];
                src_y_mesh_tbl[mesh_idx][knotidx] = (float)g_mesh_data[grid_idx].pmesh_src[8 * mesh_idx + 2 * knotidx + 1];
            }
        }
        break;
    default:
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not supported\n");
        return BM_NOT_SUPPORTED;
    }

    int dst_height, dst_width;
    if (fisheye_config->rotate_index == 1 || fisheye_config->rotate_index == 3) {
        dst_height = fisheye_config->OutW_disp;
        dst_width  = fisheye_config->OutH_disp;
    } else {
        dst_height = fisheye_config->OutH_disp;
        dst_width  = fisheye_config->OutW_disp;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_tbl_num = %d\n", mesh_tbl_num);      // for debug

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
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_id_list_entry_num = %d\n", mesh_id_list_entry_num);      // for debug

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
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh table size (bytes) = %d\n", (reorder_mesh_tbl_entry_num * 4));
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh id list size (bytes) = %d\n", (reorder_mesh_id_list_entry_num * 2));

    clock_gettime(CLOCK_MONOTONIC, &end);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "time consumed: %fms\n",(float)get_diff_in_us(start, end) / 1000);     // for debug
    return 0;
}

static bm_status_t bm_dwa_rotation_check_size(rotation_e enRotation, const gdc_task_attr_s *pstTask)
{
    if (enRotation >= ROTATION_MAX) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "invalid rotation(%d).\n", enRotation);
        return BM_ERR_FAILURE;
    }

    if (enRotation == ROTATION_90 || enRotation == ROTATION_270 || enRotation == ROTATION_XY_FLIP) {
        if (pstTask->img_in.video_frame.width < pstTask->img_in.video_frame.height) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "rotation(%d) invalid: 'output width(%d) < input height(%d)'\n",
                        enRotation, pstTask->img_out.video_frame.width,
                        pstTask->img_in.video_frame.height);
            return BM_ERR_FAILURE;
        }
        if (pstTask->img_out.video_frame.height < pstTask->img_in.video_frame.width) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "rotation(%d) invalid: 'output height(%d) < input width(%d)'\n",
                        enRotation, pstTask->img_out.video_frame.height,
                        pstTask->img_in.video_frame.width);
            return BM_ERR_FAILURE;
        }
    } else {
        if (pstTask->img_out.video_frame.width < pstTask->img_in.video_frame.width) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "rotation(%d) invalid: 'output width(%d) < input width(%d)'\n",
                        enRotation, pstTask->img_out.video_frame.width,
                        pstTask->img_in.video_frame.width);
            return BM_ERR_FAILURE;
        }
        if (pstTask->img_out.video_frame.height < pstTask->img_in.video_frame.height) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "rotation(%d) invalid: 'output height(%d) < input height(%d)'\n",
                        enRotation, pstTask->img_out.video_frame.height,
                        pstTask->img_in.video_frame.height);
            return BM_ERR_FAILURE;
        }
    }

    return BM_SUCCESS;
}

static bm_status_t bm_get_valid_tsk_mesh_by_name(bm_handle_t handle, const char *name, u8 *idx)
{
    u8 i = DWA_MAX_TSK_MESH;

    if (!name) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "tsk mesh name is null\n");
        *idx = i;
        return BM_ERR_FAILURE;
    }
    for (i = 0; i < DWA_MAX_TSK_MESH; i++) {
        if (strcmp(dwa_tskMesh[i].Name, name) == 0 /*&& dwa_tskMesh[i].paddr*/) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "got remain tsk mesh[%d-%s-%llx]\n", i, dwa_tskMesh[i].Name, dwa_tskMesh[i].mem.u.device.device_addr);       // for debug
            *idx = i;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "idx = (%d)\n", *idx);       // for debug
            return BM_SUCCESS;
        }
    }
    *idx = get_idle_tsk_mesh(handle);
    if (*idx >= DWA_MAX_TSK_MESH) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "tsk mesh count(%d) is out of range(%d)\n", *idx + 1, DWA_MAX_TSK_MESH);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "idx = (%d)\n", *idx);
        return BM_ERR_FAILURE;
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "start alloc new tsk mesh-[%d]-[%s]\n", *idx, name);       // for debug
        strcpy(dwa_tskMesh[*idx].Name, name);
        return BM_ERR_FAILURE;
    }
}

static void bm_dwa_mesh_gen_rotation(size_s in_size,
                                     size_s out_size,
                                     rotation_e rot,
                                     uint64_t mesh_phy_addr,
                                     void *mesh_vir_addr)
{
    bm_fisheye_attr *fisheye_config;
    ldc_attr_s stLDCAttr = { .aspect = true, .xy_ratio = 100,
        .center_x_offset = 0, .center_y_offset = 0, .distortion_ratio = 0 };
    bm_fisheye_region_attr *fisheye_region;
    int nMeshhor, nMeshVer;

    fisheye_config = (bm_fisheye_attr*)calloc(1, sizeof(*fisheye_config));
    if (!fisheye_config) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye config\n");
        free(fisheye_config);
        return;
    }
    fisheye_region = (bm_fisheye_region_attr*)calloc(1, sizeof(*fisheye_region) * MAX_REGION_NUM);
    if (!fisheye_region) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye region config\n");
        free(fisheye_config);
        free(fisheye_region);
        return;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "in_size(%d %d) out_size(%d %d)\n"
        , in_size.width, in_size.height
        , out_size.width, out_size.height);

    bm_ldc_attr_map(&stLDCAttr, in_size, fisheye_config, fisheye_region);
    dwa_get_mesh_size(&nMeshhor, &nMeshVer);
    fisheye_region[0].MeshHor = nMeshhor;
    fisheye_region[0].MeshVer = nMeshVer;

    fisheye_config->rotate_index = rot;

    int X_TILE_NUMBER, Y_TILE_NUMBER;
    u32 mesh_id_size, mesh_tbl_size;
    u64 mesh_id_phy_addr, mesh_tbl_phy_addr;

    X_TILE_NUMBER = DIV_UP(in_size.width, DWA_MESH_SLICE_BASE_W);
    Y_TILE_NUMBER = DIV_UP(in_size.height, DWA_MESH_SLICE_BASE_H);

    // calculate mesh_id/mesh_tbl's size in bytes.
    mesh_tbl_size = 0x20000;
    mesh_id_size = 0x40000;

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

    // Provide virtual address to write mesh.
    reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
    generate_mesh_on_fisheye(NULL, fisheye_config, fisheye_region, X_TILE_NUMBER, Y_TILE_NUMBER
        , (uint16_t *)mesh_vir_addr
        , reorder_mesh_tbl, mesh_tbl_phy_addr, (void *)NULL);

    free(fisheye_config);
    free(fisheye_region);
}

static void bm_fisheye_attr_map(const fisheye_attr_s *pstFisheyeAttr,
                                size_s out_size,
                                bm_fisheye_attr *fisheye_config,
                                bm_fisheye_region_attr *fisheye_region)
{
    fisheye_config->MntMode = pstFisheyeAttr->mount_mode;
    fisheye_config->OutW_disp = out_size.width;
    fisheye_config->OutH_disp = out_size.height;
    fisheye_config->InCenterX = pstFisheyeAttr->hor_offset;
    fisheye_config->InCenterY = pstFisheyeAttr->ver_offset;
    // TODO: how to handl radius
    fisheye_config->InRadius = MIN(fisheye_config->InCenterX, fisheye_config->InCenterY);
    fisheye_config->FStrength = pstFisheyeAttr->fan_strength;
    fisheye_config->TCoef  = pstFisheyeAttr->trapezoid_coef;

    fisheye_config->RgnNum = pstFisheyeAttr->region_num;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "OutW_disp(%d) OutH_disp(%d)\n", fisheye_config->OutW_disp, fisheye_config->OutH_disp);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "InCenterX(%d) InCenterY(%d)\n", fisheye_config->InCenterX, fisheye_config->InCenterY);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "FStrength(%lf) TCoef(%lf) RgnNum(%d)\n", fisheye_config->FStrength, fisheye_config->TCoef, fisheye_config->RgnNum);

    for (int i = 0; i < MAX_REGION_NUM; ++i)
        fisheye_region[i].RegionValid = 0;

    for (int i = 0; i < fisheye_config->RgnNum; ++i) {
        fisheye_region[i].RegionValid = 1;

        fisheye_region[i].ZoomH = pstFisheyeAttr->fisheye_region_attr[i].hor_zoom;
        fisheye_region[i].ZoomV = pstFisheyeAttr->fisheye_region_attr[i].ver_zoom;
        fisheye_region[i].Pan = pstFisheyeAttr->fisheye_region_attr[i].pan;
        fisheye_region[i].Tilt = pstFisheyeAttr->fisheye_region_attr[i].tilt;
        fisheye_region[i].OutW = pstFisheyeAttr->fisheye_region_attr[i].stoutrect.width;
        fisheye_region[i].OutH = pstFisheyeAttr->fisheye_region_attr[i].stoutrect.height;
        fisheye_region[i].OutX = pstFisheyeAttr->fisheye_region_attr[i].stoutrect.x;
        fisheye_region[i].OutY = pstFisheyeAttr->fisheye_region_attr[i].stoutrect.y;
        fisheye_region[i].InRadius = pstFisheyeAttr->fisheye_region_attr[i].in_radius;
        fisheye_region[i].OutRadius = pstFisheyeAttr->fisheye_region_attr[i].out_radius;
        if (pstFisheyeAttr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_NORMAL) {
            fisheye_region[i].MeshVer = 16;
            fisheye_region[i].MeshHor = 16;
            fisheye_region[i].ViewMode = PROJECTION_REGION;
            fisheye_region[i].ThetaX = 0.4*M_PI;
            fisheye_region[i].ThetaZ = 0;
            fisheye_region[i].ThetaY = 0;
        } else if (pstFisheyeAttr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_180_PANORAMA) {
            fisheye_region[i].MeshVer = 32;
            fisheye_region[i].MeshHor = 32;
            fisheye_region[i].ViewMode = PROJECTION_PANORAMA_180;
        } else if (pstFisheyeAttr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_360_PANORAMA) {
            fisheye_region[i].MeshVer = (fisheye_config->RgnNum == 1) ? 64 : 32;
            fisheye_region[i].MeshHor = (fisheye_config->RgnNum == 1) ? 64 : 32;
            fisheye_region[i].ViewMode = PROJECTION_PANORAMA_360;
            fisheye_region[i].ThetaX = M_PI / 4;
            fisheye_region[i].ThetaZ = 0;
            fisheye_region[i].ThetaY = 0;
            fisheye_region[i].PanEnd = fisheye_region[i].Pan
                + 360 * pstFisheyeAttr->fisheye_region_attr[i].hor_zoom / 4096;
        }
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "Region(%d) ViewMode(%d) MeshVer(%d) MeshHor(%d)\n"
            , i, fisheye_region[i].ViewMode, fisheye_region[i].MeshVer, fisheye_region[i].MeshHor);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "ZoomH(%lf) ZoomV(%lf) Pan(%d) Tilt(%d) PanEnd(%d)\n"
            , fisheye_region[i].ZoomH, fisheye_region[i].ZoomV
            , fisheye_region[i].Pan, fisheye_region[i].Tilt, fisheye_region[i].PanEnd);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "InRadius(%lf) OutRadius(%lf) Rect(%d %d %d %d)\n"
            , fisheye_region[i].InRadius, fisheye_region[i].OutRadius
            , fisheye_region[i].OutX, fisheye_region[i].OutY
            , fisheye_region[i].OutW, fisheye_region[i].OutH);
    }
}

static bm_status_t dwa_mesh_gen_fisheye(size_s in_size, size_s out_size, const fisheye_attr_s *pstFisheyeAttr,
                                        uint64_t mesh_phy_addr, void *mesh_vir_addr, rotation_e rot, char *grid)
{
    bm_fisheye_attr *fisheye_config;
    bm_fisheye_region_attr *fisheye_region;
    bmcv_usage_mode UseMode;

    fisheye_config = (bm_fisheye_attr *)calloc(1, sizeof(*fisheye_config));
    if (!fisheye_config) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye config\n");
        free(fisheye_config);
        return BM_ERR_NOMEM;
    }
    fisheye_region = (bm_fisheye_region_attr *)calloc(1, sizeof(*fisheye_region) * MAX_REGION_NUM);
    if (!fisheye_region) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye region config\n");
        free(fisheye_config);
        free(fisheye_region);
        return BM_ERR_NOMEM;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "in_size(%d %d) out_size(%d %d)\n"
        , in_size.width, in_size.height
        , out_size.width, out_size.height);

    if (pstFisheyeAttr->grid_info_attr.enable)
    {
        UseMode = MODE_STEREO_FIT;
    } else {
        UseMode = pstFisheyeAttr->use_mode;
    }

    if (UseMode > 0) {
        double x0, y0, r;   // infos of src_img, (x0,y0) = center of image,  r = radius of image.
        x0 = pstFisheyeAttr->hor_offset;
        y0 = pstFisheyeAttr->ver_offset;
        r = MIN(x0, y0);

        fisheye_config->Enable = true;
        fisheye_config->BgEnable = true;
        fisheye_config->MntMode = pstFisheyeAttr->mount_mode;
        fisheye_config->UsageMode = UseMode;
        fisheye_config->OutW_disp = out_size.width;
        fisheye_config->OutH_disp = out_size.height;
        fisheye_config->BgColor.R = 0;
        fisheye_config->BgColor.G = 0;
        fisheye_config->BgColor.B = 0;
        fisheye_config->InCenterX = x0; // front-end set.
        fisheye_config->InCenterY = y0; // front-end set.
        fisheye_config->InRadius = r;   // front-end set.
        fisheye_config->FStrength = pstFisheyeAttr->fan_strength;

        bm_load_frame_config(fisheye_config);
        bm_load_region_config(fisheye_config, fisheye_region);
    } else
        bm_fisheye_attr_map(pstFisheyeAttr, out_size, fisheye_config, fisheye_region);

    fisheye_config->rotate_index = rot;

    int X_TILE_NUMBER, Y_TILE_NUMBER, TMP_X_TILE_NUMBER, TMP_Y_TILE_NUMBER;
    unsigned int  mesh_id_size, mesh_tbl_size;
    unsigned long long mesh_id_phy_addr, mesh_tbl_phy_addr;

    X_TILE_NUMBER = DIV_UP(in_size.width, DWA_MESH_SLICE_BASE_W);
    Y_TILE_NUMBER = DIV_UP(in_size.height, DWA_MESH_SLICE_BASE_H);
    TMP_X_TILE_NUMBER = DIV_UP(out_size.width, DWA_MESH_SLICE_BASE_W);
    TMP_Y_TILE_NUMBER = DIV_UP(out_size.height, DWA_MESH_SLICE_BASE_H);
    X_TILE_NUMBER = MAX(X_TILE_NUMBER, TMP_X_TILE_NUMBER);
    Y_TILE_NUMBER = MAX(Y_TILE_NUMBER, TMP_Y_TILE_NUMBER);

    // calculate mesh_id/mesh_tbl's size in bytes.
    mesh_tbl_size = 0x60000;
    mesh_id_size = 0x30000;

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

    // Provide virtual address to write mesh.
    reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
    if (pstFisheyeAttr->grid_info_attr.enable == true)
        generate_mesh_on_fisheye(&(pstFisheyeAttr->grid_info_attr), fisheye_config, fisheye_region, X_TILE_NUMBER, Y_TILE_NUMBER
        , (uint16_t *)mesh_vir_addr
        , reorder_mesh_tbl, mesh_tbl_phy_addr, grid);
    else
        generate_mesh_on_fisheye(NULL, fisheye_config, fisheye_region, X_TILE_NUMBER, Y_TILE_NUMBER
        , (uint16_t *)mesh_vir_addr
        , reorder_mesh_tbl, mesh_tbl_phy_addr, (void *)NULL);
    free(fisheye_config);
    free(fisheye_region);

    return BM_SUCCESS;
}

static void generate_mesh_on_faces(bm_mesh_param *param, const affine_attr_s *pstAttr)
{
    const u8 w_knot_num = param->width/pstAttr->dest_size.width + 1;
    const u8 h_knot_num = param->height/pstAttr->dest_size.height + 1;
    // Limit slice's width/height to avoid unnecessary DRAM write(by bg-color)
    u16 width_slice = 0, height_slice = 0;
    u16 i, j, x, y, knot_idx;
    point_s dst_knot_tbl[w_knot_num * h_knot_num];
    point_s dst_mesh_tbl[param->mesh_num][4];
    u16 tbl_idx = 0;
    u32 *mesh_tbl = param->mesh_tbl_addr;

    // generate node
    for (j = 0; j < h_knot_num; ++j) {
        y = j * pstAttr->dest_size.height;
        for (i = 0; i < w_knot_num; ++i) {
            knot_idx = j * w_knot_num + i;
            x = i * pstAttr->dest_size.width;
            dst_knot_tbl[knot_idx].x = x;
            dst_knot_tbl[knot_idx].y = y;
        }
    }

    // map node to each mesh
    for (i = 0; i < param->mesh_num; ++i) {
        knot_idx = i + (i / (param->width/pstAttr->dest_size.width));    // there is 1 more node than mesh each row

        dst_mesh_tbl[i][0] =  dst_knot_tbl[knot_idx];
        dst_mesh_tbl[i][1] =  dst_knot_tbl[knot_idx + 1];
        dst_mesh_tbl[i][2] =  dst_knot_tbl[knot_idx + w_knot_num];
        dst_mesh_tbl[i][3] =  dst_knot_tbl[knot_idx + w_knot_num + 1];

        if (dst_mesh_tbl[i][1].x > width_slice) width_slice = dst_mesh_tbl[i][1].x;
        if (dst_mesh_tbl[i][2].y > height_slice) height_slice = dst_mesh_tbl[i][2].y;

        for (j = 0; j < 4; ++j) {
            mesh_tbl[tbl_idx++] = pstAttr->region_attr[i][j].x * (double)(1 << DEWARP_COORD_NBITS);
            mesh_tbl[tbl_idx++] = pstAttr->region_attr[i][j].y * (double)(1 << DEWARP_COORD_NBITS);
            mesh_tbl[tbl_idx++] = dst_mesh_tbl[i][j].x * (double)(1 << DEWARP_COORD_NBITS);
            mesh_tbl[tbl_idx++] = dst_mesh_tbl[i][j].y * (double)(1 << DEWARP_COORD_NBITS);
        }
    }

    param->width = width_slice;
    param->height = height_slice;
    bm_generate_mesh_id(param, dst_mesh_tbl);
}

static void dwa_mesh_gen_affine(size_s in_size, size_s out_size, const affine_attr_s *pstAffineAttr
    , uint64_t mesh_phy_addr, void *mesh_vir_addr)
{
    const int MAX_MESH_NUM_A_TILE = 4;
    int X_TILE_NUMBER, Y_TILE_NUMBER;
    int owidth, oheight;
    int Y_SUBTILE_NUMBER;
    u32 mesh_id_size, mesh_tbl_size;
    u64 mesh_id_phy_addr, mesh_tbl_phy_addr;

    owidth = out_size.width;
    oheight = out_size.height;
    X_TILE_NUMBER = (DIV_UP(in_size.width, 1024) > 1) ? DIV_UP(in_size.width, 1024) << 1
        : 2;
    Y_TILE_NUMBER = (DIV_UP(in_size.height, 1024) > 1) ? DIV_UP(in_size.height, 1024) << 1
        : 2;

    // calculate mesh_id/mesh_tbl's size in bytes.
    Y_SUBTILE_NUMBER = ceil((float)out_size.height / (float)NUMBER_Y_LINE_A_SUBTILE);
    mesh_tbl_size = X_TILE_NUMBER * Y_TILE_NUMBER * 16 * sizeof(u32);
    mesh_id_size = MAX_MESH_NUM_A_TILE * Y_SUBTILE_NUMBER * X_TILE_NUMBER * sizeof(u16);

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    bm_mesh_param param;

    param.width = owidth;
    param.height = oheight;
    param.mesh_id_addr = mesh_vir_addr;
    param.mesh_tbl_addr = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);;
    param.mesh_tbl_phy_addr = mesh_tbl_phy_addr;
    param.slice_num_w = 1;
    param.slice_num_h = 1;
    param.mesh_num = pstAffineAttr->region_num;
    generate_mesh_on_faces(&param, pstAffineAttr);
}

static bm_status_t dwa_mesh_gen_ldc(size_s in_size, size_s out_size, const ldc_attr_s *pstLDCAttr,
                                    uint64_t mesh_phy_addr, void *mesh_vir_addr, rotation_e rot, char *grid)
{
    bm_fisheye_attr *fisheye_config;
    bm_fisheye_region_attr *fisheye_region;

    fisheye_config = (bm_fisheye_attr*)calloc(1, sizeof(*fisheye_config));
    if (!fisheye_config) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye config\n");
        free(fisheye_config);
        return BM_ERR_FAILURE;
    }
    fisheye_region = (bm_fisheye_region_attr*)calloc(1, sizeof(*fisheye_region) * MAX_REGION_NUM);
    if (!fisheye_region) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye region config\n");
        free(fisheye_config);
        free(fisheye_region);
        return BM_ERR_FAILURE;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "in_size(%d %d) out_size(%d %d)\n"
        , in_size.width, in_size.height
        , out_size.width, out_size.height);

    bm_ldc_attr_map(pstLDCAttr, out_size, fisheye_config, fisheye_region);
    fisheye_config->rotate_index = rot;

    int X_TILE_NUMBER, Y_TILE_NUMBER, TMP_X_TILE_NUMBER, TMP_Y_TILE_NUMBER;
    u32 mesh_id_size;
    u32 mesh_tbl_size;
    u64 mesh_id_phy_addr, mesh_tbl_phy_addr;

    X_TILE_NUMBER = DIV_UP(in_size.width, DWA_MESH_SLICE_BASE_W);
    Y_TILE_NUMBER = DIV_UP(in_size.height, DWA_MESH_SLICE_BASE_H);
    TMP_X_TILE_NUMBER = DIV_UP(out_size.width, DWA_MESH_SLICE_BASE_W);
    TMP_Y_TILE_NUMBER = DIV_UP(out_size.height, DWA_MESH_SLICE_BASE_H);
    X_TILE_NUMBER = MAX(X_TILE_NUMBER, TMP_X_TILE_NUMBER);
    Y_TILE_NUMBER = MAX(Y_TILE_NUMBER, TMP_Y_TILE_NUMBER);

    // calculate mesh_id/mesh_tbl's size in bytes.
    mesh_tbl_size = 0x100000;
    mesh_id_size = 0x50000;

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    int *reorder_mesh_tbl[SLICE_W_CNT_MAX * SLICE_H_CNT_MAX];

    // Provide virtual address to write mesh.
    reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
    if (pstLDCAttr->grid_info_attr.enable == true)
        generate_mesh_on_fisheye(&(pstLDCAttr->grid_info_attr), fisheye_config, fisheye_region, X_TILE_NUMBER, Y_TILE_NUMBER
            , (uint16_t *)mesh_vir_addr, reorder_mesh_tbl, mesh_tbl_phy_addr, grid);
    else
        generate_mesh_on_fisheye(&(pstLDCAttr->grid_info_attr), fisheye_config, fisheye_region, X_TILE_NUMBER, Y_TILE_NUMBER
            , (uint16_t *)mesh_vir_addr, reorder_mesh_tbl, mesh_tbl_phy_addr, (void *)NULL);

    free(fisheye_config);
    free(fisheye_region);

    return BM_SUCCESS;
}

static bm_status_t dwa_mesh_gen_warp(size_s in_size, size_s out_size, const warp_attr_s *pstWarpAttr,
                                     uint64_t mesh_phy_addr, void *mesh_vir_addr, char *grid)
{
    bm_fisheye_attr *fisheye_config;
    bm_fisheye_region_attr *fisheye_region;

    fisheye_config = (bm_fisheye_attr*)calloc(1, sizeof(*fisheye_config));
    if (!fisheye_config) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye config\n");
        free(fisheye_config);
        return BM_ERR_FAILURE;
    }
    fisheye_region = (bm_fisheye_region_attr*)calloc(1, sizeof(*fisheye_region) * MAX_REGION_NUM);
    if (!fisheye_region) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "memory insufficient for fisheye region config\n");
        free(fisheye_config);
        free(fisheye_region);
        return BM_ERR_FAILURE;
    }

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "in_size(%d %d) out_size(%d %d)\n"
        , in_size.width, in_size.height
        , out_size.width, out_size.height);

    fisheye_config->MntMode = 0;
    fisheye_config->OutW_disp = out_size.width;
    fisheye_config->OutH_disp = out_size.height;
    fisheye_config->FStrength = 0;
    fisheye_config->TCoef = 0;

    fisheye_config->RgnNum = 1;

    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "OutW_disp(%d) OutH_disp(%d)\n", fisheye_config->OutW_disp, fisheye_config->OutH_disp);

    fisheye_region[0].RegionValid = 1;
    fisheye_region[0].ViewMode = PROJECTION_STEREO_FIT;

    int X_TILE_NUMBER, Y_TILE_NUMBER;
    u32 mesh_id_size;
    u32 mesh_tbl_size;
    u64 mesh_id_phy_addr, mesh_tbl_phy_addr;

    X_TILE_NUMBER = DIV_UP(in_size.width, DWA_MESH_SLICE_BASE_W);
    Y_TILE_NUMBER = DIV_UP(in_size.height, DWA_MESH_SLICE_BASE_H);

    // calculate mesh_id/mesh_tbl's size in bytes.
    mesh_tbl_size = 0x60000;
    mesh_id_size = 0x30000;

    // Decide the position of mesh in memory.
    mesh_id_phy_addr = mesh_phy_addr;
    mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
                , mesh_id_phy_addr, mesh_tbl_phy_addr);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

    int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

    // Provide virtual address to write mesh.
    reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
    generate_mesh_on_fisheye(&(pstWarpAttr->grid_info_attr), fisheye_config, fisheye_region, X_TILE_NUMBER, Y_TILE_NUMBER
        , (uint16_t *)mesh_vir_addr, reorder_mesh_tbl, mesh_tbl_phy_addr, grid);
    free(fisheye_config);
    free(fisheye_region);

    return BM_SUCCESS;
}

static bm_status_t bm_dwa_add_ldc_task(bm_handle_t handle, int fd, GDC_HANDLE hHandle, gdc_task_attr_s *pstTask
    , const ldc_attr_s *pstLDCAttr, rotation_e enRotation, char *grid)
{
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(ID_GDC, pstTask);
    MOD_CHECK_NULL_PTR(ID_GDC, pstLDCAttr);
    CHECK_DWA_FORMAT(pstTask->img_in, pstTask->img_out);

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

    struct gdc_task_attr attr;
    u8 idx;
    pthread_mutex_lock(&mutex);
    ret = bm_get_valid_tsk_mesh_by_name(handle, pstTask->name, &idx);

    if (ret != BM_SUCCESS) {
        size_s in_size, out_size;
        u32 mesh_1st_size, mesh_2nd_size;
        u64 paddr;
        bm_device_mem_t pmem;

        in_size.width = pstTask->img_in.video_frame.width;
        in_size.height = pstTask->img_in.video_frame.height;
        out_size.width = pstTask->img_out.video_frame.width;
        out_size.height = pstTask->img_out.video_frame.height;
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
        /*
        // fwrite mesh_id_table & mesh_table for debug
        char mesh_name[128];
        snprintf(mesh_name, 128, "./fwrite_gen_mesh_%d_%d_%d_%d_%d_%d_%d.bin", pstLDCAttr->aspect, pstLDCAttr->x_ratio, pstLDCAttr->y_ratio,
                 pstLDCAttr->xy_ratio, pstLDCAttr->center_x_offset, pstLDCAttr->center_y_offset, pstLDCAttr->distortion_ratio);
        FILE *fp = fopen(mesh_name, "wb");
        if (!fp) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "open file[%s] failed.\n", mesh_name);
            free(buffer);
            fclose(fp);
            return BM_ERR_FAILURE;
        }
        if (fwrite((void *)buffer, 0x150000, 1, fp) != 1) {
            free(buffer);
            fclose(fp);
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "fwrite data error\n");
        }
        fclose(fp);
        */
        ret = bm_memcpy_s2d(handle, pmem, (void*)buffer);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_memcpy_s2d failed!\n");
            free(buffer);
            pthread_mutex_unlock(&mutex);
            return BM_ERR_NOMEM;
        }
        free(buffer);

        dwa_tskMesh[idx].mem = pmem;
    }
    pthread_mutex_unlock(&mutex);

    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.img_in, &pstTask->img_in, sizeof(attr.img_in));
    memcpy(&attr.img_out, &pstTask->img_out, sizeof(attr.img_out));
    //memcpy(attr.private_data, pstTask->private_data, sizeof(attr.private_data));
    memcpy(&attr.ldc_attr, pstLDCAttr, sizeof(*pstLDCAttr));
    attr.reserved = pstTask->reserved;
    attr.private_data[0] = dwa_tskMesh[idx].mem.u.device.device_addr;
    attr.private_data[3] = pstTask->privatedata[3];

    pstTask->privatedata[0] = dwa_tskMesh[idx].mem.u.device.device_addr;
    pstTask->privatedata[1] = (u64)((uintptr_t)dwa_tskMesh[idx].mem.u.system.system_addr);
    return dwa_add_ldc_task(fd, &attr);
}

static bm_status_t bm_dwa_add_rotation_task(bm_handle_t handle, int fd, DWA_HANDLE hHandle, gdc_task_attr_s *pstTask, rotation_e enRotation)
{
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(ID_GDC, pstTask);
    CHECK_DWA_FORMAT(pstTask->img_in, pstTask->img_out);

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

    struct gdc_task_attr attr;
    u64 paddr;
    bm_device_mem_t pmem;
    size_s in_size, out_size;

    in_size.width = pstTask->img_in.video_frame.width;
    in_size.height = pstTask->img_in.video_frame.height;
    out_size.width = pstTask->img_out.video_frame.width;
    out_size.height = pstTask->img_out.video_frame.height;

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

        dwa_tskMesh[idx].mem = pmem;
    }
    pthread_mutex_unlock(&mutex);
    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.img_in, &pstTask->img_in, sizeof(attr.img_in));
    memcpy(&attr.img_out, &pstTask->img_out, sizeof(attr.img_out));
    //memcpy(attr.private_data, pstTask->private_data, sizeof(attr.private_data));
    attr.reserved = pstTask->reserved;
    attr.rotation = enRotation;
    attr.private_data[0] = dwa_tskMesh[idx].mem.u.device.device_addr;

    pstTask->privatedata[0] = dwa_tskMesh[idx].mem.u.device.device_addr;
    pstTask->privatedata[1] = (u64)((uintptr_t)dwa_tskMesh[idx].mem.u.system.system_addr);
    return dwa_add_rotation_task(fd, &attr);
}

static bm_status_t bm_dwa_add_correction_task(bm_handle_t handle, int fd, GDC_HANDLE hHandle, gdc_task_attr_s *pstTask,
                const fisheye_attr_s *pstFishEyeAttr, char *grid){
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(ID_GDC, pstTask);
    MOD_CHECK_NULL_PTR(ID_GDC, pstFishEyeAttr);
    CHECK_DWA_FORMAT(pstTask->img_in, pstTask->img_out);

    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_FAILURE;
    }

    if (pstFishEyeAttr->enable) {
        if(!pstFishEyeAttr->grid_info_attr.enable) {
            if (pstFishEyeAttr->region_num == 0) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "RegionNum(%d) can't be 0 if enable fisheye.\n",
                            pstFishEyeAttr->region_num);
                return BM_ERR_FAILURE;
            }
            if (pstFishEyeAttr->use_mode == MODE_01_1O || pstFishEyeAttr->use_mode == MODE_STEREO_FIT) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "FISHEYE not support MODE_01_1O and MODE_STEREO_FIT.\n");
                return BM_ERR_PARAM;
            }
            if (((u32)pstFishEyeAttr->hor_offset > pstTask->img_in.video_frame.width) ||
                ((u32)pstFishEyeAttr->ver_offset > pstTask->img_in.video_frame.height)) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "center pos(%d %d) out of frame size(%d %d).\n",
                            pstFishEyeAttr->hor_offset, pstFishEyeAttr->ver_offset,
                            pstTask->img_in.video_frame.width, pstTask->img_in.video_frame.height);
                return BM_ERR_PARAM;
            }
            for (u32 i = 0; i < pstFishEyeAttr->region_num; ++i) {
                if ((pstFishEyeAttr->mount_mode == FISHEYE_WALL_MOUNT) &&
                    (pstFishEyeAttr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_360_PANORAMA)) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Rgn(%d): WALL_MOUNT not support Panorama_360.\n", i);
                    return BM_ERR_PARAM;
                }
                if ((pstFishEyeAttr->mount_mode == FISHEYE_CEILING_MOUNT) &&
                    (pstFishEyeAttr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_180_PANORAMA)) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Rgn(%d): CEILING_MOUNT not support Panorama_180.\n", i);
                    return BM_ERR_PARAM;
                }
                if ((pstFishEyeAttr->mount_mode == FISHEYE_DESKTOP_MOUNT) &&
                    (pstFishEyeAttr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_180_PANORAMA)) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Rgn(%d): DESKTOP_MOUNT not support Panorama_180.\n", i);
                    return BM_ERR_PARAM;
                }
            }
        }
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "FishEyeAttr is not be enabled.\n");
        return BM_ERR_FAILURE;
    }

    struct gdc_task_attr attr;

    u8 idx;
    pthread_mutex_lock(&mutex);
    ret = bm_get_valid_tsk_mesh_by_name(handle, pstTask->name, &idx);
    if (ret != BM_SUCCESS) {
        size_s in_size, out_size;
        u64 paddr;
        bm_device_mem_t pmem;

        in_size.width = pstTask->img_in.video_frame.width;
        in_size.height = pstTask->img_in.video_frame.height;
        out_size.width = pstTask->img_out.video_frame.width;
        out_size.height = pstTask->img_out.video_frame.height;

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

        dwa_tskMesh[idx].mem = pmem;
    }
    pthread_mutex_unlock(&mutex);

    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.img_in, &pstTask->img_in, sizeof(attr.img_in));
    memcpy(&attr.img_out, &pstTask->img_out, sizeof(attr.img_out));
    //memcpy(attr.private_data, pstTask->private_data, sizeof(attr.private_data));
    memcpy(&attr.fisheye_attr, pstFishEyeAttr, sizeof(*pstFishEyeAttr));
    attr.reserved = pstTask->reserved;
    attr.private_data[0] = dwa_tskMesh[idx].mem.u.device.device_addr;
    attr.private_data[3] = pstTask->privatedata[3];

    pstTask->privatedata[0] = dwa_tskMesh[idx].mem.u.device.device_addr;
    pstTask->privatedata[1] = (u64)((uintptr_t)dwa_tskMesh[idx].mem.u.system.system_addr);
    return dwa_add_correction_task(fd, &attr);

}

static bm_status_t bm_dwa_add_affine_task(bm_handle_t handle, int fd, GDC_HANDLE hHandle, gdc_task_attr_s *pstTask, const affine_attr_s *pstAffineAttr)
{
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(ID_GDC, pstTask);
    MOD_CHECK_NULL_PTR(ID_GDC, pstAffineAttr);
    CHECK_DWA_FORMAT(pstTask->img_in, pstTask->img_out);

    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_FAILURE;
    }

    if (pstAffineAttr->region_num == 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "u32RegionNum(%d) can't be zero.\n", pstAffineAttr->region_num);
        return BM_ERR_FAILURE;
    }

    if (pstAffineAttr->dest_size.width > pstTask->img_out.video_frame.width) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dest's width(%d) can't be larger than frame's width(%d)\n",
                    pstAffineAttr->dest_size.width, pstTask->img_out.video_frame.width);
        return BM_ERR_FAILURE;
    }
    for (u32 i = 0; i < pstAffineAttr->region_num; ++i) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "region_num(%d) (%f, %f) (%f, %f) (%f, %f) (%f, %f)\n", i,
                    pstAffineAttr->region_attr[i][0].x, pstAffineAttr->region_attr[i][0].y,
                    pstAffineAttr->region_attr[i][1].x, pstAffineAttr->region_attr[i][1].y,
                    pstAffineAttr->region_attr[i][2].x, pstAffineAttr->region_attr[i][2].y,
                    pstAffineAttr->region_attr[i][3].x, pstAffineAttr->region_attr[i][3].y);
        if ((pstAffineAttr->region_attr[i][0].x < 0) || (pstAffineAttr->region_attr[i][0].y < 0) ||
            (pstAffineAttr->region_attr[i][1].x < 0) || (pstAffineAttr->region_attr[i][1].y < 0) ||
            (pstAffineAttr->region_attr[i][2].x < 0) || (pstAffineAttr->region_attr[i][2].y < 0) ||
            (pstAffineAttr->region_attr[i][3].x < 0) || (pstAffineAttr->region_attr[i][3].y < 0)) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "region_num(%d) affine point can't be negative\n", i);
            return BM_ERR_FAILURE;
        }
        if ((pstAffineAttr->region_attr[i][1].x < pstAffineAttr->region_attr[i][0].x) ||
            (pstAffineAttr->region_attr[i][3].x < pstAffineAttr->region_attr[i][2].x)) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "region_num(%d) point1/3's x should be bigger than 0/2's\n", i);
            return BM_ERR_FAILURE;
        }
        if ((pstAffineAttr->region_attr[i][2].y < pstAffineAttr->region_attr[i][0].y) ||
            (pstAffineAttr->region_attr[i][3].y < pstAffineAttr->region_attr[i][1].y)) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "region_num(%d) point2/3's y should be bigger than 0/1's\n", i);
            return BM_ERR_FAILURE;
        }
    }

    struct gdc_task_attr attr;

    u8 idx;
    pthread_mutex_lock(&mutex);
    ret = bm_get_valid_tsk_mesh_by_name(handle, pstTask->name, &idx);
    if (ret != BM_SUCCESS) {
        size_s in_size, out_size;
        u64 paddr;
        bm_device_mem_t pmem;

        in_size.width = pstTask->img_in.video_frame.width;
        in_size.height = pstTask->img_in.video_frame.height;
        out_size.width = pstTask->img_out.video_frame.width;
        out_size.height = pstTask->img_out.video_frame.height;

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

        dwa_tskMesh[idx].mem = pmem;
    }
    pthread_mutex_unlock(&mutex);

    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.img_in, &pstTask->img_in, sizeof(attr.img_in));
    memcpy(&attr.img_out, &pstTask->img_out, sizeof(attr.img_out));
    //memcpy(attr.private_data, pstTask->private_data, sizeof(attr.private_data));
    memcpy(&attr.affine_attr, pstAffineAttr, sizeof(*pstAffineAttr));
    attr.reserved = pstTask->reserved;
    attr.private_data[0] = dwa_tskMesh[idx].mem.u.device.device_addr;

    pstTask->privatedata[0] = dwa_tskMesh[idx].mem.u.device.device_addr;
    pstTask->privatedata[1] = (u64)((uintptr_t)dwa_tskMesh[idx].mem.u.system.system_addr);
    return dwa_add_affine_task(fd, &attr);
}

static bm_status_t bm_dwa_add_dewarp_task(bm_handle_t handle, int fd, GDC_HANDLE hHandle, gdc_task_attr_s *pstTask
    , const warp_attr_s *pstWarpAttr, char *grid)
{
    bm_status_t ret = BM_SUCCESS;
    MOD_CHECK_NULL_PTR(ID_GDC, pstTask);
    MOD_CHECK_NULL_PTR(ID_GDC, pstWarpAttr);
    CHECK_DWA_FORMAT(pstTask->img_in, pstTask->img_out);

    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle.\n");
        return BM_ERR_FAILURE;
    }

    if (!pstWarpAttr->enable)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "WarpAttr is not be enabled.\n");
        return BM_ERR_FAILURE;
    }

    if (!pstWarpAttr->grid_info_attr.enable)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "GridInfoAttr is not be enabled.\n");
        return BM_ERR_FAILURE;
    }

    struct gdc_task_attr attr;

    u8 idx;
    pthread_mutex_lock(&mutex);
    ret = bm_get_valid_tsk_mesh_by_name(handle, pstTask->name, &idx);
    if (ret != BM_SUCCESS) {
        size_s in_size, out_size;
        u64 paddr;
        bm_device_mem_t pmem;

        in_size.width = pstTask->img_in.video_frame.width;
        in_size.height = pstTask->img_in.video_frame.height;
        out_size.width = pstTask->img_out.video_frame.width;
        out_size.height = pstTask->img_out.video_frame.height;

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

        dwa_tskMesh[idx].mem = pmem;
    }
    pthread_mutex_unlock(&mutex);

    memset(&attr, 0, sizeof(attr));
    attr.handle = hHandle;
    memcpy(&attr.img_in, &pstTask->img_in, sizeof(attr.img_in));
    memcpy(&attr.img_out, &pstTask->img_out, sizeof(attr.img_out));
    //memcpy(attr.private_data, pstTask->private_data, sizeof(attr.private_data));
    memcpy(&attr.warp_attr, pstWarpAttr, sizeof(*pstWarpAttr));
    attr.reserved = pstTask->reserved;
    attr.private_data[0] = dwa_tskMesh[idx].mem.u.device.device_addr;

    pstTask->privatedata[0] = dwa_tskMesh[idx].mem.u.device.device_addr;
    pstTask->privatedata[1] = (u64)((uintptr_t)dwa_tskMesh[idx].mem.u.system.system_addr);
    return dwa_add_warp_task(fd, &attr);
}

static bm_status_t bm_dwa_add_task(bm_handle_t handle, int fd, bm_dwa_basic_param *param, void *ptr)
{
    rotation_e enRotation;
    affine_attr_s *stAffineAttr;
    bm_dewarp_attr_and_grid *dewarp_attr_and_grid;
    bm_gdc_attr_and_grid *gdc_attr_and_grid;
    bm_fisheye_attr_and_grid *fisheye_attr_and_grid;

    s32 ret = BM_ERR_FAILURE;

    if (!param) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dwa_basic fail, null ptr for test param\n");
        return BM_ERR_FAILURE;
    }

    switch(param->op){
        case DWA_TEST_FISHEYE:
            fisheye_attr_and_grid = (bm_fisheye_attr_and_grid *)ptr;

            ret = bm_dwa_add_correction_task(handle, fd, param->hHandle, &param->stTask, &(fisheye_attr_and_grid->fisheye_attr), fisheye_attr_and_grid->grid);
            if(ret) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_add_correction_task failed\n");
            }
            break;
        case DWA_TEST_ROT:
            enRotation = *(rotation_e *)ptr;

            ret = bm_dwa_add_rotation_task(handle, fd, param->hHandle, &param->stTask, enRotation);
            if(ret){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_add_rotation_task failed\n");
            }
            break;
        case DWA_TEST_LDC:
            enRotation = (rotation_e)param->stTask.reserved;
            gdc_attr_and_grid = (bm_gdc_attr_and_grid *)ptr;

            ret = bm_dwa_add_ldc_task(handle, fd, param->hHandle, &param->stTask, &(gdc_attr_and_grid->ldc_attr), enRotation, gdc_attr_and_grid->grid);
            if (ret){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_add_ldc_task failed\n");
            }
            break;
        case DWA_TEST_AFFINE:
            stAffineAttr = (affine_attr_s *)ptr;

            ret = bm_dwa_add_affine_task(handle, fd, param->hHandle, &param->stTask, stAffineAttr);
            if(ret){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_add_affine_task failed\n");
            }
            break;
        case DWA_TEST_DEWARP:
            dewarp_attr_and_grid = (bm_dewarp_attr_and_grid *)ptr;

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

// static bm_status_t bm_dwa_init(int fd)
// {
//     bm_status_t ret = BM_SUCCESS;
//     // if (bm_get_dwa_fd(&fd) != BM_SUCCESS)
//     //     return BM_ERR_FAILURE;

//     ret = (bm_status_t)dwa_init(fd);
//     if (ret != BM_SUCCESS) {
//         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "dwa init fail\n");
//         return ret;
//     }

//     return ret;
// }

static bm_status_t bm_dwa_begin_job(int fd, DWA_HANDLE *phHandle)
{
    MOD_CHECK_NULL_PTR(ID_GDC, phHandle);

    struct gdc_handle_data cfg;

    memset(&cfg, 0, sizeof(cfg));

    if (dwa_begin_job(fd, &cfg))
        return BM_ERR_FAILURE;

    *phHandle = cfg.handle;
    return BM_SUCCESS;
}

static bm_status_t bm_dwa_set_job_identity(int fd, DWA_HANDLE hHandle,gdc_identity_attr_s *identity_attr)
{
    MOD_CHECK_NULL_PTR(ID_GDC, identity_attr);

    if(!hHandle){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_FAILURE;
    }

    struct gdc_identity_attr cfg = {0};
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
    pixel_format_e pixel_format = 0;

    memset(&param->stVideoFrameIn,0,sizeof(param->stVideoFrameIn));
    memset(&param->stVideoFrameOut,0,sizeof(param->stVideoFrameOut));

    bm_image_format_to_cvi(input_image->image_format, input_image->data_type, &pixel_format);
    bm_image_format_to_cvi(output_image->image_format, output_image->data_type, &pixel_format);
    bm_image_to_cvi_frame(input_image, pixel_format, &param->stVideoFrameIn);
    bm_image_to_cvi_frame(output_image, pixel_format, &param->stVideoFrameOut);

    memcpy(&param->stTask.img_in, &param->stVideoFrameIn, sizeof(param->stVideoFrameIn));
    memcpy(&param->stTask.img_out, &param->stVideoFrameOut, sizeof(param->stVideoFrameOut));

    return ret;
}

static bm_status_t bm_dwa_end_job(int fd, GDC_HANDLE hHandle)
{
    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "null hHandle");
        return BM_ERR_PARAM;
    }

    struct gdc_handle_data cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.handle = hHandle;
    return dwa_end_job(fd, &cfg);
}

bm_status_t bm_dwa_get_frame(int fd,
                             bm_image *output_image,
                             bm_dwa_basic_param *param)
{
    bm_status_t ret = BM_SUCCESS;
    pixel_format_e pixel_format = 0;

    bm_image_format_to_cvi(output_image->image_format, output_image->data_type, &pixel_format);
    bm_image_to_cvi_frame(output_image, pixel_format, &param->stVideoFrameOut);

    // memcpy(&param->stTask.stImgOut, &param->stVideoFrameOut, sizeof(param->stVideoFrameOut));
    memcpy(&param->stVideoFrameOut, &param->stTask.img_out, sizeof(param->stTask.img_out));

    for(int i = 0; i < output_image->image_private->plane_num; i++){
        output_image->image_private->data[i].u.device.device_addr = param->stVideoFrameOut.video_frame.phyaddr[i];
    }

    return ret;
}

bm_status_t bm_dwa_get_chn_frame(int fd, gdc_identity_attr_s *identity_attr, video_frame_info_s *pstFrameInfo, int s32MilliSec)
{
    bm_status_t ret;
    struct gdc_chn_frm_cfg cfg;

    MOD_CHECK_NULL_PTR(ID_GDC, identity_attr);
    MOD_CHECK_NULL_PTR(ID_GDC, pstFrameInfo);

    memset(&cfg, 0, sizeof(cfg));
    memcpy(&cfg.identity.attr, identity_attr, sizeof(*identity_attr));
    cfg.milli_sec = s32MilliSec;

    ret = dwa_get_chn_frm(fd, &cfg);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "identity[%s-%d-%d] get chn frame fail, Ret[%d]\n", identity_attr->name , identity_attr->mod_id, identity_attr->id, ret);
        return BM_ERR_FAILURE;
    }
    memcpy(pstFrameInfo, &cfg.video_frame, sizeof(*pstFrameInfo));

    return ret;
}

bm_status_t bm_dwa_cancel_job(int fd, GDC_HANDLE hHandle)
{
    bm_status_t ret = BM_SUCCESS;
    if (!hHandle) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_cancel_job failed, null hHandle for test param.\n");
        return BM_ERR_PARAM;
    }

    struct gdc_handle_data cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.handle = hHandle;
    ret = (bm_status_t)ioctl(fd, LDC_CANCEL_JOB, &cfg);
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

    // ret = bm_dwa_init(fd);
    // if (ret != BM_SUCCESS) {
    //     bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "BM_DWA_Init failed!\n");
    //     return BM_ERR_FAILURE;
    // }

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

    // ret = bm_dwa_get_chn_frame(fd, &param->identity, &param->stVideoFrameOut, 500);
    // if (ret != BM_SUCCESS) {
    //     bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bm_dwa_get_chn_frame failed!\n");
    //     ret = BM_ERR_FAILURE;
    //     goto fail;
    // }

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
    rotation_e rotation_mode = (rotation_e)rot_mode;
    bm_rot_name rot_name;
    char md5_str[MD5_STRING_LENGTH + 1];
    memset(&rot_name, 0, sizeof(rot_name));
    memset(&param, 0, sizeof(param));

    param.size_in.width = input_image.width;
    param.size_in.height = input_image.height;
    param.size_out.width = output_image.width;
    param.size_out.height = output_image.height;

    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.mod_id = ID_GDC;
    param.identity.id = 0;

    param.identity.sync_io = true;
    param.op = DWA_TEST_ROT;

    rot_name.param = param;
    rot_name.rotation_mode = rotation_mode;
    md5_get((unsigned char *)&rot_name, sizeof(rot_name), md5_str, 0);

    snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, "%s", md5_str);
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
    bm_gdc_attr_and_grid gdc_with_grid;
    char md5_str[MD5_STRING_LENGTH + 1];
    memset(&gdc_name, 0, sizeof(gdc_name));
    memset(&param, 0, sizeof(param));
    memset(&gdc_with_grid, 0, sizeof(gdc_with_grid));

    param.size_in.width = input_image.width;
    param.size_in.height = input_image.height;
    param.size_out.width = output_image.width;
    param.size_out.height = output_image.height;

    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.mod_id = ID_GDC;
    param.identity.id = 0;

    param.identity.sync_io = true;
    if (ldc_attr.bBgColor == true)
        param.stTask.privatedata[3] = ldc_attr.u32BgColor;
    param.op = DWA_TEST_LDC;

    if (ldc_attr.grid_info.size == 0) {
        gdc_with_grid.ldc_attr.aspect = ldc_attr.bAspect;
        gdc_with_grid.ldc_attr.center_x_offset = ldc_attr.s32CenterXOffset;
        gdc_with_grid.ldc_attr.center_y_offset = ldc_attr.s32CenterYOffset;
        gdc_with_grid.ldc_attr.distortion_ratio = ldc_attr.s32DistortionRatio;
        gdc_with_grid.ldc_attr.x_ratio = ldc_attr.s32XRatio;
        gdc_with_grid.ldc_attr.xy_ratio = ldc_attr.s32XYRatio;
        gdc_with_grid.ldc_attr.y_ratio = ldc_attr.s32YRatio;
        gdc_with_grid.ldc_attr.grid_info_attr.enable = false;
        gdc_with_grid.grid = NULL;

        gdc_name.param = param;
        gdc_name.ldc_attr = gdc_with_grid.ldc_attr;

        md5_get((unsigned char *)&gdc_name, sizeof(gdc_name), md5_str, 0);
        snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, "%s", md5_str);
        // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_gdc");
    } else {
        gdc_with_grid.ldc_attr.grid_info_attr.enable = true;
        gdc_with_grid.grid = (char *)ldc_attr.grid_info.u.system.system_addr;

        gdc_name.param = param;
        gdc_name.ldc_attr = gdc_with_grid.ldc_attr;

        char md5_grid_info[MD5_STRING_LENGTH + 1];
        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "ldc_attr.grid_size = [%d]\n", ldc_attr.grid_info.size);
        md5_get((unsigned char *)gdc_with_grid.grid, ldc_attr.grid_info.size, md5_grid_info, 0);
        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "ldc_attr.grid_info[0] = [%d]\n", gdc_with_grid.grid[0]);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "ldc_attr.grid_info[1] = [%d]\n", gdc_with_grid.grid[1]);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "ldc_attr.grid_info[2] = [%d]\n", gdc_with_grid.grid[2]);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "ldc_attr.grid_info[3] = [%d]\n", gdc_with_grid.grid[3]);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "ldc_attr.grid_info[4] = [%d]\n", gdc_with_grid.grid[4]);
        strcpy(gdc_with_grid.ldc_attr.grid_info_attr.grid_bind_name, md5_grid_info);
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "grid_info md5 = [%s]\n", md5_grid_info);
        strcpy(gdc_name.grid, md5_grid_info);
        md5_get((unsigned char *)&gdc_name, sizeof(gdc_name), md5_str, 0);
        snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, "%s", md5_str);
        strcpy(gdc_with_grid.ldc_attr.grid_info_attr.grid_file_name, "grid_info_79_43_3397_80_45_1280x720.dat");
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
    fisheye_attr_s dwa_fisheye_attr;
    bm_fisheye_name fisheye_name;
    bm_fisheye_attr_and_grid fisheye_with_grid;
    char md5_str[MD5_STRING_LENGTH + 1];
    memset(&param, 0, sizeof(param));
    memset(&dwa_fisheye_attr, 0, sizeof(dwa_fisheye_attr));
    memset(&fisheye_with_grid, 0, sizeof(fisheye_with_grid));

    param.size_in.width = input_image.width;
    param.size_in.height = input_image.height;
    param.size_out.width = output_image.width;
    param.size_out.height = output_image.height;
    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.mod_id = ID_GDC;
    param.identity.id = 0;
    param.identity.sync_io = true;
    if (fisheye_attr.bBgColor == true)
        param.stTask.privatedata[3] = fisheye_attr.u32BgColor;

    param.op = DWA_TEST_FISHEYE;

    if (fisheye_attr.grid_info.size == 0) {
        fisheye_with_grid.fisheye_attr.enable = fisheye_attr.bEnable;
        fisheye_with_grid.fisheye_attr.enable_bg_color = fisheye_attr.bBgColor;
        fisheye_with_grid.fisheye_attr.bg_color = fisheye_attr.u32BgColor;
        fisheye_with_grid.fisheye_attr.hor_offset = fisheye_attr.s32HorOffset;
        fisheye_with_grid.fisheye_attr.ver_offset = fisheye_attr.s32VerOffset;
        fisheye_with_grid.fisheye_attr.mount_mode = (fisheye_mount_mode_e)fisheye_attr.enMountMode;
        fisheye_with_grid.fisheye_attr.use_mode = (usage_mode)fisheye_attr.enUseMode;
        fisheye_with_grid.fisheye_attr.fisheye_region_attr[0].view_mode = (fisheye_view_mode_e)(fisheye_attr.enViewMode);
        fisheye_with_grid.fisheye_attr.region_num = fisheye_attr.u32RegionNum;
        fisheye_with_grid.fisheye_attr.grid_info_attr.enable = false;
        fisheye_with_grid.grid = NULL;

        fisheye_name.param = param;
        fisheye_name.fisheye_attr = fisheye_with_grid.fisheye_attr;
        md5_get((unsigned char *)&fisheye_name, sizeof(fisheye_name), md5_str, 0);
        snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, "%s", md5_str);
        // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_fisheye");
    } else {
        fisheye_with_grid.fisheye_attr.enable = fisheye_attr.bEnable;
        fisheye_with_grid.fisheye_attr.grid_info_attr.enable = true;
        fisheye_with_grid.grid = (char *)fisheye_attr.grid_info.u.system.system_addr;

        fisheye_name.param = param;
        fisheye_name.fisheye_attr = fisheye_with_grid.fisheye_attr;
        char md5_grid_info[MD5_STRING_LENGTH + 1];
        // for debug
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "fisheye_attr.grid_size = [%d]\n", fisheye_attr.grid_info.size);
        md5_get((unsigned char *)fisheye_with_grid.grid, fisheye_attr.grid_info.size, md5_grid_info, 0);
        strcpy(fisheye_with_grid.fisheye_attr.grid_info_attr.grid_bind_name, md5_grid_info);
        strcpy(fisheye_name.grid, md5_grid_info);
        md5_get((unsigned char *)&fisheye_name, sizeof(fisheye_name), md5_str, 0);
        snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, "%s", md5_str);
        strcpy(fisheye_with_grid.fisheye_attr.grid_info_attr.grid_file_name, "L_grid_info_68_68_4624_70_70_dst_2240x2240_src_2240x2240.dat");
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
    affine_attr_s dwa_affine_attr;
    bm_affine_name affine_name;
    char md5_str[MD5_STRING_LENGTH + 1];
    memset(&param, 0, sizeof(param));
    memset(&dwa_affine_attr, 0, sizeof(dwa_affine_attr));
    memset(&affine_name, 0, sizeof(affine_name));

    param.size_in.width = input_image.width;
    param.size_in.height = input_image.height;
    param.size_out.width = output_image.width;
    param.size_out.height = output_image.height;
    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.mod_id = ID_GDC;
    param.identity.id = 0;
    param.identity.sync_io = true;

    param.op = DWA_TEST_AFFINE;

    affine_name.param = param;
    affine_name.affine_attr = dwa_affine_attr;
    md5_get((unsigned char *)&affine_name, sizeof(affine_name), md5_str, 0);
    snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, "%s", md5_str);
    // snprintf(param.identity.Name, sizeof(param.identity.Name), "job_affine");

    dwa_affine_attr.region_num = affine_attr.u32RegionNum;
    memcpy(dwa_affine_attr.region_attr, affine_attr.astRegionAttr, sizeof(affine_attr.astRegionAttr));    // user input
    dwa_affine_attr.dest_size.width = affine_attr.stDestSize.u32Width;
    dwa_affine_attr.dest_size.height = affine_attr.stDestSize.u32Height;

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
    bm_dewarp_attr_and_grid dewarp_with_grid;
    memset(&param, 0, sizeof(param));
    memset(&dewarp_name, 0, sizeof(dewarp_name));
    memset(&dewarp_with_grid, 0, sizeof(dewarp_with_grid));

    char md5_str[MD5_STRING_LENGTH + 1];
    param.size_in.width = input_image.width;
    param.size_in.height = input_image.height;
    param.size_out.width = output_image.width;
    param.size_out.height = output_image.height;
    bm_image_format_to_cvi(input_image.image_format, input_image.data_type, &param.enPixelFormat);
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "param.enPixelFormat = [%d]\n", param.enPixelFormat);
    param.identity.mod_id = ID_GDC;
    param.identity.id = 0;
    param.identity.sync_io = true;

    param.op = DWA_TEST_DEWARP;

    dewarp_with_grid.dewarp_attr.enable = true;
    dewarp_with_grid.dewarp_attr.grid_info_attr.enable = true;
    dewarp_with_grid.grid = (char *)grid_info.u.system.system_addr;
    dewarp_name.param = param;
    dewarp_name.dewarp_attr = dewarp_with_grid.dewarp_attr;

    char md5_grid_info[MD5_STRING_LENGTH + 1];
    // for debug
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_TRACE, "dewarp_attr.grid_size = [%d]\n", grid_info.size);
    md5_get((unsigned char *)dewarp_with_grid.grid, grid_info.size, md5_grid_info, 0);
    strcpy(dewarp_with_grid.dewarp_attr.grid_info_attr.grid_bind_name, md5_grid_info);
    strcpy(dewarp_name.grid, md5_grid_info);
    md5_get((unsigned char *)&dewarp_name, sizeof(dewarp_name), md5_str, 0);
    snprintf(param.stTask.name, sizeof(param.stTask.name) + 1, "%s", md5_str);
    strcpy(dewarp_with_grid.dewarp_attr.grid_info_attr.grid_file_name, "grid_info_79_43_3397_80_45_1280x720.dat");

    ret = bm_dwa_basic(handle, input_image, output_image, &param, (void *)&dewarp_with_grid);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dwa basic failed \n");
    }
    return ret;
}
#endif