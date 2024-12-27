#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "bmcv_api_ext_c.h"
#include <pthread.h>

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))


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

extern void topk_reference(int k, int batch, int batch_num, int batch_stride, int descending, bool bottom_index_valid,
                    float* bottom_data, int* bottom_index, float* top_data_ref, int* top_index_ref);

static int parameters_check(int k)
{
    if (k > 100){
        printf("Unsupported size : k_max = 100 \n");
        return -1;
    }
    return 0;
}


void fill(float* data, int* index, int batch, int batch_num, int batch_stride){
  for(int i = 0; i < batch; i++){
    for(int j = 0; j < batch_num; j++){
      data[i * batch_stride + j] = rand() % 10000 * 1.0f;
      index[i * batch_stride + j] = i * batch_stride + j;
    }
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
                  int descending, bool bottom_index_valid) {
  struct timeval t1, t2, t3;
  bm_status_t ret = BM_SUCCESS;
  printf("k: %d\n", k);
  printf("batch: %d\n", batch);
  printf("batch_num: %d\n", batch_num);
  printf("batch_stride: %d\n", batch_stride);
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
  printf("Batch_topk TPU using time = %ld(us)\n", TIME_COST_US(t2, t3));
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
      printf("Compare success for topk data and index!\n");
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
void* test_batch_topk(void* args) {
    batch_topk_thread_arg_t* batch_topk_thread_arg = (batch_topk_thread_arg_t*)args;
    bm_handle_t handle = batch_topk_thread_arg->handle;
    int loop_num = batch_topk_thread_arg->loop_num;
    int batch_num = batch_topk_thread_arg->batch_num;
    int k = batch_topk_thread_arg->k;
    int descending = batch_topk_thread_arg->descending;
    int batch = batch_topk_thread_arg->batch;
    bool bottom_index_valid = batch_topk_thread_arg->bottom_index_valid;
    int batch_stride = batch_topk_thread_arg->batch_stride;
    for (int i = 0; i < loop_num; i++) {
        if (0 != test_topk_v2(handle, k, batch, batch_num, batch_stride, descending, bottom_index_valid)) {
            printf("TEST BATCH_TOPK FAILED\n");
            exit(-1);
        }
        printf("------[TEST BATCH_TOPK] TEST BATCH_TOPK PASSED!------\n");
    }
    return (void*)0;
}

int main(int argc, char* args[]) {
  struct timespec tp;
  clock_gettime(0, &tp);
  int seed = tp.tv_nsec;
  srand(seed);
  int loop = 1;
  int batch_num = 10 + rand() % 999990;
  int k = 1 + rand() % 100;
  k = k < batch_num ? k : batch_num;
  int descending = rand() % 2;
  int batch = rand() % 32 + 1;
  bool bottom_index_valid = true;
  int thread_num = 1;
  int check = 0;

  if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: \n");
        printf("%s thread_num loop batch_num k batch descending bottom_index_valid \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 1\n", args[0]);
        printf("%s 1 1 50 20 2 0 0 \n", args[0]);
        return 0;
  }

  if (argc > 1) thread_num = atoi(args[1]);
  if (argc > 2) loop = atoi(args[2]);
  if (argc > 3) batch_num = atoi(args[3]);
  if (argc > 4) k = atoi(args[4]);
  if (argc > 5) batch = atoi(args[5]);
  if (argc > 6) descending = atoi(args[6]);
  if (argc > 7) bottom_index_valid = atoi(args[7]);
  check = parameters_check(k);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

  bm_handle_t handle;
  bm_status_t ret = bm_dev_request(&handle, 0);
  if (ret != BM_SUCCESS) {
    printf("Create bm handle failed. ret = %d\n", ret);
    return -1;
  }
  int batch_stride = batch_num;
  printf("seed = %d\n", seed);
  // test for multi-thread
  pthread_t pid[thread_num];
  batch_topk_thread_arg_t batch_topk_thread_arg[thread_num];
  for (int i = 0; i < thread_num; i++) {
      batch_topk_thread_arg[i].loop_num = loop;
      batch_topk_thread_arg[i].handle = handle;
      batch_topk_thread_arg[i].k = k;
      batch_topk_thread_arg[i].batch = batch;
      batch_topk_thread_arg[i].batch_num = batch_num;
      batch_topk_thread_arg[i].batch_stride = batch_stride;
      batch_topk_thread_arg[i].descending = descending;
      batch_topk_thread_arg[i].bottom_index_valid = bottom_index_valid;
      if (pthread_create(pid + i, NULL, test_batch_topk, batch_topk_thread_arg + i) != 0) {
          printf("create thread failed\n");
          return -1;
      }
  }
  for (int i = 0; i < thread_num; i++) {
      int ret = pthread_join(pid[i], NULL);
      if (ret != 0) {
          printf("Thread join failed\n");
          exit(-1);
      }
  }
  bm_dev_free(handle);
  return ret;
}
