#include <stdint.h>
#include <stdio.h>
#include "bmcv_common.h"
#include "bmcv_internal.h"

extern bm_status_t bm_vpss_convert_to(
    bm_handle_t          handle,
    int                  input_num,
    bmcv_convert_to_attr convert_to_attr,
    bm_image *           input,
    bm_image *           output);

bm_status_t bmcv_convert_to_internal(bm_handle_t          handle,
                                     bmcv_convert_to_attr convert_to_attr,
                                     bm_image             input,
                                     bm_image             output) {
    bm_device_mem_t input_mem[3];
    bm_image_get_device_mem(input, input_mem);
    bm_device_mem_t output_mem[3];
    bm_image_get_device_mem(output, output_mem);

    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_convert_to_t arg;
    int if_core0 = 1, if_core1 = 0;
    const char *tpu_env = NULL;

    int input_str[3];
    int output_str[3];
    bm_image_get_stride(input, input_str);
    bm_image_get_stride(output, output_str);
    arg.channel              = bm_image_get_plane_num(input);
    for (int i = 0; i < arg.channel; i++) {
        arg.input_img_addr[i] = bm_mem_get_device_addr(input_mem[i]);
        arg.output_img_addr[i] = bm_mem_get_device_addr(output_mem[i]);
        arg.img_w[i] = input.image_private->memory_layout[i].W;
        arg.img_h[i] = input.image_private->memory_layout[i].H;
        arg.img_in_stride[i] = input_str[i];
        arg.img_out_stride[i] = output_str[i];
    }
    arg.input_img_data_type  = input.data_type;
    arg.output_img_data_type = output.data_type;
    arg.alpha_0              = convert_to_attr.alpha_0;
    arg.beta_0               = convert_to_attr.beta_0;
    arg.alpha_1              = convert_to_attr.alpha_1;
    arg.beta_1               = convert_to_attr.beta_1;
    arg.alpha_2              = convert_to_attr.alpha_2;
    arg.beta_2               = convert_to_attr.beta_2;
    arg.is_packed            = 0;
    if (input.image_format == FORMAT_RGB_PLANAR ||
        input.image_format == FORMAT_BGR_PLANAR) {
        arg.channel = 3;
        for (int i = 1; i < arg.channel; i++) {
            arg.img_in_stride[i] = input_str[0];
            arg.img_out_stride[i] = output_str[0];
            arg.input_img_addr[i] = arg.input_img_addr[i-1] + arg.img_in_stride[0] * arg.img_h[0];
            arg.output_img_addr[i] = arg.output_img_addr[i-1] + arg.img_out_stride[0] * arg.img_h[0];
            arg.img_w[i] = arg.img_w[0];
            arg.img_h[i] = arg.img_h[0];
        }
    }

    if (input.image_format == FORMAT_BGR_PACKED ||
        input.image_format == FORMAT_RGB_PACKED ||
        input.image_format == FORMAT_YUV444_PACKED ||
        input.image_format == FORMAT_YVU444_PACKED) {
        arg.channel = 3;
        arg.is_packed = 1;
        arg.img_h[1] = arg.img_h[0];
        arg.img_h[2] = arg.img_h[0];
    }

    unsigned int chipid = BM1688;
    bm_get_chipid(handle, &chipid);
    switch (chipid){
        case BM1688_PREV:
        case BM1688:
        tpu_env = getenv("TPU_CORES");
        if (tpu_env) {
            if (strcmp(tpu_env, "0") == 0) {
            } else if (strcmp(tpu_env, "1") == 0) {
                if_core0 = 0;
                if_core1 = 1;
            } else if (strcmp(tpu_env, "2") == 0 || strcmp(tpu_env, "both") == 0) {
                if_core1 = 1;
            } else {
                fprintf(stderr, "Invalid TPU_CORES value: %s\n", tpu_env);
                fprintf(stderr, "Available options: 0, 1, 2/both\n");
                exit(EXIT_FAILURE);
            }
        }

        if (if_core0 && if_core1) {
            int core_list[BM1688_MAX_CORES];
            int core_nums = 0;
            int base_msg_id = 0;

            bm_api_cv_convert_to_dual_core_t dual_arg;
            bm_api_cv_convert_to_dual_core_t dual_params[BM1688_MAX_CORES];
            tpu_launch_param_t tpu_params[BM1688_MAX_CORES];

            if (if_core0) core_list[core_nums++] = 0;
            if (if_core1) core_list[core_nums++] = 1;

            for (int n = 0; n < core_nums; n++)
                base_msg_id |= 1 << core_list[n];

            dual_arg.core_num = core_nums;
            dual_arg.base_msg_id = base_msg_id;

            dual_arg.channel = arg.channel;
            for (int i = 0; i < arg.channel; i++) {
                dual_arg.input_img_addr[i] = arg.input_img_addr[i];
                dual_arg.output_img_addr[i] = arg.output_img_addr[i];
                dual_arg.img_w[i] = arg.img_w[i];
                dual_arg.img_h[i] = arg.img_h[i];
                dual_arg.img_in_stride[i] = arg.img_in_stride[i];
                dual_arg.img_out_stride[i] = arg.img_out_stride[i];
            }

            dual_arg.input_img_data_type  = arg.input_img_data_type;
            dual_arg.output_img_data_type = arg.output_img_data_type;
            dual_arg.alpha_0              = arg.alpha_0;
            dual_arg.beta_0               = arg.beta_0;
            dual_arg.alpha_1              = arg.alpha_1;
            dual_arg.beta_1               = arg.beta_1;
            dual_arg.alpha_2              = arg.alpha_2;
            dual_arg.beta_2               = arg.beta_2;
            dual_arg.is_packed            = arg.is_packed;

            for (int n = 0; n < core_nums; n++) {
                dual_arg.core_id = n;
                dual_params[n] = dual_arg;

                tpu_params[n].core_id = n;
                tpu_params[n].param_data = &dual_params[n];
                tpu_params[n].param_size = sizeof(bm_api_cv_convert_to_dual_core_t);
            }

            ret = bm_tpu_kernel_launch_dual_core(handle, "cv_convert_to_dualcore", tpu_params, core_list, core_nums);
            if(ret != BM_SUCCESS){
                BMCV_ERR_LOG("convert_to sync api error\r\n");
                return ret;
            }
        } else {
            int core_id = if_core1 == 1 ? 1 : 0;
                ret = bm_tpu_kernel_launch(handle, "cv_convert_to_bm1688", (u8 *)&arg, sizeof(arg), core_id);
            if(ret != BM_SUCCESS){
                BMCV_ERR_LOG("convert_to sync api error\r\n");
                return ret;
            }
        }
        break;
    default:
        printf("ChipID is NOT supported\n");
        break;
        return BM_NOT_SUPPORTED;
    }

    return ret;
}

static bm_status_t bmcv_convert_to_check(bm_handle_t          handle,
                                         int                  image_num,
                                         bmcv_convert_to_attr convert_to_attr,
                                         bm_image *           input,
                                         bm_image *           output) {
    UNUSED(convert_to_attr);
    if (handle == NULL) {
        bmlib_log("CONVERT TO", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_FAILURE;
    }
    if (image_num == 0) {
        BMCV_ERR_LOG("input image num not support:%d\n", image_num);

        return BM_NOT_SUPPORTED;
    }
    for (int i = 0; i < image_num; i++) {
        if ((input[i].width != output[i].width) ||
            (input[i].height != output[i].height)) {
            BMCV_ERR_LOG("input size must be same to output\n");

            return BM_NOT_SUPPORTED;
        }
    }
    for (int i = 0; i < image_num - 1; i++) {
        if ((input[i].width != input[i + 1].width) ||
            (output[i].height != output[i + 1].height) ||
            (input[i].image_format != input[i + 1].image_format) ||
            (output[i].image_format != output[i + 1].image_format) ||
            (input[i].data_type != input[i + 1].data_type) ||
            (output[i].data_type != output[i + 1].data_type) ||
            (input[i].image_format != output[i].image_format)) {
            BMCV_ERR_LOG("input attr must be same to output\n");

            return BM_NOT_SUPPORTED;
        }
    }

    for (int i = 0; i < image_num - 1; i++) {
        if ((input[i].image_format != FORMAT_RGB_PLANAR) &&
            (input[i].image_format != FORMAT_RGB_PACKED) &&
            (input[i].image_format != FORMAT_BGR_PLANAR) &&
            (input[i].image_format != FORMAT_BGR_PACKED) &&
            (input[i].image_format != FORMAT_GRAY) &&
            (input[i].image_format != FORMAT_YUV420P)){
            BMCV_ERR_LOG("image format not support\r\n");

            return BM_NOT_SUPPORTED;
        }
    }
    if ((input[0].data_type == DATA_TYPE_EXT_FP16) ||
        (output[0].data_type == DATA_TYPE_EXT_FP16)||
        (input[0].data_type == DATA_TYPE_EXT_BF16) ||
        (output[0].data_type == DATA_TYPE_EXT_BF16)){
        BMCV_ERR_LOG("data type not support\n");

        return BM_NOT_SUPPORTED;
    }

    for (int i = 0; i < image_num; i++) {
        if    (input[i].image_format != output[i].image_format) {
            BMCV_ERR_LOG("image format not support\n");
            return BM_NOT_SUPPORTED;
        }
    }

    return BM_SUCCESS;
}

bm_status_t bmcv_image_convert_to(
    bm_handle_t          handle,
    int                  input_num,
    bmcv_convert_to_attr convert_to_attr,
    bm_image *           input,
    bm_image *           output)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;

    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
      return ret;

    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            if(input->data_type == DATA_TYPE_EXT_1N_BYTE && (input->image_format == FORMAT_RGB_PLANAR || input->image_format == FORMAT_BGR_PLANAR /* || input->image_format == FORMAT_GRAY */)) {
                ret = bm_vpss_convert_to(handle, input_num, convert_to_attr, input, output);
            } else{
                if (BM_SUCCESS != bmcv_convert_to_check(handle, input_num, convert_to_attr, input, output)) {
                    BMCV_ERR_LOG("convert_to param error\r\n");
                    return BM_ERR_FAILURE;
                }

                for (int output_idx = 0;
                    output_idx < input_num;
                    output_idx++) {
                    if (!bm_image_is_attached(output[output_idx])) {
                        if (BM_SUCCESS !=
                            bm_image_alloc_dev_mem(output[output_idx], BMCV_HEAP1_ID)) {
                            for (int free_idx = 0; free_idx < output_idx; free_idx++) {
                                bm_device_mem_t dmem;
                                bm_image_get_device_mem(output[free_idx], &dmem);
                                bm_free_device(handle, dmem);
                            }
                            return BM_ERR_FAILURE;
                        }
                    }
                }
                for (int i = 0; i < input_num; i++) {
                    bmcv_convert_to_internal(handle, convert_to_attr, input[i], output[i]);
                }
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}