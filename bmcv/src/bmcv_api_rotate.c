#include "bmcv_common.h"
#include "bmcv_internal.h"

bm_status_t bmcv_rotate_check(
        bm_handle_t handle,
        bm_image    input,
        bm_image    output,
        int         rotation_angle) {
    if (handle == NULL) {
        bmlib_log("ROTATE", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    bm_image_format_ext      src_format = input.image_format;
    bm_image_data_format_ext src_type   = input.data_type;
    bm_image_format_ext      dst_format = output.image_format;
    bm_image_data_format_ext dst_type   = output.data_type;

    if (src_format != dst_format) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "input and output image format is NOT same\n");
        return BM_ERR_PARAM;
    }
    if (src_type != DATA_TYPE_EXT_1N_BYTE || dst_type != DATA_TYPE_EXT_1N_BYTE) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Not supported image data type\n");
        return BM_NOT_SUPPORTED;
    }
    if (input.width > 8192 || input.height > 8192) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Unsupported size : size_max = 8192 x 8192\n");
        return BM_NOT_SUPPORTED;
    }
    if (input.width < 8 || input.height < 8) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Unsupported size : size_min = 8 x 8\n");
        return BM_NOT_SUPPORTED;
    }
    if (rotation_angle != 90 && rotation_angle != 180 && rotation_angle != 270) {
        bmlib_log("ROTATE", BMLIB_LOG_ERROR, "Not supported rotation_angle\n");
        return BM_NOT_SUPPORTED;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_rotate_direct(
        bm_handle_t handle,
        bm_image    input,
        bm_image    output,
        int         rotation_angle) {
    bm_status_t ret    = BM_SUCCESS;
    unsigned    chipid = BM1688;

    ret = bmcv_rotate_check(handle, input, output, rotation_angle);
    if (BM_SUCCESS != ret) {
        return ret;
    }
    ret = bm_get_chipid(handle, &chipid);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "rotate get chipid error! \n");
        return ret;
    }

    bmcv_rot_mode rot_mode = BMCV_ROTATION_0;
    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            switch (rotation_angle) {
                case 90:
                    rot_mode = BMCV_ROTATION_90;
                    break;
                case 270:
                    rot_mode = BMCV_ROTATION_270;
                    break;
                default:
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                              "ERROR! Rotation_angle is not supported!\n");
                    return BM_NOT_SUPPORTED;
                    break;
            }
            ret = bmcv_ldc_rot(handle, input, output, rot_mode);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv ldc execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }
    return ret;
}

bm_status_t bmcv_image_rotate(
        bm_handle_t handle,
        bm_image    input,
        bm_image    output,
        int         rotation_angle) {
    bm_status_t ret = BM_SUCCESS;
    bm_image    temp_in, temp_out;
    ret = bmcv_rotate_check(handle, input, output, rotation_angle);
    if (BM_SUCCESS != ret) {
        return ret;
    }

    if (rotation_angle == 180) {
        ret = bmcv_image_flip(handle, input, output, ROTATE_180);
        goto exit;
    }
    switch (input.image_format) {
        case FORMAT_GRAY:
        case FORMAT_NV12:
        case FORMAT_NV21:
            ret = bmcv_image_rotate_direct(handle, input, output, rotation_angle);
            break;
        default:
            bm_image_create(handle, input.height, input.width, FORMAT_NV12, DATA_TYPE_EXT_1N_BYTE,
                            &temp_in, NULL);
            bm_image_create(handle, output.height, output.width, FORMAT_NV12, DATA_TYPE_EXT_1N_BYTE,
                            &temp_out, NULL);
            bm_image_alloc_dev_mem(temp_in, BMCV_HEAP1_ID);
            bm_image_alloc_dev_mem(temp_out, BMCV_HEAP1_ID);
            ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, input, &temp_in, CSC_MAX_ENUM, NULL,
                                                    BMCV_INTER_LINEAR, NULL); // input->NV12
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "input vpp_csc_matrix_convert error\n");
                bm_image_destroy(&temp_in);
                bm_image_destroy(&temp_out);
                return ret;
            }
            ret = bmcv_image_rotate_direct(handle, temp_in, temp_out, rotation_angle);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv_image_rotate_direct error\n");
                bm_image_destroy(&temp_in);
                bm_image_destroy(&temp_out);
                return ret;
            }
            ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, temp_out, &output, CSC_MAX_ENUM,
                                                    NULL, BMCV_INTER_LINEAR, NULL); // NV12->output
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "output vpp_csc_matrix_convert error\n");
                bm_image_destroy(&temp_in);
                bm_image_destroy(&temp_out);
                return ret;
            }
            bm_image_destroy(&temp_in);
            bm_image_destroy(&temp_out);
            break;
    }

exit:
    return ret;
}