#include "bmcv_common.h"
#include "bmcv_internal.h"

struct image_warpper {
    bm_image* inner;
    int image_num;
};

bm_status_t bmcv_image_transpose(bm_handle_t handle, bm_image input, bm_image output)
{
    bm_device_mem_t in_dev_mem[3], out_dev_mem[3];
    bm_status_t ret = BM_SUCCESS;
    struct image_warpper* input_ = (struct image_warpper*)malloc(sizeof(struct image_warpper));
    int i, p;
    int num = 1;
    int plane_num;
    int input_c;
    int type_len;
    bool in_need_convert = false;
    bm_api_cv_transpose_t api;
    unsigned int chipid;
    int core_id = 0;

    if (handle == NULL) {
        bmlib_log("TRANSPOSE", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_FAILURE;
    }
    if (input.image_format != FORMAT_RGB_PLANAR &&
        input.image_format != FORMAT_BGR_PLANAR &&
        input.image_format != FORMAT_GRAY) {
        bmlib_log("TRANSPOSE", BMLIB_LOG_ERROR, \
                "color_space of input only support RGB_PLANAR/BGR_PLANAR/GRAY!\r\n");
        return BM_NOT_SUPPORTED;
    }
    if (output.image_format != FORMAT_RGB_PLANAR &&
        output.image_format != FORMAT_BGR_PLANAR &&
        output.image_format != FORMAT_GRAY) {
        bmlib_log("TRANSPOSE", BMLIB_LOG_ERROR, \
                "color_space of output only support RGB_PLANAR/BGR_PLANAR/GRAY!\r\n");
        return BM_NOT_SUPPORTED;
    }
    if (input.image_format != output.image_format) {
        bmlib_log("TRANSPOSE", BMLIB_LOG_ERROR, \
                "input and output should be same image_format!\r\n");
        return BM_NOT_SUPPORTED;
    }
    if (input.data_type != output.data_type) {
        bmlib_log("TRANSPOSE", BMLIB_LOG_ERROR, \
                "input and output should be same data_type!\r\n");
        return BM_NOT_SUPPORTED;
    }
    if (input.width != output.height || input.height != output.width) {
        bmlib_log("TRANSPOSE", BMLIB_LOG_ERROR, \
                "input width should equal to output height and input \
                height should equal to output width!\r\n");
        return BM_NOT_SUPPORTED;
    }

    ret = bm_image_get_device_mem(output, out_dev_mem);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot get output device mem\n");
        return ret;
    }
    input_->image_num = num;
    input_->inner = (bm_image*)malloc(num * sizeof(bm_image));
    for (i = 0; i < num; ++i) {
        ret = bm_image_create(handle, input.height, input.width, input.image_format, \
                            input.data_type, input_->inner + i, NULL);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_INFO, "create intermediate bm image failed, \
                    the call may incorrect %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
            return ret;
        }
    }

    plane_num = bm_image_get_plane_num(input);
    for (p = 0; p < plane_num; p++) {
        if ((input.image_private->memory_layout[p].pitch_stride !=
             input_->inner[0].image_private->memory_layout[p].pitch_stride) ||
            (input.image_private->memory_layout[p].channel_stride !=
             input_->inner[0].image_private->memory_layout[p].channel_stride) ||
            (input.image_private->memory_layout[p].batch_stride !=
             input_->inner[0].image_private->memory_layout[p].batch_stride)) {
            in_need_convert = true;
            break;
        }
    }

    if (in_need_convert) {
        ret = bm_image_alloc_dev_mem(input_->inner[0], BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "alloc intermediate memory failed\n",
                    filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
        }
        for (p = 0; p < plane_num; ++p) {
            ret = update_memory_layout(handle, input.image_private->data[p], \
                                    input.image_private->memory_layout[p],
                                    input_->inner[0].image_private->data[p],
                                    input_->inner[0].image_private->memory_layout[p]);
        }
    } else {
        ret = bm_image_attach(input_->inner[0], input.image_private->data);
    }

    input_c = (input.image_format == FORMAT_GRAY) ? 1 : 3;
    type_len = (input.data_type == DATA_TYPE_EXT_1N_BYTE || \
                input.data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED) ? \
                INT8_SIZE : FLOAT_SIZE;

    ret = bm_image_get_device_mem(input_->inner[0], in_dev_mem);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot get input device mem\n");
        return ret;
    }
    api.input_global_mem_addr  = bm_mem_get_device_addr(in_dev_mem[0]);
    api.output_global_mem_addr = bm_mem_get_device_addr(out_dev_mem[0]);
    api.channel                = input_c;
    api.height                 = input.height;
    api.width                  = input.width;
    api.type_len               = type_len;

    ret = bm_get_chipid(handle, &chipid);
    if (ret != BM_SUCCESS) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    switch(chipid) {
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_transpose", (u8*)&api,
                                                sizeof(api), core_id);
            if (ret != BM_SUCCESS) {
                bmlib_log("TRANSPOSE", BMLIB_LOG_ERROR, "transpose sync api error\n");
                return ret;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    bm_image_destroy(&(input_->inner[0]));
    free(input_->inner);
    free(input_);
    return ret;
}