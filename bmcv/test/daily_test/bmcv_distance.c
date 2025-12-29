#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_misc.h"

#define DIM_MAX 8

enum op {
FP32 = 0,
FP16 = 1,
};

int main() {
    int dim = 1 + rand() % 8;
    int len = 1 + rand() % 40960;
    int ret = 0;
    enum op op = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    float pnt32[DIM_MAX] = {0};
    float* XHost = (float*)malloc(len * dim * sizeof(float));
    float* tpu_out = (float*)malloc(len * sizeof(float));
    fp16* XHost16 = (fp16*)malloc(len * dim * sizeof(fp16));
    fp16* tpu_out_fp16 = (fp16*)malloc(len * sizeof(fp16));
    fp16 pnt16[DIM_MAX] = {0};
    int round = 1;
    int i;

    printf("len = %d, dim = %d, op = %d\n", len, dim, op);

    for (i = 0; i < dim; ++i) {
        pnt32[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
        pnt16[i] = fp32tofp16(pnt32[i], round);
    }

    for (i = 0; i < len * dim; ++i) {
        XHost[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
        XHost16[i] = fp32tofp16(XHost[i], round);
    }
    if (op == FP32) {
        enum bm_data_type_t dtype = DT_FP32;
        bm_device_mem_t XDev, YDev;

        ret = bm_malloc_device_byte(handle, &XDev, len * dim * sizeof(float));
        ret = bm_malloc_device_byte(handle, &YDev, len * sizeof(float));
        ret = bm_memcpy_s2d(handle, XDev, XHost);
        ret = bmcv_distance(handle, XDev, YDev, dim, pnt32, len, dtype);
        ret = bm_memcpy_d2s(handle, tpu_out, YDev);

        bm_free_device(handle, XDev);
        bm_free_device(handle, YDev);
    } else {
        enum bm_data_type_t dtype = DT_FP16;
        bm_device_mem_t XDev, YDev;

        ret = bm_malloc_device_byte(handle, &XDev, len * dim * sizeof(float) / 2);
        ret = bm_malloc_device_byte(handle, &YDev, len * sizeof(float) / 2);
        ret = bm_memcpy_s2d(handle, XDev, XHost16);
        ret = bmcv_distance(handle, XDev, YDev, dim, (const void *)pnt16, len, dtype);
        ret = bm_memcpy_d2s(handle, tpu_out_fp16, YDev);

        bm_free_device(handle, XDev);
        bm_free_device(handle, YDev);

        for (i = 0; i < len; ++i) {
        tpu_out[i] = fp16tofp32(tpu_out_fp16[i]);
        }
    }

    free(XHost16);
    free(tpu_out_fp16);
    free(XHost);
    free(tpu_out);

    bm_dev_free(handle);
    return ret;
}