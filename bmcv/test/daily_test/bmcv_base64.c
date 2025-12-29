#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "bmcv_api_ext_c.h"



int main() {
    int original_len = (rand() % 134217728) + 1; //128M
    int encoded_len = (original_len + 2) / 3 * 4;
    char* src = (char *)malloc((original_len + 3) * sizeof(char));
    char* dst = (char *)malloc((encoded_len + 3) * sizeof(char));
    for (int j = 0; j < original_len; j++){
        src[j] = (char)((rand() % 256) + 1);
    }
    bm_handle_t handle;
    int ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    unsigned long lenth[2];
    lenth[0] = (unsigned long)original_len;

    bmcv_base64_enc(handle, bm_mem_from_system(src), bm_mem_from_system(dst), lenth);
    bmcv_base64_dec(handle, bm_mem_from_system(dst), bm_mem_from_system(src), lenth);

    bm_dev_free(handle);
    free(src);
    free(dst);
    return 0;
}