#include "bmcv_common.h"
#include "bmcv_internal.h"

typedef enum gemm_bm_image_data_format_ext_ {
    GEMM_DATA_TYPE_EXT_FLOAT32,
    GEMM_DATA_TYPE_EXT_1N_BYTE,
    GEMM_DATA_TYPE_EXT_4N_BYTE,
    GEMM_DATA_TYPE_EXT_1N_BYTE_SIGNED,
    GEMM_DATA_TYPE_EXT_4N_BYTE_SIGNED,
    GEMM_DATA_TYPE_EXT_FP16,
    GEMM_DATA_TYPE_EXT_BF16,
} gemm_bm_image_data_format_ext;

bm_status_t bmcv_gemm(bm_handle_t handle, bool is_A_trans, bool is_B_trans, int M, int N, int K, float alpha,
                      bm_device_mem_t A, int lda, bm_device_mem_t B, int ldb, float beta, bm_device_mem_t C,
                      int ldc)
{
    bm_device_mem_t A_mem, B_mem, C_mem;
    bm_api_cv_gemm_t api;
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid;
    int core_id = 0;

    if (handle == NULL) {
        printf("GEMM Can not get handle!\r\n");
        return BM_ERR_FAILURE;
    }

    if (bm_mem_get_type(A) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_neuron(handle, &A_mem, A, true, M, K, 1, 1);
        if (ret != BM_SUCCESS) {
            printf("A_mem convert error\r\n");
            goto err0;
        }
    } else {
        A_mem = A;
    }

    if (bm_mem_get_type(B) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_coeff(handle, &B_mem, B, true, K * N);
        if ( ret != BM_SUCCESS) {
            printf("B_mem convert error\r\n");
            goto err1;
        }
    } else {
        B_mem = B;
    }

    if (bm_mem_get_type(C) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_neuron(handle, &C_mem, C, true, M, N, 1, 1);
        if (ret != BM_SUCCESS) {
            printf("C_mem convert error\r\n");
            goto err2;
        }
    } else {
        C_mem = C;
    }

    api.A_global_offset = bm_mem_get_device_addr(A_mem);
    api.B_global_offset = bm_mem_get_device_addr(B_mem),
    api.C_global_offset = bm_mem_get_device_addr(C_mem),
    api.Y_global_offset = bm_mem_get_device_addr(C_mem),
    api.M = M;
    api.N = N;
    api.K = K;
    api.is_A_trans = is_A_trans;
    api.is_B_trans = is_B_trans;
    api.alpha = alpha;
    api.beta = beta;
    api.in_dtype = DATA_TYPE_EXT_FLOAT32;
    api.out_dtype = DATA_TYPE_EXT_FLOAT32;

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        goto err3;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_gemm", (u8*)(&api), sizeof(api), core_id);
            if(ret != BM_SUCCESS){
                bmlib_log("GEMM", BMLIB_LOG_ERROR, "gemm sync api error\n");
                goto err3;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    if (bm_mem_get_type(C) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(C), C_mem);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_d2s error\r\n");
            goto err3;
        }
        bm_free_device(handle, C_mem);
    }

    if (bm_mem_get_type(A) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, A_mem);
    }

    if (bm_mem_get_type(B) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, B_mem);
    }

    return BM_SUCCESS;

err3:
    if (bm_mem_get_type(C) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, C_mem);
    }
err2:
    if (bm_mem_get_type(B) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, B_mem);
    }
err1:
    if (bm_mem_get_type(A) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, A_mem);
    }
err0:
    return BM_ERR_FAILURE;
}

bm_status_t bmcv_gemm_ext(bm_handle_t handle, bool is_A_trans, bool is_B_trans, int M, int N, int K,
                          float alpha, bm_device_mem_t A, bm_device_mem_t B, float beta,
                          bm_device_mem_t C, bm_device_mem_t Y, bm_image_data_format_ext input_dtype,
                          bm_image_data_format_ext output_dtype)
{
    bm_device_mem_t A_mem, B_mem, C_mem, Y_mem;
    bm_api_cv_gemm_t api;
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid;
    int core_id = 0;

    if (handle == NULL) {
        printf("GEMM Can not get handle!\r\n");
        return BM_ERR_FAILURE;
    }

    if (is_A_trans && !is_B_trans) {
        printf("GEMM Can not support L_trans and R_not_trans!\r\n");
        return BM_ERR_FAILURE;
    }

    if(input_dtype == DATA_TYPE_EXT_FP16 && is_A_trans && M > 64) {
        printf("GEMM It only support M <= 64 when A is trans and input_dtype is FP16\n");
        return BM_NOT_SUPPORTED;
    }

    A_mem = A;
    B_mem = B;
    C_mem = C;
    Y_mem = Y;

    api.A_global_offset = bm_mem_get_device_addr(A_mem);
    api.B_global_offset = bm_mem_get_device_addr(B_mem);
    api.C_global_offset = bm_mem_get_device_addr(C_mem);
    api.Y_global_offset = bm_mem_get_device_addr(Y_mem);
    api.M = M;
    api.N = N;
    api.K = K;
    api.is_A_trans = is_A_trans;
    api.is_B_trans = is_B_trans;
    api.alpha = alpha;
    api.beta = beta;

    if (input_dtype == DATA_TYPE_EXT_FP16) {
        api.in_dtype = GEMM_DATA_TYPE_EXT_FP16;
    } else if (input_dtype == DATA_TYPE_EXT_FLOAT32) {
        api.in_dtype = GEMM_DATA_TYPE_EXT_FLOAT32;
    }

    if (output_dtype == DATA_TYPE_EXT_FP16) {
        api.out_dtype = GEMM_DATA_TYPE_EXT_FP16;
    } else if (output_dtype == DATA_TYPE_EXT_FLOAT32) {
        api.out_dtype = GEMM_DATA_TYPE_EXT_FLOAT32;
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_gemm", (u8*)(&api), sizeof(api), core_id);
            if(ret != BM_SUCCESS){
                bmlib_log("GEMM", BMLIB_LOG_ERROR, "gemm sync api error\n");
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


bm_status_t bmcv_gemm_u64(bm_handle_t handle, bool is_A_trans, bool is_B_trans, int M, int N, int K, float alpha,
    bm_device_mem_u64_t A, int lda, bm_device_mem_u64_t B, int ldb, float beta, bm_device_mem_u64_t C,
    int ldc)
{
bm_device_mem_u64_t A_mem, B_mem, C_mem;
bm_api_cv_gemm_t api;
bm_status_t ret = BM_SUCCESS;
unsigned int chipid;
int core_id = 0;

if (handle == NULL) {
printf("GEMM Can not get handle!\r\n");
return BM_ERR_FAILURE;
}

if (bm_mem_get_type_u64(A) == BM_MEM_TYPE_SYSTEM) {
ret = bm_mem_convert_system_to_device_neuron_u64(handle, &A_mem, A, true, M, K, 1, 1);
if (ret != BM_SUCCESS) {
printf("A_mem convert error\r\n");
goto err0;
}
} else {
A_mem = A;
}

if (bm_mem_get_type_u64(B) == BM_MEM_TYPE_SYSTEM) {
ret = bm_mem_convert_system_to_device_coeff_u64(handle, &B_mem, B, true, K * N);
if ( ret != BM_SUCCESS) {
printf("B_mem convert error\r\n");
goto err1;
}
} else {
B_mem = B;
}

if (bm_mem_get_type_u64(C) == BM_MEM_TYPE_SYSTEM) {
ret = bm_mem_convert_system_to_device_neuron_u64(handle, &C_mem, C, true, M, N, 1, 1);
if (ret != BM_SUCCESS) {
printf("C_mem convert error\r\n");
goto err2;
}
} else {
C_mem = C;
}

api.A_global_offset = bm_mem_get_device_addr_u64(A_mem);
api.B_global_offset = bm_mem_get_device_addr_u64(B_mem),
api.C_global_offset = bm_mem_get_device_addr_u64(C_mem),
api.Y_global_offset = bm_mem_get_device_addr_u64(C_mem),
api.M = M;
api.N = N;
api.K = K;
api.is_A_trans = is_A_trans;
api.is_B_trans = is_B_trans;
api.alpha = alpha;
api.beta = beta;
api.in_dtype = DATA_TYPE_EXT_FLOAT32;
api.out_dtype = DATA_TYPE_EXT_FLOAT32;

ret = bm_get_chipid(handle, &chipid);
if (ret) {
printf("get chipid is error !\n");
goto err3;
}

switch(chipid) {
case BM1688_PREV:
case BM1688:
ret = bm_tpu_kernel_launch(handle, "cv_gemm", (u8*)(&api), sizeof(api), core_id);
if(ret != BM_SUCCESS){
bmlib_log("GEMM", BMLIB_LOG_ERROR, "gemm sync api error\n");
goto err3;
}
break;
default:
printf("BM_NOT_SUPPORTED!\n");
ret = BM_NOT_SUPPORTED;
break;
}

if (bm_mem_get_type_u64(C) == BM_MEM_TYPE_SYSTEM) {
ret = bm_memcpy_d2s_u64(handle, bm_mem_get_system_addr_u64(C), C_mem);
if (ret != BM_SUCCESS) {
printf("bm_memcpy_d2s error\r\n");
goto err3;
}
bm_free_device_u64(handle, C_mem);
}

if (bm_mem_get_type_u64(A) == BM_MEM_TYPE_SYSTEM) {
bm_free_device_u64(handle, A_mem);
}

if (bm_mem_get_type_u64(B) == BM_MEM_TYPE_SYSTEM) {
bm_free_device_u64(handle, B_mem);
}

return BM_SUCCESS;

err3:
if (bm_mem_get_type_u64(C) == BM_MEM_TYPE_SYSTEM) {
bm_free_device_u64(handle, C_mem);
}
err2:
if (bm_mem_get_type_u64(B) == BM_MEM_TYPE_SYSTEM) {
bm_free_device_u64(handle, B_mem);
}
err1:
if (bm_mem_get_type_u64(A) == BM_MEM_TYPE_SYSTEM) {
bm_free_device_u64(handle, A_mem);
}
err0:
return BM_ERR_FAILURE;
}

bm_status_t bmcv_gemm_ext_u64(bm_handle_t handle, bool is_A_trans, bool is_B_trans, int M, int N, int K,
        float alpha, bm_device_mem_u64_t A, bm_device_mem_u64_t B, float beta,
        bm_device_mem_u64_t C, bm_device_mem_u64_t Y, bm_image_data_format_ext input_dtype,
        bm_image_data_format_ext output_dtype)
{
bm_device_mem_u64_t A_mem, B_mem, C_mem, Y_mem;
bm_api_cv_gemm_t api;
bm_status_t ret = BM_SUCCESS;
unsigned int chipid;
int core_id = 0;

if (handle == NULL) {
printf("GEMM Can not get handle!\r\n");
return BM_ERR_FAILURE;
}

if (is_A_trans && !is_B_trans) {
printf("GEMM Can not support L_trans and R_not_trans!\r\n");
return BM_ERR_FAILURE;
}

if(input_dtype == DATA_TYPE_EXT_FP16 && is_A_trans && M > 64) {
printf("GEMM It only support M <= 64 when A is trans and input_dtype is FP16\n");
return BM_NOT_SUPPORTED;
}

A_mem = A;
B_mem = B;
C_mem = C;
Y_mem = Y;

api.A_global_offset = bm_mem_get_device_addr_u64(A_mem);
api.B_global_offset = bm_mem_get_device_addr_u64(B_mem);
api.C_global_offset = bm_mem_get_device_addr_u64(C_mem);
api.Y_global_offset = bm_mem_get_device_addr_u64(Y_mem);
api.M = M;
api.N = N;
api.K = K;
api.is_A_trans = is_A_trans;
api.is_B_trans = is_B_trans;
api.alpha = alpha;
api.beta = beta;

if (input_dtype == DATA_TYPE_EXT_FP16) {
api.in_dtype = GEMM_DATA_TYPE_EXT_FP16;
} else if (input_dtype == DATA_TYPE_EXT_FLOAT32) {
api.in_dtype = GEMM_DATA_TYPE_EXT_FLOAT32;
}

if (output_dtype == DATA_TYPE_EXT_FP16) {
api.out_dtype = GEMM_DATA_TYPE_EXT_FP16;
} else if (output_dtype == DATA_TYPE_EXT_FLOAT32) {
api.out_dtype = GEMM_DATA_TYPE_EXT_FLOAT32;
}

ret = bm_get_chipid(handle, &chipid);
if (ret) {
printf("get chipid is error !\n");
return BM_ERR_FAILURE;
}

switch(chipid) {
case BM1688_PREV:
case BM1688:
ret = bm_tpu_kernel_launch(handle, "cv_gemm", (u8*)(&api), sizeof(api), core_id);
if(ret != BM_SUCCESS){
bmlib_log("GEMM", BMLIB_LOG_ERROR, "gemm sync api error\n");
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