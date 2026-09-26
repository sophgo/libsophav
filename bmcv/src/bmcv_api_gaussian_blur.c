#include "stdlib.h"
#include "bmcv_internal.h"
#include "bmcv_common.h"
#include <string.h>
#include <float.h>
#include <math.h>

static bool is_dual_core_enabled(void) {
    const char *env = getenv("TPU_CORES");
    return env && (strcmp(env, "2") == 0 || strcmp(env, "both") == 0);
}

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
    if (kw != 3 && kw != 5 && kw != 7 && kw != 9) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "The kernel size only support 3, 5, 7, 9!\n");
        return BM_ERR_PARAM;
    }
    if (kw != kh) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "The kernel size only support square kernel (kw=%d, kh=%d)!\n", kw, kh);
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

    if (image_sw < 8 || image_sw > 8192) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "image width out of range [8, 8192]: %d!\r\n", image_sw);
        return BM_ERR_PARAM;
    }
    if (image_sh < 8 || image_sh > 8192) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "image height out of range [8, 8192]: %d!\r\n", image_sh);
        return BM_ERR_PARAM;
    }
    if (src_format != FORMAT_RGB_PLANAR &&
        src_format != FORMAT_BGR_PLANAR &&
        src_format != FORMAT_RGB_PACKED &&
        src_format != FORMAT_BGR_PACKED &&
        src_format != FORMAT_BGRP_SEPARATE &&
        src_format != FORMAT_RGBP_SEPARATE &&
        src_format != FORMAT_YUV444P &&
        src_format != FORMAT_NV12 &&
        src_format != FORMAT_NV21 &&
        src_format != FORMAT_NV16 &&
        src_format != FORMAT_NV61 &&
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
    if ((src_format == FORMAT_NV12 || src_format == FORMAT_NV21) && ((image_sw | image_sh) & 1)) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "NV12/NV21 format requires even width and height (w=%d, h=%d)\n", image_sw, image_sh);
        return BM_NOT_SUPPORTED;
    }
    if ((src_format == FORMAT_NV16 || src_format == FORMAT_NV61) && (image_sw & 1)) {
        bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "NV16/NV61 format requires even width (w=%d)\n", image_sw);
        return BM_NOT_SUPPORTED;
    }
    // NV format under dual-core mode (TPU_CORES=2/both) only supports up to 1920x1080
    if (src_format == FORMAT_NV12 || src_format == FORMAT_NV21 ||
        src_format == FORMAT_NV16 || src_format == FORMAT_NV61) {
        if (is_dual_core_enabled()) {
            if (image_sw > 1920 || image_sh > 1080) {
                bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR,
                          "NV format with dual-core only supports up to 1920x1080 (w=%d, h=%d)!\n",
                          image_sw, image_sh);
                return BM_NOT_SUPPORTED;
            }
        }
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_gaussian_blur(bm_handle_t handle, bm_image input, bm_image output, int kw,
                                     int kh, float sigmaX, float sigmaY) {
    bm_status_t ret = BM_SUCCESS;
    float* tpu_kernel = (float*)malloc(sizeof(float) * kw * kh);
    sg_device_mem_st kernel_mem;
    bool output_alloc_flag = false;
    int if_core0 = 1, if_core1 = 0;
    const char *tpu_env = NULL;

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
        ret = sg_image_alloc_dev_mem(output, BMCV_HEAP1_ID);
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
    sg_api_cv_gaussian_blur_t api;
    api.channel = channel;
    api.kernel_addr = bm_mem_get_device_addr(kernel_mem.bm_device_mem);
    api.kh = kh;
    api.kw = kw;
    api.delta = 0;
    api.is_packed = (input.image_format == FORMAT_RGB_PACKED || input.image_format == FORMAT_BGR_PACKED);
    api.out_type = 0;   // 0-uint8  1-uint16
    api.format = 0;

    // NV format temp buffer flags and pointers
    int is_nv_format = (input.image_format == FORMAT_NV12 || input.image_format == FORMAT_NV21 ||
                        input.image_format == FORMAT_NV16 || input.image_format == FORMAT_NV61);
    bm_device_mem_t nv_temp_in_mem, nv_temp_out_mem;
    int nv_temp_in_alloc = 0, nv_temp_out_alloc = 0;

    if (is_nv_format) {
        size_t temp_in_size = (size_t)stride_i[0] * input.height * 2;
        size_t temp_out_size = (size_t)stride_o[0] * input.height * 2;
        if (bm_malloc_device_byte(handle, &nv_temp_in_mem, temp_in_size) != BM_SUCCESS) goto nv_cleanup;
        nv_temp_in_alloc = 1;
        if (bm_malloc_device_byte(handle, &nv_temp_out_mem, temp_out_size) != BM_SUCCESS) goto nv_cleanup;
        nv_temp_out_alloc = 1;

        api.channel = 3;
        api.width = input.width;
        api.height = input.height;
        api.stride_i = stride_i[0];
        api.stride_o = stride_o[0];
        api.input_addr[0] = bm_mem_get_device_addr(input_mem[0]);
        api.input_addr[1] = bm_mem_get_device_addr(input_mem[1]);
        api.input_addr[2] = bm_mem_get_device_addr(nv_temp_in_mem);
        api.output_addr[0] = bm_mem_get_device_addr(output_mem[0]);
        api.output_addr[1] = bm_mem_get_device_addr(output_mem[1]);
        api.output_addr[2] = bm_mem_get_device_addr(nv_temp_out_mem);
        api.format = input.image_format;
    } else {
        for (int i = 0; i < channel; i++) {
            api.input_addr[i] = bm_mem_get_device_addr(input_mem[i]);
            api.output_addr[i] = bm_mem_get_device_addr(output_mem[i]);
            api.width = input.image_private->memory_layout[i].W / (api.is_packed ? 3 : 1);
            api.height = input.image_private->memory_layout[i].H;
            api.stride_i = stride_i[i];
            api.stride_o = stride_o[i];
        }
    }
    if (input.image_format == FORMAT_RGB_PLANAR ||
        input.image_format == FORMAT_BGR_PLANAR) {
        api.channel = 3;
        for (int i = 0; i < 3; i++) {
            api.stride_i = stride_i[0];
            api.stride_o = stride_o[0];
            api.width = input.width;
            api.height = input.height;
            api.input_addr[i] = bm_mem_get_device_addr(input_mem[0]) + input.height * stride_i[0] * i;
            api.output_addr[i] = bm_mem_get_device_addr(output_mem[0]) + input.height * stride_i[0] * i;
        }
    }
    ret = bm_get_chipid(handle, &chipid);
    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            tpu_env = getenv("TPU_CORES");
            if (tpu_env) {
                if (strcmp(tpu_env, "0") == 0) {
                    bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_DEBUG, "Use TPU Core0\n");
                } else if (strcmp(tpu_env, "1") == 0) {
                    if_core0 = 0;
                    if_core1 = 1;
                    bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_DEBUG, "Use TPU Core1\n");
                } else if (is_dual_core_enabled()) {
                    if_core1 = 1;
                    bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_DEBUG, "Use ALL TPU Cores(0 and 1)\n");
                } else {
                    bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "Invalid TPU_CORES value: %s\n", tpu_env);
                    bmlib_log("GAUSSIAN_BLUR", BMLIB_LOG_ERROR, "Available options: 0, 1, 2/both\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (if_core0 && if_core1) {
                int core_list[BM1688_MAX_CORES] = {0, 1};

                tpu_launch_param_t tpu_params[BM1688_MAX_CORES];
                sg_api_cv_gaussian_blur_dual_core_t dual_api;
                sg_api_cv_gaussian_blur_dual_core_t dual_params[BM1688_MAX_CORES];

                memcpy(&dual_api, &api, sizeof(sg_api_cv_gaussian_blur_t));

                dual_api.core_num = BM1688_MAX_CORES;
                dual_api.base_msg_id = BM1688_BASE_MSG_ID;

                for (int n = 0; n < BM1688_MAX_CORES; n++) {
                    dual_api.core_id = n;

                    dual_params[n] = dual_api;
                    tpu_params[n].core_id = n;
                    tpu_params[n].param_data = &dual_params[n];
                    tpu_params[n].param_size = sizeof(sg_api_cv_gaussian_blur_dual_core_t);
                }

                ret = bm_tpu_kernel_launch_dual_core(handle, "cv_gaussian_blur_dual_core_split_col", tpu_params, core_list, BM1688_MAX_CORES);
            } else {
                int core_id = if_core1 == 1 ? 1 : 0;
                ret = bm_tpu_kernel_launch(handle, "cv_gaussian_blur_split_col", (u8 *)&api, sizeof(api), core_id);
            }

            if (BM_SUCCESS != ret) {
                bmlib_log("gaussian_blur", BMLIB_LOG_ERROR, "gaussian_blur sync api error\n");
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

    // NV post-processing is now done on device side

nv_cleanup:
    if (nv_temp_in_alloc)  bm_free_device(handle, nv_temp_in_mem);
    if (nv_temp_out_alloc) bm_free_device(handle, nv_temp_out_mem);

    sg_free_device_mem(handle, kernel_mem);
    free(tpu_kernel);
    return ret;
}