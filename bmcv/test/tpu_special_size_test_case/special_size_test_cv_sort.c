#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "bmcv_api_ext_c.h"

#ifdef __linux__
  #include <unistd.h>
  #include <sys/time.h>
#else
  #include <windows.h>
#endif

#define SORT_SUCCESS (0)
#define SORT_FAILED (-1)
#define MAX_SORT_NUM (500000)
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef float bm_sort_data_type_t;
typedef enum { ASCEND_ORDER, DESCEND_ORDER } cdma_sort_order_e;

typedef struct {
    int   index;
    float val;
} __attribute__((packed)) sort_t;

typedef struct {
    int loop_num;
    int data_num;
    int sort_num;
    bm_handle_t handle;
} sort_thread_arg_t;

bool isEqual(sort_t *cdma_res, sort_t *ref_res, int size) {
    for (int i = 0; i < size; i++) {
        if (cdma_res[i].val != ref_res[i].val) {
            printf("[val error] index: %d, cdma: %f, ref: %f\n", i, cdma_res[i].val, ref_res[i].val);
            return false;
        }
    }
    return true;
}

static void merge_ascend(sort_t ref_res[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    sort_t L[n1], R[n2];

    for (int i = 0; i < n1; i++) {
        L[i] = ref_res[left + i];
    }
    for (int j = 0; j < n2; j++) {
        R[j] = ref_res[mid + 1 + j];
    }

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].val <= R[j].val) {
            ref_res[k] = L[i];
            i++;
        } else {
        ref_res[k] = R[j];
        j++;
        }
        k++;
    }
    while (i < n1) {
        ref_res[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        ref_res[k] = R[j];
        j++;
        k++;
    }
}

static void merge_descend(sort_t ref_res[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    sort_t L[n1], R[n2];

    for (int i = 0; i < n1; i++) {
        L[i] = ref_res[left + i];
    }
    for (int j = 0; j < n2; j++) {
        R[j] = ref_res[mid + 1 + j];
    }

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].val >= R[j].val) {
            ref_res[k] = L[i];
            i++;
        } else {
            ref_res[k] = R[j];
            j++;
        }
        k++;
    }
    while (i < n1) {
        ref_res[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        ref_res[k] = R[j];
        j++;
        k++;
    }
}

static void mergeSort(sort_t ref_res[], int left, int right, bool is_ascend) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(ref_res, left, mid, is_ascend);
        mergeSort(ref_res, mid + 1, right, is_ascend);
        if (is_ascend) {
            merge_ascend(ref_res, left, mid, right);
        }else{
            merge_descend(ref_res, left, mid, right);
        }
    }
}

static bool push_unstable_map(const sort_t  *cdma_iter,
                              const sort_t  *ref_iter,
                              sort_t        *cdma_res,
                              int           *loop_cnt,
                              int            size) {
    sort_t *cdma_unstable_map = (sort_t*)malloc(size * sizeof(sort_t));
    sort_t *ref_unstable_map  = (sort_t*)malloc(size * sizeof(sort_t));
    float init_val = cdma_iter->val;
    int i = 0;
    while (cdma_iter != NULL) {
        float cdma_val   = cdma_iter->val;
        float ref_val    = ref_iter->val;
        int   cdma_index = cdma_iter->index;
        int   ref_index  = ref_iter->index;
        if (cdma_val != ref_val) {
            printf("cmp error in halfway!!!\r\n");
            free(cdma_unstable_map);
            free(ref_unstable_map);
            return false;
        } else if (cdma_val != init_val) {
            break;
        }
        cdma_unstable_map[i].index = cdma_index;
        cdma_unstable_map[i].val   = cdma_val;
        ref_unstable_map[i].index  = ref_index;
        ref_unstable_map[i].val    = ref_val;
        cdma_iter++;
        ref_iter++;
        *loop_cnt += 1;
        i++;
    }
    if (!isEqual(cdma_unstable_map,cdma_unstable_map,size)) {
        printf("cache map not equal!!!\r\n");
        free(cdma_unstable_map);
        free(ref_unstable_map);
        return false;
    }
    free(cdma_unstable_map);
    free(ref_unstable_map);
    return true;
}

static bool get_cdma_result(bm_handle_t                 handle,
                            cdma_sort_order_e           order,
                            int                         src_index[],
                            bm_sort_data_type_t        *src_data,
                            sort_t                     *cdma_res,
                            bool                        index_enable,
                            bool                        auto_index,
                            int                         data_num,
                            int                         sort_num,
                            long int *tpu_time) {
    struct timeval t1, t2;
    bm_sort_data_type_t *src_data_p  = src_data;
    int                 *src_index_p = src_index;
    int                  data_cnt    = data_num;
    int                  sort_cnt    = sort_num;
    int                 *dst_index_p = NULL;
    bm_sort_data_type_t *dst_data_p  = NULL;
    dst_index_p = (int*)malloc(sort_num * sizeof(int));
    dst_data_p = (bm_sort_data_type_t*)malloc(sort_num * sizeof(int));

    //printf("index_enable: %d, auto_index: %d\n",index_enable, auto_index);
    gettimeofday(&t1, NULL);
    bmcv_sort(handle, bm_mem_from_system(src_index_p), bm_mem_from_system(src_data_p), data_cnt,
              bm_mem_from_system(dst_index_p), bm_mem_from_system(dst_data_p), sort_cnt, (int)order,
              index_enable, auto_index);
    gettimeofday(&t2, NULL);
    printf("Sort TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);
    for (int i = 0; i < sort_cnt; i++) {
        cdma_res[i].index = dst_index_p[i];
        cdma_res[i].val   = dst_data_p[i];
    }
    free(dst_index_p);
    free(dst_data_p);
    return true;
}

static bool unstable_process(sort_t *cdma_res, sort_t *ref_res, int sort_num) {
    sort_t *cdma_iter_temp = &cdma_res[0];
    sort_t *ref_iter_temp  = &ref_res[0];
    bool push_res = true;
    int size = sort_num;

    for (int i = 0; i < size; i++) {
        if (cdma_iter_temp->index != ref_iter_temp->index) {
            int temp_i = i;
            push_res   = push_unstable_map(cdma_iter_temp, ref_iter_temp, cdma_res, &i, size);
            if (false == push_res) {
                printf("cmp error[%d],res_index:%d,res_val:%f,ref_index:%d,ref_val:%f\r\n",i,
                       cdma_res[i].index, cdma_res[i].val, ref_res[i].index, ref_res[i].val);
                return false;
            }
            int step       = i - temp_i;
            cdma_iter_temp = cdma_iter_temp + step;
            ref_iter_temp  = ref_iter_temp + step;
        } else {
            cdma_iter_temp++;
            ref_iter_temp++;
        }
    }

    return true;
}

static bool result_compare(sort_t *cdma_res, sort_t *ref_res, bool index_enable, int sort_num) {
    bool unstable_res = true;
    int size =  sort_num;

    for (int i = 0; i < size; i++) {
        if (cdma_res[i].val != ref_res[i].val) {
            printf("[val error] index: %d, got: %f, ref: %f\n", i, cdma_res[i].val, ref_res[i].val);
            return false;
        }
    }
    if (!index_enable) {
        return true;
    } else {
        if (isEqual(cdma_res, ref_res, size)) {
            return true;
        } else {
            unstable_res = unstable_process(cdma_res, ref_res, sort_num);
        }
        return unstable_res;
    }
}

int32_t cv_sort_test_rand(bm_handle_t handle, cdma_sort_order_e order, int data_number, int sort_number,
                          long int *cpu_time, long int *tpu_time, int index_enable, int auto_index) {
    struct timeval t1, t2;
    int data_num = data_number;
    int sort_num = sort_number;
    bm_sort_data_type_t *src_data = (bm_sort_data_type_t*)malloc(data_num * sizeof(float));
    int *src_data_index = (int*)malloc(data_num * sizeof(int));
    sort_t *ref_res = (sort_t*)malloc(data_num * sizeof(sort_t));
    sort_t *cdma_res = (sort_t*)malloc(sort_num * sizeof(sort_t));
    bm_sort_data_type_t *dst_data = (bm_sort_data_type_t*)malloc(sort_num * sizeof(bm_sort_data_type_t));
    int *dst_data_index = (int*)malloc(sort_num * sizeof(int));

    //printf("data num: %d, sort num: %d\n", data_num, sort_num);

    // produce src data and index
    for (int32_t i = 0; i < data_num; i++) {
        if(auto_index){
          src_data_index[i] = i;
        }else{
          src_data_index[i] = rand() % MAX_SORT_NUM;
        }
        ref_res[i].index = src_data_index[i];
        ref_res[i].val = ((float)(rand() % MAX_SORT_NUM)) / 100;
        src_data[i] = ref_res[i].val;
    }
    // cdma_result
    get_cdma_result(handle, order, src_data_index, src_data, cdma_res,
                    index_enable, auto_index, data_num, sort_num, tpu_time);
    // ref result
    int size = data_num;
    gettimeofday(&t1, NULL);
    if (order == ASCEND_ORDER) {
        mergeSort(ref_res, 0, size - 1, true);
    } else {
        mergeSort(ref_res, 0, size - 1, false);
    }
    gettimeofday(&t2, NULL);
    printf("Sort CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *cpu_time += TIME_COST_US(t1, t2);
    //result_compare
    if (true != result_compare(cdma_res, ref_res, index_enable, sort_num)) {
        printf("----SORT TEST ERROR!!!----\r\n");
        free(src_data);
        free(src_data_index);
        free(ref_res);
        free(cdma_res);
        free(dst_data);
        free(dst_data_index);
        return -1;
    }

    // release memory
    free(src_data);
    free(src_data_index);
    free(ref_res);
    free(cdma_res);
    free(dst_data);
    free(dst_data_index);
    return SORT_SUCCESS;
}

int32_t main(int32_t argc, char **argv) {
    printf("log will be saved in special_size_test_cv_sort.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_sort.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    int dev_id = 0;
    int special_data_num[] = {1, 10, 100, 1000, 10000, 100000, 500000};
    int special_sort_num[] = {1, 10, 100, 1000, 10000, 100000, 500000};
    bool index_enable = rand() % 2 ? true : false;
    bool auto_index = rand() % 2 ? true : false;
    int ret = 0;
    bm_handle_t handle;
    bm_status_t req = bm_dev_request(&handle, dev_id);
    if (req != BM_SUCCESS) {
        printf("create bm handle for dev%d failed!\n", dev_id);
        exit(-1);
    }
    for (int i = 0; i < 7; i++) {
        int data_num = special_data_num[i];
        for (int j = 0; j < 7; j++) {
            int sort_num = special_sort_num[j];
            if (sort_num <= data_num) {
                long int cputime = 0;
                long int tputime = 0;
                long int *cpu_time = &cputime;
                long int *tpu_time = &tputime;

                printf("data_num = %d, sort_num = %d, index_enable = %d, auto_index = %d\n", data_num, sort_num, index_enable, auto_index);
                for (int loop = 0; loop < 3; loop++) {
                    if (-1 == cv_sort_test_rand(handle, DESCEND_ORDER, data_num, sort_num, cpu_time, tpu_time, index_enable, auto_index)) {
                        printf("TEST SORT FAILED\n");
                        return -1;
                    }
                }
                printf("------average CPU time = %ldus------\n", cputime / 3);
                printf("------average TPU time = %ldus------\n", tputime / 3);
            }
        }
    }
    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}