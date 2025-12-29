#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include "bmcv_api_ext_c.h"


int main() {
    int batch_num = 10 + rand() % 999990;
    int k = 1 + rand() % 100;
    k = k < batch_num ? k : batch_num;
    int descending = rand() % 2;
    int batch = rand() % 32 + 1;
    bool bottom_index_valid = true;

    printf("batch_topk_params: \n");
    printf("batch_num = %d, k = %d, descending = %d, batch = %d, bottom_index_valid = %d \n", batch_num, k, descending, batch, bottom_index_valid);
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
    printf("Create bm handle failed. ret = %d\n", ret);
    return -1;
    }

    int batch_stride = batch_num;

    float* bottom_data = (float*)malloc(batch * batch_stride * sizeof(float));
    int* bottom_index = (int*)malloc(batch * batch_stride * sizeof(int));
    float* top_data = (float*)malloc(batch * batch_stride * sizeof(float));
    int* top_index = (int*)malloc(batch * batch_stride * sizeof(int));
    float* top_data_ref = (float*)malloc(batch * k * sizeof(float));
    int* top_index_ref = (int*)malloc(batch * k * sizeof(int));
    float* buffer = (float*)malloc(3 * batch_stride * sizeof(float));

    for(int i = 0; i < batch; i++){
    for(int j = 0; j < batch_num; j++){
        bottom_data[i * batch_stride + j] = rand() % 10000 * 1.0f;
        bottom_index[i * batch_stride + j] = i * batch_stride + j;
    }
    }

    ret = bmcv_batch_topk(handle, bm_mem_from_system((void*)bottom_data), bm_mem_from_system((void*)bottom_index),
                    bm_mem_from_system((void*)top_data), bm_mem_from_system((void*)top_index),
                    bm_mem_from_system((void*)buffer), bottom_index_valid, k, batch, &batch_num,
                    true, batch_stride, descending);

    bm_dev_free(handle);

    free(bottom_data);
    free(bottom_index);
    free(top_data);
    free(top_data_ref);
    free(top_index);
    free(top_index_ref);
    free(buffer);

    return ret;
}