#include "bmcv_common.h"
#include "bmcv_internal.h"

bm_status_t bmcv_matmul(bm_handle_t handle, int M, int N, int K, bm_device_mem_t A, bm_device_mem_t B,
                        bm_device_mem_t C, int  A_sign, int B_sign, int rshift_bit, int result_type,
                        bool is_B_trans, float alpha, float beta)
{
    bm_device_mem_t input_mem, weight_mem, output_mem;
    bm_status_t ret = BM_SUCCESS;
    int byte_num;
    unsigned int chipid;
    bm_api_cv_matmul_t api;
    int core_id = 0;

    if (handle == NULL) {
        bmlib_log("MATMUL", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }

    if (bm_mem_get_type(A) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_neuron_byte(handle, &input_mem, A, true, M, 1, K, 1);
        if (ret) {
            printf("bm_mem_convert_system_to_device_neuron_byte input_mem is error !\n");
            return ret;
        }
    } else {
        input_mem = A;
    }

    if (bm_mem_get_type(B) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_coeff_byte(handle, &weight_mem, B, true, K * N);
        if (ret) {
            printf("bm_mem_convert_system_to_device_coeff_byte weight_mem is error !\n");
            return ret;
        }
    } else {
        weight_mem = B;
    }

    if (bm_mem_get_type(C) == BM_MEM_TYPE_SYSTEM) {
        byte_num = result_type ? 2 * result_type : 1;
        ret = bm_mem_convert_system_to_device_neuron_byte(handle, &output_mem, C, false, \
                                                        M, N * byte_num, 1, 1);
        if (ret) {
            printf("bm_mem_convert_system_to_device_neuron_byte output_mem is error !\n");
            return ret;
        }
    } else {
        output_mem = C;
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("bm_get_chipid is error !\n");
        goto exit;
    }

    api.M = M;
    api.N = N;
    api.K = K;
    api.A_addr = bm_mem_get_device_addr(input_mem);
    api.B_addr = bm_mem_get_device_addr(weight_mem);
    api.C_addr = bm_mem_get_device_addr(output_mem);
    api.A_sign = A_sign;
    api.B_sign = B_sign;
    api.rshift_bit = rshift_bit;
    api.result_type = result_type;
    api.is_B_trans = is_B_trans;
    api.alpha = alpha;
    api.beta = beta;

    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_matmul", (u8*)&api,
                                                sizeof(api), core_id);
            if (ret != BM_SUCCESS) {
                bmlib_log("Matmul", BMLIB_LOG_ERROR, "matmul sync api error\n");
                goto exit;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

exit:
    if (bm_mem_get_type(A) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, input_mem);
    }

    if (bm_mem_get_type(B) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, weight_mem);
    }

    if (bm_mem_get_type(C) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(C), output_mem);
        if (ret != BM_SUCCESS) {
            BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
            bm_free_device(handle, output_mem);
            return ret;
        }
        bm_free_device(handle, output_mem);
    }

    return ret;
}

bm_status_t bmcv_matmul_u64(bm_handle_t handle, int M, int N, int K, bm_device_mem_u64_t A, bm_device_mem_u64_t B,
    bm_device_mem_u64_t C, int  A_sign, int B_sign, int rshift_bit, int result_type,
    bool is_B_trans, float alpha, float beta)
{
    bm_device_mem_u64_t input_mem, weight_mem, output_mem;
    bm_status_t ret = BM_SUCCESS;
    int byte_num;
    unsigned int chipid;
    bm_api_cv_matmul_t api;
    int core_id = 0;

    if (handle == NULL) {
        bmlib_log("MATMUL", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }

    if (bm_mem_get_type_u64(A) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_neuron_byte_u64(handle, &input_mem, A, true, M, 1, K, 1);
        if (ret) {
            printf("bm_mem_convert_system_to_device_neuron_byte input_mem is error !\n");
            return ret;
        }
    } else {
        input_mem = A;
    }

    if (bm_mem_get_type_u64(B) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_coeff_byte_u64(handle, &weight_mem, B, true, K * N);
        if (ret) {
            printf("bm_mem_convert_system_to_device_coeff_byte weight_mem is error !\n");
            return ret;
        }
    } else {
        weight_mem = B;
    }

    if (bm_mem_get_type_u64(C) == BM_MEM_TYPE_SYSTEM) {
        byte_num = result_type ? 2 * result_type : 1;
        ret = bm_mem_convert_system_to_device_neuron_byte_u64(handle, &output_mem, C, false, \
                                            M, N * byte_num, 1, 1);
        if (ret) {
            printf("bm_mem_convert_system_to_device_neuron_byte output_mem is error !\n");
            return ret;
        }
    } else {
        output_mem = C;
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("bm_get_chipid is error !\n");
        goto exit;
    }

    api.M = M;
    api.N = N;
    api.K = K;
    api.A_addr = bm_mem_get_device_addr_u64(input_mem);
    api.B_addr = bm_mem_get_device_addr_u64(weight_mem);
    api.C_addr = bm_mem_get_device_addr_u64(output_mem);
    api.A_sign = A_sign;
    api.B_sign = B_sign;
    api.rshift_bit = rshift_bit;
    api.result_type = result_type;
    api.is_B_trans = is_B_trans;
    api.alpha = alpha;
    api.beta = beta;

    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_matmul", (u8*)&api,
                                    sizeof(api), core_id);
        if (ret != BM_SUCCESS) {
            bmlib_log("Matmul", BMLIB_LOG_ERROR, "matmul sync api error\n");
            goto exit;
        }
        break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
        break;
    }

exit:
    if (bm_mem_get_type_u64(A) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device_u64(handle, input_mem);
    }

    if (bm_mem_get_type_u64(B) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device_u64(handle, weight_mem);
    }

    if (bm_mem_get_type_u64(C) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_memcpy_d2s_u64(handle, bm_mem_get_system_addr_u64(C), output_mem);
        if (ret != BM_SUCCESS) {
            BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
            bm_free_device_u64(handle, output_mem);
            return ret;
        }
        bm_free_device_u64(handle, output_mem);
    }

    return ret;
}