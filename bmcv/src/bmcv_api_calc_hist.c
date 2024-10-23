#include "bmcv_common.h"
#include "bmcv_internal.h"

#define CHN_NUM_MAX 3

bm_status_t bmcv_calc_hist(bm_handle_t handle, bm_device_mem_t input, bm_device_mem_t output, int C,
                            int H, int W, const int* channels, int dims, const int* histSizes,
                            const float* ranges, int inputDtype)
{
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_calc_hist_index_t api[CHN_NUM_MAX];
    int dsize = inputDtype == 0 ? sizeof(float) : sizeof(uint8_t);
    int imageSize = H * W;
    bm_device_mem_t idxDev[CHN_NUM_MAX];
    float* idxHost[CHN_NUM_MAX];
    float* outputHost = NULL;
    int total = 1;
    int i;
    int binX, binY, binZ;
    int HistSize_2D = histSizes[1];
    int HistSize_3D = histSizes[2];
    unsigned int chipid;
    int core_id = 0;

    if (dims <= 0 || dims > CHN_NUM_MAX) {
        return BM_ERR_PARAM;
    }

    for (i = 0; i < dims; ++i) {
        total *= histSizes[i];
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    for (i = 0; i < dims; ++i) {
        ret = bm_malloc_device_byte(handle, &idxDev[i], imageSize * sizeof(float));
        if (ret != BM_SUCCESS) {
            printf("the bm_malloc_device_byte failed!\n");
            return ret;
        }
        api[i].Xaddr = bm_mem_get_device_addr(input) + channels[i] * imageSize * dsize;
        api[i].Yaddr = bm_mem_get_device_addr(idxDev[i]);
        api[i].a = (float)((histSizes[i]) / (ranges[i * 2 + 1] - ranges[2 * i]));
        api[i].b = -ranges[2 * i] * api[i].a;
        api[i].len = imageSize;
        api[i].xdtype = inputDtype;
        api[i].upper = (float)(histSizes[i]);

        switch(chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch(handle, "cv_calc_hist", (u8*)(&(api[i])),
                                                    sizeof(api[i]), core_id);
                if(ret != BM_SUCCESS){
                    for (i = 0; i < dims; i++) {
                        bm_free_device(handle, idxDev[i]);
                    }
                    bmlib_log("CALC_HIST", BMLIB_LOG_ERROR, "calc_hist sync api error\n");
                    return ret;
                }
                break;
            default:
                printf("BM_NOT_SUPPORTED!\n");
                ret = BM_NOT_SUPPORTED;
                break;
        }
    }

    outputHost = (float*)malloc(total * sizeof(float));
    memset(outputHost, 0.f, total * sizeof(float));

    for (i = 0; i < dims; i++) {
        idxHost[i] = (float *)malloc(imageSize * sizeof(float));
        memset(idxHost[i], 0.f, imageSize * sizeof(float));
    }

    for (i = 0; i < dims; ++i) {
        ret = bm_memcpy_d2s(handle, idxHost[i], idxDev[i]);
        if (ret) {
            printf("bm_memcpy_d2s failed!\n");
            goto exit;
        }
    }

    for (i = 0; i < imageSize; ++i) {
        if (dims == 1) {
            binX = (int)(idxHost[0][i]);
            outputHost[binX]++;
        } else if (dims == 2) {
            binX = (int)(idxHost[0][i]);
            binY = (int)(idxHost[1][i]);
            outputHost[binX * HistSize_2D + binY]++;
        } else if (dims == 3) {
            binX = (int)(idxHost[0][i]);
            binY = (int)(idxHost[1][i]);
            binZ = (int)(idxHost[2][i]);
            outputHost[binX * (HistSize_2D * HistSize_3D) + binY * HistSize_3D + binZ]++;
        }
    }
    ret = bm_memcpy_s2d(handle, output, outputHost);
    if (ret) {
        printf("bm_memcpy_s2d failed!\n");
        goto exit;
    }

exit:
    for (i = 0; i < dims; i++) {
        free(idxHost[i]);
        bm_free_device(handle, idxDev[i]);
    }

    free(outputHost);
    return ret;
}

bm_status_t bmcv_calc_hist_with_weight(bm_handle_t handle, bm_device_mem_t input, bm_device_mem_t output,
                                       const float *weight, int C, int H, int W, const int *channels,
                                       int dims, const int *histSizes, const float *ranges, int inputDtype)
{
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_calc_hist_index_t api[CHN_NUM_MAX];
    int dsize = inputDtype == 0 ? sizeof(float) : sizeof(uint8_t);
    int imageSize = H * W;
    bm_device_mem_t idxDev[CHN_NUM_MAX];
    float* idxHost[CHN_NUM_MAX];
    float* outputHost = NULL;
    int total = 1;
    int i;
    int binX, binY, binZ;
    int HistSize_2D = histSizes[1];
    int HistSize_3D = histSizes[2];
    unsigned int chipid;
    int core_id = 0;

    if (dims <= 0 || dims > CHN_NUM_MAX) {
        return BM_ERR_PARAM;
    }

    for (i = 0; i < dims; ++i) {
        total *= histSizes[i];
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    for (i = 0; i < dims; ++i) {
        ret = bm_malloc_device_byte(handle, &idxDev[i], imageSize * sizeof(float));
        if (ret != BM_SUCCESS) {
            printf("the bm_malloc_device_byte failed!\n");
            return ret;
        }
        api[i].Xaddr = bm_mem_get_device_addr(input) + channels[i] * imageSize * dsize;
        api[i].Yaddr = bm_mem_get_device_addr(idxDev[i]);
        api[i].a = (float)(histSizes[i]) / (ranges[i * 2 + 1] - ranges[2 * i]);
        api[i].b = -ranges[2 * i] * api[i].a;
        api[i].len = imageSize;
        api[i].xdtype = inputDtype;
        api[i].upper = (float)(histSizes[i]);
        switch(chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch(handle, "cv_calc_hist", (u8*)&((api[i])),
                                                    sizeof(api[i]), core_id);
                if(ret != BM_SUCCESS){
                    for (i = 0; i < dims; i++) {
                        bm_free_device(handle, idxDev[i]);
                    }
                    bmlib_log("CALC_HIST", BMLIB_LOG_ERROR, "calc_hist sync api error\n");
                    return ret;
                }
                break;
            default:
                printf("BM_NOT_SUPPORTED!\n");
                ret = BM_NOT_SUPPORTED;
                break;
        }
    }

    outputHost = (float*)malloc(total * sizeof(float));
    memset(outputHost, 0.f, total * sizeof(float));

    for (i = 0; i < dims; i++) {
        idxHost[i] = (float *)malloc(imageSize * sizeof(float));
        memset(idxHost[i], 0.f, imageSize * sizeof(float));
    }

    for (i = 0; i < dims; ++i) {
        ret = bm_memcpy_d2s(handle, idxHost[i], idxDev[i]);
        if (ret) {
            printf("bm_memcpy_d2s failed!\n");
            goto exit;
        }
    }

    for (i = 0; i < imageSize; ++i) {
        if (dims == 1) {
            binX = (int)(idxHost[0][i]);
            outputHost[binX] += weight[i];
        } else if (dims == 2) {
            binX = (int)(idxHost[0][i]);
            binY = (int)(idxHost[1][i]);
            outputHost[binX * HistSize_2D + binY] += weight[i];
        } else if (dims == 3) {
            binX = (int)(idxHost[0][i]);
            binY = (int)(idxHost[1][i]);
            binZ = (int)(idxHost[2][i]);
            outputHost[binX * (HistSize_2D * HistSize_3D) + binY * HistSize_3D + binZ] += weight[i];
        }
    }
    ret = bm_memcpy_s2d(handle, output, outputHost);
    if (ret) {
        printf("bm_memcpy_s2d failed!\n");
        goto exit;
    }

exit:
    for (i = 0; i < dims; i++) {
        free(idxHost[i]);
        bm_free_device(handle, idxDev[i]);
    }

    free(outputHost);
    return ret;
}