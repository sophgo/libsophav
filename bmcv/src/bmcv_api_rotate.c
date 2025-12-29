#ifndef BM_PCIE_MODE
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
    if (input.width > 4608 || input.height > 4608) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Unsupported size : size_max = 4608 x 4608\n");
        return BM_NOT_SUPPORTED;
    }
    if (input.width < 64 || input.height < 64) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Unsupported size : size_min = 64 x 64\n");
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

bm_status_t bmcv_image_rotate(bm_handle_t handle, bm_image input, bm_image output, int rotation_angle)
{
    bm_status_t ret = BM_SUCCESS;
    bm_image temp_in, temp_out;
    bm_image pad_img, pad_img_out;
    bool if_pad_w = false;
    bool if_pad_h = false;
    int pad_width, pad_height = 0;
    int pad_width_out, pad_height_out = 0;
    bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;

    ret = bmcv_rotate_check(handle, input, output, rotation_angle);
    if (ret != BM_SUCCESS) {
        printf("bmcv_rotate_check failed!\n");
        return ret;
    }

    if (rotation_angle == 180) {
        ret = bmcv_image_flip(handle, input, output, ROTATE_180);
        if (ret != BM_SUCCESS) {
            printf("bmcv_rotate_check failed!\n");
        }
        return ret;
    }

    if (input.height % 64 != 0) {
        if_pad_h = true;
        pad_height = ALIGN(input.height, 64);
    } else {
        pad_height = input.height;
    }

    if (input.width % 64 != 0) {
        if_pad_w = true;
        pad_width = ALIGN(input.width, 64);
    } else {
        pad_width = input.width;
    }

    if (output.height % 64 != 0) {
        pad_height_out = ALIGN(output.height, 64);
    } else {
        pad_height_out = output.height;
    }

    if (output.width % 64 != 0) {
        pad_width_out = ALIGN(output.width, 64);
    } else {
        pad_width_out = output.width;
    }

    if (if_pad_h || if_pad_w) {
        bm_image_create(handle, pad_height, pad_width, input.image_format, DATA_TYPE_EXT_1N_BYTE, &pad_img, NULL);
        bm_image_create(handle, pad_height_out, pad_width_out, output.image_format, DATA_TYPE_EXT_1N_BYTE, &pad_img_out, NULL);
        bm_image_alloc_dev_mem(pad_img, BMCV_HEAP1_ID);
        bm_image_alloc_dev_mem(pad_img_out, BMCV_HEAP1_ID);
        bmcv_padding_attr_t padding_rect = {.dst_crop_stx = 0, .dst_crop_sty = 0, .dst_crop_w = input.width,
                                            .dst_crop_h = input.height, .padding_r = 0, .padding_g = 0,
                                            .padding_b = 0, .if_memset = 1};
        ret = bmcv_image_vpp_convert_padding(handle, 1, input, &pad_img, &padding_rect, NULL, algorithm);
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_vpp_convert_padding failed!\n");
            goto exit;
        }
    }

    switch (input.image_format) {
        case FORMAT_GRAY:
        case FORMAT_NV12:
        case FORMAT_NV21:
            if (if_pad_h || if_pad_w) {
                ret = bmcv_image_rotate_direct(handle, pad_img, pad_img_out, rotation_angle);
                if (ret != BM_SUCCESS) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv_image_rotate_direct error\n");
                    goto exit;
                }
                break;
            } else {
                ret = bmcv_image_rotate_direct(handle, input, output, rotation_angle);
                if (ret != BM_SUCCESS) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv_image_rotate_direct error\n");
                    goto exit;
                }
                break;
            }
        default:
            if (if_pad_h || if_pad_w) {
                bm_image_create(handle, pad_img.height, pad_img.width, FORMAT_NV12, DATA_TYPE_EXT_1N_BYTE,
                                &temp_in, NULL);
                bm_image_create(handle, pad_img_out.height, pad_img_out.width, FORMAT_NV12, DATA_TYPE_EXT_1N_BYTE,
                                &temp_out, NULL);
                bm_image_alloc_dev_mem(temp_in, BMCV_HEAP1_ID);
                bm_image_alloc_dev_mem(temp_out, BMCV_HEAP1_ID);
                ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, pad_img, &temp_in, CSC_MAX_ENUM, NULL,
                                                    BMCV_INTER_LINEAR, NULL); // input->NV12
                if (ret != BM_SUCCESS) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "input vpp_csc_matrix_convert error\n");
                    bm_image_destroy(&temp_in);
                    bm_image_destroy(&temp_out);
                    goto exit;
                }
            } else {
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
                    goto exit;
                }
            }

            ret = bmcv_image_rotate_direct(handle, temp_in, temp_out, rotation_angle);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv_image_rotate_direct error\n");
                bm_image_destroy(&temp_in);
                bm_image_destroy(&temp_out);
                goto exit;
            }
            if (if_pad_h || if_pad_w) {
                ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, temp_out, &pad_img_out, CSC_MAX_ENUM,
                                                        NULL, BMCV_INTER_LINEAR, NULL); // NV12->output
                if (ret != BM_SUCCESS) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "output vpp_csc_matrix_convert error\n");
                    bm_image_destroy(&temp_in);
                    bm_image_destroy(&temp_out);
                    goto exit;
                }
            } else {
                ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, temp_out, &output, CSC_MAX_ENUM,
                                                        NULL, BMCV_INTER_LINEAR, NULL); // NV12->output
                if (ret != BM_SUCCESS) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "output vpp_csc_matrix_convert error\n");
                    bm_image_destroy(&temp_in);
                    bm_image_destroy(&temp_out);
                    goto exit;
                }
            }
            bm_image_destroy(&temp_in);
            bm_image_destroy(&temp_out);
            break;
    }

    if (if_pad_h || if_pad_w) {
        if (rotation_angle == 90) {
            bmcv_rect_t crop_rect = {.start_x = pad_img_out.width - output.width, .start_y = 0, .crop_w = output.width, .crop_h = output.height};
            ret = bmcv_image_vpp_convert(handle, 1, pad_img_out, &output, &crop_rect, algorithm);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv_image_vpp_convert error\n");
                goto exit;
            }
        } else if (rotation_angle == 270) {
            bmcv_rect_t crop_rect = {.start_x = 0, .start_y = pad_img_out.height - output.height, .crop_w = output.width, .crop_h = output.height};
            ret = bmcv_image_vpp_convert(handle, 1, pad_img_out, &output, &crop_rect, algorithm);
            if (ret != BM_SUCCESS) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv_image_vpp_convert error\n");
                goto exit;
            }
        } else {
                printf("Rotation_angle is not supported!");
                goto exit;
        }
    }

exit:
    if (if_pad_h || if_pad_w) {
        bm_image_destroy(&pad_img);
        bm_image_destroy(&pad_img_out);
    }
    return ret;
}
#endif