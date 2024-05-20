#include "bmcv_common.h"
#include "bmcv_internal.h"
#include "stdlib.h"

#define ONLY_MIN    0
#define ONLY_MAX    1
#define BOTH        2

static bm_status_t bmcv_min_max_check(int len) {
    if (len > 260144 || len < 50) {
        bmlib_log("min_max", BMLIB_LOG_ERROR, "Unsupported size! len_size range : 50 ~ 260144 \n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_min_max(bm_handle_t handle, bm_device_mem_t input, float* minVal,
                        float* maxVal, int len)
{
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t minDev, maxDev;
    unsigned int chipid;
    uint8_t npu_num;
    int core_id = 0;
    float* min_npu = NULL;
    float* max_npu = NULL;
    bm_api_cv_min_max_t api;

    ret = bmcv_min_max_check(len);
    if(ret != BM_SUCCESS) {
        BMCV_ERR_LOG("min_max check error\r\n");
        return ret;
    }
    if (handle == NULL) {
        bmlib_log("MIN_MAX", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }

    if (minVal == NULL && maxVal == NULL) {
        return ret;
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return ret;
    }

    if (chipid == BM1688) {
        npu_num = 32;
    } else {
        npu_num = 64;
    }

    min_npu = (float*)malloc(npu_num * sizeof(float));
    max_npu = (float*)malloc(npu_num * sizeof(float));

    if (maxVal != NULL)
        ret = bm_malloc_device_byte(handle, &maxDev, npu_num * sizeof(float));

    if (minVal != NULL)
        ret = bm_malloc_device_byte(handle, &minDev, npu_num * sizeof(float));

    api.inputAddr = bm_mem_get_device_addr(input);
    api.minAddr = bm_mem_get_device_addr(minDev);
    api.maxAddr = bm_mem_get_device_addr(maxDev);
    api.len = len;
    api.mode = maxVal == NULL ? ONLY_MIN : (minVal == NULL ? ONLY_MAX : BOTH);


    switch(chipid) {
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_min_max", (u8*)&api,
                                                sizeof(api), core_id);
            if(BM_SUCCESS != ret){
                bmlib_log("MIN_MAX", BMLIB_LOG_ERROR, "min_max sync api error\n");
                return ret;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    if (maxVal != NULL) {
        ret = bm_memcpy_d2s(handle, max_npu, maxDev);
        *maxVal = max_npu[0];
        for (int i = 1; i < npu_num; ++i)
            *maxVal = *maxVal > max_npu[i] ? *maxVal : max_npu[i];
        bm_free_device(handle, maxDev);
    }

    if (minVal != NULL) {
        ret = bm_memcpy_d2s(handle, min_npu, minDev);
        *minVal = min_npu[0];
        for (int i = 1; i < npu_num; ++i)
            *minVal = *minVal < min_npu[i] ? *minVal : min_npu[i];
        bm_free_device(handle, minDev);
    }

    return ret;
}