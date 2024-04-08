#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif

#include "bmcv_api_ext_c.h"
#include "bmcv_a2_dwa_internal.h"

bm_status_t bmcv_dwa_rot_internel(
    bm_handle_t          handle,
    bm_image             input_image,
    bm_image             output_image,
    bmcv_rot_mode        rot_mode);

bm_status_t bmcv_dwa_gdc_internel(
    bm_handle_t          handle,
    bm_image             input_image,
    bm_image             output_image,
    bmcv_gdc_attr        ldc_attr);

bm_status_t bmcv_dwa_fisheye_internel(
    bm_handle_t          handle,
    bm_image             input_image,
    bm_image             output_image,
    bmcv_fisheye_attr_s  fisheye_attr);

bm_status_t bmcv_dwa_affine_internel(
    bm_handle_t          handle,
    bm_image             input_image,
    bm_image             output_image,
    bmcv_affine_attr_s   affine_attr);

bm_status_t bmcv_dwa_dewarp_internel(
    bm_handle_t          handle,
    bm_image             input_image,
    bm_image             output_image,
    bm_device_mem_t      grid_info);