#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"
#include "bmcv_common.h"
#include <string.h>
#define KERNEL_SIZE 3 * 3 * 3 * 4 * 32

static bm_status_t bmcv_bayer2rgb_check(
    bm_handle_t handle,
    unsigned char* convd_kernel,
    bm_image input,
    bm_image output) {
    if (handle == NULL) {
        BMCV_ERR_LOG("bayer2rgb can not get handle!\n");
        return BM_ERR_FAILURE;
    }
    bm_image_format_ext src_format = input.image_format;
    bm_image_format_ext dst_format = output.image_format;
    bm_image_data_format_ext src_data_type = input.data_type;
    bm_image_data_format_ext dst_type = output.data_type;

    if (convd_kernel == NULL) {
        BMCV_ERR_LOG("The convd_kernel is nullptr !\n");
        return BM_ERR_FAILURE;
    }

    if (src_format != FORMAT_BAYER && src_format != FORMAT_BAYER_RG8) {
        BMCV_ERR_LOG("src_img format not supported !\n");
        return BM_NOT_SUPPORTED;
    }

    if (dst_format != FORMAT_RGB_PLANAR) {
        BMCV_ERR_LOG("dst_img format not supported !\n");
        return BM_NOT_SUPPORTED;
    }
    if (src_data_type != DATA_TYPE_EXT_1N_BYTE || dst_type != DATA_TYPE_EXT_1N_BYTE) {
        BMCV_ERR_LOG("src_type or dst_type not support\n");
        return BM_NOT_SUPPORTED;
    }

    if ((input.height != output.height) || (input.width != output.width)) {
        BMCV_ERR_LOG("The width and height of the input and output should be same !\n");
        return BM_ERR_FAILURE;
    }

    if ((input.height % 2 != 0) || (input.width % 2 != 0)) {
        BMCV_ERR_LOG("The width and height of the image need to be divisible by 2 !\n");
        return BM_ERR_FAILURE;
    }
    if (input.height > 8192 || input.width > 8192) {
        BMCV_ERR_LOG("The width and height of the image should not be greater than 8192!\n");
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_bayer2rgb(
        bm_handle_t handle,
        unsigned char* convd_kernel,
        bm_image input,
        bm_image output) {
    unsigned int chipid;
    int core_id = 0;
    bm_status_t ret = BM_SUCCESS;
    if(BM_SUCCESS !=bmcv_bayer2rgb_check(handle, convd_kernel, input, output)) {
        BMCV_ERR_LOG("bayer2rgb_check error\r\n");
        return BM_ERR_FAILURE;
    }

    if (!bm_image_is_attached(output)) {
        ret = sg_image_alloc_dev_mem(output, BMCV_HEAP1_ID);
        if (ret != BM_SUCCESS) {
            return ret;
        }
    }

    bm_device_mem_t input_mem;
    bm_device_mem_t output_mem;
    bm_image_get_device_mem(input, &input_mem);
    bm_image_get_device_mem(output, &output_mem);

    sg_device_mem_st input_dev_buffer_padding_ul;
    sg_device_mem_st input_dev_buffer_padding_br;
    sg_device_mem_st sys_addr_ul;
    sg_device_mem_st sys_addr_br;
    sg_device_mem_st convd_kernel_data;
    sg_device_mem_st sys_addr_temp_g;
    sg_device_mem_st sys_addr_temp_b;

    bm_api_cv_bayer2rgb_t param;
    param.height = input.height;
    param.width  = input.width;
    param.dst_fmt     = output.image_format;
    if(input.image_format == FORMAT_BAYER) {
        param.src_type = 0;
    } else {
        param.src_type = 1;
    }
    param.input_addr  = bm_mem_get_device_addr(input_mem);
    param.output_addr = bm_mem_get_device_addr(output_mem);

    ret = sg_malloc_device_mem(handle, &sys_addr_ul, input.height * input.width * 4);
    if (BM_SUCCESS != ret) {
        bm_image_destroy(&output);
        goto FREEMEM;
    }
    ret = sg_malloc_device_mem(handle, &sys_addr_br, input.height * input.width * 4);
    if (BM_SUCCESS != ret) {
        bm_image_destroy(&output);
        goto FREEMEM;
    }
    ret = sg_malloc_device_mem(handle, &sys_addr_temp_g, input.width * input.height * 4);
    if (BM_SUCCESS != ret) {
        bm_image_destroy(&output);
        goto FREEMEM;
    }
    ret = sg_malloc_device_mem(handle, &sys_addr_temp_b, input.width * input.height * 4);
    if (BM_SUCCESS != ret) {
        bm_image_destroy(&output);
        goto FREEMEM;
    }

    param.sys_mem_addr_temp_g = bm_mem_get_device_addr(sys_addr_temp_g.bm_device_mem);
    param.sys_mem_addr_temp_b = bm_mem_get_device_addr(sys_addr_temp_b.bm_device_mem);
    param.sys_mem_addr_temp_ul = bm_mem_get_device_addr(sys_addr_ul.bm_device_mem);
    param.sys_mem_addr_temp_br = bm_mem_get_device_addr(sys_addr_br.bm_device_mem);

    ret = sg_malloc_device_mem(handle, &convd_kernel_data, KERNEL_SIZE * sizeof(unsigned char));
    if (BM_SUCCESS != ret) {
        bm_image_destroy(&output);
        goto FREEMEM;
    }
    bm_memcpy_s2d(handle, convd_kernel_data.bm_device_mem, convd_kernel);
    param.kernel_addr = bm_mem_get_device_addr(convd_kernel_data.bm_device_mem);

    ret = sg_malloc_device_mem(handle, &input_dev_buffer_padding_ul, (input.width + 2) * (input.height + 2) * sizeof(char));
    if (BM_SUCCESS != ret) {
        bm_image_destroy(&output);
        goto FREEMEM;
    }
    ret = sg_malloc_device_mem(handle, &input_dev_buffer_padding_br, (input.width + 2) * (input.height + 2) * sizeof(char));
    if (BM_SUCCESS != ret) {
        bm_image_destroy(&output);
        goto FREEMEM;
    }
    param.input_addr_padding_up_left = bm_mem_get_device_addr(input_dev_buffer_padding_ul.bm_device_mem);
    param.input_addr_padding_bottom_right = bm_mem_get_device_addr(input_dev_buffer_padding_br.bm_device_mem);


    bm_get_chipid(handle, &chipid);

    switch(chipid)
    {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_bayer2rgb", (u8 *)&param, sizeof(param), core_id);
            if (BM_SUCCESS != ret) {
                bmlib_log("BAYER2RGB", BMLIB_LOG_ERROR, "bayer2rgb sync api error\n");
                bm_image_destroy(&output);
                goto FREEMEM;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }

FREEMEM:
    sg_free_device_mem(handle, convd_kernel_data);
    sg_free_device_mem(handle, input_dev_buffer_padding_ul);
    sg_free_device_mem(handle, input_dev_buffer_padding_br);
    sg_free_device_mem(handle, sys_addr_temp_g);
    sg_free_device_mem(handle, sys_addr_temp_b);
    sg_free_device_mem(handle, sys_addr_ul);
    sg_free_device_mem(handle, sys_addr_br);

    return ret;
}
