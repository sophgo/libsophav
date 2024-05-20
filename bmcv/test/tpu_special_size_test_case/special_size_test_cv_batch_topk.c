#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "bmcv_api_ext_c.h"
#include <pthread.h>

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int first;
    float second;
} Pair;

typedef struct {
    int loop_num;
    int k;
    int batch;
    int batch_num;
    int batch_stride;
    int descending;
    int bottom_index_valid;
    bm_handle_t handle;
} batch_topk_thread_arg_t;


static void merge_ascend(Pair ref_res[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    Pair L[n1], R[n2];

    for (int i = 0; i < n1; i++) {
        L[i] = ref_res[left + i];
    }
    for (int j = 0; j < n2; j++) {
        R[j] = ref_res[mid + 1 + j];
    }

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].second <= R[j].second) {
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

static void merge_descend(Pair ref_res[], int left, int mid, int right) {
  int n1 = mid - left + 1;
  int n2 = right - mid;
  Pair L[n1], R[n2];

  for (int i = 0; i < n1; i++) {
    L[i] = ref_res[left + i];
  }
  for (int j = 0; j < n2; j++) {
    R[j] = ref_res[mid + 1 + j];
  }

  int i = 0, j = 0, k = left;
  while (i < n1 && j < n2) {
    if (L[i].second >= R[j].second) {
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

static void merge_sort(Pair ref_res[], int left, int right, bool is_ascend) {
  if (left < right) {
    int mid = left + (right - left) / 2;
    merge_sort(ref_res, left, mid, is_ascend);
    merge_sort(ref_res, mid + 1, right, is_ascend);
    if (is_ascend) {
      merge_ascend(ref_res, left, mid, right);
    }else{
      merge_descend(ref_res, left, mid, right);
    }
  }
}

void fill(float* data, int* index, int batch, int batch_num, int batch_stride){
  for(int i = 0; i < batch; i++){
    for(int j = 0; j < batch_num; j++){
      data[i * batch_stride + j] = rand() % 10000 * 1.0f;
      index[i * batch_stride + j] = i * batch_stride + j;
    }
  }
}

void topk_reference(int k, int batch, int batch_num, int batch_stride, int descending, bool bottom_index_valid,
                    float* bottom_data, int* bottom_index, float* top_data_ref, int* top_index_ref) {
  for (int b = 0; b < batch; b++) {
    Pair* bottom_vec = (Pair*)malloc(batch_num * sizeof(Pair));
    for (int i = 0; i < batch_num; i++) {
      int offset = b * batch_stride + i;
      float data = bottom_data[offset];
      if (bottom_index_valid) {
        bottom_vec[i].first = bottom_index[offset];
        bottom_vec[i].second = data;
      } else {
        bottom_vec[i].first = i;
        bottom_vec[i].second = data;
      }
    }

    if (descending) {
      merge_sort(bottom_vec, 0, batch_num - 1, 0);
    } else {
      merge_sort(bottom_vec, 0, batch_num - 1, 1);
    }

    for (int i = 0; i < k; i++) {
      top_index_ref[b * k + i] = bottom_vec[i].first;
      top_data_ref[b * k + i] = bottom_vec[i].second;
    }
    free(bottom_vec);
  }
}

static int cmp_float(const float* ref, const float* got, int size) {
  for (int i = 0; i < size; i++) {
    if (ref[i] != got[i]) {
      printf("cmp error[%d]: ref:%f, got:%f", i, ref[i], got[i]);
      return -1;
    }
  }
  return 0;
}

static int cmp_int(const int* ref, const int* got, int size) {
  for (int i = 0; i < size; i++) {
    if (ref[i] != got[i]) {
      printf("cmp error[%d]: ref:%d, got:%d", i, ref[i], got[i]);
      return -1;
    }
  }
  return 0;
}

int test_topk_v2 (bm_handle_t handle, int k, int batch, int batch_num, int batch_stride,
                  int descending, bool bottom_index_valid, long int *cpu_time, long int *tpu_time) {
  struct timeval t1, t2, t3;
  bm_status_t ret = BM_SUCCESS;

  float* bottom_data = (float*)malloc(batch * batch_stride * sizeof(float));
  int* bottom_index = (int*)malloc(batch * batch_stride * sizeof(int));
  float* top_data = (float*)malloc(batch * batch_stride * sizeof(float));
  int* top_index = (int*)malloc(batch * batch_stride * sizeof(int));
  float* top_data_ref = (float*)malloc(batch * k * sizeof(float));
  int* top_index_ref = (int*)malloc(batch * k * sizeof(int));
  float* buffer = (float*)malloc(3 * batch_stride * sizeof(float));

  fill(bottom_data, bottom_index, batch, batch_num, batch_stride);
  gettimeofday(&t1, NULL);
  topk_reference(k, batch, batch_num, batch_stride, descending, bottom_index_valid, bottom_data,
                 bottom_index, top_data_ref, top_index_ref);
  gettimeofday(&t2, NULL);
  ret = bmcv_batch_topk(handle, bm_mem_from_system((void*)bottom_data), bm_mem_from_system((void*)bottom_index),
                        bm_mem_from_system((void*)top_data), bm_mem_from_system((void*)top_index),
                        bm_mem_from_system((void*)buffer), bottom_index_valid, k, batch, &batch_num,
                        true, batch_stride, descending);
  gettimeofday(&t3, NULL);
  printf("Batch_topk CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
  *cpu_time += TIME_COST_US(t1, t2);
  printf("Batch_topk TPU using time = %ld(us)\n", TIME_COST_US(t2, t3));
  *tpu_time += TIME_COST_US(t2, t3);
  if (ret != BM_SUCCESS) {
    printf("batch topk false \n");
    ret = BM_ERR_FAILURE;
    goto exit;
  }
  if (ret == BM_SUCCESS) {
    int data_cmp = -1;
    int index_cmp = -1;
    data_cmp = cmp_float(top_data_ref, top_data, batch * k);
    index_cmp = cmp_int(top_index_ref, top_index, batch * k);
    if (data_cmp == 0 && index_cmp == 0) {
      //printf("Compare success for topk data and index!\n");
    } else {
      printf("Compare failed for topk data and index!\n");
      ret = BM_ERR_FAILURE;
      goto exit;
    }
  } else {
    printf("Compare failed for topk data and index!\n");
    ret = BM_ERR_FAILURE;
    goto exit;
  }

exit:
  free(bottom_data);
  free(bottom_index);
  free(top_data);
  free(top_data_ref);
  free(top_index);
  free(top_index_ref);
  free(buffer);
  return ret;
}

int main(int argc, char* args[]) {
  printf("log will be saved in special_size_test_cv_batch_topk.txt\n");
  FILE *original_stdout = stdout;
  FILE *file = freopen("special_size_test_cv_batch_topk.txt", "w", stdout);
  if (file == NULL) {
      fprintf(stderr,"无法打开文件\n");
      return 1;
  }
  struct timespec tp;
  clock_gettime(0, &tp);
  int seed = tp.tv_nsec;
  srand(seed);
  int special_batch_num[6] = {10, 100, 1000, 10000, 100000, 1000000};
  int speical_k[3] = {1, 10, 100};
  int speical_batch[4] = {1, 10, 20, 32};
  int descending = 0;
  bool bottom_index_valid = true;

  bm_handle_t handle;
  bm_status_t ret = bm_dev_request(&handle, 0);
  if (ret != BM_SUCCESS) {
    printf("Create bm handle failed. ret = %d\n", ret);
    return -1;
  }
  for(int n = 0; n < 4; n++){
    int batch = speical_batch[n];
    for (int i = 0; i < 6; i++) {
      int batch_num = special_batch_num[i];
      for (int j = 0; j < 3; j++) {
        int k = speical_k[j];
        k = k <= batch_num ? k : batch_num;
        int batch_stride = batch_num;
        long int cputime = 0;
        long int tputime = 0;
        long int *cpu_time = &cputime;
        long int *tpu_time = &tputime;
        printf("k = %d, batch_num = %d, batch = %d\n", k, batch_num, batch);
        for (int i = 0; i < 3; i++) {
          if (0 != test_topk_v2(handle, k, batch, batch_num, batch_stride, descending, bottom_index_valid, cpu_time, tpu_time)) {
              printf("TEST BATCH_TOPK FAILED\n");
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
