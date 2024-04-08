#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif
#include "bmcv_api_ext_c.h"

bm_status_t bm_ldc_rot_internal(
    bm_handle_t          handle,
    bm_image             in_image,
    bm_image             out_image,
    bmcv_rot_mode        rot_mode);

bm_status_t bm_ldc_gdc_internal(
    bm_handle_t          handle,
    bm_image             in_image,
    bm_image             out_image,
    bmcv_gdc_attr        ldc_attr);

bm_status_t bm_ldc_gdc_gen_mesh_internal(
    bm_handle_t          handle,
    bm_image             in_image,
    bm_image             out_image,
    bmcv_gdc_attr        ldc_attr,
    bm_device_mem_t      dmem);

bm_status_t bm_ldc_gdc_load_mesh_internal(
    bm_handle_t          handle,
    bm_image             in_image,
    bm_image             out_image,
    bm_device_mem_t      dmem);