#ifndef BMCV_A2_DPU_INTERNAL_H
#define BMCV_A2_DPU_INTERNAL_H

#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif

#include "bmcv_a2_common_internal.h"

#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b))?(a):(b))
#endif

#define minmax(a, b, c)  (((a) < (b)) ? (b):((a) > (c)) ? (c) : (a))

#define DEFAULT_WIDTH_MESH_NUM 16 // offline calibration mesh number
#define DEFAULT_HEIGHT_MESH_NUM 16 // offline calibration mesh number.
#define PHASEBIT 8 // pixel interpolation phase
#define MAX_FRAME_MESH_NUM 10000
#define MAX_REGION_NUM 1
#define QFORMAT_M 10 // mesh coordinate's floating precision bit number.
#define TWOS_COMPLEMENT 1 // ifndef: sign bit representation:
#define USE_OLD 0

#define LDC_ALIGN 64
#define LDC_MAX_TSK_MESH (32)
#define TILESIZE 64 // HW: data Tile Size
#define HW_MESH_SIZE 8

#define BM_HEAP_0 0
#define BM_HEAP_1 1
#define BMCV_HEAP_1_START 8589934592 // heap 1 start address


#define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE

#define BM_LDC_PRT(fmt...)                                \
    do {                                                  \
        printf("[%s]-%d: ", __func__, __LINE__);          \
        printf(fmt);                                      \
    } while (0)

typedef struct COORD2D_INT_HW {
    u8 xcor[3]; // s13.10, 24bit
} __attribute__((packed)) bm_coord2d_int_hw;

typedef struct _MESH_DATA_ALL_S {
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

typedef bm_mesh_data_all_s bm_meshdata_all;

typedef struct COORD2D {
    double xcor;
    double ycor;
} bm_coord2d;

typedef struct MESH_STRUCT {
    bm_coord2d knot[4];
    int idx;
} bm_mesh_struct;

typedef struct Vector2D {
    double x;
    double y;
} bm_vector2D;

typedef struct COORD2D_INT {
    int xcor;
    int ycor;
} bm_coord2d_int;

typedef struct _TSK_MESH_ATTR_S {
    char Name[32];
    u64 paddr;
    void *vaddr;
} bm_tsk_mesh_attr_s;

typedef struct _vb_cal_config_s bm_vb_cal_config_s;

typedef struct LDC_ATTR {
    int Enable; // dewarp engine on/off

    int RgnNum; // dewarp Region Number.
    int TotalMeshNum; // total mesh numbers
    int OutW_disp; // output display frame width
    int OutH_disp; // output display frame height
    int InCenterX; // input fisheye center position X
    int InCenterY; // input fisheye center position Y
    int InRadius; // input fisheye radius in pixels.

    // frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
    bm_mesh_struct DstRgnMeshInfo[MAX_FRAME_MESH_NUM];

    // frame-based src mesh info.
    bm_mesh_struct SrcRgnMeshInfo[MAX_FRAME_MESH_NUM];

    // frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
    bm_mesh_struct DstRgnMeshInfoExt[9 * MAX_FRAME_MESH_NUM];

    // frame-based src mesh info.
    bm_mesh_struct SrcRgnMeshInfoExt[9 * MAX_FRAME_MESH_NUM];

    // frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
    bm_mesh_struct DstRgnMeshInfoExt2ND[9 * MAX_FRAME_MESH_NUM];

    // frame-based src mesh info.
    bm_mesh_struct SrcRgnMeshInfoExt2ND[9 * MAX_FRAME_MESH_NUM];

    // frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
    bm_mesh_struct DstRgnMeshInfo_1st[MAX_FRAME_MESH_NUM];

    // frame-based src mesh info.
    bm_mesh_struct SrcRgnMeshInfo_1st[MAX_FRAME_MESH_NUM];

    // frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
    bm_mesh_struct DstRgnMeshInfo_2nd[MAX_FRAME_MESH_NUM];

    // frame-based src mesh info.
    bm_mesh_struct SrcRgnMeshInfo_2nd[MAX_FRAME_MESH_NUM];

    // How many tile is sliced in a frame horizontally.
    int SliceX_Num;

    // How many tile is sliced in a frame vertically.
    int SliceY_Num;
} bm_ldc_attr;

typedef struct LDC_RGN_ATTR {
    int RgnIndex; // region index
    int InRadius; // For Panorama.360 mode, inner radius.
    int OutRadius; // For Panorama.360 mode, outer radius.
    int Pan; // Region PTZ-Pan.
    int HorZoom; // adjust horizontal zoom in Region PTZ.
    int VerZoom; // adjust vertical zoom in Region PTZ.
    int OutX; // region initial position x on display frame.
    int OutY; // region initial position y on display frame.
    int OutW; // region width.
    int OutH; // region height.
    int MeshHor; // mesh counts horizontal
    int MeshVer; // mesh counts vertical

    // to give region default view center
    // User set rotation angle around X-axis, create angle between
    // vector to Z-axis. (start position @ [0,0,1])
    int ThetaX;

    int ThetaZ; // User set rotation angle around Z-axis

    int ZoomV; // User Set for Region
    int PanEnd; // For Panorama Case Only
    int RegionValid; // label valid/ invalid for each region

    // initial buffer to store destination mesh info. max: 32*32
    bm_mesh_struct DstRgnMeshInfo[MAX_FRAME_MESH_NUM];

    // extend to 3x3 range.
    bm_mesh_struct DstRgnMeshInfoExt[9 * MAX_FRAME_MESH_NUM];

    // initial buffer to store source mesh info
    bm_mesh_struct SrcRgnMeshInfo[MAX_FRAME_MESH_NUM];

    // extend to 3x3 range.
    bm_mesh_struct SrcRgnMeshInfoExt[9 * MAX_FRAME_MESH_NUM];
} bm_ldc_rgn_attr;

typedef struct _reg_ldc{
    int ldc_en;

    int stage2_rotate_type; // 0: +90, 1: -90
    int bg_color_y_r;
    int bg_color_u_g;
    int bg_color_v_b;

    int dst_x0;
    int dst_y0;

    int src_xstr_s1;
    int src_xend_s1;
    int src_xstr_s2;
    int src_xend_s2;
} bm_reg_ldc;

typedef enum _BM_LDC_OP {
    BM_LDC_ROT = 0,
    BM_LDC_GDC,
    BM_LDC_GDC_GEN_MESH,
    BM_LDC_GDC_LOAD_MESH,
} bm_ldc_op;

typedef struct _BM_LDC_BASIC_PARAM {
    size_s size_in;
    size_s size_out;
    bmcv_rot_mode enRotation;
    video_frame_info_s stVideoFrameIn;
    video_frame_info_s stVideoFrameOut;
    pixel_format_e enPixelFormat;
    GDC_HANDLE hHandle;
    gdc_task_attr_s stTask;
    gdc_identity_attr_s identity;        // Define the identity attribute of GDC (To support multi-path), each job has a unique id
    bm_ldc_op op;
    bool needPef;
    int heap_num;
} bm_ldc_basic_param;

typedef struct _BM_GEN_MESH_PARAM {
    ldc_attr_s           ldc_attr;
    bm_device_mem_t      dmem;
} bm_gen_mesh_param;

typedef struct _BM_GDC_ATTR_AND_GRID_INFO {
    ldc_attr_s ldc_attr;
    char *grid;
} bm_gdc_attr_and_grid_info;