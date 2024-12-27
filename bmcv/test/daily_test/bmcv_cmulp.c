#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    int L = 1 + rand() % 4096;
    int batch = 1 + rand() % 1980;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    float* XRHost = (float*)malloc(L * batch * sizeof(float));
    float* XIHost = (float*)malloc(L * batch * sizeof(float));
    float* PRHost = (float*)malloc(L * sizeof(float));
    float* PIHost = (float*)malloc(L * sizeof(float));
    float* tpu_YR = (float*)malloc(L * batch * sizeof(float));
    float* tpu_YI = (float*)malloc(L * batch * sizeof(float));
    int i;

    for (i = 0; i < L * batch; ++i) {
        XRHost[i] = rand() % 5 - 2;
        XIHost[i] = rand() % 5 - 2;
    }
    for (i = 0; i < L; ++i) {
        PRHost[i] = rand() % 5 - 2;
        PIHost[i] = rand() % 5 - 2;
    }

    bm_device_mem_t XRDev, XIDev, PRDev, PIDev, YRDev, YIDev;

    ret = bm_malloc_device_byte(handle, &XRDev, L * batch * sizeof(float));
    ret = bm_malloc_device_byte(handle, &XIDev, L * batch * sizeof(float));
    ret = bm_malloc_device_byte(handle, &PRDev, L * sizeof(float));
    ret = bm_malloc_device_byte(handle, &PIDev, L * sizeof(float));
    ret = bm_malloc_device_byte(handle, &YRDev, L * batch * sizeof(float));
    ret = bm_malloc_device_byte(handle, &YIDev, L * batch * sizeof(float));
    ret = bm_memcpy_s2d(handle, XRDev, XRHost);
    ret = bm_memcpy_s2d(handle, XIDev, XIHost);
    ret = bm_memcpy_s2d(handle, PRDev, PRHost);
    ret = bm_memcpy_s2d(handle, PIDev, PIHost);

    ret = bmcv_cmulp(handle, XRDev, XIDev, PRDev, PIDev, YRDev, YIDev, batch, L);
    if (ret != BM_SUCCESS) {
        printf("bmcv_cmulp failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_memcpy_d2s(handle, tpu_YR, YRDev);
    ret = bm_memcpy_d2s(handle, tpu_YI, YIDev);

    if (ret) {
        printf("the tpu cuml failed!, ret = %d\n", ret);
        return ret;
    }

    printf("test whether correct: XRHOST=%f, XIHOST=%f, PRHOST=%f, PIHOST=%f, tpu_YR=%f, tpu_YI=%f\n", XRHost[0], XIHost[0], PRHost[0], PIHost[0], tpu_YR[0], tpu_YI[0]);

    bm_free_device(handle, XRDev);
    bm_free_device(handle, XIDev);
    bm_free_device(handle, YRDev);
    bm_free_device(handle, YIDev);
    bm_free_device(handle, PRDev);
    bm_free_device(handle, PIDev);
    free(XRHost);
    free(XIHost);
    free(PRHost);
    free(PIHost);
    free(tpu_YR);
    free(tpu_YI);

    bm_dev_free(handle);
    return ret;
}