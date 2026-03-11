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
        int input_w)
{
    bm_status_t ret;
    int if_core0 = 1, if_core1 = 0;
    const char *tpu_env;
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
            tpu_env = getenv("TPU_CORES");
            if (tpu_env) {
                if (strcmp(tpu_env, "0") == 0) {
                    bmlib_log("AXPY", BMLIB_LOG_DEBUG, "Use TPU Core0\n");
                } else if (strcmp(tpu_env, "1") == 0) {
                    if_core0 = 0;
                    if_core1 = 1;
                    bmlib_log("AXPY", BMLIB_LOG_DEBUG, "Use TPU Core1\n");
                } else if (strcmp(tpu_env, "2") == 0 || strcmp(tpu_env, "both") == 0) {
                    if_core1 = 1;
                    bmlib_log("AXPY", BMLIB_LOG_DEBUG, "Use All TPU Cores(0 and 1)\n");
                } else {
                    bmlib_log("AXPY", BMLIB_LOG_ERROR, "Invalid TPU_CORES value: %s\n", tpu_env);
                    bmlib_log("AXPY", BMLIB_LOG_ERROR, "Available options: 0, 1, 2/both\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (if_core0 && if_core1) {
                bm_api_cv_axpy_dual_core_t dual_api;
                int core_list[BM1688_MAX_CORES] = {0, 1};

                bm_api_cv_axpy_dual_core_t dual_params[BM1688_MAX_CORES];
                tpu_launch_param_t tpu_params[BM1688_MAX_CORES];

                memcpy(&dual_api, &api, sizeof(bm_api_cv_axpy_t));

                dual_api.core_num = BM1688_MAX_CORES;
                dual_api.base_msg_id = BM1688_BASE_MSG_ID;

                for (int n = 0; n < BM1688_MAX_CORES; n++) {
                    dual_api.core_id = n;

                    dual_params[n] = dual_api;
                    tpu_params[n].core_id = n;
                    tpu_params[n].param_data = &dual_params[n];
                    tpu_params[n].param_size = sizeof(bm_api_cv_axpy_dual_core_t);
                }

                ret = bm_tpu_kernel_launch_dual_core(handle, "cv_axpy_dual_core", tpu_params, core_list, BM1688_MAX_CORES);

            } else {
                int core_id = if_core1 == 1 ? 1 : 0;
                ret = bm_tpu_kernel_launch(handle, "cv_axpy", (u8 *)&api, sizeof(api), core_id);
            }

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
