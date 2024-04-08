#include "bmcv_common.h"
#include "bmcv_internal.h"
#include <stdio.h>

#define MAX_TOPK 30
typedef float bm_fm_data_type_t;
typedef int8_t bm_fm_data_type_fix8b_t;

bm_status_t bmcv_feature_match_normalized(bm_handle_t handle, bm_device_mem_t input_data_global_addr,
                                        bm_device_mem_t db_data_global_addr, bm_device_mem_t db_feature_global_addr,
                                        bm_device_mem_t output_sorted_similarity_global_addr,
                                        bm_device_mem_t output_sorted_index_global_addr,
                                        int batch_size, int feature_size, int db_size)
{
    if (batch_size > 10) {
        printf("batch_size should not be greater than 10, input is :%d\r\n", batch_size);
        return BM_ERR_FAILURE;
    }
    if (feature_size > 1000) {
        printf("feature_size should not be greater than 1000, input is :%d\r\n", feature_size);
        return BM_ERR_FAILURE;
    }
    if (db_size > 90000) {
        printf("db_size should not be greater than 90000, input is :%d\r\n", db_size);
        return BM_ERR_FAILURE;
    }
    bm_api_cv_feature_match_1684x_t arg;
    bm_device_mem_t db_data_trans_addr, output_temp_addr;
    bm_device_mem_t input_data_global_addr_device, db_data_global_addr_device;
    bm_device_mem_t db_feature_global_addr_device;
    bm_device_mem_t output_sorted_similarity_global_addr_device;
    bm_device_mem_t output_sorted_index_global_addr_device;
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid;
    int core_id = 0;

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    if (bm_mem_get_type(input_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_malloc_device_byte(handle, &input_data_global_addr_device,
                                    sizeof(bm_fm_data_type_t) * batch_size * feature_size);
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_malloc_device_byte error!");
            goto err0;
        }
        ret = bm_memcpy_s2d(handle, input_data_global_addr_device,
                          bm_mem_get_system_addr(input_data_global_addr));
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_memcpy_s2d error!");
            goto err1;
        }
    } else {
        input_data_global_addr_device = input_data_global_addr;
    }

    if (bm_mem_get_type(db_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_malloc_device_byte(handle, &db_data_global_addr_device,
                                  sizeof(bm_fm_data_type_t) * db_size * feature_size);
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_malloc_device_byte error!");
            goto err1;
        }
        ret = bm_memcpy_s2d(handle, db_data_global_addr_device,
                            bm_mem_get_system_addr(db_data_global_addr));
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_memcpy_s2d error!");
            goto err2;
        }
    } else {
        db_data_global_addr_device = db_data_global_addr;
    }

    if (bm_mem_get_type(db_feature_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_malloc_device_byte(handle, &db_feature_global_addr_device,
                                    sizeof(bm_fm_data_type_t) * db_size);
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_malloc_device_byte error!");
            goto err2;
        }
        ret = bm_memcpy_s2d(handle, db_feature_global_addr_device,
                          bm_mem_get_system_addr(db_feature_global_addr));
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_memcpy_s2d error!");
            goto err3;
        }
    } else {
        db_feature_global_addr_device = db_feature_global_addr;
    }

    if (bm_mem_get_type(output_sorted_similarity_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_malloc_device_byte(handle, &output_sorted_similarity_global_addr_device,
                                    sizeof(bm_fm_data_type_t) * batch_size);
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_malloc_device_byte error!");
            goto err3;
        }
    } else {
        output_sorted_similarity_global_addr_device = output_sorted_similarity_global_addr;
    }

    if (bm_mem_get_type(output_sorted_index_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_malloc_device_byte(handle, &output_sorted_index_global_addr_device,
                                  sizeof(int) * batch_size);
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_malloc_device_byte error!");
            goto err4;
        }
    } else {
        output_sorted_index_global_addr_device = output_sorted_index_global_addr;
    }

    ret = bm_malloc_device_byte(handle, &db_data_trans_addr, feature_size * db_size * sizeof(float));
    if (ret != BM_SUCCESS) {
        bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_malloc_device_byte error!");
        goto err4;
    }

    ret = bm_malloc_device_byte(handle, &output_temp_addr, batch_size * db_size * sizeof(float));
    if (ret != BM_SUCCESS) {
        bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_malloc_device_byte error!");
        goto err4;
    }

    arg.db_data_trans_addr = bm_mem_get_device_addr(db_data_trans_addr);
    arg.output_temp_addr = bm_mem_get_device_addr(output_temp_addr);
    arg.input_data_global_addr = bm_mem_get_device_addr(input_data_global_addr_device);
    arg.db_data_global_addr = bm_mem_get_device_addr(db_data_global_addr_device);
    arg.db_feature_global_addr = bm_mem_get_device_addr(db_feature_global_addr_device);
    arg.output_sorted_similarity_global_addr = bm_mem_get_device_addr(output_sorted_similarity_global_addr_device);
    arg.output_sorted_index_global_addr = bm_mem_get_device_addr(output_sorted_index_global_addr_device);
    arg.feature_size = feature_size;
    arg.batch_size = batch_size;
    arg.db_size = db_size;

    switch(chipid) {
        case BM1686:
            ret = bm_tpu_kernel_launch(handle, "cv_feature_match_1684x", (u8*)(&arg),
                                                sizeof(arg), core_id);
            if (ret != BM_SUCCESS) {
                bmlib_log("FEATURE_MATCH", BMLIB_LOG_ERROR, "FEATURE_MATCH sync api error\n");
                goto err5;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    if (bm_mem_get_type(output_sorted_similarity_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(output_sorted_similarity_global_addr),
                            output_sorted_similarity_global_addr_device);
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_memcpy_d2s error!");
            goto err5;
        }
        bm_free_device(handle, output_sorted_similarity_global_addr_device);
    }

    if (bm_mem_get_type(output_sorted_index_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(output_sorted_index_global_addr),
                            output_sorted_index_global_addr_device);
        if (ret != BM_SUCCESS) {
            bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_memcpy_d2s error!");
            goto err5;
        }
        bm_free_device(handle, output_sorted_index_global_addr_device);
    }

    if (bm_mem_get_type(input_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, input_data_global_addr_device);
    }
    if (bm_mem_get_type(db_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, db_data_global_addr_device);
    }
    if (bm_mem_get_type(db_feature_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, db_feature_global_addr_device);
    }
    return ret;

err5:
    if (bm_mem_get_type(output_sorted_index_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, output_sorted_index_global_addr_device);
    }
    bm_free_device(handle, db_data_trans_addr);
    bm_free_device(handle, output_temp_addr);
err4:
    if (bm_mem_get_type(output_sorted_similarity_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, output_sorted_similarity_global_addr_device);
    }
err3:
    if (bm_mem_get_type(db_feature_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, db_feature_global_addr_device);
    }
err2:
    if (bm_mem_get_type(db_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, db_data_global_addr_device);
    }
err1:
    if (bm_mem_get_type(input_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, input_data_global_addr_device);
    }
err0:
    return BM_ERR_FAILURE;
}

bm_status_t bmcv_feature_match(bm_handle_t handle, bm_device_mem_t input_data_global_addr,
                            bm_device_mem_t db_data_global_addr,
                            bm_device_mem_t output_sorted_similarity_global_addr,
                            bm_device_mem_t output_sorted_index_global_addr,
                            int batch_size, int feature_size, int db_size,
                            int sort_cnt,int rshiftbits)
{
    if (sort_cnt > MAX_TOPK) {
        printf("sort_cnt is toot large\r\n");
        return BM_ERR_FAILURE;
    }
    if(rshiftbits < 0) {
        printf("rshiftbits is invalid\r\n");
        return BM_ERR_FAILURE;
    }
    if (batch_size > 10) {
        printf("batch_size should not be greater than 10, input is :%d\r\n", batch_size);
        return BM_ERR_FAILURE;
    }
    if (feature_size > 3000) {
        printf("feature_size should not be greater than 3000, input is :%d\r\n", feature_size);
        return BM_ERR_FAILURE;
    }
    if (db_size > 100000) {
        printf("db_size should not be greater than 100000, input is :%d\r\n", db_size);
        return BM_ERR_FAILURE;
    }

    bm_api_cv_feature_match_fix8b_1684x_t arg;
    bm_device_mem_t input_data_global_addr_device, db_data_global_addr_device;
    bm_device_mem_t output_sorted_similarity_global_addr_device;
    bm_device_mem_t output_sorted_index_global_addr_device;
    bm_device_mem_t arm_tmp_buffer;
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid;
    int core_id = 0;

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    if (((sizeof(short) + sizeof(int)) * batch_size * sort_cnt) >= 0x4000000) {
        printf("tmp buffer is not enough\r\n");
        return BM_ERR_FAILURE;
    }

    if (bm_mem_get_type(input_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_malloc_device_byte(handle, &input_data_global_addr_device,
                                  sizeof(bm_fm_data_type_fix8b_t) * batch_size * feature_size);
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte error\r\n");
            goto err0;
        }
        ret = bm_memcpy_s2d(handle, input_data_global_addr_device,
                            bm_mem_get_system_addr(input_data_global_addr));
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d error\r\n");
            goto err1;
        }
    } else {
        input_data_global_addr_device = input_data_global_addr;
    }

    if (bm_mem_get_type(db_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret =  bm_malloc_device_byte(handle, &db_data_global_addr_device,
                                    sizeof(bm_fm_data_type_fix8b_t) * db_size * feature_size);
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte error\r\n");
            goto err1;
        }
        ret = bm_memcpy_s2d(handle, db_data_global_addr_device, bm_mem_get_system_addr(db_data_global_addr));
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d error\r\n");
            goto err2;
        }
    } else {
        db_data_global_addr_device = db_data_global_addr;
    }

    if (bm_mem_get_type(output_sorted_similarity_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_malloc_device_byte(handle, &output_sorted_similarity_global_addr_device,
                                  sizeof(short) * batch_size * sort_cnt);
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte error\r\n");
            goto err2;
        }
    } else {
        output_sorted_similarity_global_addr_device = output_sorted_similarity_global_addr;
    }
    if (bm_mem_get_type(output_sorted_index_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_malloc_device_byte(handle, &output_sorted_index_global_addr_device,
                                  sizeof(int) * batch_size * sort_cnt);
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte error\r\n");
            goto err3;
        }
    } else {
        output_sorted_index_global_addr_device = output_sorted_index_global_addr;
    }

    ret = bm_malloc_device_byte(handle, &arm_tmp_buffer, batch_size * db_size * sizeof(float));
    if (ret != BM_SUCCESS) {
        bmlib_log("FEATURE MATCH", BMLIB_LOG_ERROR, "bm_malloc_device_byte error!");
        goto err4;
    }

    arg.input_data_global_addr = bm_mem_get_device_addr(input_data_global_addr_device);
    arg.db_data_global_addr = bm_mem_get_device_addr(db_data_global_addr_device);
    arg.output_similarity_global_addr = bm_mem_get_device_addr(output_sorted_similarity_global_addr_device);
    arg.output_sorted_index_global_addr = bm_mem_get_device_addr(output_sorted_index_global_addr_device);
    arg.feature_size = feature_size;
    arg.batch_size = batch_size;
    arg.db_size = db_size;
    arg.sort_cnt = sort_cnt;
    arg.arm_tmp_buffer = bm_mem_get_device_addr(arm_tmp_buffer);
    arg.rshiftbits = rshiftbits;

    switch(chipid) {
        case BM1686:
            ret = bm_tpu_kernel_launch(handle, "cv_feature_match_fix8b_1684x", (u8*)(&arg),
                                                sizeof(arg), core_id);
            if (ret != BM_SUCCESS) {
                bmlib_log("FEATURE_MATCH_FIX_8B", BMLIB_LOG_ERROR, "FEATURE_MATCH_FIX_8B sync api error\n");
                goto err5;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    bm_free_device(handle, arm_tmp_buffer);

    if (bm_mem_get_type(output_sorted_similarity_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(output_sorted_similarity_global_addr),
                            output_sorted_similarity_global_addr_device);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_d2s error\r\n");
            goto err4;
        }
        bm_free_device(handle, output_sorted_similarity_global_addr_device);
    }

    if (bm_mem_get_type(output_sorted_index_global_addr) == BM_MEM_TYPE_SYSTEM) {
        ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(output_sorted_index_global_addr),
                            output_sorted_index_global_addr_device);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_d2s error\r\n");
            goto err5;
        }
        bm_free_device(handle, output_sorted_index_global_addr_device);
    }

    if (bm_mem_get_type(input_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, input_data_global_addr_device);
    }
    if (bm_mem_get_type(db_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, db_data_global_addr_device);
    }

    return ret;

err5:
    bm_free_device(handle, arm_tmp_buffer);
err4:
    if (bm_mem_get_type(output_sorted_index_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, output_sorted_index_global_addr_device);
    }
err3:
    if (bm_mem_get_type(output_sorted_similarity_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, output_sorted_similarity_global_addr_device);
    }
err2:
    if (bm_mem_get_type(db_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, db_data_global_addr_device);
    }
err1:
    if (bm_mem_get_type(input_data_global_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, input_data_global_addr_device);
    }
err0:
    return BM_ERR_FAILURE;
}