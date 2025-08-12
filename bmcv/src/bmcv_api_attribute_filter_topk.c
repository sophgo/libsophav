#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"

bm_status_t bmcv_attribute_filter_topk_check(
        bm_handle_t handle,
        int data_type,
        int data_count,
        int per_row_num,
        int max_space_points,
        int time_range_min,
        int time_range_max,
        int attr3_flag,
        int topk,
        float threshold) {
    if (handle == NULL) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    if (data_type != 5 && data_type != 8) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "data_type only support 5 and 8!\n");
        return BM_ERR_PARAM;
    }
    if (data_count <= 0 || data_count > 50000000) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "data_count should be 1~50000000!\n");
        return BM_ERR_PARAM;
    }
    if (topk > data_count) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "topk should not be greater than data_count!\n");
        return BM_ERR_PARAM;
    }
    if ((max_space_points < 1 && max_space_points != -1) || max_space_points > 300000) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "max_space_points should be -1 or 1~300000!\n");
        return BM_ERR_PARAM;
    }
    if (time_range_min < 0 || time_range_min > 315359999) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "time_range_min should be 0~315359999!\n");
        return BM_ERR_PARAM;
    }
    if (time_range_max < time_range_min || time_range_max > 315360000) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "time_range_max should be time_range_min~315360000!\n");
        return BM_ERR_PARAM;
    }
    if (attr3_flag != 0 && attr3_flag != 1) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "attr3_flag should be 0 or 1!\n");
        return BM_ERR_PARAM;
    }
    if (topk < 1 || topk > 50000) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "topk should be 1~50000!\n");
        return BM_ERR_PARAM;
    }
    if (data_type == 8) {
        if ((uint32_t)threshold < 0) {
            bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "when data_type = 8, threshold should be greater than 0!\n");
            return BM_ERR_PARAM;
        }
    }
    if ((data_count < per_row_num) || (data_count % per_row_num != 0)) {
        bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "data_count should be a multiple of %d!\n", per_row_num);
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_attribute_filter_topk(
        bm_handle_t handle,
        int data_type,
        int data_count,
        int max_space_points,
        int time_range_min,
        int time_range_max,
        int attr3_flag,
        int topk,
        float threshold,
        int8_t attr_mask,
        bm_device_mem_t space_bits_dev_mem,
        bm_device_mem_t space_points_dev_mem,
        bm_device_mem_t time_points_dev_mem,
        bm_device_mem_t attributes_dev_mem,
        bm_device_mem_t similarity_data_dev_mem,
        bm_device_mem_t filtered_idx_dev_mem,
        bm_device_mem_t filtered_similarity_data_dev_mem,
        bm_device_mem_t topk_idx_dev_mem,
        bm_device_mem_t topk_data_dev_mem) {
    int per_row_num = 1024;
    bm_status_t ret = bmcv_attribute_filter_topk_check(handle, data_type, data_count, per_row_num, max_space_points, time_range_min,
                                            time_range_max, attr3_flag, topk, threshold);
    if(ret != BM_SUCCESS) {
        printf("bmcv_attribute_filter_topk_check failed!\n");
        return ret;
    }
    unsigned int chipid;
    if (bm_get_chipid(handle, &chipid) != BM_SUCCESS) {
        printf("bm_get_chipid error!\n");
        return BM_ERR_FAILURE;
    }
    sg_api_attribute_filter_topk_t api;
    api.data_type = data_type;
    api.data_count = data_count;
    api.per_row_num = per_row_num;
    api.max_space_points = max_space_points;
    api.time_range_min = time_range_min;
    api.time_range_max = time_range_max;
    api.attr3_flag = attr3_flag;
    api.topk = topk;
    api.threshold = threshold;
    api.attr_mask = attr_mask;
    api.space_bits_dev_mem = bm_mem_get_device_addr(space_bits_dev_mem);
    api.space_points_dev_mem = bm_mem_get_device_addr(space_points_dev_mem);
    api.time_points_dev_mem = bm_mem_get_device_addr(time_points_dev_mem);
    api.attributes_dev_mem = bm_mem_get_device_addr(attributes_dev_mem);
    api.similarity_data_dev_mem = bm_mem_get_device_addr(similarity_data_dev_mem);
    api.filtered_idx_dev_mem = bm_mem_get_device_addr(filtered_idx_dev_mem);
    api.filtered_similarity_data_dev_mem = bm_mem_get_device_addr(filtered_similarity_data_dev_mem);
    api.topk_data_dev_mem = bm_mem_get_device_addr(topk_data_dev_mem);
    api.topk_idx_dev_mem = bm_mem_get_device_addr(topk_idx_dev_mem);
    int core_id = 0;
    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_attribute_filter_topk", (u8 *)&api, sizeof(api), core_id);
            if(BM_SUCCESS != ret) {
                bmlib_log("ATTRIBUTE_FILTER_TOPK", BMLIB_LOG_ERROR, "attribute_filter_topk sync api error\n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            printf("Not support this chipid!\n");
            break;
    }
    return ret;
}