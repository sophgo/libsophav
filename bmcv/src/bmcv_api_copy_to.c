#include <stdint.h>
#include <stdio.h>
#include "bmcv_common.h"
#include "bmcv_internal.h"

typedef enum { COPY_TO_GRAY = 0, COPY_TO_BGR, COPY_TO_RGB } padding_corlor_e;
typedef enum { PLANNER = 0, PACKED } padding_format_e;

bm_status_t bmcv_image_copy_to_vpss(
    bm_handle_t         handle,
    bmcv_copy_to_atrr_t copy_to_attr,
    bm_image            input,
    bm_image            output);

bm_status_t bmcv_copy_to_check(bmcv_copy_to_atrr_t copy_to_attr,
                               bm_image            input,
                               bm_image            output,
                               int                 data_size) {
    if ((input.data_type != output.data_type) ||
        (input.image_format != output.image_format)) {
        printf("[CopyTo] input data_type and image_format must be same to output!\r\n");

        return BM_NOT_SUPPORTED;
    }
    // image format check
    if ((input.image_format != FORMAT_RGB_PLANAR) &&
        (input.image_format != FORMAT_RGB_PACKED) &&
        (input.image_format != FORMAT_BGR_PLANAR) &&
        (input.image_format != FORMAT_BGR_PACKED) &&
        (input.image_format != FORMAT_GRAY)) {
        printf("[CopyTo] image format not support\r\n");

        return BM_NOT_SUPPORTED;
    }

    if ((input.data_type == DATA_TYPE_EXT_FP16) ||
        (output.data_type == DATA_TYPE_EXT_FP16)||
        (input.data_type == DATA_TYPE_EXT_BF16) ||
        (output.data_type == DATA_TYPE_EXT_BF16)){
        printf("data type not support\n");

        return BM_NOT_SUPPORTED;
    }

    // shape check
    if (input.width > output.width) {
        printf("[CopyTo] input.with should be less than or equal to output's\r\n");

        return BM_NOT_SUPPORTED;
    }
    if (input.height > output.height) {
        printf("[CopyTo] input.height should be less than or equal to output's\r\n");

        return BM_NOT_SUPPORTED;
    }
    int in_image_stride[3] = {0};
    bm_image_get_stride(input, in_image_stride);
    int out_image_stride[3] = {0};
    bm_image_get_stride(output, out_image_stride);
    // compare by bytes
    if (((copy_to_attr.start_x + input.width) * data_size) >
        out_image_stride[0]) {
        printf("[CopyTo] width exceeds range\r\n");

        return BM_NOT_SUPPORTED;
    }
    // compare by elems
    if ((copy_to_attr.start_y + input.height) > output.height) {
        printf("[CopyTo] height exceeds range\r\n");

        return BM_NOT_SUPPORTED;
    }

    return BM_SUCCESS;
}

bm_status_t bmcv_image_copy_to_(bm_handle_t         handle,
                               bmcv_copy_to_atrr_t copy_to_attr,
                               bm_image            input,
                               bm_image            output) {
    if (handle == NULL) {
        printf("[CopyTo] Can not get handle!\r\n");

        return BM_ERR_FAILURE;
    }
    int data_size         = 1;
    int data_type         = STORAGE_MODE_1N_INT8;
    int bgr_or_rgb        = COPY_TO_BGR;
    int channel           = 3;
    int planner_or_packed = PLANNER;
    unsigned int chipid = BM1688;
    bm_status_t ret = BM_SUCCESS;

    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret){
        printf("get chipid failed\n");
        return ret;
    }

    switch (input.image_format) {
        case FORMAT_RGB_PLANAR: {
            bgr_or_rgb        = COPY_TO_RGB;
            planner_or_packed = PLANNER;
            channel           = 3;
            break;
        }
        case FORMAT_RGB_PACKED: {
            bgr_or_rgb        = COPY_TO_RGB;
            planner_or_packed = PACKED;
            channel           = 3;
            break;
        }
        case FORMAT_BGR_PLANAR: {
            bgr_or_rgb        = COPY_TO_BGR;
            planner_or_packed = PLANNER;
            channel           = 3;
            break;
        }
        case FORMAT_BGR_PACKED: {
            bgr_or_rgb        = COPY_TO_BGR;
            planner_or_packed = PACKED;
            channel           = 3;
            break;
        }
        case FORMAT_GRAY: {
            bgr_or_rgb        = COPY_TO_GRAY;
            planner_or_packed = PLANNER;
            channel           = 1;
            break;
        }
        default: {
            bgr_or_rgb        = COPY_TO_BGR;
            planner_or_packed = PLANNER;
            channel           = 3;
            break;
        }
    }
    switch (input.data_type) {
        case DATA_TYPE_EXT_FLOAT32: {
            data_size = 4;
            data_type = STORAGE_MODE_1N_FP32;
            break;
        }
        default: {
            data_size = 1;
            data_type = STORAGE_MODE_1N_INT8;
            break;
        }
    }
    int elem_byte_stride =
        (planner_or_packed == PACKED) ? (data_size * 3) : (data_size);
    if (BM_SUCCESS !=
        bmcv_copy_to_check(copy_to_attr, input, output, elem_byte_stride)) {
        printf("[CopyTo] bmcv_copy_to_check error!\r\n");

        return BM_ERR_FAILURE;
    }
    if (!bm_image_is_attached(output)) {
        if (BM_SUCCESS != bm_image_alloc_dev_mem(output, BMCV_HEAP1_ID)) {
            printf("[CopyTo] bm_image_alloc_dev_mem error!\r\n");

            return BM_ERR_FAILURE;
        }
    }
    bm_device_mem_t in_dev_mem, out_dev_mem;
    bm_image_get_device_mem(input, &in_dev_mem);
    unsigned long long in_dev_addr        = bm_mem_get_device_addr(in_dev_mem);
    int in_image_stride[3] = {0};
    bm_image_get_stride(input, in_image_stride);
    int out_image_stride[3] = {0};
    bm_image_get_stride(output, out_image_stride);
    bm_image_get_device_mem(output, &out_dev_mem);
    unsigned long long padding_dev_addr = bm_mem_get_device_addr(out_dev_mem);
    unsigned long long out_dev_addr     = padding_dev_addr +
                       out_image_stride[0] * copy_to_attr.start_y +
                       copy_to_attr.start_x * elem_byte_stride;
    bm_api_cv_copy_to_t arg;
    arg.input_image_addr   = in_dev_addr;
    arg.padding_image_addr = padding_dev_addr;
    arg.output_image_addr  = out_dev_addr;
    arg.C                  = channel;
    arg.input_w_stride     = in_image_stride[0] / data_size;
    arg.input_w            = input.width;
    arg.input_h            = input.height;
    arg.padding_w_stride   = out_image_stride[0] / data_size;
    arg.padding_w          = output.width;
    arg.padding_h          = output.height;
    arg.data_type          = data_type;
    arg.bgr_or_rgb         = bgr_or_rgb;
    arg.planner_or_packed  = planner_or_packed;
    arg.padding_b          = (int)copy_to_attr.padding_b;
    arg.padding_r          = (int)copy_to_attr.padding_r;
    arg.padding_g          = (int)copy_to_attr.padding_g;
    arg.if_padding         = copy_to_attr.if_padding;

    int core_id = 0;
    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            if(BM_SUCCESS != bm_tpu_kernel_launch(handle, "sg_cv_copy_to", (u8 *)&arg, sizeof(arg), core_id)){
                    printf("copy_to launch api error\r\n");
                    return BM_ERR_FAILURE;
                }
                break;

        default:
            printf("BM_NOT_SUPPORTED!\n");
            return BM_NOT_SUPPORTED;
            break;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_copy_to(
    bm_handle_t         handle,
    bmcv_copy_to_atrr_t copy_to_attr,
    bm_image            input,
    bm_image            output)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int data_type = input.data_type;

    if((data_type == DATA_TYPE_EXT_FLOAT32) || (data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED)){
        ret = bmcv_image_copy_to_(handle, copy_to_attr, input, output);
    }else if(data_type == DATA_TYPE_EXT_1N_BYTE){
        ret = bmcv_image_copy_to_vpss(handle, copy_to_attr, input, output);
    }else{
        printf("BM_NOT_SUPPORTED datatype(%d)!\n", (int)data_type);
        ret = BM_NOT_SUPPORTED;
    }

    return ret;
}
