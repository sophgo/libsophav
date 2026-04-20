#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"
#include "bmcv_common.h"

typedef struct sg_api_raw12_to_uint16 {
    unsigned long long input_addr;
    unsigned long long output_addr;
    int width;
    int height;
}__attribute__((packed)) sg_api_raw12_to_uint16_t;

static bm_status_t bmcv_raw12_to_uint16_check(bm_handle_t handle, int width, int height) {
    if (handle == NULL) {
        bmlib_log("RAW12_TO_UINT16", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    if (width != 640) {
        bmlib_log("RAW12_TO_UINT16", BMLIB_LOG_ERROR, "width must be 640!\r\n");
        return BM_ERR_PARAM;
    }
    if (height != 480) {
        bmlib_log("RAW12_TO_UINT16", BMLIB_LOG_ERROR, "height must be 480!\r\n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_raw12_to_uint16(bm_handle_t handle, bm_device_mem_t input_dev_mem, bm_device_mem_t output_dev_mem,
                                   int width, int height) {
    bm_status_t ret = BM_SUCCESS;
    ret = bmcv_raw12_to_uint16_check(handle, width, height);
    if (BM_SUCCESS != ret) {
        bmlib_log("RAW12_TO_UINT16", BMLIB_LOG_ERROR, "bmcv_raw12_to_uint16_check failed!\r\n");
        return ret;
    }
    sg_api_raw12_to_uint16_t api;
    api.input_addr = bm_mem_get_device_addr(input_dev_mem);
    api.output_addr = bm_mem_get_device_addr(output_dev_mem);
    api.width = width;
    api.height = height;
    unsigned int chipid;
    bm_get_chipid(handle, &chipid);
    int core_id = 0;
    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "raw12_to_uint16_a2_84x", (u8*)&api, sizeof(api), core_id);
            if(BM_SUCCESS != ret){
                bmlib_log("RAW12_TO_UINT16", BMLIB_LOG_ERROR, "raw12_to_uint16 sync api error\n");
                return BM_ERR_FAILURE;
            }
            break;
        default:
            printf("ChipID is NOT supported\n");
            break;
    }
    if (ret != BM_SUCCESS) {
        printf("raw12_to_uint16 tpu_kernel_launch failed\n");
        return BM_ERR_FAILURE;
    }

    return BM_SUCCESS;
}