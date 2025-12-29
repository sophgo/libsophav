#include "bmcv_common.h"
#include "bmcv_internal.h"
#include "test_misc.h"

static float fp16tofp32_(fp16 val)
{
    union fp16_data dfp16;
    dfp16.ndata = val;
    uint32_t sign = (dfp16.idata & 0x8000) << 16;
    uint32_t mantissa = (dfp16.idata & 0x03FF);
    uint32_t exponent = (dfp16.idata & 0x7C00) >> 10;
    union conv_ conv;
    float sign_f = 0;
    memcpy(&sign_f, &sign, sizeof(float));
    if (exponent == 0) {
        if (mantissa == 0) {
            return sign_f;
        } else {
            exponent = 1;
            while ((mantissa & 0x0400) == 0) {
                mantissa <<= 1;
                exponent++;
            }
            mantissa &= 0x03FF;
            exponent = (127 - 15 - exponent) << 23;
            mantissa <<= 13;
            conv.i = *((uint32_t*)(&sign)) | (*((uint32_t*)(&mantissa))) | \
                    (*((uint32_t*)(&exponent)));
            return conv.f;
        }
    } else if (exponent == 0x1F) {
        if (mantissa == 0) {
            return sign_f / 0.0f;
        } else {
            return sign_f / 0.0f;
        }
    } else {
        exponent = (exponent + (127 - 15)) << 23;
        mantissa <<= 13;
        conv.i = *((uint32_t*)(&sign)) | (*((uint32_t*)(&mantissa))) | (*((uint32_t*)(&exponent)));
        return conv.f;
    }
}

bm_status_t bmcv_distance(bm_handle_t handle, bm_device_mem_t input, bm_device_mem_t output,
                        int dim, const void *pnt, int len, int dtype)
{
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_distance_t api;
    unsigned int chipid;
    int i;

    int if_core0 = 1, if_core1 = 0;
    const char *tpu_env = NULL;

    api.Xaddr = bm_mem_get_device_addr(input);
    api.Yaddr = bm_mem_get_device_addr(output);
    api.dim = dim;
    api.len = len;
    api.dtype = dtype;

    if (dtype != DT_FP32 && dtype != DT_FP16) {
        printf("the dtype is error!\n");
        return BM_ERR_PARAM;
    }

    if (dim <= 0 || dim > DISTANCE_DIM_MAX) {
        printf("the dim is error!\n");
        return BM_ERR_PARAM;
    }

    if (dtype == DT_FP16) {
        fp16 *pnt16 = (fp16 *)pnt;
        for (i = 0; i < dim; ++i) {
            api.pnt[i] = fp16tofp32_(pnt16[i]);
        }
    } else {
        float *pnt32 = (float *)pnt;
        for (i = 0; i < dim; ++i) {
            api.pnt[i] = pnt32[i];
        }
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
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

        if (if_core0 && if_core1) {
            int core_list[BM1688_MAX_CORES];
            int core_nums = 0;
            int base_msg_id = 0;

            tpu_launch_param_t tpu_params[BM1688_MAX_CORES];
            bm_api_cv_distance_dual_core_t dual_api;
            bm_api_cv_distance_dual_core_t dual_params[BM1688_MAX_CORES];

            if (if_core0) core_list[core_nums++] = 0;
            if (if_core1) core_list[core_nums++] = 1;

            memcpy(&dual_api, &api, sizeof(bm_api_cv_distance_t));

            for (int n = 0; n < core_nums; n++)
                base_msg_id |= 1 << core_list[n];

            dual_api.core_num = core_nums;
            dual_api.base_msg_id = base_msg_id;

            for (int n = 0; n < core_nums; n++) {
                dual_api.core_id = n;
                dual_params[n] = dual_api;
                tpu_params[n].core_id = n;
                tpu_params[n].param_data = &dual_params[n];
                tpu_params[n].param_size = sizeof(bm_api_cv_distance_dual_core_t);
            }

            ret = bm_tpu_kernel_launch_dual_core(handle, "cv_distance_dual_core", tpu_params, core_list, core_nums);
            if(ret != BM_SUCCESS){
                BMCV_ERR_LOG("convert_to sync api error\r\n");
                return ret;
            }
        } else {
            int core_id = if_core1 == 1 ? 1 : 0;
            ret = bm_tpu_kernel_launch(handle, "cv_distance", (u8*)&api,
                                                sizeof(api), core_id);
            if(ret != BM_SUCCESS){
                bmlib_log("DISTANCE", BMLIB_LOG_ERROR, "distance sync api error\n");
                return ret;
            }
        }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}