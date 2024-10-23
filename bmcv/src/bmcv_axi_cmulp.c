#include "bmcv_common.h"
#include "bmcv_internal.h"

bm_status_t bmcv_cmulp(bm_handle_t handle, bm_device_mem_t inputReal, bm_device_mem_t inputImag,
                        bm_device_mem_t pointReal, bm_device_mem_t pointImag,
                        bm_device_mem_t outputReal, bm_device_mem_t outputImag,
                        int batch, int len)
{
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_cmulp_t api;
    int core_id = 0;
    unsigned int chipid;

    api.XR = bm_mem_get_device_addr(inputReal);
    api.XI = bm_mem_get_device_addr(inputImag);
    api.PR = bm_mem_get_device_addr(pointReal);
    api.PI = bm_mem_get_device_addr(pointImag);
    api.YR = bm_mem_get_device_addr(outputReal);
    api.YI = bm_mem_get_device_addr(outputImag);
    api.batch = batch;
    api.len = len;


    if (bm_get_chipid(handle, &chipid) != BM_SUCCESS) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_cmulp", (u8*)&api, sizeof(api), core_id);
            if(ret != BM_SUCCESS){
                bmlib_log("CMULP", BMLIB_LOG_ERROR, "cv_cmulp sync api error\n");
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