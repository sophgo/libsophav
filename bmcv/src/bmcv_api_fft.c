#include "bmcv_common.h"
#include "bmcv_internal.h"
#include <math.h>

#define EU_NUM 32

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)
#define EU_ALIGN(a) ALIGN((a), EU_NUM)

#define M_PI 3.14159265358979323846
#define CONFIG_LOCAL_MEM_ADDRWIDTH 18
#define LOCAL_MEM_SIZE (1 << CONFIG_LOCAL_MEM_ADDRWIDTH)

bm_status_t bmcv_fft_1d_create_plan_by_factors(bm_handle_t handle, int batch, int len, const int *factors,
                                                int factorSize, bool forward, struct FFT1DPlan** plan)
{
    bm_status_t ret = BM_SUCCESS;
    *plan = (struct FFT1DPlan*)malloc(sizeof(struct FFT1DPlan));
    memset(*plan, 0, sizeof(struct FFT1DPlan));
    float* ER = (float*)malloc(len * sizeof(float));
    float* EI = (float*)malloc(len * sizeof(float));
    int* plan_factors = (int*)malloc(factorSize * sizeof(int));
    struct FFTPlan plan_type;
    int L = len;
    int j = 0;
    int k = 0;
    int i = 0;
    int m = 0;
    double s, ang;

    plan_type.type = FFTType_1D;
    (*plan)->type = plan_type;
    (*plan)->batch = batch;
    (*plan)->L = len;
    (*plan)->forward = forward;
    (*plan)->factors_size = factorSize;


    for (i = 0; i < factorSize; ++i) {
        if (factors[i] == 1 || L % factors[i] != 0 || (factors[i] != 2 && factors[i] != 3 &&
            factors[i] != 4 && factors[i] != 5)) {
            printf("INVALID FACTORS.\n");
            return BM_ERR_PARAM;
        }
        plan_factors[j] = factors[i];
        j++;
        L /= factors[i];
    }
    (*plan)->factors = plan_factors;

    if (L != 1) {
        printf("INVALID FACTORS.\n");
        return BM_ERR_PARAM;
    }

    for (i = factorSize - 1; i > 0; --i) {
        L *= factors[i];
        s = 2.0 * M_PI / (L * factors[i - 1]);
        float cos_value[L * factors[i - 1]];
        float sin_value[L * factors[i - 1]];
        for (k = 0; k < L * factors[i - 1]; ++k) {
            ang = forward ? - s * k : s * k;
            cos_value[k] = cos(ang);
            sin_value[k] = sin(ang);
        }
        for (j = 1; j < factors[i - 1]; ++j) {
            for (k = 0; k < L; ++k) {
                ER[m] = (cos_value[(k * j) % (L * factors[i - 1])]);
                EI[m] = (sin_value[(k * j) % (L * factors[i - 1])]);
                m++;
            }
        }
    }

    if (factorSize > 1) {
        if (m == 0) {
            return BM_ERR_FAILURE;
        }
        if (bm_malloc_device_byte(handle, &((*plan)->ER), m * sizeof(float)) != BM_SUCCESS) {
            bmlib_log("FFT", BMLIB_LOG_ERROR, "alloc ER failed!\r\n");
            return BM_ERR_FAILURE;
        }
        (*plan)->ER_flag = true;

        if (bm_malloc_device_byte(handle, &((*plan)->EI), m * sizeof(float)) != BM_SUCCESS) {
            bmlib_log("FFT", BMLIB_LOG_ERROR, "alloc EI failed!\r\n");
            bm_free_device(handle, (*plan)->ER);
            (*plan)->ER_flag = false;
            return BM_ERR_FAILURE;
        }
        (*plan)->EI_flag = true;

        if (bm_memcpy_s2d(handle, (*plan)->ER, (void*)ER) != BM_SUCCESS) {
            bmlib_log("FFT", BMLIB_LOG_ERROR, "ER s2d failed!\r\n");
            bm_free_device(handle, (*plan)->ER);
            (*plan)->ER_flag = false;
            bm_free_device(handle, (*plan)->EI);
            (*plan)->EI_flag = false;
            return BM_ERR_FAILURE;
        }
        if (bm_memcpy_s2d(handle, (*plan)->EI, (void*)EI) != BM_SUCCESS) {
            bmlib_log("FFT", BMLIB_LOG_ERROR, "EI s2d failed!\r\n");
            bm_free_device(handle, (*plan)->ER);
            (*plan)->ER_flag = false;
            bm_free_device(handle, (*plan)->EI);
            (*plan)->EI_flag = false;
            return BM_ERR_FAILURE;
        }
    }

    free(ER);
    free(EI);
    return ret;
}

static bm_status_t bmcv_fft_0d_create_plan(bm_handle_t handle, int batch, struct FFT0DPlan** plan)
{
    bm_status_t ret = BM_SUCCESS;
    struct FFTPlan plan_type;
    plan_type.type = FFTType_0D;

    *plan = (struct FFT0DPlan*)malloc(sizeof(struct FFT0DPlan));

    (*plan)->type = plan_type;
    (*plan)->batch = batch;
    return ret;
}

bm_status_t bmcv_fft_1d_create_plan(bm_handle_t handle, int batch, int len, bool forward, void** plan)
{
    bm_status_t ret = BM_SUCCESS;
    int* F = (int*)malloc(len * sizeof(int));
    int L = len;
    int i = 0;

    if (L == 1) {
        ret = bmcv_fft_0d_create_plan(handle, batch, (struct FFT0DPlan**)plan);
        if (ret != BM_SUCCESS) {
            printf("bmcv_fft_0d_create_plan failed!\n");
        }
        free(F);
        return ret;
    }

    while (L % 5 == 0) {
        F[i++] = 5;
        L /= 5;
    }
    while (L % 3 == 0) {
        F[i++] = 3;
        L /= 3;
    }

    while (L % 2 == 0) {
        F[i++] = 2;
        L /= 2;
    }

    if (L != 1) {
        printf("INVALID INPUT SIZE (ONLY PRODUCT OF 2^a, 3^b, 4^c, 5^d IS SUPPORTED).\n");
        return BM_ERR_PARAM;
    }

    if (i > 1) {
        if (F[i - 1] == 2 && F[i - 2] == 2) {
            i--;
            F[i - 1] = 4;
        }
    }

    if (EU_ALIGN(len / F[0]) * 2 + EU_ALIGN(len) * 3 > LOCAL_MEM_SIZE / 4) {
        printf("THE LENGTH OF INPUT IS TOO LARGE.\n");
        free(F);
        return BM_ERR_PARAM;
    }

    ret = bmcv_fft_1d_create_plan_by_factors(handle, batch, len, F, i, forward, (struct FFT1DPlan**)plan);
    free(F);
    return ret;
}

bm_status_t bmcv_fft_2d_create_plan(bm_handle_t handle, int M, int N, bool forward,
                                    void** Plan)
{
    if (M == 1)
        return bmcv_fft_1d_create_plan(handle, 1, N, forward, Plan);
    else if (N == 1)
        return bmcv_fft_1d_create_plan(handle, 1, M, forward, Plan);
    bm_status_t ret = BM_SUCCESS;
    struct FFT2DPlan** plan = (struct FFT2DPlan**)Plan;

    *plan = (struct FFT2DPlan*)malloc(sizeof(struct FFT2DPlan));
    memset(*plan, 0, sizeof(struct FFT2DPlan));

    struct FFTPlan plan_type;
    plan_type.type = FFTType_2D;
    (*plan)->type = plan_type;

    if (bmcv_fft_1d_create_plan(handle, M, N, forward, &((*plan)->MP)) != BM_SUCCESS) {
        bmlib_log("FFT-2D", BMLIB_LOG_ERROR, "create MP failed!\r\n");
        return BM_ERR_FAILURE;
    }
    if (bmcv_fft_1d_create_plan(handle, N, M, forward, &((*plan)->NP)) != BM_SUCCESS) {
        bmlib_log("FFT-2D", BMLIB_LOG_ERROR, "create NP failed!\r\n");
        return BM_ERR_FAILURE;
    }
    if (bm_malloc_device_byte(handle, &((*plan)->TR), M * N * 4) != BM_SUCCESS) {
        bmlib_log("FFT", BMLIB_LOG_ERROR, "alloc TR failed!\r\n");
        return BM_ERR_FAILURE;
    }
    (*plan)->TR_flag = true;
    if (bm_malloc_device_byte(handle, &((*plan)->TI), M * N * 4) != BM_SUCCESS) {
        bmlib_log("FFT", BMLIB_LOG_ERROR, "alloc TI failed!\r\n");
        bm_free_device(handle, (*plan)->TR);
        (*plan)->TR_flag = false;
        return BM_ERR_FAILURE;
    }
    (*plan)->TI_flag = true;
    (*plan)->trans = 1;

    return ret;
}

static bm_status_t bmcv_fft_0d_execute(bm_handle_t handle, bm_device_mem_t inputReal, bm_device_mem_t inputImag,
                                       bm_device_mem_t outputReal, bm_device_mem_t outputImag,
                                       const void *plan, bool realInput)
{
    bm_status_t ret = BM_SUCCESS;
    struct FFT0DPlan* P = (struct FFT0DPlan*)plan;

    if (bm_memcpy_d2d_with_core(handle, outputReal, 0, inputReal, 0, P->batch, 0) != BM_SUCCESS) {
        bmlib_log("FFT", BMLIB_LOG_ERROR, "memcpy d2d failed!\r\n");
        return BM_ERR_FAILURE;
    }

    if (realInput) {
        if (bm_memset_device(handle, 0x0, outputImag) != BM_SUCCESS) {
            bmlib_log("FFT", BMLIB_LOG_ERROR, "memset device failed!\r\n");
            return BM_ERR_FAILURE;
        }
    } else {
        if (bm_memcpy_d2d_with_core(handle, outputImag, 0, inputImag, 0, P->batch * 4, 0) != BM_SUCCESS) {
            bmlib_log("FFT", BMLIB_LOG_ERROR, "memcpy d2d failed!\r\n");
            return BM_ERR_FAILURE;
        }
    }

    return ret;
}

static bm_status_t bmcv_fft_1d_execute_bm1684X(bm_handle_t handle, bm_device_mem_t inputReal,
                                       bm_device_mem_t inputImag, bm_device_mem_t outputReal,
                                       bm_device_mem_t outputImag, const void *plan, bool realInput,
                                       int trans)
{
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_fft_t api;
    struct FFT1DPlan* P = (struct FFT1DPlan*)plan;
    int i;
    int core_id = 0;
    unsigned int chipid;

    api.XR = bm_mem_get_device_addr(inputReal);
    api.XI = realInput ? 0 : bm_mem_get_device_addr(inputImag);
    api.YR = bm_mem_get_device_addr(outputReal);
    api.YI = bm_mem_get_device_addr(outputImag);
    api.ER = bm_mem_get_device_addr(P->ER);
    api.EI = bm_mem_get_device_addr(P->EI);
    api.batch = P->batch;
    api.len = P->L;
    api.forward = P->forward ? 1 : 0;
    api.realInput = realInput ? 1 : 0;
    api.trans = trans;
    api.factorSize = P->factors_size;
    api.normalize = false;

    for (i = 0; i < P->factors_size; ++i) {
        api.factors[i] = P->factors[i];
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }
    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_fft", (u8*)(&api), sizeof(api), core_id);
            if(ret != BM_SUCCESS){
                bmlib_log("FFT_1D", BMLIB_LOG_ERROR, "fft_1d sync api error\n");
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

static bm_status_t bmcv_fft_1d_execute(bm_handle_t handle, bm_device_mem_t inputReal,
                                       bm_device_mem_t inputImag, bm_device_mem_t outputReal,
                                       bm_device_mem_t outputImag, const void *plan, bool realInput,
                                       int trans)
{
    unsigned int chipid;
    bm_status_t ret = BM_SUCCESS;

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }
    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bmcv_fft_1d_execute_bm1684X(handle, inputReal, inputImag, outputReal, outputImag,
                                            plan, realInput, trans);
            if(ret != BM_SUCCESS){
                bmlib_log("FFT", BMLIB_LOG_ERROR, "fft_1d sync api error\n");
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

static bm_status_t bmcv_fft_2d_execute(bm_handle_t handle, bm_device_mem_t inputReal, bm_device_mem_t inputImag,
                                       bm_device_mem_t outputReal, bm_device_mem_t outputImag,
                                       const void *plan, bool realInput)
{
    bm_status_t ret = BM_SUCCESS;
    struct FFT2DPlan* P = (struct FFT2DPlan*)plan;

    if (BM_SUCCESS != bmcv_fft_1d_execute(handle, inputReal, inputImag, P->TR, P->TI,
                                          P->MP, realInput, P->trans)) {
        bmlib_log("FFT-2D", BMLIB_LOG_ERROR, "exec MP failed!\r\n");
        return BM_ERR_FAILURE;
    }
    if (BM_SUCCESS != bmcv_fft_1d_execute(handle, P->TR, P->TI, outputReal, outputImag, P->NP,
                                          false, P->trans)) {
        bmlib_log("FFT-2D", BMLIB_LOG_ERROR, "exec NP failed!\r\n");
        return BM_ERR_FAILURE;
    }
    return ret;
}

bm_status_t bmcv_fft_execute(bm_handle_t handle, bm_device_mem_t inputReal, bm_device_mem_t inputImag,
                            bm_device_mem_t outputReal, bm_device_mem_t outputImag, const void *plan)
{
    bm_status_t ret = BM_SUCCESS;
    struct FFTPlan* Plan = (struct FFTPlan*)(plan);

    switch (Plan->type) {
    case FFTType_0D:
        ret = bmcv_fft_0d_execute(handle, inputReal, inputImag, outputReal, outputImag, plan, false);
        break;
    case FFTType_1D:
        ret = bmcv_fft_1d_execute(handle, inputReal, inputImag, outputReal, outputImag, plan, false, 0);
        break;
    case FFTType_2D:
        ret = bmcv_fft_2d_execute(handle, inputReal, inputImag, outputReal, outputImag, plan, false);
        break;
    default:
        printf("INVALID PLAN TYPE.\n");
        return BM_ERR_PARAM;
    }
    return ret;
}

bm_status_t bmcv_fft_execute_real_input(bm_handle_t handle, bm_device_mem_t inputReal, bm_device_mem_t outputReal,
                                        bm_device_mem_t outputImag, const void *plan)
{
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t dummy;
    struct FFTPlan* Plan = (struct FFTPlan*)(plan);

    switch (Plan->type) {
    case FFTType_0D:
        ret = bmcv_fft_0d_execute(handle, inputReal, dummy, outputReal, outputImag, plan, true);
        break;
    case FFTType_1D:
        ret = bmcv_fft_1d_execute(handle, inputReal, dummy, outputReal, outputImag, plan, true, 0);
        break;
    case FFTType_2D:
        ret = bmcv_fft_2d_execute(handle, inputReal, dummy, outputReal, outputImag, plan, true);
        break;
    default:
        printf("INVALID PLAN TYPE.\n");
        return BM_ERR_PARAM;
    }
    return ret;
}

void bmcv_fft_destroy_plan(bm_handle_t handle, void *plan)
{
    struct FFTPlan* Plan = (struct FFTPlan*)(plan);

    if (Plan->type == FFTType_0D) {
        struct FFT0DPlan* Plan_0D = (struct FFT0DPlan*)(plan);
        free(Plan_0D);
    } else if (Plan->type == FFTType_1D) {
        struct FFT1DPlan* Plan_1D = (struct FFT1DPlan*)(plan);
        if (Plan_1D->factors_size > 1) {
            bm_free_device(handle,Plan_1D->EI);
            Plan_1D->EI_flag = false;
            bm_free_device(handle, Plan_1D->ER);
            Plan_1D->ER_flag = false;
        }
        free(Plan_1D->factors);
        free(Plan_1D);
    } else if (Plan->type == FFTType_2D) {
        struct FFT2DPlan* Plan_2D = (struct FFT2DPlan*)(plan);
        if (Plan_2D->MP != NULL || Plan_2D->NP != NULL) {
            bmcv_fft_destroy_plan(handle, Plan_2D->MP);
            bmcv_fft_destroy_plan(handle, Plan_2D->NP);
            bm_free_device(handle, Plan_2D->TR);
            Plan_2D->TR_flag = false;
            bm_free_device(handle, Plan_2D->TI);
            Plan_2D->TI_flag = false;
        }

    }
}