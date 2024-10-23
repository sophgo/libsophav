#include <stdint.h>
#include <stdio.h>
#include "bmcv_common.h"
#include "bmcv_internal.h"

#define MAX_INPUT_NUM (4)
#define SET_DATA_FORMAT(img_color_type, data_size, channel)                    \
    do {                                                                       \
        switch (img_color_type) {                                              \
            case (UINT8_C1): {                                                 \
                channel   = 1;                                                 \
                data_size = 1;                                                 \
                break;                                                         \
            }                                                                  \
            case (UINT8_C3): {                                                 \
                channel   = 3;                                                 \
                data_size = 1;                                                 \
                break;                                                         \
            }                                                                  \
            case (INT8_C1): {                                                  \
                channel   = 1;                                                 \
                data_size = 1;                                                 \
                break;                                                         \
            }                                                                  \
            case (INT8_C3): {                                                  \
                channel   = 3;                                                 \
                data_size = 1;                                                 \
                break;                                                         \
            }                                                                  \
            case (FLOAT32_C1): {                                               \
                channel   = 1;                                                 \
                data_size = 4;                                                 \
                break;                                                         \
            }                                                                  \
            case (FLOAT32_C3): {                                               \
                channel   = 3;                                                 \
                data_size = 4;                                                 \
                break;                                                         \
            }                                                                  \
            default: {                                                         \
                channel   = 3;                                                 \
                data_size = 1;                                                 \
                break;                                                         \
            }                                                                  \
        }                                                                      \
    } while (0)

extern bm_status_t bm_vpss_convert_to(
    bm_handle_t          handle,
	int                  input_num,
	bmcv_convert_to_attr convert_to_attr,
	bm_image *           input,
	bm_image *           output);

static convert_storage_mode_e convert_to_type_translation(
    bm_image_data_format_ext in_format, bm_image_data_format_ext out_format) {
    convert_storage_mode_e convert_storage_mode;
    convert_storage_mode = CONVERT_1N_TO_1N;
    return convert_storage_mode;
}

static cv_color_e convert_to_color_translation(
    bm_image_data_format_ext data_format, int channel) {
    cv_color_e cv_color = UINT8_C3;
    if ((DATA_TYPE_EXT_1N_BYTE == data_format) &&
        (3 == channel)) {
        cv_color = UINT8_C3;
    } else if ((DATA_TYPE_EXT_1N_BYTE == data_format) &&
               (1 == channel)) {
        cv_color = UINT8_C1;
    } else if ((DATA_TYPE_EXT_1N_BYTE_SIGNED == data_format) &&
               (3 == channel)) {
        cv_color = INT8_C3;
    } else if ((DATA_TYPE_EXT_1N_BYTE_SIGNED == data_format) &&
               (1 == channel)) {
        cv_color = INT8_C1;
    } else if ((DATA_TYPE_EXT_FLOAT32 == data_format) && (3 == channel)) {
        cv_color = FLOAT32_C3;
    } else if ((DATA_TYPE_EXT_FLOAT32 == data_format) && (1 == channel)) {
        cv_color = FLOAT32_C1;
    }

    return cv_color;
}

bm_status_t bmcv_convert_to_internal(bm_handle_t          handle,
                                     bmcv_convert_to_attr convert_to_attr,
                                     int                  w_stride,
                                     bm_image_tensor      input,
                                     bm_image_tensor      output) {
    bm_device_mem_t input_img_addr;
    bm_device_mem_t output_img_addr;
    bm_status_t ret = BM_SUCCESS;
    int core_id = 0;
    int             img_w                = input.image.width;
    int             img_h                = input.image.height;
    int             convert_storage_mode = convert_to_type_translation(
        input.image.data_type, output.image.data_type);
    int image_num = input.image_n;
    int input_img_data_type =
        convert_to_color_translation(input.image.data_type, input.image_c);
    int output_img_data_type =
        convert_to_color_translation(output.image.data_type, output.image_c);
    bm_api_cv_convert_to_t arg;

    bm_image_tensor_get_device_mem(input, &input_img_addr);
    if (!bm_image_tensor_is_attached(output)) {
        if (BM_SUCCESS !=
            bm_image_tensor_alloc_dev_mem(output, BMCV_HEAP0_ID)) {
            return BM_ERR_FAILURE;
        }
    }
    bm_image_tensor_get_device_mem(output, &output_img_addr);
    int in_channel = 0, in_data_size = 0;
    int out_channel = 0, out_data_size = 0;
    int is_4N_mode = ((CONVERT_4N_TO_1N == convert_storage_mode) ||
                      (CONVERT_4N_TO_4N == convert_storage_mode))
                         ? (1)
                         : (0);

    SET_DATA_FORMAT(input_img_data_type, in_data_size, in_channel);
    SET_DATA_FORMAT(output_img_data_type, out_data_size, out_channel);
    image_num        = (1 == is_4N_mode) ? ((image_num + 3) / 4) : (image_num);
    int in_img_size  = in_channel * in_data_size * img_w * img_h * image_num;
    int out_img_size = out_channel * out_data_size * img_w * img_h * image_num;
    if (is_4N_mode) {
        in_img_size  = in_img_size * 4;
        out_img_size = out_img_size * 4;
    }

    arg.input_img_addr       = bm_mem_get_device_addr(input_img_addr);
    arg.img_w                = img_w;
    arg.img_w_stride         = w_stride;
    arg.img_h                = img_h;
    arg.input_img_data_type  = input_img_data_type;
    arg.output_img_data_type = output_img_data_type;
    arg.alpha_0              = convert_to_attr.alpha_0;
    arg.beta_0               = convert_to_attr.beta_0;
    arg.alpha_1              = convert_to_attr.alpha_1;
    arg.beta_1               = convert_to_attr.beta_1;
    arg.alpha_2              = convert_to_attr.alpha_2;
    arg.beta_2               = convert_to_attr.beta_2;
    arg.convert_storage_mode = convert_storage_mode;
    arg.image_num            = image_num;
    arg.output_img_addr      = bm_mem_get_device_addr(output_img_addr);
    unsigned int chipid = BM1688;
    bm_get_chipid(handle, &chipid);
    switch (chipid){
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_convert_to", (u8 *)&arg, sizeof(arg), core_id);
            if(ret != BM_SUCCESS){
                BMCV_ERR_LOG("convert_to sync api error\r\n");
                return ret;
            }
        break;
    default:
        printf("ChipID is NOT supported\n");
        break;
        return BM_NOT_SUPPORTED;
    }

    return ret;
}

static bm_status_t bm_convert_to_get_stride(bm_image input, int *w_stride) {
    int data_size = 1;
    switch (input.data_type) {
        case DATA_TYPE_EXT_FLOAT32:
            data_size = 4;
            break;
        default:
            data_size = 1;
            break;
    }

    bm_image_get_stride(input, w_stride);
    *w_stride = *w_stride / data_size;

    return BM_SUCCESS;
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
        int width_stride = 0;
        bm_convert_to_get_stride(output[i], &width_stride);
        if (output[i].width != width_stride) {
            BMCV_ERR_LOG("output width must be equal to stride\n");

            return BM_NOT_SUPPORTED;
        }
    }
    for (int i = 0; i < image_num - 1; i++) {
        if ((input[i].width != input[i + 1].width) ||
            (output[i].height != output[i + 1].height) ||
            (input[i].image_format != input[i + 1].image_format) ||
            (output[i].image_format != output[i + 1].image_format) ||
            (input[i].data_type != input[i + 1].data_type) ||
            (output[i].data_type != output[i + 1].data_type)) {
            BMCV_ERR_LOG("input attr must be same to output\n");

            return BM_NOT_SUPPORTED;
        }
    }
    if (!(((input[0].data_type == DATA_TYPE_EXT_1N_BYTE) &&
           (output[0].data_type == DATA_TYPE_EXT_FLOAT32)) ||
          ((input[0].data_type == DATA_TYPE_EXT_1N_BYTE) &&
           (output[0].data_type == DATA_TYPE_EXT_1N_BYTE)) ||
          ((input[0].data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED) &&
           (output[0].data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED)) ||
          ((input[0].data_type == DATA_TYPE_EXT_1N_BYTE) &&
           (output[0].data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED)) ||
          ((input[0].data_type == DATA_TYPE_EXT_FLOAT32) &&
           (output[0].data_type == DATA_TYPE_EXT_FLOAT32)))) {
        BMCV_ERR_LOG("data type not support\n");

        return BM_NOT_SUPPORTED;
    }
    if ((input[0].data_type == DATA_TYPE_EXT_FP16) ||
        (output[0].data_type == DATA_TYPE_EXT_FP16)||
        (input[0].data_type == DATA_TYPE_EXT_BF16) ||
        (output[0].data_type == DATA_TYPE_EXT_BF16)){
        BMCV_ERR_LOG("data type not support\n");

        return BM_NOT_SUPPORTED;
    }

    if (((input[0].image_format != FORMAT_BGR_PLANAR) &&
         (input[0].image_format != FORMAT_RGB_PLANAR) &&
         (input[0].image_format != FORMAT_GRAY)) ||
        (input[0].image_format != output[0].image_format)) {
        BMCV_ERR_LOG("image format not support\n");

        return BM_NOT_SUPPORTED;
    }

    return BM_SUCCESS;
}

bm_status_t bmcv_image_convert_to_(bm_handle_t          handle,
                                   int                  input_num,
                                   bmcv_convert_to_attr convert_to_attr,
                                   bm_image *           input,
                                   bm_image *           output) {
    if (BM_SUCCESS != bmcv_convert_to_check(
                          handle, input_num, convert_to_attr, input, output)) {
        BMCV_ERR_LOG("convert_to param error\r\n");

        return BM_ERR_FAILURE;
    }
    int             in_concat_status  = 0;
    int             out_concat_status = 0;
    bm_image_tensor in_tensor[MAX_INPUT_NUM];
    bm_image_tensor out_tensor[32];
    int             w_stride = 0;
    bm_convert_to_get_stride(input[0], &w_stride);
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
    if (BM_SUCCESS ==
        concat_images_to_tensor(handle, input_num, input, in_tensor)) {
        in_concat_status = 1;
    }
    if (BM_SUCCESS ==
        concat_images_to_tensor(handle, input_num, output, out_tensor)) {
        out_concat_status = 1;
    }
    if ((in_concat_status == 1) && (out_concat_status == 1)) {
        bmcv_convert_to_internal(
            handle, convert_to_attr, w_stride, in_tensor[0], out_tensor[0]);
        bm_image_tensor_destroy(in_tensor[0]);
        bm_image_tensor_destroy(out_tensor[0]);
    } else {
        if (1 == in_concat_status) {
            bm_image_tensor_destroy(in_tensor[0]);
        }
        if (1 == out_concat_status) {
            bm_image_tensor_destroy(out_tensor[0]);
        }
        for (int i = 0; i < input_num; i++) {
            concat_images_to_tensor(handle, 1, &input[i], &in_tensor[i]);
            concat_images_to_tensor(handle, 1, &output[i], &out_tensor[i]);
            bmcv_convert_to_internal(
                handle, convert_to_attr, w_stride, in_tensor[i], out_tensor[i]);
            bm_image_tensor_destroy(in_tensor[i]);
            bm_image_tensor_destroy(out_tensor[i]);
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
    int loop = (input_num + 3) / 4;
    int i = 0;

    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret)
      return ret;

    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            if(input->data_type == DATA_TYPE_EXT_FLOAT32 || input->data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED){
                for (i = 0; i < loop; i++) {
                    int num = (i == loop - 1) ? (input_num - (loop - 1) * 4) : 4;
                    ret = bmcv_image_convert_to_(handle, num, convert_to_attr, input + i * 4, output + i * 4);
                    if (ret != BM_SUCCESS) return ret;
                }
            } else{
                ret = bm_vpss_convert_to(handle, input_num, convert_to_attr, input, output);
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}
