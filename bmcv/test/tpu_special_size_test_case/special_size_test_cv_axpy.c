#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"
// #include "test_misc.h"
#ifdef __linux__
  #include <pthread.h>
  #include <unistd.h>
  #include <sys/time.h>
#else
  #include <windows.h>
#endif

#define B_CONST_VALUE 0
#define S_CONST_VALUE -0.2f

#define CEHCK(x) { \
     if (x) {} \
     else {fprintf(stderr, "Check failed: %s, file %s, line: %d\n", #x, __FILE__, __LINE__); return -1; } \
}

static int N = 1;
static int C = 64; //(64 * 2 + (64 >> 1))
static int H = 64;
static int W = 64;
// #define TENSOR_SIZE N * C * H * W
#define BMDNN_COMPARE_EPSILON (1e-4)
#define bm_min(x, y) (((x)) < ((y)) ? (x) : (y))
#define bm_max(x, y) (((x)) > ((y)) ? (x) : (y))
#define IS_NAN(x) ((((x >> 23) & 0xff) == 0xff) && ((x & 0x7fffff) != 0))

#define BM_API_ID_MD_LINEAR 5

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef union {
  int ival;
  float fval;
} IF_VAL;

typedef struct {
    int trials;
    bm_handle_t handle;
} cv_axpy_thread_arg_t;

int array_cmp_axpy(float *p_exp, float *p_got, int len, const char *info_label, float delta) {
  int idx = 0;
  int total = 0;
  for (idx = 0; idx < len; idx++) {
    if (bm_max(fabs(p_exp[idx]), fabs(p_got[idx])) > 1.0) {
      // compare rel
      if (bm_min(fabs(p_exp[idx]), fabs(p_got[idx])) < 1e-20) {
        printf("%s rel error at index %d exp %.20f got %.20f\n", info_label, idx, p_exp[idx], p_got[idx]);
        total++;
        if (1024 < total) {return -1;}
      }
      if (fabs(p_exp[idx] - p_got[idx]) / bm_min(fabs(p_exp[idx]), fabs(p_got[idx])) > delta) {
        printf("%s rel error at index %d exp %.20f got %.20f\n", info_label, idx, p_exp[idx], p_got[idx]);
        total++;
        if (1024 < total) {return -1;}
      }
    } else {
      // compare abs
      if (fabs(p_exp[idx] - p_got[idx]) > delta) {
        printf("%s abs error at index %d exp %.20f got %.20f\n", info_label, idx, p_exp[idx], p_got[idx]);
        total++;
        if (1024 < total) {return -1;}
      }
    }

    IF_VAL if_val_exp, if_val_got;
    if_val_exp.fval = p_exp[idx];
    if_val_got.fval = p_got[idx];
    if (IS_NAN(if_val_got.ival) && !IS_NAN(if_val_exp.ival)) {
      printf("There are nans in %s idx %d\n", info_label, idx);
      printf("floating form exp %.10f got %.10f\n", if_val_exp.fval, if_val_got.fval);
      printf("hex form exp %8.8x got %8.8x\n", if_val_exp.ival, if_val_got.ival);
      return -2;
    }
  }
  if (0 < total) {return -1;}
  return 0;
}

static int test_axpy_random(int trials, bm_handle_t handle, long int* cpu_time, long int* tpu_time)
{
  float *tensor_X = malloc(N * C * H * W * sizeof(float));
  float *tensor_A = malloc(N * C * sizeof(float));
  float *tensor_Y = malloc(N * C * H * W * sizeof(float));
  float *tensor_F = malloc(N * C * H * W * sizeof(float));
  float *tensor_F_cmp = malloc(N * C * H * W * sizeof(float));
  int ret = 0;
  int idx_trial;
  struct timeval t1, t2, t3;

  for (idx_trial = 0; idx_trial < trials; idx_trial++) {
        gettimeofday(&t1, NULL);
        for (int idx = 0; idx < N * C * H * W; idx++) {
            tensor_X[idx] = (float)idx - 5.0f;
            tensor_Y[idx] = (float)idx / 3.0f - 8.2f; //y
        }

        for (int idx = 0; idx < N * C; idx++) {
            tensor_A[idx] = (float)idx * 1.5f + 1.0f;
        }

        memset(tensor_F, 0, sizeof(float)*N * C * H * W);
        memset(tensor_F_cmp, 0, sizeof(float)*N * C * H * W);
        for (int i=0; i< N; i++){
            for (int j=0; j <C; j++){
                for (int k=0; k<H*W; k++){
                    tensor_F_cmp[i*C*H*W + j*H*W +k] = tensor_A[i*C+j] * tensor_X[i*C*H*W + j*H*W +k] + tensor_Y[i*C*H*W + j*H*W +k];
                }
            }
        }
        gettimeofday(&t2, NULL);
        printf("The %d trials, Axpy CPU using time = %ld(us)\n", idx_trial, TIME_COST_US(t1, t2));
        *cpu_time += TIME_COST_US(t1, t2);
        ret = bmcv_image_axpy(handle,
                              bm_mem_from_system((void *)tensor_A),
                              bm_mem_from_system((void *)tensor_X),
                              bm_mem_from_system((void *)tensor_Y),
                              bm_mem_from_system((void *)tensor_F),
                              N, C, H, W);
        gettimeofday(&t3, NULL);
        printf("The %d trials, Axpy TPU using time = %ld(us)\n", idx_trial, TIME_COST_US(t2, t3));
        *tpu_time += TIME_COST_US(t2, t3);
        if (ret){
            printf("test Axpy failed \n");
            ret = BM_ERR_FAILURE;
            break;
        } else {
            int cmp_res = array_cmp_axpy(
                            (float*)tensor_F_cmp,
                            (float*)tensor_F,
                            N * C * H * W, "axpy", BMDNN_COMPARE_EPSILON);
            if ( cmp_res != 0) {
                printf("Compare TPU with CPU: error, not equal, cmp fail \n");
                ret = BM_ERR_FAILURE;
                break;
            } else {
                printf("Compare TPU with CPU: they are equal,cmp success \n");
            }
        }
    }

    free(tensor_X);
    free(tensor_A);
    free(tensor_Y);
    free(tensor_F);
    free(tensor_F_cmp);
    return ret;
}

// void* test_axpy(void* args) {
//     cv_axpy_thread_arg_t* cv_axpy_thread_arg = (cv_axpy_thread_arg_t*)args;
//     int trials = cv_axpy_thread_arg->trials;
//     bm_handle_t handle = cv_axpy_thread_arg->handle;

//     int ret = test_axpy_random(trials, handle);
//     if (ret) {
//         printf("------Test axpy Failed!------\n");
//         return (void*)-1;
//     }
//     printf("------Test axpy Successed!------\n");
//     return (void*)0;
// }

// int main(int argc, char* args[]){
//   struct timespec tp;
//   clock_gettime(0, &tp);
//   int seed = tp.tv_nsec;
//   srand(seed);
//   int thread_num = 1;
//   int trials = 1;
//   int ret = 0;
//   bm_handle_t handle;
//   ret = bm_dev_request(&handle, 0);
//   if (ret != BM_SUCCESS) {
//       printf("bm_dev_request failed. ret = %d\n", ret);
//       return -1;
//   }

//   if (argc == 2 && atoi(args[1]) == -1) {
//     printf("%s thread_num trials \n", args[0]);
//     printf("example:\n");
//     printf("%s \n", args[0]);
//     printf("%s 2\n", args[0]);
//     printf("%s 1 3\n", args[0]);
//     return 0;
//   }
//   if (argc > 1) thread_num = atoi(args[1]);
//   if (argc > 2) trials = atoi(args[2]);

//   printf("seed = %d\n", seed);
//     // test for multi-thread
//     pthread_t pid[thread_num];
//     cv_axpy_thread_arg_t cv_axpy_thread_arg[thread_num];
//     for (int i = 0; i < thread_num; i++) {
//         cv_axpy_thread_arg[i].trials = trials;
//         cv_axpy_thread_arg[i].handle = handle;
//         if (pthread_create(&pid[i], NULL, test_axpy, &cv_axpy_thread_arg[i]) != 0) {
//             printf("create thread failed\n");
//             return -1;
//         }
//     }
//     for (int i = 0; i < thread_num; i++) {
//         ret = pthread_join(pid[i], NULL);
//         if (ret != 0) {
//             printf("Thread join failed\n");
//             exit(-1);
//         }
//     }

//     bm_dev_free(handle);
//     return ret;
// }

int main(int argc, char* args[]){
    printf("log will be saved in special_size_test_cv_axpy.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_axpy.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return -1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int trials = 1;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    int N_num[5] = {1, 2, 4, 8, 10};
    int C_num[5] = {1, 2, 4, 8, 10};
    int H_num[6] = {1, 16, 32, 64, 128, 256};
    int W_num[6] = {1, 16, 32, 64, 128, 256};
    for (int i = 0; i < 5; ++i) {
        N = N_num[i];
        C = C_num[i];
        for (int j = 0; j < 6; ++j) {
            H = H_num[j];
            W = W_num[j];
            long int cputime = 0;
            long int tputime = 0;
            printf("the N = %d, C = %d, H = %d, W = %d\n", N, C, H, W);
            for (int loop = 0; loop < 10; ++loop) {
                ret = test_axpy_random(trials, handle, &cputime, &tputime);
                if (ret) {
                    printf("test_axpy_random failed\n");
                    return -1;
                }
            }
            printf("-----------average AXPY CPU time = %ldus----------\n", cputime / 10);
            printf("-----------average AXPY TPU time = %ldus----------\n", tputime / 10);
        }
    }
    fclose(stdout);
    stdout = original_stdout;
    bm_dev_free(handle);
    return ret;
}