#ifndef BM_PCIE_MODE
#include <stdio.h>
#include <stdlib.h>
#include "bmcv_a2_ldc_ext.h"
#include "bmcv_internal.h"

#define ldc_align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
#define LDC_STRIDE_ALIGN 64

#define TILESIZE 64 // HW: data Tile Size
#define HW_MESH_SIZE 8
#define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE
typedef struct COORD2D_INT_HW {
    u8 xcor[3]; // s13.10, 24bit
} __attribute__((packed)) COORD2D_INT_HW;

bm_status_t bm_ldc_image_calc_stride(bm_handle_t handle,
                                     int img_h,
                                     int img_w,
                                     bm_image_format_ext image_format,
                                     bm_image_data_format_ext data_type,
                                     int *stride)
{
    bm_status_t ret = BM_SUCCESS;
    bm_image_private image_private;
    ret = fill_default_image_private(&image_private, img_h, img_w, image_format, data_type);
    for(int i = 0; i < image_private.plane_num; i++)
        stride[i] = ALIGN(image_private.memory_layout[i].pitch_stride, LDC_STRIDE_ALIGN);
    return ret;
}

void test_mesh_gen_get_1st_size(u32 u32Width, u32 u32Height, u32 *mesh_1st_size)
{
    if (!mesh_1st_size)
        return;

    u32 ori_src_width = u32Width;
    u32 ori_src_height = u32Height;

    // In LDC Processing, width & height aligned to TILESIZE
    u32 src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
    u32 src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

    // modify frame size
    u32 dst_height_s1 = src_height_s1;
    u32 dst_width_s1 = src_width_s1;
    u32 num_tilex_s1 = dst_width_s1 / TILESIZE;
    u32 num_tiley_s1 = dst_height_s1 / TILESIZE;

    // 4 = 4 knots in a mesh
    *mesh_1st_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE * MESH_NUM_ATILE * num_tilex_s1 * num_tiley_s1 * 4;
}

void test_mesh_gen_get_2nd_size(u32 u32Width, u32 u32Height, u32 *mesh_2nd_size)
{
    if (!mesh_2nd_size)
        return;

    u32 ori_src_width = u32Width;
    u32 ori_src_height = u32Height;

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
    *mesh_2nd_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE * MESH_NUM_ATILE * num_tilex_s2 * num_tiley_s2 * 4;
}

void test_mesh_gen_get_size(u32 u32Width,
                            u32 u32Height,
                            u32 *mesh_1st_size,
                            u32 *mesh_2nd_size)
{
    if (!mesh_1st_size || !mesh_2nd_size)
        return;

    test_mesh_gen_get_1st_size(u32Width, u32Height, mesh_1st_size);
    test_mesh_gen_get_2nd_size(u32Width, u32Height, mesh_2nd_size);
}

bm_status_t bmcv_ldc_rot(bm_handle_t          handle,
                         bm_image             in_image,
                         bm_image             out_image,
                         bmcv_rot_mode        rot_mode)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "LDC get chipid error! \n");
        return ret;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ldc_rot_internal(handle, in_image, out_image, rot_mode);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv ldc execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }
    return ret;
}

bm_status_t bmcv_ldc_gdc(bm_handle_t          handle,
                         bm_image             in_image,
                         bm_image             out_image,
                         bmcv_gdc_attr        ldc_attr)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "LDC get chipid error! \n");
        return ret;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ldc_gdc_internal(handle, in_image, out_image, ldc_attr);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv ldc execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }
    return ret;
}

bm_status_t bmcv_ldc_gdc_gen_mesh(bm_handle_t          handle,
                                  bm_image             in_image,
                                  bm_image             out_image,
                                  bmcv_gdc_attr        ldc_attr,
                                  bm_device_mem_t      dmem)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "LDC get chipid error! \n");
        return ret;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ldc_gdc_gen_mesh_internal(handle, in_image, out_image, ldc_attr, dmem);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv ldc execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }
    return ret;
}

bm_status_t bmcv_ldc_gdc_load_mesh(bm_handle_t          handle,
                                   bm_image             in_image,
                                   bm_image             out_image,
                                   bm_device_mem_t      dmem)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "LDC get chipid error! \n");
        return ret;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_ldc_gdc_load_mesh_internal(handle, in_image, out_image, dmem);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv ldc execution failed!\n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support!\n");
            break;
    }
    return ret;
}
#else
#include "bmcv_internal.h"
bm_status_t bmcv_ldc_rot(
    bm_handle_t          handle,
    bm_image             in_image,
    bm_image             out_image,
    bmcv_rot_mode        rot_mode) {
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pcie not support!\n");
    return BM_NOT_SUPPORTED;
}

bm_status_t bmcv_ldc_gdc(
    bm_handle_t          handle,
    bm_image             in_image,
    bm_image             out_image,
    bmcv_gdc_attr        ldc_attr){
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pcie not support!\n");
    return BM_NOT_SUPPORTED;
}

bm_status_t bmcv_ldc_gdc_gen_mesh(
    bm_handle_t          handle,
    bm_image             in_image,
    bm_image             out_image,
    bmcv_gdc_attr        ldc_attr,
    bm_device_mem_t      dmem){
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pcie not support!\n");
    return BM_NOT_SUPPORTED;
};

bm_status_t bmcv_ldc_gdc_load_mesh(
    bm_handle_t          handle,
    bm_image             in_image,
    bm_image             out_image,
    bm_device_mem_t      dmem){
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "pcie not support!\n");
    return BM_NOT_SUPPORTED;
};
#endif