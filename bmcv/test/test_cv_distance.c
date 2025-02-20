#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_misc.h"
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#define DIM_MAX 8
#define ERR_MAX 1e-3
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))
#define max(a, b) ((a) > (b) ? (a) : (b))

enum op {
    FP32 = 0,
    FP16 = 1,
};

typedef struct {
    int loop;
    int len;
    int dim;
    enum op op;
    bm_handle_t handle;
} cv_distance_thread_arg_t;

extern int cpu_distance_fp32(float* XHost, float* cpu_out, int L, int dim, float pnt32[]);
extern int cpu_distance_fp16(fp16* XHost16, fp16* cpu_out_fp16, int L,
                            int dim, fp16 pnt16[]);

static int parameters_check(int len, int dim, enum op op_value)
{
    int error = 0;
    if (len > 40960){
        printf("Unsupported size : len_max = 40960 \n");
        error = -1;
    }
    if (dim > 8) {
        printf("Unsupported dim : dim = 1~8 \n");
        error = -1;
    }
    if (op_value != 0 && op_value != 1) {
        printf("Unsupported size : op_FP32 = 0, op_FP16 = 1 \n");
        error = -1;
    }
    return error;
}

static int tpu_distance_fp32(float* XHost, float* YHost, int L,int dim, float pnt32[], bm_handle_t handle)
{
    enum bm_data_type_t dtype = DT_FP32;
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t XDev, YDev;
    struct timeval t1, t2;

    ret = bm_malloc_device_byte(handle, &XDev, L * dim * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bmcv malloc device XDEV mem failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_malloc_device_byte(handle, &YDev, L * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bmcv malloc device YDEV mem failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_memcpy_s2d(handle, XDev, XHost);
    if (ret != BM_SUCCESS) {
        printf("bmcv memcpy s2d failed. ret = %d\n", ret);
        return -1;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_distance(handle, XDev, YDev, dim, pnt32, L, dtype);
    if (ret != BM_SUCCESS) {
        printf("bmcv distance failed. ret = %d\n", ret);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("Distance TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = bm_memcpy_d2s(handle, YHost, YDev);
    if (ret != BM_SUCCESS) {
        printf("bmcv memcpy d2s failed. ret = %d\n", ret);
        return -1;
    }

    bm_free_device(handle, XDev);
    bm_free_device(handle, YDev);


    return 0;
}

static int tpu_distance_fp16(fp16* XHost16, fp16* YHost16, int L,
                            int dim, fp16 pnt16[], bm_handle_t handle)
{
    enum bm_data_type_t dtype = DT_FP16;
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t XDev, YDev;
    struct timeval t1, t2;

    ret = bm_malloc_device_byte(handle, &XDev, L * dim * sizeof(float) / 2);
    if (ret != BM_SUCCESS) {
        printf("bmcv malloc device XDEV mem failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_malloc_device_byte(handle, &YDev, L * sizeof(float) / 2);
    if (ret != BM_SUCCESS) {
        printf("bmcv malloc device XDEV mem failed. ret = %d\n", ret);
        return -1;
    }

    ret = bm_memcpy_s2d(handle, XDev, XHost16);
    if (ret != BM_SUCCESS) {
        printf("bmcv memcpy s2d failed. ret = %d\n", ret);
        return -1;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_distance(handle, XDev, YDev, dim, (const void *)pnt16, L, dtype);
    if (ret != BM_SUCCESS) {
        printf("bmcv distance failed. ret = %d\n", ret);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("Distance TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = bm_memcpy_d2s(handle, YHost16, YDev);
    if (ret != BM_SUCCESS) {
        printf("bmcv memcpy d2s failed. ret = %d\n", ret);
        return -1;
    }

    bm_free_device(handle, XDev);
    bm_free_device(handle, YDev);
    return 0;
}

static int cmp_result(float* tpu_out, float* cpu_out, int len)
{
    int i;
    float devia;

    for(i = 0; i < len; ++i) {
        devia = fabs(tpu_out[i] - cpu_out[i]) / max(tpu_out[i], cpu_out[i]);

        if (devia >= ERR_MAX) {
            printf("the index =%d, the tpu_out = %f, the cpu_out = %f, the devia = %f, \
                    the err_max is %f, failed!\n",
                    i, tpu_out[i], cpu_out[i], devia, ERR_MAX);
            return -1;
        }
    }
    return 0;
}

static int test_distance(int dim, int L, int op, bm_handle_t handle)
{
    float pnt32[DIM_MAX] = {0};
    float* XHost = (float*)malloc(L * dim * sizeof(float));
    float* tpu_out = (float*)malloc(L * sizeof(float));
    float* cpu_out = (float*)malloc(L * sizeof(float));
    fp16* XHost16 = (fp16*)malloc(L * dim * sizeof(fp16));
    fp16* tpu_out_fp16 = (fp16*)malloc(L * sizeof(fp16));
    fp16* cpu_out_fp16 = (fp16*)malloc(L * sizeof(fp16));
    fp16 pnt16[DIM_MAX] = {0};
    int round = 1;
    int ret = 0;
    int i;
    struct timeval t1, t2;
    printf("len = %d, dim = %d, op = %d\n", L, dim, op);
    for (i = 0; i < dim; ++i) {
        pnt32[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
        pnt16[i] = fp32tofp16(pnt32[i], round);
    }

    for (i = 0; i < L * dim; ++i) {
        XHost[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
        XHost16[i] = fp32tofp16(XHost[i], round);
    }

    if (op == FP32) {
        ret = tpu_distance_fp32(XHost, tpu_out, L, dim, pnt32, handle);
        if (ret) {
            printf("tpu calc distance fp32 error!\n");
            return -1;
        }
        gettimeofday(&t1, NULL);
        ret = cpu_distance_fp32(XHost, cpu_out, L, dim, pnt32);
        if (ret) {
            printf("cpu calc distance fp32 error!\n");
            return -1;
        }
        gettimeofday(&t2, NULL);
        printf("Distance CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

        ret = cmp_result(tpu_out, cpu_out, L);
        if (ret) {
            printf("the tpu_distance_fp32 & cpu_distance_fp32 cmp failed, ret = %d\n", ret);
            return -1;
        }
        printf("Compare fp32 TPU with CPU success!\n");
    } else {
        ret = tpu_distance_fp16(XHost16, tpu_out_fp16, L, dim, pnt16, handle);
        if (ret) {
            printf("tpu calc distance fp16 error!\n");
            return -1;
        }
        gettimeofday(&t1, NULL);
        ret = cpu_distance_fp16(XHost16, cpu_out_fp16, L, dim, pnt16);
        if (ret) {
            printf("cpu calc distance fp16 error!\n");
            return -1;
        }
        gettimeofday(&t2, NULL);
        printf("Distance CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

        for (i = 0; i < L; ++i) {
            tpu_out[i] = fp16tofp32(tpu_out_fp16[i]);
            cpu_out[i] = fp16tofp32(cpu_out_fp16[i]);
        }

        ret = cmp_result(tpu_out, cpu_out, L);
        if (ret) {
            printf("the tpu_distance_fp16 & cpu_distance_fp16 cmp failed, ret = %d\n", ret);
            return -1;
        }
        printf("Compare fp16 TPU with CPU success!\n");
    }

    free(XHost16);
    free(tpu_out_fp16);
    free(cpu_out_fp16);
    free(XHost);
    free(tpu_out);
    free(cpu_out);

    return ret;
}

void* thread_test_distance(void* args) {
    cv_distance_thread_arg_t* cv_distance_thread_arg = (cv_distance_thread_arg_t*)args;
    int loop = cv_distance_thread_arg->loop;
    int len = cv_distance_thread_arg->len;
    int dim = cv_distance_thread_arg->dim;
    enum op op = cv_distance_thread_arg->op;
    bm_handle_t handle = cv_distance_thread_arg->handle;

    for (int i = 0; i < loop; ++i) {
        int ret = test_distance(dim, len, op, handle);
        if (ret) {
            printf("------Test Distance Failed!------\n");
            exit(-1);
        }
        printf("------Test Distance Successed!------\n");
    }
    return (void*)0;
}


int main(int argc, char* args[])
{
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int thread_num = 1;
    int dim = 1 + rand() % 8;
    int len = 1 + rand() % 40960;
    int ret = 0;
    enum op op = 0;
    int loop = 1;
    int check = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop op(0/1, 0 is fp32, 1 is fp16) len dim \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 10\n", args[0]);
        printf("%s 2 1 1\n", args[0]);
        printf("%s 1 1 1 512 3 \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) op = (enum op)atoi(args[3]);
    if (argc > 4) len = atoi(args[4]);
    if (argc > 5) dim = atoi(args[5]);


    check = parameters_check(len, dim, op);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }
    printf("seed = %d\n", seed);
    // test for multi-thread
    pthread_t pid[thread_num];
    cv_distance_thread_arg_t cv_distance_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_distance_thread_arg[i].loop = loop;
        cv_distance_thread_arg[i].len = len;
        cv_distance_thread_arg[i].dim = dim;
        cv_distance_thread_arg[i].op = op;
        cv_distance_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, thread_test_distance, &cv_distance_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return ret;
}