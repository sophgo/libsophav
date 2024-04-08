#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"

static bm_status_t bmcv_hamming_distance_check(bm_handle_t handle, int input1_num, int input2_num) {
    if (handle == NULL) {
        bmlib_log("HAMMING_DISTANCE", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    if (input1_num <= 0 || input1_num > 100) {
        bmlib_log("HAMMING_DISTANCE", BMLIB_LOG_ERROR, "input1_num should be 1~100!\n");
        return BM_ERR_PARAM;
    }
    if (input2_num <= 0 || input2_num > 2500) {
        bmlib_log("HAMMING_DISTANCE", BMLIB_LOG_ERROR, "input2_num should be 1~2500!\n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_hamming_distance(bm_handle_t handle, bm_device_mem_t input1, bm_device_mem_t input2,
                                  bm_device_mem_t output, int bits_len, int input1_num, int input2_num) {
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_hamming_distance_t api;
    unsigned int chipid;
    int core_id = 0;

    ret = bmcv_hamming_distance_check(handle, input1_num, input2_num);
    if (BM_SUCCESS != ret) {
        return ret;
    }
    api.input_query_global_offset = bm_mem_get_device_addr(input1);
    api.input_database_global_offset = bm_mem_get_device_addr(input2);
    api.output_global_offset = bm_mem_get_device_addr(output);
    api.bits_len = bits_len;
    api.left_row = input1_num;
    api.right_row = input2_num;

    if (bm_get_chipid(handle, &chipid) != BM_SUCCESS) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    switch (chipid) {
        case BM1686:
            ret = bm_tpu_kernel_launch(handle, "cv_hanming_distance_1684x", (u8 *)&api, sizeof(api), core_id);
            if(BM_SUCCESS != ret) {
                bmlib_log("HM_DISTANCE", BMLIB_LOG_ERROR, "hm_distance sync api error\n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            printf("BM_NOT_SUPPORTED!\n");
            break;
    }
    return ret;
}
