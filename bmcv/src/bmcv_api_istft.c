#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"

#define M_PI 3.14159265358979323846

enum Pad_mode {
    CONSTANT = 0,
    REFLECT = 1,
};

enum Win_mode {
    HANN = 0,
    HAMM = 1,
};

typedef struct {
    int64_t XR;
    int64_t XI;
    int64_t window;
    int batch;
    int L;
    bool real_input;
} __attribute__((aligned(4))) bm_api_win;

static bm_status_t bmcv_win(bm_device_mem_t XR, bm_device_mem_t XI, bm_device_mem_t WinDev, int num_frm,
                            int n_fft, bool real_input, bm_handle_t handle)
{
    bm_api_win api;
    int core_id = 0;
    unsigned int chipid;
    bm_status_t ret = BM_SUCCESS;

    api.XR = bm_mem_get_device_addr(XR);
    api.XI = bm_mem_get_device_addr(XI);
    api.window = bm_mem_get_device_addr(WinDev);
    api.batch = num_frm;
    api.L = n_fft;
    api.real_input = real_input;

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_fft_window", (u8*)(&api), sizeof(api), core_id);
            if(ret != BM_SUCCESS){
                bmlib_log("FFT_WINDOW", BMLIB_LOG_ERROR, "cv_fft_window sync api error\n");
                return ret;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

static bm_status_t bmcv_fft_apply(bm_handle_t handle, bm_device_mem_t inputReal,
                                bm_device_mem_t inputImag, bm_device_mem_t outputReal,
                                bm_device_mem_t outputImag, const void *plan, bool realInput,
                                int trans, bool normalize)
{
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_fft_t api;
    struct FFT1DPlan* P = (struct FFT1DPlan*)plan;
    int i;
    int core_id = 0;
    unsigned int chipid;

    api.XR = bm_mem_get_device_addr(inputReal);
    api.XI = realInput ? 0 : bm_mem_get_device_addr(inputImag);
    api.YR = bm_mem_get_device_addr(outputReal);
    api.YI = bm_mem_get_device_addr(outputImag);
    api.ER = bm_mem_get_device_addr(P->ER);
    api.EI = bm_mem_get_device_addr(P->EI);
    api.batch = P->batch;
    api.len = P->L;
    api.forward = P->forward ? 1 : 0;
    api.realInput = realInput ? 1 : 0;
    api.trans = trans;
    api.factorSize = P->factors_size;
    api.normalize = normalize;

    for (i = 0; i < P->factors_size; ++i) {
        api.factors[i] = P->factors[i];
    }

    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_fft", (u8*)(&api), sizeof(api), core_id);
            if(ret != BM_SUCCESS){
                bmlib_log("FFT_1D", BMLIB_LOG_ERROR, "fft_1d sync api error\n");
                return ret;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

static bm_status_t bmcv_fft_calculate(bm_handle_t handle, float* XRHost, float* XIHost, float* YR,
                                    float* YI, int batch, int L, bool realInput,
                                    int pad_mode, int n_fft, int win_mode, int hop_len,
                                    bool normalize)
{
    bm_status_t ret = BM_SUCCESS;
    void* plan = NULL;
    int hop, i, j;
    int pad_len = n_fft / 2;
    int pad_sig_len = L + 2 * pad_len;
    // int hop_len = n_fft / 4;
    int num_frm = 1 + L / hop_len; /* YRHost col*/
    int row_num = n_fft / 2 + 1;
    bm_device_mem_t XRDev, XIDev, YRDev, YIDev, WinDev; //, outdevr, outdevi, out_windev;
    float* input_XR = (float*)malloc(batch * num_frm * n_fft * sizeof(float));
    float* input_XI = (float*)malloc(batch * num_frm * n_fft * sizeof(float));
    float* window = (float*)malloc(n_fft * sizeof(float));
    float* win_square = (float*)malloc(n_fft * sizeof(float));
    float* cumul_win_squa = (float*)malloc(pad_sig_len * sizeof(float));

    memset(cumul_win_squa, 0, pad_sig_len * sizeof(float));

    if (win_mode == HANN) {
        for (i = 0; i < n_fft; i++) {
            window[i] = 0.5 * (1 - cos(2 * M_PI * (float)i / n_fft));
            win_square[i] = window[i] * window[i];
        }
    } else if (win_mode == HAMM) {
        for (i = 0; i < n_fft; i++) {
            window[i] = 0.54 - 0.46 * cos(2 * M_PI * (float)i / n_fft);
            win_square[i] = window[i] * window[i];
        }
    }

    ret = bm_malloc_device_byte(handle, &YRDev, batch * n_fft * num_frm * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto exit0;
    }

    ret = bm_malloc_device_byte(handle, &YIDev, batch * n_fft * num_frm * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto exit1;
    }

    ret = bm_malloc_device_byte(handle, &WinDev, n_fft * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto exit2;
    }

    ret = bmcv_fft_1d_create_plan(handle, batch * num_frm, n_fft, false, &plan);
    if (ret != BM_SUCCESS) {
        printf("bmcv_fft_1d_create_plan failed!\n");
        goto exit3;
    }

    for (j = 0; j < batch; ++j) {
        for (hop = 0; hop < num_frm; ++hop) {
            memcpy(&input_XR[j * num_frm * n_fft + hop * n_fft], &XRHost[j * row_num * num_frm + hop * row_num], row_num * sizeof(float));
            memcpy(&input_XI[j * num_frm * n_fft + hop * n_fft], &XIHost[j * row_num * num_frm + hop * row_num], row_num * sizeof(float));
            for (i = 1; i < row_num - 1; ++i) {
                input_XR[j * num_frm * n_fft + hop * n_fft + n_fft - i] = XRHost[j * row_num * num_frm + hop * row_num + i];
                input_XI[j * num_frm * n_fft + hop * n_fft + n_fft - i] = -1 * XIHost[j * row_num * num_frm + hop * row_num + i];
            }
        }
    }

    ret = bm_malloc_device_byte(handle, &XRDev, batch * n_fft * num_frm * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto exit4;
    }

    ret = bm_memcpy_s2d(handle, XRDev, input_XR);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d failed!\n");
        goto exit5;
    }

    ret = bm_malloc_device_byte(handle, &XIDev, batch * n_fft * num_frm * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto exit5;
    }

    ret = bm_memcpy_s2d(handle, XIDev, input_XI);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d failed!\n");
        goto exit6;
    }

    ret = bm_memcpy_s2d(handle, WinDev, window);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d failed!\n");
        goto exit6;
    }

    ret = bmcv_fft_apply(handle, XRDev, XIDev, YRDev, YIDev, plan, false, 0, normalize);
    if (ret != BM_SUCCESS) {
        printf("bmcv_fft_execute failed!\n");
        goto exit6;
    }

    ret = bmcv_win(YRDev, YIDev, WinDev, batch * num_frm, n_fft, realInput, handle);
    if (ret != BM_SUCCESS) {
        printf("bmcv_win failed!\n");
        goto exit6;
    }

    ret = bm_memcpy_d2s(handle, input_XR, YRDev);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed!\n");
        goto exit6;
    }

    if (!realInput) {
        ret = bm_memcpy_d2s(handle, input_XI, YIDev);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_d2s failed!\n");
            goto exit6;
        }
    }

    for (hop = 0; hop < num_frm; ++hop) {
        for (i = 0; i < n_fft; ++i) {
            cumul_win_squa[hop * hop_len + i] += win_square[i];
        }
    }

    for (j = 0; j < batch; j++) {
        for (hop = 0; hop < num_frm; ++hop) {
            for (i = 0; i < n_fft; ++i) {
                YR[j * pad_sig_len + hop * hop_len + i] += input_XR[j * num_frm * n_fft + hop * n_fft + i];
                if (!realInput) {
                    YI[j * pad_sig_len + hop * hop_len + i] += input_XI[j * num_frm * n_fft + hop * n_fft + i];
                }
                //cumul_win_squa[hop * hop_len + i] += win_square[i];
            }
        }
        for (i = 0; i < pad_sig_len; ++i) {
            YR[j * pad_sig_len + i] = YR[j * pad_sig_len + i] / cumul_win_squa[i];
            if (!realInput) {
                YI[j * pad_sig_len + i] = YI[j * pad_sig_len + i] / cumul_win_squa[i];
            }
        }
    }

exit6:
    bm_free_device(handle, XIDev);
exit5:
    bm_free_device(handle, XRDev);
exit4:
    bmcv_fft_destroy_plan(handle, plan);
exit3:
    bm_free_device(handle, WinDev);
exit2:
    bm_free_device(handle, YIDev);
exit1:
    bm_free_device(handle, YRDev);
exit0:
    free(input_XI);
    free(input_XR);
    free(window);
    free(win_square);
    free(cumul_win_squa);
    return ret;
}

static bm_status_t bmcv_tran_res(bm_handle_t handle, float* YR, float* YI, int batch, int row, int col)
{
    bm_status_t ret = BM_SUCCESS;
    int i, j;
    bm_image input_img, output_img;
    int stride_byte = col * sizeof(float);
    unsigned char** host;
    float* Y_tmp;

    ret = bm_image_create(handle, row, col, FORMAT_GRAY, DATA_TYPE_EXT_FLOAT32, &input_img, &stride_byte);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot create input_bm_image\n");
        goto exit0;
    }

    ret = bm_image_alloc_dev_mem(input_img, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("Alloc bm dev input mem failed. ret = %d\n", ret);
        goto exit0;
    }

    ret = bm_image_create(handle, col, row, FORMAT_GRAY, DATA_TYPE_EXT_FLOAT32, &output_img, NULL);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot create output_bm_image\n");
        goto exit1;
    }

    ret = bm_image_alloc_dev_mem(output_img, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("Alloc bm dev output mem failed. ret = %d\n", ret);
        goto exit1;
    }

    for (j = 0; j < batch; ++j) {
        Y_tmp = YR + j * row * col;
        host = (unsigned char**)&Y_tmp;
        for (i = 0; i < input_img.image_private->plane_num; i++) {
            ret = bm_memcpy_s2d(input_img.image_private->handle, input_img.image_private->data[i], host[i]);
            if (ret != BM_SUCCESS) {
                printf("bm_memcpy_s2d error, src addr %p, dst addr 0x%llx\n",
                    host[i], bm_mem_get_device_addr(input_img.image_private->data[i]));
                goto exit2;
            } /* host = host + image.image_private->plane_byte_size[i]; */
        }

        ret = bmcv_image_transpose(handle, input_img, output_img);
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_transpose error!");
            goto exit2;
        }

        for (i = 0; i < output_img.image_private->plane_num; i++) {
            ret = bm_memcpy_d2s(output_img.image_private->handle, host[i], output_img.image_private->data[i]);
            if (ret != BM_SUCCESS) {
                printf("bm_memcpy_d2s error, src addr 0x%llx, dst addr %p\n",
                    bm_mem_get_device_addr(output_img.image_private->data[i]), host[i]);
                goto exit2;
            } /* host = host + image.image_private->plane_byte_size[i]; */
        }

        Y_tmp = YI + j * row * col;
        host = (unsigned char**)&Y_tmp;
        for (i = 0; i < input_img.image_private->plane_num; i++) {
            ret = bm_memcpy_s2d(input_img.image_private->handle, input_img.image_private->data[i], host[i]);
            if (ret != BM_SUCCESS) {
                printf("bm_memcpy_s2d error, src addr %p, dst addr 0x%llx\n",
                    host[i], bm_mem_get_device_addr(input_img.image_private->data[i]));
                goto exit2;
            } /* host = host + image.image_private->plane_byte_size[i]; */
        }

        ret = bmcv_image_transpose(handle, input_img, output_img);
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_transpose error!");
            goto exit2;
        }

        for (i = 0; i < output_img.image_private->plane_num; i++) {
            ret = bm_memcpy_d2s(output_img.image_private->handle, host[i], output_img.image_private->data[i]);
            if (ret != BM_SUCCESS) {
                printf("bm_memcpy_d2s error, src addr 0x%llx, dst addr %p\n",
                    bm_mem_get_device_addr(output_img.image_private->data[i]), host[i]);
                goto exit2;
            } /* host = host + image.image_private->plane_byte_size[i]; */
        }
    }

exit2:
    bm_image_destroy(&output_img);
exit1:
    bm_image_destroy(&input_img);
exit0:
    return ret;
}

bm_status_t bmcv_istft(bm_handle_t handle, float* XRHost, float* XIHost, float* YRHost,
                        float* YIHost, int batch, int L, bool realInput,
                        int pad_mode, int n_fft, int win_mode, int hop_len, bool normalize)
{
    bm_status_t ret = BM_SUCCESS;
    // int hop_len = n_fft / 4;
    int num_frm = 1 + L / hop_len; /* YRHost col*/
    int num_len = n_fft / 2 + 1; /* YRHost col*/
    int pad_len = n_fft / 2;
    int pad_sig_len = L + 2 * pad_len;
    int i;
    float* YR = (float*)malloc(batch * pad_sig_len * sizeof(float));
    float* YI = (float*)malloc(batch * pad_sig_len * sizeof(float));

    memset(YR, 0, batch * pad_sig_len * sizeof(float));
    memset(YI, 0, batch * pad_sig_len * sizeof(float));

    ret = bmcv_tran_res(handle, XRHost, XIHost, batch, num_len, num_frm);
    if (ret != BM_SUCCESS) {
        printf("bmcv_trans_res failed !\n");
        goto exit;
    }

    ret = bmcv_fft_calculate(handle, XRHost, XIHost, YR, YI, batch, L, realInput,
                            pad_mode, n_fft, win_mode, hop_len, normalize);
    if (ret != BM_SUCCESS) {
        printf("bmcv_fft_calculate failed!\n");
        goto exit;
    }

    for (i = 0; i < batch; ++i) {
        memcpy(&YRHost[i * L], &YR[i * pad_sig_len + n_fft / 2], L * sizeof(float));
        if (!realInput) {
            memcpy(&YIHost[i * L], &YI[ i * pad_sig_len + n_fft / 2], L * sizeof(float));
        }
    }

exit:
    free(YI);
    free(YR);
    return ret;
}
