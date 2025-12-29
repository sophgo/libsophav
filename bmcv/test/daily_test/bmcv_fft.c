#include "bmcv_api_ext_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int main()
{
    bm_handle_t handle;
    int ret = 0;
    int i;
    int L = 100;
    int batch = 100;
    bool forward = true;
    bool realInput = false;

    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    float* XRHost = (float*)malloc(L * batch * sizeof(float));
    float* XIHost = (float*)malloc(L * batch * sizeof(float));
    float* YRHost_tpu = (float*)malloc(L * batch * sizeof(float));
    float* YIHost_tpu = (float*)malloc(L * batch * sizeof(float));

    for (i = 0; i < L * batch; ++i) {
        XRHost[i] = (float)rand() / RAND_MAX;
        XIHost[i] = realInput ? 0 : ((float)rand() / RAND_MAX);
    }

    bm_device_mem_t XRDev, XIDev, YRDev, YIDev;
    void* plan = NULL;

    ret = bm_malloc_device_byte(handle, &XRDev, L * batch * sizeof(float));
    ret = bm_malloc_device_byte(handle, &XIDev, L * batch * sizeof(float));
    ret = bm_malloc_device_byte(handle, &YRDev, L * batch * sizeof(float));
    ret = bm_malloc_device_byte(handle, &YIDev, L * batch * sizeof(float));

    ret = bm_memcpy_s2d(handle, XRDev, XRHost);
    ret = bm_memcpy_s2d(handle, XIDev, XIHost);

    ret = bmcv_fft_2d_create_plan(handle, L, batch, forward, &plan);

    ret = bmcv_fft_execute(handle, XRDev, XIDev, YRDev, YIDev, plan);
    if (ret != BM_SUCCESS) {
        printf("bmcv_fft_execute failed!\n");
        if (plan != NULL) {
            bmcv_fft_destroy_plan(handle, plan);
        }
    }

    ret = bm_memcpy_d2s(handle, (void*)YRHost_tpu, YRDev);
    ret = bm_memcpy_d2s(handle, (void*)YIHost_tpu, YIDev);

    if (plan != NULL) {
        bmcv_fft_destroy_plan(handle, plan);
    }

    free(XRHost);
    free(XIHost);
    free(YRHost_tpu);
    free(YIHost_tpu);

    bm_dev_free(handle);
    return ret;
}