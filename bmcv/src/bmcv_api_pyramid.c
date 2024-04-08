#include "bmcv_common.h"
#include "bmcv_internal.h"

static bm_status_t bmcv_pyramid_check(bm_handle_t handle, bm_image input, bm_image output, bool is_down)
{
    bm_image_format_ext src_format = input.image_format;
    bm_image_data_format_ext src_type = input.data_type;
    bm_image_format_ext dst_format = output.image_format;
    bm_image_data_format_ext dst_type = output.data_type;
    int image_sh = input.height;
    int image_sw = input.width;
    int image_dh = output.height;
    int image_dw = output.width;

    if (handle == NULL) {
        bmlib_log("PYRAMID", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    if (image_sw + 5 - 1 >= 2048 || image_sh > 4096) {
        bmlib_log("PYRAMID", BMLIB_LOG_ERROR, "Unsupported size : size_max = 2043(width) x 4096(height)!\r\n");
        return BM_NOT_SUPPORTED;
    }
    if (image_sh < 3 || image_sw < 3) {
        bmlib_log("PYRAMID", BMLIB_LOG_ERROR, "Unsupported size : size_min = 3 x 3!\r\n");
        return BM_NOT_SUPPORTED;
    }
    if (src_format != FORMAT_GRAY || dst_format != FORMAT_GRAY) {
        bmlib_log("PYRAMID", BMLIB_LOG_ERROR, "Image format only support GRAY\n");
        return BM_NOT_SUPPORTED;
    }
    if (src_type != DATA_TYPE_EXT_1N_BYTE || dst_type != DATA_TYPE_EXT_1N_BYTE) {
        bmlib_log("PYRAMID", BMLIB_LOG_ERROR, "Not supported image data type\n");
        return BM_NOT_SUPPORTED;
    }
    if (is_down && (abs(image_sh - image_dh * 2) > 1 || abs(image_sw - image_dw * 2) > 1)) {
        bmlib_log("PYRAMID", BMLIB_LOG_ERROR, "input and output image size not valid\n");
        return BM_NOT_SUPPORTED;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_pyramid_down(bm_handle_t handle, bm_image input, bm_image output)
{
    bm_status_t ret = BM_SUCCESS;
    int kw = 5, kh = 5;
    bm_device_mem_t kernel_mem;
    int stride_i, stride_o;
    bm_device_mem_t input_mem, output_mem;
    bm_api_cv_pyramid_t api;
    unsigned int chipid;
    int core_id = 0;

    ret = bmcv_pyramid_check(handle, input, output, true);
    if (ret != BM_SUCCESS) {
        printf("the bmcv_image_pyramid_down check failed!\n");
        return ret;
    }

    float kernel[25] = {1, 4, 6, 4, 1, 4, 16, 24, 16, 4, 6, 24, 36, 24, 6, \
                        4, 16, 24, 16, 4, 1, 4, 6, 4, 1};

    ret = bm_malloc_device_byte(handle, &kernel_mem, kw * kh * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("the bmcv_malloc kernel_mem failed!\n");
        return ret;
    }

    ret = bm_memcpy_s2d(handle, kernel_mem, kernel);
    if (ret != BM_SUCCESS) {
        printf("the bmcv_memcpy_s2d kernel_mem failed!\n");
        bm_free_device(handle, kernel_mem);
        return ret;
    }


    ret = bm_image_get_stride(input, &stride_i);
    if (ret != BM_SUCCESS) {
        bm_free_device(handle, kernel_mem);
        printf("the bm_image_get_stride input failed!\n");
        return ret;
    }

    ret = bm_image_get_stride(output, &stride_o);
    if (ret != BM_SUCCESS) {
        printf("the bm_image_get_stride output failed!\n");
        bm_free_device(handle, kernel_mem);
        return ret;
    }

    ret = bm_image_get_device_mem(input, &input_mem);
    if (ret != BM_SUCCESS) {
        printf("the bm_image_get_device_mem input failed!\n");
        bm_free_device(handle, kernel_mem);
        return ret;
    }

    ret = bm_image_get_device_mem(output, &output_mem);
    if (ret != BM_SUCCESS) {
        printf("the bm_image_get_device_mem output failed!\n");
        bm_free_device(handle, kernel_mem);
        return ret;
    }

    api.kernel_addr = bm_mem_get_device_addr(kernel_mem);
    api.input_addr = bm_mem_get_device_addr(input_mem);
    api.output_addr = bm_mem_get_device_addr(output_mem);
    api.width = input.width;
    api.height = input.height;
    api.ow = output.width;
    api.oh = output.height;
    api.stride_i = stride_i;
    api.stride_o = stride_o;

    ret = bm_get_chipid(handle, &chipid);
    if (ret != BM_SUCCESS) {
        printf("get chipid is error !\n");
        bm_free_device(handle, kernel_mem);
        return BM_ERR_FAILURE;
    }

    switch(chipid) {
        case BM1686:
            ret = bm_tpu_kernel_launch(handle, "cv_pyramid", (u8*)&api,
                                                sizeof(api), core_id);
            if (ret != BM_SUCCESS) {
                bmlib_log("PYRAMID", BMLIB_LOG_ERROR, "pyramid sync api error\n");
                bm_free_device(handle, kernel_mem);
                return ret;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            bm_free_device(handle, kernel_mem);
            ret = BM_NOT_SUPPORTED;
            break;
    }

    bm_free_device(handle, kernel_mem);

    return ret;
}