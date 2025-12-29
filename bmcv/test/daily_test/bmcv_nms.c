#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"

int main() {
    int L = 50 + rand() % 260095;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    float min_tpu = 0, max_tpu = 0;
    float* XHost = (float*)malloc(L * sizeof(float));
    int i;

    for (i = 0; i < L; ++i)
        XHost[i] = (float)((rand() % 2 ? 1 : -1) * (rand() % 1000 + (rand() % 100000) * 0.01));

    bm_device_mem_t XDev;

    ret = bm_malloc_device_byte(handle, &XDev, L * sizeof(float));
    ret = bm_memcpy_s2d(handle, XDev, XHost);
    ret = bmcv_min_max(handle, XDev, &min_tpu, &max_tpu, L);
    if (ret != BM_SUCCESS) {
        printf("Calculate bm min_max failed. ret = %d\n", ret);
        return -1;
    }

    bm_free_device(handle, XDev);

    free(XHost);

    bm_dev_free(handle);
    return ret;
}
