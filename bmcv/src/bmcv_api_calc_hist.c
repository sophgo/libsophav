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
    const char *tpu_env = NULL;

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

    int if_core0 = 1, if_core1 = 0;
    tpu_env = getenv("TPU_CORES");
    if (tpu_env) {
        if (strcmp(tpu_env, "0") == 0) {
        } else if (strcmp(tpu_env, "1") == 0) {
            if_core0 = 0;
            if_core1 = 1;
        } else if (strcmp(tpu_env, "2") == 0 || strcmp(tpu_env, "both") == 0) {
            if_core1 = 1;
        } else {
            fprintf(stderr, "Invalid TPU_CORES value: %s\n", tpu_env);
            fprintf(stderr, "Available options: 0, 1, 2/both\n");
            exit(EXIT_FAILURE);
        }
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
                if (if_core0 && if_core1) {
                    int core_list[BM1688_MAX_CORES];
                    int core_nums = 0;
                    int base_msg_id = 0;

                    bm_api_cv_calc_hist_dual_core_t dual_api;
                    bm_api_cv_calc_hist_dual_core_t dual_param[BM1688_MAX_CORES];
                    tpu_launch_param_t tpu_params[BM1688_MAX_CORES];

                    if (if_core0) core_list[core_nums++] = 0;
                    if (if_core1) core_list[core_nums++] = 1;

                    for(int n = 0; n < core_nums; n++)
                        base_msg_id |= 1 << core_list[n];

                    dual_api.Xaddr = api[i].Xaddr;
                    dual_api.Yaddr = api[i].Yaddr;
                    dual_api.a = api[i].a;
                    dual_api.b = api[i].b;
                    dual_api.len = api[i].len;
                    dual_api.xdtype = api[i].xdtype;
                    dual_api.upper = api[i].upper;

                    dual_api.core_nums = core_nums;
                    dual_api.base_msg_id = base_msg_id;

                    for (int n = 0; n < core_nums; n++) {
                        dual_api.core_id = n;

                        dual_param[n] = dual_api;

                        tpu_params[n].core_id = n;
                        tpu_params[n].param_data = &dual_param[n];
                        tpu_params[n].param_size = sizeof(bm_api_cv_calc_hist_dual_core_t);
                    }

                    ret = bm_tpu_kernel_launch_dual_core(handle, "cv_calc_hist_dual_core", tpu_params, core_list, core_nums);
                    if (ret != BM_SUCCESS) {
                        bmlib_log("CALC_HIST", BMLIB_LOG_ERROR, "calc_hist sync api error\n");
                        return ret;
                    }

                } else {
                    int core_id = if_core1 == 1 ? 1 : 0;
                    ret = bm_tpu_kernel_launch(handle, "cv_calc_hist", (u8*)(&(api[i])),
                                                    sizeof(api[i]), core_id);
                    if(ret != BM_SUCCESS){
                        for (i = 0; i < dims; i++) {
                            bm_free_device(handle, idxDev[i]);
                        }
                        bmlib_log("CALC_HIST", BMLIB_LOG_ERROR, "calc_hist sync api error\n");
                        return ret;
                    }
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
    const char *tpu_env = NULL;

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

    int if_core0 = 1, if_core1 = 0;
    tpu_env = getenv("TPU_CORES");
    if (tpu_env) {
        if (strcmp(tpu_env, "0") == 0) {
        } else if (strcmp(tpu_env, "1") == 0) {
            if_core0 = 0;
            if_core1 = 1;
        } else if (strcmp(tpu_env, "2") == 0 || strcmp(tpu_env, "both") == 0) {
            if_core1 = 1;
        } else {
            fprintf(stderr, "Invalid TPU_CORES value: %s\n", tpu_env);
            fprintf(stderr, "Available options: 0, 1, 2/both\n");
            exit(EXIT_FAILURE);
        }
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
                if (if_core0 && if_core1) {
                    int core_list[BM1688_MAX_CORES];
                    int core_nums = 0;
                    int base_msg_id = 0;

                    bm_api_cv_calc_hist_dual_core_t dual_api;
                    bm_api_cv_calc_hist_dual_core_t dual_param[BM1688_MAX_CORES];
                    tpu_launch_param_t tpu_params[BM1688_MAX_CORES];

                    if (if_core0) core_list[core_nums++] = 0;
                    if (if_core1) core_list[core_nums++] = 1;

                    for(int n = 0; n < core_nums; n++)
                        base_msg_id |= 1 << core_list[n];

                    dual_api.Xaddr = api[i].Xaddr;
                    dual_api.Yaddr = api[i].Yaddr;
                    dual_api.a = api[i].a;
                    dual_api.b = api[i].b;
                    dual_api.len = api[i].len;
                    dual_api.xdtype = api[i].xdtype;
                    dual_api.upper = api[i].upper;

                    dual_api.core_nums = core_nums;
                    dual_api.base_msg_id = base_msg_id;

                    for (int n = 0; n < core_nums; n++) {
                        dual_api.core_id = n;

                        dual_param[n] = dual_api;

                        tpu_params[n].core_id = n;
                        tpu_params[n].param_data = &dual_param[n];
                        tpu_params[n].param_size = sizeof(bm_api_cv_calc_hist_dual_core_t);
                    }

                    ret = bm_tpu_kernel_launch_dual_core(handle, "cv_calc_hist_dual_core", tpu_params, core_list, core_nums);
                    if (ret != BM_SUCCESS) {
                        bmlib_log("CALC_HIST", BMLIB_LOG_ERROR, "calc_hist sync api error\n");
                        return ret;
                    }

                } else {
                    int core_id = if_core1 == 1 ? 1 : 0;
                    ret = bm_tpu_kernel_launch(handle, "cv_calc_hist", (u8*)(&(api[i])),
                                                    sizeof(api[i]), core_id);
                    if(ret != BM_SUCCESS){
                        for (i = 0; i < dims; i++) {
                            bm_free_device(handle, idxDev[i]);
                        }
                        bmlib_log("CALC_HIST", BMLIB_LOG_ERROR, "calc_hist sync api error\n");
                        return ret;
                    }
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