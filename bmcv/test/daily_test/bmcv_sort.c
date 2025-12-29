#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "bmcv_api_ext_c.h"

#define MAX_SORT_NUM (500000)

typedef float bm_sort_data_type_t;
typedef enum { ASCEND_ORDER, DESCEND_ORDER } cdma_sort_order_e;

typedef struct {
    int   index;
    float val;
} __attribute__((packed)) sort_t;

int32_t main() {
    int dev_id = 0;
    int data_num = 1 + rand() % 500000;
    int sort_num = 1 + rand() % data_num;
    int ret = 0;
    bm_handle_t handle;
    cdma_sort_order_e order1 = DESCEND_ORDER;
    // cdma_sort_order_e order2 = ASCEND_ORDER;
    ret = bm_dev_request(&handle, dev_id);

    bm_sort_data_type_t *src_data = (bm_sort_data_type_t*)malloc(data_num * sizeof(float));
    int *src_index_p = (int*)malloc(data_num * sizeof(int));
    sort_t *ref_res = (sort_t*)malloc(data_num * sizeof(sort_t));
    sort_t *cdma_res = (sort_t*)malloc(sort_num * sizeof(sort_t));
    bm_sort_data_type_t *dst_data = (bm_sort_data_type_t*)malloc(sort_num * sizeof(bm_sort_data_type_t));
    int *dst_data_index = (int*)malloc(sort_num * sizeof(int));
    bool index_enable = rand() % 2 ? true : false;
    bool auto_index = rand() % 2 ? true : false;
    printf("data num: %d, sort num: %d\n", data_num, sort_num);

    // produce src data and index
    for (int32_t i = 0; i < data_num; i++) {
        if(auto_index){
        src_index_p[i] = i;
        }else{
        src_index_p[i] = rand() % MAX_SORT_NUM;
        }
        ref_res[i].index = src_index_p[i];
        ref_res[i].val = ((float)(rand() % MAX_SORT_NUM)) / 100;
        src_data[i] = ref_res[i].val;
    }

    int                 *dst_index_p = NULL;
    bm_sort_data_type_t *dst_data_p  = NULL;
    dst_index_p = (int*)malloc(sort_num * sizeof(int));
    dst_data_p = (bm_sort_data_type_t*)malloc(sort_num * sizeof(int));

    bmcv_sort(handle, bm_mem_from_system(src_index_p), bm_mem_from_system(src_data), data_num,
            bm_mem_from_system(dst_index_p), bm_mem_from_system(dst_data_p), sort_num, (int)order1,
            index_enable, auto_index);

    for (int i = 0; i < sort_num; i++) {
        cdma_res[i].index = dst_index_p[i];
        cdma_res[i].val   = dst_data_p[i];
    }
    free(dst_index_p);
    free(dst_data_p);

    // release memory
    free(src_data);
    free(src_index_p);
    free(ref_res);
    free(cdma_res);
    free(dst_data);
    free(dst_data_index);

    bm_dev_free(handle);
    return ret;
}
