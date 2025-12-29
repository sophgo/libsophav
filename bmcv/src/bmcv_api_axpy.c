#include "bmcv_common.h"
#include "bmcv_internal.h"

bm_status_t  bmcv_image_axpy(
        bm_handle_t handle,
        bm_device_mem_t tensor_A,
        bm_device_mem_t tensor_X,
        bm_device_mem_t tensor_Y,
        bm_device_mem_t tensor_F,
        int input_n,
        int input_c,
        int input_h,
        int input_w) {

    bm_status_t ret;
    int core_id = 0;
    bm_device_mem_t tensor_A_mem, tensor_X_mem, tensor_Y_mem, tensor_F_mem;
    if (bm_mem_get_type(tensor_A) == BM_MEM_TYPE_SYSTEM) {
        ret =  bm_mem_convert_system_to_device_neuron(
            handle, &tensor_A_mem, tensor_A,
            true,
            input_n, input_c, 1, 1);
    }else {
        tensor_A_mem = tensor_A;
    }

    if ( bm_mem_get_type(tensor_X) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_neuron(
            handle, &tensor_X_mem, tensor_X,
            true,
            input_n, input_c, input_h, input_w);
    }else {
        tensor_X_mem = tensor_X;
    }
    if ( bm_mem_get_type(tensor_Y) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_neuron(
            handle, &tensor_Y_mem, tensor_Y,
            true,
            input_n, input_c, input_h, input_w);
    }else {
        tensor_Y_mem = tensor_Y;
    }
    if ( bm_mem_get_type(tensor_F) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_mem_convert_system_to_device_neuron(
        handle, &tensor_F_mem, tensor_F,
            true,
            input_n, input_c, input_h, input_w);
    }else {
        tensor_F_mem = tensor_F;
    }

    bm_api_cv_axpy_t api;
    api.A_global_offset = bm_mem_get_device_addr(tensor_A_mem);
    api.F_global_offset = bm_mem_get_device_addr(tensor_F_mem);
    api.X_global_offset =  bm_mem_get_device_addr(tensor_X_mem);
    api.Y_global_offset = bm_mem_get_device_addr(tensor_Y_mem);
    api.input_c = input_c;
    api.input_n = input_n;
    api.input_h = input_h;
    api.input_w = input_w;

    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
      return ret;

    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_axpy", (u8 *)&api, sizeof(api), core_id);
            if (BM_SUCCESS != ret) {
                bmlib_log("AXPY", BMLIB_LOG_ERROR, "axpy sync api error\n");
                if(bm_mem_get_type(tensor_F) == BM_MEM_TYPE_SYSTEM){
                    bm_free_device(handle, tensor_F_mem);
                }

                if (bm_mem_get_type(tensor_X) == BM_MEM_TYPE_SYSTEM) {
                    bm_free_device(handle, tensor_X_mem);
                }
                if (bm_mem_get_type(tensor_Y) == BM_MEM_TYPE_SYSTEM) {
                    bm_free_device(handle, tensor_Y_mem);
                }
                if (bm_mem_get_type(tensor_A) == BM_MEM_TYPE_SYSTEM) {
                    bm_free_device(handle, tensor_A_mem);
                }

                return ret;
            }
            break;

        default:
            printf("BM_NOT_SUPPORTED! \n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    if(bm_mem_get_type(tensor_F) == BM_MEM_TYPE_SYSTEM){
        bm_memcpy_d2s(handle, bm_mem_get_system_addr(tensor_F), tensor_F_mem);
        bm_free_device(handle, tensor_F_mem);
    }

    if (bm_mem_get_type(tensor_X) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, tensor_X_mem);
    }
    if (bm_mem_get_type(tensor_Y) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, tensor_Y_mem);
    }

    if (bm_mem_get_type(tensor_A) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, tensor_A_mem);
    }

    return BM_SUCCESS;
}
