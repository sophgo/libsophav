#include "stdlib.h"
#include "bmcv_internal.h"
#include "bmcv_common.h"
#include <string.h>
#include <float.h>
#include <math.h>

static int get_gaussian_sep_kernel(int n, float sigma, float *k_sep) {
    const int SMALL_GAUSSIAN_SIZE = 3;
    static const float small_gaussian_tab[3] = {0.25f, 0.5f, 0.25f};
    const float* fixed_kernel = n % 2 == 1 && n <= SMALL_GAUSSIAN_SIZE && sigma <= 0 ? small_gaussian_tab : 0;
    float sigmaX = sigma > 0 ? sigma : ((n - 1) * 0.5 - 1) * 0.3 + 0.8;
    float scale2X = -0.5 / (sigmaX * sigmaX);
    float sum = 0;
    int i;

    for (i = 0; i < n; i++) {
        float x = i - (n - 1) * 0.5;
        float t = fixed_kernel ? fixed_kernel[i] : exp(scale2X * x * x);
        k_sep[i] = t;
        sum += k_sep[i];
    }
    sum = 1./sum;
    for (i = 0; i < n; i++) {
        k_sep[i] = k_sep[i] * sum;
    }
    return 0;
}

static void create_gaussian_kernel(float* kernel, int kw, int kh, float sigma1, float sigma2) {
    float* k_sep_x = (float* )malloc(sizeof(float) * kw);
    float* k_sep_y = (float* )malloc(sizeof(float) * kh);

    if(sigma2 <= 0) sigma2 = sigma1;
    // automatic detection of kernel size from sigma
    if (kw <= 0 && sigma1 > 0 ) kw = (int)round(sigma1 * 3 * 2 + 1) | 1;
    if (kh <= 0 && sigma2 > 0 ) kh = (int)round(sigma2 * 3 * 2 + 1) | 1;
    sigma1 = sigma1 < 0 ? 0 : sigma1;
    sigma2 = sigma2 < 0 ? 0 : sigma2;
    get_gaussian_sep_kernel(kw, sigma1, k_sep_x);
    if (kh == kw && abs(sigma1 - sigma2) < DBL_EPSILON) {
        get_gaussian_sep_kernel(kw, sigma1, k_sep_y);
    } else {
        get_gaussian_sep_kernel(kh, sigma2, k_sep_y);
    }
    for (int i = 0; i < kh; i++) {
        for (int j = 0; j < kw; j++) {
            kernel[i * kw + j] = k_sep_y[i] * k_sep_x[j];
        }
    }
    free(k_sep_x);
    free(k_sep_y);
}

static bm_status_t bmcv_gaussian_blur_check(bm_handle_t handle, bm_image input, bm_image output,
                                            int kw, int kh) {
    if (handle == NULL) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    if (kw > 3 || kh > 3) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "The kernel size only support 3 now!\n");
        return BM_ERR_PARAM;
    }
    bm_image_format_ext src_format = input.image_format;
    bm_image_data_format_ext src_type = input.data_type;
    bm_image_format_ext dst_format = output.image_format;
    bm_image_data_format_ext dst_type = output.data_type;
    int image_sh = input.height;
    int image_sw = input.width;
    int image_dh = output.height;
    int image_dw = output.width;

    if (image_sw < 2 || image_sw > 4096) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "image width should not be less than 2 or greater than 4096!\r\n");
        return BM_NOT_SUPPORTED;
    }
    if (src_format != FORMAT_RGB_PLANAR &&
        src_format != FORMAT_BGR_PLANAR &&
        src_format != FORMAT_RGB_PACKED &&
        src_format != FORMAT_BGR_PACKED &&
        src_format != FORMAT_BGRP_SEPARATE &&
        src_format != FORMAT_RGBP_SEPARATE &&
        src_format != FORMAT_GRAY) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "Not supported input image format!\n");
        return BM_NOT_SUPPORTED;
    }
    if (dst_format != src_format) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "input and output image format should be same!\n");
        return BM_NOT_SUPPORTED;
    }
    if (src_type != DATA_TYPE_EXT_1N_BYTE ||
        dst_type != DATA_TYPE_EXT_1N_BYTE) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "Not supported image data type\n");
        return BM_NOT_SUPPORTED;
    }
    if (image_sh != image_dh || image_sw != image_dw) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "input and output image size should be same\n");
        return BM_NOT_SUPPORTED;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_gaussian_blur(bm_handle_t handle, bm_image input, bm_image output, int kw,
                                     int kh, float sigmaX, float sigmaY) {
    bm_status_t ret = BM_SUCCESS;
    float* tpu_kernel = (float*)malloc(sizeof(float) * kw * kh);
    sg_device_mem_st kernel_mem;
    bool output_alloc_flag = false;

    ret = bmcv_gaussian_blur_check(handle, input, output, kw, kh);
    if (BM_SUCCESS != ret) {
        free(tpu_kernel);
        return ret;
    }
    create_gaussian_kernel(tpu_kernel, kw, kh, sigmaX, sigmaY);
    ret = sg_malloc_device_mem(handle, &kernel_mem, kw * kh * sizeof(float));
    if (BM_SUCCESS != ret) {
        free(tpu_kernel);
        return ret;
    }
    ret = bm_memcpy_s2d(handle, kernel_mem.bm_device_mem, tpu_kernel);
    if (BM_SUCCESS != ret) {
        sg_free_device_mem(handle, kernel_mem);
        free(tpu_kernel);
        return ret;
    }
    if (!bm_image_is_attached(output)) {
        ret = sg_image_alloc_dev_mem(output, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            sg_free_device_mem(handle, kernel_mem);
            free(tpu_kernel);
            return ret;
        }
        output_alloc_flag = true;
    }

    // construct and send api
    unsigned int chipid;
    int stride_i[3], stride_o[3];
    bm_device_mem_t input_mem[3];
    bm_device_mem_t output_mem[3];
    bm_image_get_stride(input, stride_i);
    bm_image_get_stride(output, stride_o);
    bm_image_get_device_mem(input, input_mem);
    bm_image_get_device_mem(output, output_mem);
    int channel = bm_image_get_plane_num(input);
    bm_api_cv_filter_t api;
    int core_id = 0;

    api.channel = channel;
    api.kernel_addr = bm_mem_get_device_addr(kernel_mem.bm_device_mem);
    api.kh = kh;
    api.kw = kw;
    api.delta = 0;
    api.is_packed = (input.image_format == FORMAT_RGB_PACKED || input.image_format == FORMAT_BGR_PACKED);
    api.out_type = 0;   // 0-uint8  1-uint16
    for (int i = 0; i < channel; i++) {
        api.input_addr[i] = bm_mem_get_device_addr(input_mem[i]);
        api.output_addr[i] = bm_mem_get_device_addr(output_mem[i]);
        api.width[i] = input.image_private->memory_layout[i].W / (api.is_packed ? 3 : 1);
        api.height[i] = input.image_private->memory_layout[i].H;
        api.stride_i[i] = stride_i[i];
        api.stride_o[i] = stride_o[i];
    }
    if (input.image_format == FORMAT_RGB_PLANAR || input.image_format == FORMAT_BGR_PLANAR) {
        api.channel = 3;
        api.stride_i[1] = api.stride_i[0];
        api.stride_i[2] = api.stride_i[0];
        api.stride_o[1] = api.stride_o[0];
        api.stride_o[2] = api.stride_o[0];
        api.width[1] = api.width[0];
        api.width[2] = api.width[0];
        api.height[1] = api.height[0];
        api.height[2] = api.height[0];
        api.input_addr[1] = api.input_addr[0] + api.height[0] * api.stride_i[0];
        api.input_addr[2] = api.input_addr[0] + api.height[0] * api.stride_i[0] * 2;
        api.output_addr[1] = api.output_addr[0] + api.height[0] * api.stride_i[0];
        api.output_addr[2] = api.output_addr[0] + api.height[0] * api.stride_i[0] * 2;
    }
    ret = bm_get_chipid(handle, &chipid);
    switch (chipid) {
        case BM1686:
            ret = bm_tpu_kernel_launch(handle, "cv_gaussinan_blur_", (u8 *)&api, sizeof(api), core_id);
            if (BM_SUCCESS != ret) {
                bmlib_log("gaussinan_blur", BMLIB_LOG_ERROR, "gaussinan_blur sync api error\n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            break;
    }
    if (BM_SUCCESS != ret) {
        if (output_alloc_flag) {
            bm_free_device(handle,output_mem[0]);
        }
    }
    sg_free_device_mem(handle, kernel_mem);
    free(tpu_kernel);
    return ret;
}