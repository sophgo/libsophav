#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"

#define M_PI 3.14159265358979323846

enum bdry{
    zeros = 0,
    constant,
    even,
    odd,
    none,
};

enum win{
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
    api.XI =  real_input ? 0 : bm_mem_get_device_addr(XI);
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
            printf("BMCV_WIN BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

static bm_status_t bmcv_fft_apply(bm_handle_t handle, bm_device_mem_t inputReal,
                                       bm_device_mem_t inputImag, bm_device_mem_t outputReal,
                                       bm_device_mem_t outputImag, const void *plan, bool realInput) {
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
    api.realInput = realInput ? 1 : 0;
    api.batch = P->batch;
    api.len = P->L;
    api.forward = P->forward ? 1 : 0;
    api.trans = 0;
    api.factorSize = P->factors_size;
    api.normalize = false;

    for (i = 0; i < P->factors_size; i++) {
        api.factors[i] = P->factors[i];
    }
    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("get chipid is error!\n");
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
            printf(" BMCV_FFT_APPLY BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }

    return ret;
}

static bm_status_t bmcv_fft_calculate(bm_handle_t handle, float *XRHost, float* XIHost, float* YR,
                                      float* YI, int batch, int xlen, int xlen_, int L, int window, int nperseg,
                                      int noverlap, int padded_len, int nfft, int boundary, bool real_input) {
    bm_status_t ret = BM_SUCCESS;
    int hop = nperseg - noverlap;
    void* plan = NULL;
    float* input_XR = (float*)malloc(xlen_ * sizeof(float));
    float* input_XI = (float*)malloc(xlen_ * sizeof(float));
    float* window_X = (float*)calloc(nfft, sizeof(float));

    bm_device_mem_t XRDev, XIDev, YRDev, YIDev, WinDev;
    ret = bm_malloc_device_byte(handle, &YRDev, batch * L * nfft * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte YRDev failed!\n");
        goto exit0;
    }
    ret = bm_malloc_device_byte(handle, &YIDev, batch * L * nfft * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte YIDev failed!\n");
        goto exit1;
    }
    ret = bm_malloc_device_byte(handle, &WinDev, nfft * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte WinDev failed!\n");
        goto exit2;
    }

    ret = bmcv_fft_1d_create_plan(handle, batch * L, nfft, true, &plan);
    if (ret != BM_SUCCESS) {
        printf("bmcv_fft_1d_create_plan failed!\n");
        goto exit3;
    }

    for (int j = 0; j < batch; ++j) {
        /* Initialize the input signal */
        memset(input_XR, 0, xlen_ * sizeof(float));
        memset(input_XI, 0, xlen_ * sizeof(float));

        switch (boundary) {
        case zeros:
            // Zero-padding at both ends
            for (int i = 0; i < hop; i++) {
                input_XR[i] = 0.0f;                     // Left zero-padding
                input_XR[xlen_ - padded_len - hop + i] = 0.0f; // Right zero-padding
                input_XI[i] = 0.0f;                     // Left zero-padding
                input_XI[xlen_ - padded_len - hop + i] = 0.0f; // Right zero-padding
            }
            // Copy original signal to the center
            memcpy(input_XR + hop, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI + hop, XIHost + j * xlen, xlen * sizeof(float));
            break;
        case constant:
            for (int i = 0; i < hop; i++) {
                input_XR[i] = *(XRHost + j * xlen + hop);
                input_XR[xlen_ - padded_len - hop + i] = *(XRHost + j * xlen + xlen - 1);
                input_XI[i] = *(XIHost + j * xlen + hop);
                input_XI[xlen_ - padded_len - hop + i] = *(XIHost + j * xlen + xlen - 1);
            }
            memcpy(input_XR + hop, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI + hop, XIHost + j * xlen, xlen * sizeof(float));
            break;
        case even:
            // Even symmetry padding (mirroring with repetition of last element)

            for (int i = 0; i < hop; i++) {
                // Left padding: mirror from start
                input_XR[i] = *(XRHost + j * xlen + hop - 1 - i);
                input_XI[i] = *(XIHost + j * xlen + hop - 1 - i);
                // Right padding: mirror from end
                input_XR[xlen_ - padded_len - hop + i] = *(XRHost + j * xlen + xlen - 1 - i);
                input_XI[xlen_ - padded_len - hop + i] = *(XIHost + j * xlen + xlen - 1 - i);
            }

            // Copy original signal to the center
            memcpy(input_XR + hop, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI + hop, XIHost + j * xlen, xlen * sizeof(float));
            break;

        case odd:
            // Odd symmetry padding (mirroring with sign flip)
            for (int i = 0; i < hop; i++) {
                // Left padding: mirror with sign flip
                input_XR[i] = -*(XRHost + j * xlen + hop - i);  // Note: different indexing for odd symmetry
                input_XI[i] = -*(XIHost + j * xlen + hop - i);
                // Right padding: mirror with sign flip
                input_XR[xlen_ - padded_len - hop + i] = -*(XRHost + j * xlen + xlen - 2 - i);
                input_XI[xlen_ - padded_len - hop + i] = -*(XIHost + j * xlen + xlen - 2 - i);
            }
            // Copy original signal to the center
            memcpy(input_XR + hop, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI + hop, XIHost + j * xlen, xlen * sizeof(float));
            break;
        case none:
            memcpy(input_XR, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI, XIHost + j * xlen, xlen * sizeof(float));
            break;
        default:
            printf("Unsupported boundary value! Use original signal.\n");
            memcpy(input_XR, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI, XIHost + j * xlen, xlen * sizeof(float));
            break;
        }
        for (int i = 0; i < padded_len; i++) {
            input_XR[xlen_ - padded_len + i] = 0;
            input_XI[xlen_ - padded_len + i] = 0;
        }

        for (int l = 0; l < L; l++) {
            memcpy(YR + j * L * nfft + l * nfft, input_XR + hop * l, nperseg * sizeof(float));
            memset(YR + j * L * nfft + l * nfft + nperseg, 0, (nfft - nperseg) * sizeof(float));
            if (!real_input) {
                memcpy(YI + j * L * nfft + l * nfft, input_XI + hop * l, nperseg * sizeof(float));
                memset(YI + j * L * nfft + l * nfft + nperseg, 0, (nfft - nperseg) * sizeof(float));
            }
        }
    }

    ret = bm_malloc_device_byte(handle, &XRDev, batch * L * nfft * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte XRDev failed!\n");
        goto exit4;
    }
    ret = bm_memcpy_s2d(handle, XRDev, YR);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d failed!\n");
        goto exit5;
    }
    if (!real_input) {
        ret = bm_malloc_device_byte(handle, &XIDev, batch * L * nfft * sizeof(float));
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte XIDev failed!\n");
            goto exit6;
        }
        ret = bm_memcpy_s2d(handle, XIDev, YI);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d failed!\n");
            goto exit6;
        }
    }
    if (window == HANN) {
        for (int i = 0; i < nperseg; i++) {
            window_X[i] = 0.5 * (1 - cos(2 * M_PI * (float)i / (nperseg - 1)));
        }
    } else if (window == HAMM) {
        for (int i = 0; i < nperseg; i++) {
            window_X[i] = 0.54 - 0.46 * cos(2 * M_PI * (float)i / (nperseg - 1));
        }
    }

    ret = bm_memcpy_s2d(handle, WinDev, window_X);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d WinDev failed!\n");
        goto exit6;
    }

    ret = bmcv_win(XRDev, XIDev, WinDev, batch * L, nfft, real_input, handle);
    if (ret != BM_SUCCESS) {
        printf("bmcv_win failed!\n");
        goto exit6;
    }

    ret = bmcv_fft_apply(handle, XRDev, XIDev, YRDev, YIDev, plan, real_input);
    if (ret != BM_SUCCESS) {
        printf("bmcv_fft_execute failed!\n");
        goto exit6;
    }

    ret = bm_memcpy_d2s(handle, YR, YRDev);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed!\n");
        goto exit6;
    }

    ret = bm_memcpy_d2s(handle, YI, YIDev);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed!\n");
        goto exit6;
    }

exit6:
    if (!real_input) {
        bm_free_device(handle, XIDev);
    }
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
    free(input_XR);
    free(input_XI);
    free(window_X);
    return ret;
}

static bm_status_t bmcv_tran_res(bm_handle_t handle, float* YR, float* YI, int batch, int L, int nfft)
{
    bm_status_t ret = BM_SUCCESS;
    int i, j;
    bm_image input_img, output_img;
    int stride_byte = nfft * sizeof(float);
    unsigned char** host;
    float* Y_tmp;

    ret = bm_image_create(handle, L, nfft, FORMAT_GRAY, DATA_TYPE_EXT_FLOAT32, &input_img, &stride_byte);
    if (ret != BM_SUCCESS) {
        printf("ERROR: Cannot create input_bm_image\n");
        goto exit0;
    }

    ret = bm_image_alloc_dev_mem(input_img, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("Alloc bm dev input mem failed. ret = %d\n", ret);
        goto exit0;
    }

    ret = bm_image_create(handle, nfft, L, FORMAT_GRAY, DATA_TYPE_EXT_FLOAT32, &output_img, NULL);
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
        Y_tmp = YR + j * L * nfft;
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

        Y_tmp = YI + j * L * nfft;
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

bm_status_t bmcv_stft(bm_handle_t handle, float* XRHost, float* XIHost, int batch, int xlen, int fs, int window, int nperseg, int noverlap,
                        int nfft, bool return_onesided, int boundary, bool padded, bool real_input, float* YRHost, float* YIHost, float* f, float* t)
{
    bm_status_t ret = BM_SUCCESS;
    int hop = nperseg - noverlap;
    int xlen_ = 0;
    if (boundary == zeros || boundary == constant || boundary == even || boundary == odd) {
        xlen_ = xlen + 2 * hop;
    } else {
        xlen_ = xlen;
    }
    int padded_len = 0;
    if (padded) {
        padded_len = (xlen_ - nperseg) % hop == 0 ? 0 : hop  - (xlen_ - nperseg) % hop;
    }
    xlen_ += padded_len;
    int L = 1 + (xlen_ - nperseg) / hop;
    int row_num = return_onesided ? nfft / 2 + 1 : nfft;

    for (int i = 0; i < row_num; i++) {
        f[i] = i * ((float)fs / nfft);
    }
    for (int l = 0; l < L; l++) {
        t[l] = (nperseg / 2.0 + l * hop) / fs;
    }

    float* YR = (float*)malloc(batch * L * nfft * sizeof(float));
    float* YI = (float*)malloc(batch * L * nfft * sizeof(float));

    ret = bmcv_fft_calculate(handle, XRHost, XIHost, YR, YI, batch, xlen, xlen_, L, window, nperseg, noverlap, padded_len, nfft, boundary, real_input);
    if (ret != BM_SUCCESS) {
        printf("bmcv_fft_calculate failed!\n");
        goto exit;
    }
    ret = bmcv_tran_res(handle, YR, YI, batch, L, nfft);
    if (ret != BM_SUCCESS) {
        printf("bmcv_tran_res failed!\n");
        goto exit;
    }
    for (int i = 0; i <  batch; ++i) {
        memcpy(YRHost + i * row_num * L, YR + i * L * nfft, row_num * L * sizeof(float));
        memcpy(YIHost + i * row_num * L, YI + i * L * nfft, row_num * L * sizeof(float));
    }

exit:
    free(YI);
    free(YR);
    return ret;
}