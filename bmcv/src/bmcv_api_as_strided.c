#include "bmcv_common.h"
#include "bmcv_internal.h"

bm_status_t bmcv_as_strided(bm_handle_t handle, bm_device_mem_t input, bm_device_mem_t output,
                            int input_row, int input_col, int output_row,
                            int output_col, int row_stride, int col_stride)
{
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_as_strided_t api;
    unsigned int chipid;
    int core_id = 0;

    if (handle == NULL) {
        bmlib_log("AS_STRIDED", BMLIB_LOG_ERROR, "Can not get handle!\n");
        return BM_ERR_FAILURE;
    }

    api.input_addr = bm_mem_get_device_addr(input);
    api.output_addr = bm_mem_get_device_addr(output);
    api.input_row = input_row;
    api.input_col = input_col;
    api.output_row = output_row;
    api.output_col = output_col;
    api.row_stride = row_stride;
    api.col_stride = col_stride;

    ret = bm_get_chipid(handle, &chipid);
    if (ret != BM_SUCCESS) {
        bmlib_log("AS_STRIDED", BMLIB_LOG_ERROR, "Can not get chipid!\n");
        return BM_ERR_FAILURE;
    }

    switch(chipid) {
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_as_strided", (u8 *)&api,
                                                sizeof(api), core_id);
            if (ret != BM_SUCCESS) {
                bmlib_log("AS_STRIDED", BMLIB_LOG_ERROR, "as_strided sync api error\n");
                return ret;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}