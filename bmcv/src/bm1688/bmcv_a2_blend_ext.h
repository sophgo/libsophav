#ifdef _WIN32
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT
#define DECL_IMPORT
#endif
#include "bmcv_api_ext_c.h"

bm_status_t bmcv_blending_internal(
    bm_handle_t handle,
    int input_num,
    bm_image* input,
    bm_image output,
    struct stitch_param stitch_config);
