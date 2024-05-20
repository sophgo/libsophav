#ifndef BMCV_A2_DWA_INTERNAL_H
#define BMCV_A2_DWA_INTERNAL_H

#define _USE_MATH_DEFINES

#include <sys/ioctl.h>
#include <time.h>
#include <math.h>
#include "bmcv_a2_common_internal.h"

#define BM_TRUE                1
#define MAX_REGION_NUM 5
#define MAX_ION_BUFFER_NAME 32
#define DWA_MESH_SLICE_BASE_W 2048
#define DWA_MESH_SLICE_BASE_H (DWA_MESH_SLICE_BASE_W)
#define DWA_MAX_FRAME_MESH_NUM 16384
#define CLOCK_MONOTONIC 1
#define MESH_DATA_MAX_NUM 8
#define UI_CTRL_VALUE_CENTER 180
#define DPU_MODE 1
#define WITHOUT_BIAS (0)
#define M_PI            3.14159265358979323846
#define OFFSET_SCALE (1) // new method

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define FISHEYE_MAX_REGION_NUM 4
#define AFFINE_MAX_REGION_NUM 32
#define DWA_MAX_TSK_MESH (32)
#define DWA_MESH_SIZE_ROT 0x60000
#define DWA_MESH_SIZE_AFFINE 0x20000
#define DWA_MESH_SIZE_FISHEYE 0xB0000

#define IS_VDEV_CLOSED(x) ((x) == VDEV_STATE_CLOSED)
#define IS_VDEV_OPEN(x) ((x) == VDEV_STATE_OPEN)
#define IS_VDEV_RUN(x) ((x) == VDEV_STATE_RUN)
#define IS_VDEV_STOP(x) ((x) == VDEV_STATE_STOP)

#define MESH_HOR_DEFAULT 16
#define MESH_VER_DEFAULT 16
#define DEWARP_COORD_MBITS 13
#define DEWARP_COORD_NBITS 18
#define NUMBER_Y_LINE_A_SUBTILE 4

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define minmax(a,b,c)  (((a)<(b))? (b):((a)>(c))? (c):(a))

#define MESH_ID_FST 0xfffa // frame start
#define MESH_ID_FSS 0xfffb // slice start
#define MESH_ID_FSP 0xfffc // tile start
#define MESH_ID_FTE 0xfffd // tile end
#define MESH_ID_FSE 0xfffe // slice end
#define MESH_ID_FED 0xffff // frame end
#define CLIP(x, min, max) MAX(MIN(x, max), min)

#define DIV_UP(x, a)     (((x) + ((a)-1)) / a)

#define MOD_CHECK_NULL_PTR(id, ptr)                                                                                   \
    do {                                                                                                           \
        if (!(ptr)) {                                                                                          \
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, " NULL pointer\n");                                         \
            return BM_ERR_FAILURE;                                   \
        }                                                                                                      \
    } while (0)

#define BM_DWA_PRT(fmt...)                               \
    do {                                                  \
        printf("[%s]-%d: ", __func__, __LINE__);          \
        printf(fmt);                                      \
    } while (0)

#define SAFE_FREE_POINTER(ptr)  \
    do {                    \
        if (ptr != NULL) {  \
            free(ptr);      \
            ptr = NULL;     \
        }                   \
    } while (0)

#define DWA_SUPPORT_FMT(fmt) \
    ((fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_RGB_888_PLANAR) ||    \
    (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400))

#define CHECK_DWA_FORMAT(imgIn, imgOut)                                                                                \
    do {                                                                                                           \
        if (imgIn.stVFrame.enPixelFormat != imgOut.stVFrame.enPixelFormat) {                                   \
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "in/out pixelformat(%d-%d) mismatch\n",                             \
                        imgIn.stVFrame.enPixelFormat, imgOut.stVFrame.enPixelFormat);                    \
            return BM_ERR_FAILURE;                                                              \
        }                                                                                                      \
        if (!DWA_SUPPORT_FMT(imgIn.stVFrame.enPixelFormat)) {                                                  \
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pixelformat(%d) unsupported\n", imgIn.stVFrame.enPixelFormat);     \
            return BM_ERR_FAILURE;                                                              \
        }                                                                                                      \
    } while (0)


#ifdef __arm__
typedef s32 DWA_HANDLE;
#else
typedef s64 DWA_HANDLE;
#endif

#ifdef __arm__
typedef u32 VB_BLK;
#else
typedef u64 VB_BLK;
#endif

typedef enum _BM_DWA_TEST_OP {
    DWA_TEST_FISHEYE = 0,
    DWA_TEST_ROT,
    DWA_TEST_LDC,
    DWA_TEST_AFFINE,
    DWA_TEST_DEWARP,
} bm_dwa_test_op;

typedef struct _BM_DWA_BASIC_PARAM {
    SIZE_S size_in;
    SIZE_S size_out;
    u32 u32BlkSizeIn, u32BlkSizeOut;
    VIDEO_FRAME_INFO_S stVideoFrameIn;
    VIDEO_FRAME_INFO_S stVideoFrameOut;
    PIXEL_FORMAT_E enPixelFormat;
    DWA_HANDLE hHandle;
    GDC_TASK_ATTR_S stTask;
    GDC_IDENTITY_ATTR_S identity;
    bm_dwa_test_op op;
    bool needPef;
} bm_dwa_basic_param;

typedef struct _BM_TSK_MESH_ATTR_S {
    char Name[32];
    bm_device_mem_t mem;
} bm_tsk_mesh_attr_s;

typedef struct _FORMAT_8BITS_RGB {
    unsigned char R;
    unsigned char G;
    unsigned char B;
} bm_format_8bits_rgb;

typedef struct _COORDINATE2D {
    double xcor;
    double ycor;
    double xbias;
    double ybias;
    bool valid;
} bm_coordinate2d;

typedef struct _MESH_STRUCT {
    bm_coordinate2d knot[4];
    int idx;
} bm_mesh_struct;

typedef struct _FISHEYE_ATTR {
    bool Enable;                    // dewarp engine on/off
    bool BgEnable;                  // given background color enable.

    int RgnNum;                     // dewarp Region Number.
    int TotalMeshNum;               // total mesh numbes
    int OutW_disp;                  // output display frame width
    int OutH_disp;                  // output display frame height
    int InCenterX;                  // input fisheye center position X
    int InCenterY;                  // input fisheye center position Y
    int InRadius;                   // input fisheye radius in pixels.

    double Hoffset;                 // fish-eye image center horizontal offset.
    double VOffset;                 // fish-eye image center vertical offset.
    double TCoef;                   // KeyStone Corection coefficient.
    double FStrength;               // Fan Correction coefficient.

    USAGE_MODE UsageMode;           // Panorama.360, Panorama.180, Mode1~Mode7. total = 9 for now.
    FISHEYE_MOUNT_MODE_E MntMode;   // CEILING/ FLOOR/ WALL
    bm_format_8bits_rgb BgColor;       // give background color RGB.

    //bm_mesh_struct DstRgnMeshInfo[16384];    // frame-based dst mesh info.  maximum num=128*28 = 16384 meshes.
    //bm_mesh_struct SrcRgnMeshInfo[16384];    // frame-based src mesh info.

    bm_mesh_struct DstRgnMeshInfo[DWA_MAX_FRAME_MESH_NUM]; // frame-based dst mesh info.  maximum num=128*28 = 16384 meshes.
    bm_mesh_struct SrcRgnMeshInfo[DWA_MAX_FRAME_MESH_NUM]; // frame-based src mesh info.

    int SliceX_Num;     // How many tile is sliced in a frame horizontally.
    int SliceY_Num;     // How many tile is sliced in a frame vertically.

    int rotate_index;
} bm_fisheye_attr;

typedef enum _PROJECTION_MODE {
    PROJECTION_PANORAMA_360,
    PROJECTION_PANORAMA_180,
    PROJECTION_REGION,
    PROJECTION_LDC,
    PROJECTION_STEREO_FIT,
} bm_projection_mode;

typedef struct _NODE_STRUCT {
    bm_coordinate2d node;
    bool valid;
} bm_node_struct;

typedef struct _FISHEYE_REGION_ATTR {
    int RgnIndex;       // region index
    float InRadius;     // For Panorama.360 mode, inner radius.
    float OutRadius;    // For Panorama.360 mode, outer radius.
    int Pan;            // Region PTZ-Pan.
    int Tilt;           // Region PTZ-Tilt.
    int HorZoom;        // adjust horizontal zoom in Region PTZ.
    int VerZoom;        // adjust vertical zoom in Region PTZ.
    int OutX;           // region initial position x on display frame.
    int OutY;           // region initial position y on display frame.
    int OutW;           // region width.
    int OutH;           // region height.
    int MeshHor;        // mesh counts horizontal
    int MeshVer;        // mesh counts vertical

    // to give region default view center
    double ThetaX;      // User set rotation angle around X-axis, create angle between vector to Z-axis. (start position @ [0,0,1])
    double ThetaY;      // User set rotation angle around Y-axis,
    double ThetaZ;      // User set rotation angle around Z-axis,

    double ZoomH;       // User Set for Region
    double ZoomV;       // User Set for Region
    int PanEnd;         // For Panorama Case Only
    bool RegionValid;   // label valid/ invalid for each region

    bm_mesh_struct DstRgnMeshInfo[DWA_MAX_FRAME_MESH_NUM]; // initial buffer to store destination mesh info. max: 32*32
    bm_mesh_struct SrcRgnMeshInfo[DWA_MAX_FRAME_MESH_NUM]; // initial buffer to store source mesh info
    bm_node_struct DstRgnNodeInfo[DWA_MAX_FRAME_MESH_NUM]; // for LDC or Stereo
    bm_node_struct SrcRgnNodeInfo[DWA_MAX_FRAME_MESH_NUM]; // for LDC or Stereo
    bm_projection_mode ViewMode;                           // projection method. ex: Panorama.360, Panorama.180, Region
} bm_fisheye_region_attr;

typedef struct _BM_MESH_DATA_ALL_S {
    char grid_name[64];
    bool balloc;
    int num_pairs, imgw, imgh, node_index;
    int *pgrid_src, *pgrid_dst;
    int *pmesh_src, *pmesh_dst;
    int *pnode_src, *pnode_dst;
    int mesh_w;         // unit: pixel
    int mesh_h;         // unit: pixel
    int mesh_horcnt;    // unit: mesh_w
    int mesh_vercnt;    // unit: mesh_h
    int unit_rx;        // unit: mesh_w
    int unit_ry;        // unit: mesh_h
    //int unit_ex;      // = rx + mesh_horcnt - 1
    //int unit_ey;      // = ry + mesh_vercnt - 1
    int _nbr_mesh_x, _nbr_mesh_y;
    bool _bhomo;
    float _homography[10];
    int corners[10];
    float *_pmapx, *_pmapy;
} bm_mesh_data_all_s;

typedef struct RotMatrix3D {
    double coef[4][4];
} bm_rotMatrix3d;

typedef struct Vector3D {
    double x;
    double y;
    double z;
} bm_vector3d;

typedef struct Vector2D {
    double x;
    double y;
} bm_vector2d;

typedef struct mesh_param {
    u32 width;          // dst frame's width
    u32 height;         // dst frame's height
    void *mesh_id_addr;
    void *mesh_tbl_addr;
    uint64_t mesh_tbl_phy_addr;
    u8 slice_num_w;
    u8 slice_num_h;
    u16 mesh_num;
} bm_mesh_param;

typedef struct _BM_ROT_NAME {
    bm_dwa_basic_param param;
    ROTATION_E rotation_mode;
} bm_rot_name;

typedef struct _BM_GDC_NAME {
    bm_dwa_basic_param param;
    LDC_ATTR_S ldc_attr;
    char grid[MD5_STRING_LENGTH + 1];
} bm_gdc_name;

typedef struct _BM_AFFINE_NAME {
    bm_dwa_basic_param param;
    AFFINE_ATTR_S affine_attr;
} bm_affine_name;

typedef struct _BM_FISHEYE_NAME {
    bm_dwa_basic_param param;
    FISHEYE_ATTR_S fisheye_attr;
    char grid[MD5_STRING_LENGTH + 1];
} bm_fisheye_name;

typedef struct _BM_DEWARP_NAME {
    bm_dwa_basic_param param;
    WARP_ATTR_S dewarp_attr;
    char grid[MD5_STRING_LENGTH + 1];
} bm_dewarp_name;

typedef struct _BM_DEWARP_ATTR_AND_GRID {
    WARP_ATTR_S dewarp_attr;
    char *grid;
} bm_dewarp_attr_and_grid;

typedef struct _BM_GDC_ATTR_AND_GRID {
    LDC_ATTR_S ldc_attr;
    char *grid;
} bm_gdc_attr_and_grid;

typedef struct _BM_FISHEYE_ATTR_AND_GRID {
    FISHEYE_ATTR_S fisheye_attr;
    char *grid;
} bm_fisheye_attr_and_grid;

#endif
