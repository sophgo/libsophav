#include <stdint.h>
#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"

static bm_status_t bmcv_sort_check(bm_handle_t handle, int data_cnt, int sort_cnt) {
    if (handle == NULL) {
        bmlib_log("SORT", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    if (data_cnt > 500000) {
        bmlib_log("SORT", BMLIB_LOG_ERROR, "data_cnt should be less than 500000!\n");
        return BM_ERR_PARAM;
    }
    if (sort_cnt > data_cnt) {
        bmlib_log("SORT", BMLIB_LOG_ERROR, "sort_cnt should be less than data_cnt!\n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_sort(bm_handle_t handle, bm_device_mem_t src_index_addr, bm_device_mem_t src_data_addr,
                      int data_cnt, bm_device_mem_t dst_index_addr, bm_device_mem_t dst_data_addr,
                      int sort_cnt, int order, bool index_enable, bool auto_index) {
    bm_api_cv_sort_t api;
    bool use_index_i = index_enable && (!auto_index);
    bool use_index_o = index_enable;
    bm_device_mem_t src_index_buf_device;
    bm_device_mem_t src_data_buf_device;
    bm_device_mem_t dst_index_buf_device;
    bm_device_mem_t dst_data_buf_device;
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid;
    int core_id = 0;

    ret = bmcv_sort_check(handle, data_cnt, sort_cnt);
    if (ret != BM_SUCCESS) {
        return ret;
    }
    if (bm_mem_get_type(src_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_i) {
        BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(handle, &src_index_buf_device, src_index_addr,
                                                           true, data_cnt));
    } else {
        src_index_buf_device = src_index_addr;
    }

    if (bm_mem_get_type(src_data_addr) == BM_MEM_TYPE_SYSTEM) {
        BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(handle, &src_data_buf_device, src_data_addr,
                                                           true, data_cnt));
    } else {
        src_data_buf_device = src_data_addr;
    }

    if (bm_mem_get_type(dst_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_o) {
        BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(handle, &dst_index_buf_device, dst_index_addr,
                                                           false, data_cnt));
    } else {
        dst_index_buf_device = dst_index_addr;
    }

    if (bm_mem_get_type(dst_data_addr) == BM_MEM_TYPE_SYSTEM) {
        BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(handle, &dst_data_buf_device, dst_data_addr,
                                                           false, data_cnt));
    } else {
        dst_data_buf_device = dst_data_addr;
    }

    api.src_index_addr = use_index_i ? bm_mem_get_device_addr(src_index_buf_device) : 0;
    api.src_data_addr  = bm_mem_get_device_addr(src_data_buf_device);
    api.dst_index_addr = use_index_o ? bm_mem_get_device_addr(dst_index_buf_device) : 0;
    api.dst_data_addr  = bm_mem_get_device_addr(dst_data_buf_device);
    api.order          = order;
    api.index_enable   = index_enable ? 1 : 0;
    api.auto_index     = auto_index ? 1 : 0;
    api.data_cnt       = data_cnt;
    api.sort_cnt       = sort_cnt;

    ret = bm_get_chipid(handle, &chipid);
    if (ret != BM_SUCCESS) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_sort", (u8 *)&api, sizeof(api), core_id);
            break;

        default:
            ret = BM_NOT_SUPPORTED;
            printf("BM_NOT_SUPPORTED!\n");
            break;

        if(BM_SUCCESS != ret) {
            bmlib_log("SORT", BMLIB_LOG_ERROR, "sort sync api error\n");
        }
    }

    // copy and free
    if (bm_mem_get_type(dst_data_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_mem_set_device_size(&dst_data_buf_device, sort_cnt * 4);
        if (BM_SUCCESS != bm_memcpy_d2s(handle, bm_mem_get_system_addr(dst_data_addr),
                                        dst_data_buf_device)) {
            bm_mem_set_device_size(&dst_data_buf_device, data_cnt * 4);
            BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
            goto err2;
        }
        bm_mem_set_device_size(&dst_data_buf_device, data_cnt * 4);
        bm_free_device(handle, dst_data_buf_device);
    }
    if (bm_mem_get_type(dst_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_o) {
        bm_mem_set_device_size(&dst_index_buf_device, sort_cnt * 4);
        if (BM_SUCCESS != bm_memcpy_d2s(handle, bm_mem_get_system_addr(dst_index_addr),
                                        dst_index_buf_device)) {
            bm_mem_set_device_size(&dst_index_buf_device, data_cnt * 4);
            BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
            goto err1;
        }
        bm_mem_set_device_size(&dst_index_buf_device, data_cnt * 4);
        bm_free_device(handle, dst_index_buf_device);
    }
    if (bm_mem_get_type(src_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_i) {
        bm_free_device(handle, src_index_buf_device);
    }
    if (bm_mem_get_type(src_data_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, src_data_buf_device);
    }

    return BM_SUCCESS;

err2:
    if (bm_mem_get_type(dst_data_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, dst_data_buf_device);
    }
err1:
    if (bm_mem_get_type(dst_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_o) {
        bm_free_device(handle, dst_index_buf_device);
    }
    if (bm_mem_get_type(src_data_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, src_data_buf_device);
    }
    if (bm_mem_get_type(src_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_i) {
        bm_free_device(handle, src_index_buf_device);
    }
    return BM_ERR_FAILURE;
}
