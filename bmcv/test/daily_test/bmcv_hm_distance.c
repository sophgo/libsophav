#include <math.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bmcv_api_ext_c.h"

int main() {
    int bits_len = 8;
    int input1_num = 1 + rand() % 100;
    int input2_num = 1 + rand() % 2500;
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    bm_device_mem_t input1_dev_mem;
    bm_device_mem_t input2_dev_mem;
    bm_device_mem_t output_dev_mem;

    int* input1_data = (int*)malloc(input1_num * bits_len * sizeof(int));
    int* input2_data = (int*)malloc(input2_num * bits_len * sizeof(int));
    int* output_tpu  = (int*)malloc(input1_num * input2_num * sizeof(int));

    printf("bits_len is %u\n", bits_len);
    printf("input1_data len is %u\n", input1_num);
    printf("input2_data len is %u\n", input2_num);
    memset(input1_data, 0, input1_num * bits_len * sizeof(int));
    memset(input2_data, 0, input2_num * bits_len * sizeof(int));
    memset(output_tpu,  0,  input1_num * input2_num * sizeof(int));

    // fill data
    for(int i = 0; i < input1_num * bits_len; i++) {
        input1_data[i] = rand() % 10;
    }
    for(int i = 0; i < input2_num * bits_len; i++) {
        input2_data[i] = rand() % 20 + 1;
    }
    // tpu_cal
    bm_malloc_device_byte(handle, &input1_dev_mem, input1_num * bits_len * sizeof(int));
    bm_malloc_device_byte(handle, &input2_dev_mem, input2_num * bits_len * sizeof(int));
    bm_malloc_device_byte(handle, &output_dev_mem, input1_num * input2_num * sizeof(int));
    bm_memcpy_s2d(handle, input1_dev_mem, input1_data);
    bm_memcpy_s2d(handle, input2_dev_mem, input2_data);

    bmcv_hamming_distance(handle, input1_dev_mem, input2_dev_mem, output_dev_mem, bits_len, input1_num, input2_num);

    bm_memcpy_d2s(handle, output_tpu, output_dev_mem);

    for (int i = 0; i < 8; i++) {
        printf("output_tpu[%d] is: %d\n", i, output_tpu[i]);
    }
    free(input1_data);
    free(input2_data);
    free(output_tpu);
    bm_free_device(handle, input1_dev_mem);
    bm_free_device(handle, input2_dev_mem);
    bm_free_device(handle, output_dev_mem);

    bm_dev_free(handle);
    return ret;
}