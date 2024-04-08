#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"
#include <unistd.h>

#define GRAY_SERIES 256

static float get_cdf_min(int32_t* cdf, int len)
{
    int i;

    for(i = 0; i < len; ++i) {
        if (cdf[i] != 0) {
            return (float)cdf[i];
        }
    }
    return 0.f;
}

bm_status_t bmcv_hist_balance(bm_handle_t handle, bm_device_mem_t input, bm_device_mem_t output,
                            int H, int W)
{
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_hist_balance_t1 api1;
    bm_api_cv_hist_balance_t2 api2;
    int imageSize = H * W;
    bm_device_mem_t cdf;
    int32_t* cdf_sys = NULL;
    float cdf_min = 0.f;
    unsigned int chipid;
    int core_id = 0;
    int i;

    cdf_sys = (int32_t*)malloc(GRAY_SERIES * sizeof(int32_t));
    memset(cdf_sys, 0, GRAY_SERIES * sizeof(int32_t));

    for (i = 0; i < GRAY_SERIES; ++i) {
        cdf_sys[i] = i;
    }

    ret = bm_malloc_device_byte(handle, &cdf, GRAY_SERIES * sizeof(int32_t));
    if (ret != BM_SUCCESS) {
        printf("the bm_malloc_device_byte failed!\n");
        free(cdf_sys);
        return ret;
    }

    ret = bm_memcpy_s2d(handle, cdf, cdf_sys);
    if (ret) {
        printf("bm_memcpy_s2d failed!\n");
        goto exit;
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret != BM_SUCCESS) {
        bmlib_log("HIST_BALANCE", BMLIB_LOG_ERROR, "Can not get chipid!\n");
        goto exit;
    }

    api1.Xaddr = bm_mem_get_device_addr(input);
    api1.len = imageSize;
    api1.cdf_addr = bm_mem_get_device_addr(cdf);

    switch(chipid) {
        case BM1686:
            ret = bm_tpu_kernel_launch(handle, "cv_hist_balance1", (u8*)&api1,
                                                sizeof(api1), core_id);
            if (ret != BM_SUCCESS) {
                bmlib_log("HIST_BALANCE1", BMLIB_LOG_ERROR, "cv_hist_balance1 sync api error\n");
                goto exit;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    ret = bm_memcpy_d2s(handle, cdf_sys, cdf);
    if (ret) {
        printf("bm_memcpy_s2d failed!\n");
        goto exit;
    }

    cdf_min = get_cdf_min(cdf_sys, GRAY_SERIES);

    api2.Xaddr = bm_mem_get_device_addr(input);
    api2.len = imageSize;
    api2.cdf_min = cdf_min;
    api2.cdf_addr = bm_mem_get_device_addr(cdf);
    api2.Yaddr = bm_mem_get_device_addr(output);
    switch(chipid) {
        case BM1686:
            ret = bm_tpu_kernel_launch(handle, "cv_hist_balance2", (u8*)&api2,
                                                sizeof(api2), core_id);
            if (ret != BM_SUCCESS) {
                bmlib_log("HIST_BALANCE2", BMLIB_LOG_ERROR, "cv_hist_balance2 sync api error\n");
                goto exit;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

exit:
    free(cdf_sys);
    bm_free_device(handle, cdf);
    return ret;
}