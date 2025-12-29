#ifndef BMCV_A2_DPU_INTERNAL_H
#define BMCV_A2_DPU_INTERNAL_H

#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif

#include "bmcv_api_ext_c.h"

bm_status_t bm_dpu_sgbm_disp_internal(
    bm_handle_t          handle,
    bm_image             *left_image,
    bm_image             *right_image,
    bm_image             *disp_image,
    bmcv_dpu_sgbm_attrs  *dpu_attr,
    bmcv_dpu_sgbm_mode   sgbm_mode);

bm_status_t bm_dpu_online_disp_internal(
    bm_handle_t             handle,
    bm_image                *left_image,
    bm_image                *right_image,
    bm_image                *disp_image,
    bmcv_dpu_sgbm_attrs     *dpu_attr,
    bmcv_dpu_fgs_attrs      *fgs_attr,
    bmcv_dpu_online_mode    online_mode);

bm_status_t bm_dpu_fgs_disp_internal(
    bm_handle_t          handle,
    bm_image             *guide_image,
    bm_image             *smooth_image,
    bm_image             *disp_image,
    bmcv_dpu_fgs_attrs   *fgs_attr,
    bmcv_dpu_fgs_mode    fgs_mode);

#endif
