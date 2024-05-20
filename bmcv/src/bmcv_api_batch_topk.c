#include <stdint.h>
#include <stdio.h>
#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"
#include "bmcv_common.h"

static bm_status_t bmcv_batch_topk_check(bm_handle_t handle, int batch_num, int batch, int k) {
  if (handle == NULL) {
    bmlib_log("BATCH_TOPK", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
    return BM_ERR_PARAM;
  }
  if (batch_num > 1000000) {
    bmlib_log("BATCH_TOPK", BMLIB_LOG_ERROR, "batch_num should not be greater than 1000000!\n");
    return BM_ERR_PARAM;
  }
  if (batch > 32) {
    bmlib_log("BATCH_TOPK", BMLIB_LOG_ERROR, "batch should not be greater than 32!\n");
    return BM_ERR_PARAM;
  }
  if(k > 100 || k > batch_num) {
    bmlib_log("BATCH_TOPK", BMLIB_LOG_ERROR, "k should not be greater than 100 or batch_num!\n");
    return BM_ERR_PARAM;
  }
  return BM_SUCCESS;
}

bm_status_t bmcv_batch_topk(
        bm_handle_t     handle,
        bm_device_mem_t src_data_addr,
        bm_device_mem_t src_index_addr,
        bm_device_mem_t dst_data_addr,
        bm_device_mem_t dst_index_addr,
        bm_device_mem_t buffer_addr,
        bool            src_index_valid,
        int             k,
        int             batch,
        int*            per_batch_cnt,
        bool            same_batch_cnt,
        int             src_batch_stride,
        bool            descending) {
  bm_device_mem_t src_data_device;
  bm_device_mem_t dst_data_device;
  bm_device_mem_t src_index_device;
  bm_device_mem_t dst_index_device;
  bm_device_mem_t* internal_mem_v[4];
  int size = 0;
  sg_api_batch_topk_t api;
  bm_status_t ret = BM_SUCCESS;

  ret = bmcv_batch_topk_check(handle, per_batch_cnt[0], batch, k);
  if (ret != BM_SUCCESS) {
    BMCV_ERR_LOG("batch_topk_check error\r\n");
    return ret;
  }
  if (bm_mem_get_type(src_data_addr) == BM_MEM_TYPE_SYSTEM) {
    BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(
                      handle, &src_data_device, src_data_addr,
                      true,  // needs copy
                      src_batch_stride * batch));
    internal_mem_v[size++] = &src_data_device;
  } else {
    src_data_device = src_data_addr;
  }
  if (src_index_valid && (bm_mem_get_type(src_index_addr) == BM_MEM_TYPE_SYSTEM)) {
    BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(
                      handle, &src_index_device, src_index_addr,
                      true,  // needs copy
                      src_batch_stride * batch));
    internal_mem_v[size++] = &src_index_device;
  } else {
    src_index_addr.flags.u.mem_type = BM_MEM_TYPE_DEVICE;
    src_index_device = src_index_addr;
  }
  if (bm_mem_get_type(dst_data_addr) == BM_MEM_TYPE_SYSTEM) {
      BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(
                       handle, &dst_data_device, dst_data_addr,
                       false,
                       batch * k));
      internal_mem_v[size++] = &dst_data_device;
  } else {
      dst_data_device = dst_data_addr;
  }
  if (bm_mem_get_type(dst_index_addr) == BM_MEM_TYPE_SYSTEM) {
      BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(
                       handle, &dst_index_device, dst_index_addr,
                       false,
                       batch * k));
      internal_mem_v[size++] = &dst_index_device;
  } else {
      dst_index_device = dst_index_addr;
  }
  api.input_data_addr = bm_mem_get_device_addr(src_data_device);
  api.input_index_addr = bm_mem_get_device_addr(src_index_device);
  api.output_data_addr = bm_mem_get_device_addr(dst_data_device);
  api.output_index_addr = bm_mem_get_device_addr(dst_index_device);
  api.buffer_addr = 0;
  api.input_index_valid = src_index_valid;
  api.k = k;
  api.descending = descending;
  api.batchs = batch;
  api.batch_num = per_batch_cnt[0];
  api.batch_stride = src_batch_stride;
  api.dtype = 8;

  unsigned int chipid = BM1688;
  int core_id = 0;
  ret = bm_get_chipid(handle, &chipid);
  if (BM_SUCCESS != ret) {
    return ret;
  }

  switch (chipid) {
    case BM1688:{
      bm_status_t status = BM_SUCCESS;
      status = bm_tpu_kernel_launch(handle, "cv_batch_topk", (u8 *)&api, sizeof(api), core_id);
      if (BM_SUCCESS != status) {
          BMCV_ERR_LOG("batch topk send api error\r\n");
          goto free_devmem;
      }
      break;
    }
    default:
      ret = BM_NOT_SUPPORTED;
      break;
    }
    // Copy data
    if (bm_mem_get_type(dst_data_addr) == BM_MEM_TYPE_SYSTEM) {
      ret = bm_memcpy_d2s_partial(handle, bm_mem_get_system_addr(dst_data_addr), dst_data_device,
                                  batch * k * sizeof(float));
      if (BM_SUCCESS != ret) {
        BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
        goto free_devmem;
      }
    }
    if (bm_mem_get_type(dst_index_addr) == BM_MEM_TYPE_SYSTEM) {
      ret = bm_memcpy_d2s_partial(handle, bm_mem_get_system_addr(dst_index_addr), dst_index_device,
                                  batch * k * sizeof(int));
      if (BM_SUCCESS != ret) {
        BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
        goto free_devmem;
      }
    }

free_devmem:
  for(int i = 0; i < size; i++){
    bm_free_device(handle, *internal_mem_v[i]);
  }
  return (ret == BM_SUCCESS) ? BM_SUCCESS : BM_ERR_FAILURE;
}
