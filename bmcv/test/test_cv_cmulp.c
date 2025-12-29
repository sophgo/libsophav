#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    int loop;
    int L;
    int batch;
    bm_handle_t handle;
} cv_cmulp_thread_arg_t;

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

extern int cpu_cmul(float* XRHost, float* XIHost, float* PRHost, float* PIHost,
                    float* cpu_YR, float* cpu_YI, int L, int batch);

static int parameters_check(int len, int batch)
{
    if (len > 4096 || batch > 1980){
        printf("Unsupported size : len_max = 4096, batch_max = 1980 \n");
        return -1;
    }
    return 0;
}

static int tpu_cmul(float* XRHost, float* XIHost, float* PRHost, float* PIHost,
                    float* tpu_YR, float* tpu_YI, int L, int batch, bm_handle_t handle)
{
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t XRDev, XIDev, PRDev, PIDev, YRDev, YIDev;
    struct timeval t1, t2;

    ret = bm_malloc_device_byte(handle, &XRDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte XRDev failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_malloc_device_byte(handle, &XIDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte XIDev failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_malloc_device_byte(handle, &PRDev, L * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte PRDev failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_malloc_device_byte(handle, &PIDev, L * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte PIDev failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_malloc_device_byte(handle, &YRDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte YRDev failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_malloc_device_byte(handle, &YIDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte YIDev failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_memcpy_s2d(handle, XRDev, XRHost);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d XRDev XRHost failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_memcpy_s2d(handle, XIDev, XIHost);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d XIDev XIHost failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_memcpy_s2d(handle, PRDev, PRHost);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d PRDev PRHost failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_memcpy_s2d(handle, PIDev, PIHost);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d PIDev PIHost failed. ret = %d\n", ret);
        return -1;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_cmulp(handle, XRDev, XIDev, PRDev, PIDev, YRDev, YIDev, batch, L);
    if (ret != BM_SUCCESS) {
        printf("bmcv_cmulp failed. ret = %d\n", ret);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("Cmulp TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = bm_memcpy_d2s(handle, tpu_YR, YRDev);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s tpu_YR YRDev failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_memcpy_d2s(handle, tpu_YI, YIDev);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s tpu_YI YIDevfailed. ret = %d\n", ret);
        return -1;
    }

    bm_free_device(handle, XRDev);
    bm_free_device(handle, XIDev);
    bm_free_device(handle, YRDev);
    bm_free_device(handle, YIDev);
    bm_free_device(handle, PRDev);
    bm_free_device(handle, PIDev);
    return 0;
}

static int cmp_result(float* cpu_YR, float* cpu_YI, float* tpu_YR, float* tpu_YI, int L, int batch)
{
    int ret = 0;
    int i, j;

    for (i = 0; i < batch; ++i) {
        for (j = 0; j < L; ++j) {
            if (cpu_YR[i * L + j] != tpu_YR[i * L + j] || cpu_YI[i * L + j] != tpu_YI[i * L + j]) {
                printf("the compare tpu & cpu failed, the cpu_YR = %f, the tpu_YR = %f, \
                        the cpu_YI = %f, the tpu_YI = %f\n",
                cpu_YR[i * L + j], tpu_YR[i * L + j], cpu_YI[i * L + j], tpu_YI[i * L + j]);
                return -1;
            }
        }
    }

    return ret;
}

static int test_cmulp_random(int L, int batch, bm_handle_t handle) {

    float* XRHost = (float*)malloc(L * batch * sizeof(float));
    float* XIHost = (float*)malloc(L * batch * sizeof(float));
    float* PRHost = (float*)malloc(L * sizeof(float));
    float* PIHost = (float*)malloc(L * sizeof(float));
    float* tpu_YR = (float*)malloc(L * batch * sizeof(float));
    float* tpu_YI = (float*)malloc(L * batch * sizeof(float));
    float* cpu_YR = (float*)malloc(L * batch * sizeof(float));
    float* cpu_YI = (float*)malloc(L * batch * sizeof(float));
    int ret = 0;
    int i;
    struct timeval t1, t2;

    for (i = 0; i < L * batch; ++i) {
        XRHost[i] = rand() % 5 - 2;
        XIHost[i] = rand() % 5 - 2;
    }
    for (i = 0; i < L; ++i) {
        PRHost[i] = rand() % 5 - 2;
        PIHost[i] = rand() % 5 - 2;
    }

    ret = tpu_cmul(XRHost, XIHost, PRHost, PIHost, tpu_YR, tpu_YI, L, batch, handle);
    if (ret) {
        printf("the tpu cuml failed!, ret = %d\n", ret);
        return ret;
    }

    gettimeofday(&t1, NULL);
    ret = cpu_cmul(XRHost, XIHost, PRHost, PIHost, cpu_YR, cpu_YI, L, batch);
    if (ret) {
        printf("the cpu cuml failed!, ret = %d\n", ret);
        return ret;
    }
    gettimeofday(&t2, NULL);
    printf("Cmulp CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = cmp_result(cpu_YR, cpu_YI, tpu_YR, tpu_YI, L, batch);
    if (ret) {
        printf("the tpu & cpu result compare failed!, ret = %d\n", ret);
        return ret;
    }

    free(XRHost);
    free(XIHost);
    free(PRHost);
    free(PIHost);
    free(tpu_YR);
    free(tpu_YI);
    free(cpu_YR);
    free(cpu_YI);

    return ret;
}

void* test_thread_cmulp(void* args) {
    cv_cmulp_thread_arg_t* cv_cmulp_thread_arg = (cv_cmulp_thread_arg_t*)args;
    int loop = cv_cmulp_thread_arg->loop;
    int L = cv_cmulp_thread_arg->L;
    int batch = cv_cmulp_thread_arg->batch;
    bm_handle_t handle = cv_cmulp_thread_arg->handle;
    for (int i = 0; i < loop; ++i) {
        int ret = test_cmulp_random(L, batch, handle);
        if (ret) {
            printf("------Test Cmulp Failed!------\n");
            exit(-1);
        }
        printf("------Test Cmulp Successed!------\n");
    }
    return (void*)0;
}


int main(int argc, char *args[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int thread_num = 1;
    int loop = 1;
    int L = 1 + rand() % 4096;
    int batch = 1 + rand() % 1980;
    int ret = 0;
    int check = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1){
        printf("usage: %d\n", argc);
        printf("%s thread_num loop len batch \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 1 256 20 \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) L = atoi(args[3]);
    if (argc > 4) batch = atoi(args[4]);

    check = parameters_check(L, batch);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }
    printf("seed = %d\n", seed);
    // test for multi-thread
    pthread_t pid[thread_num];
    cv_cmulp_thread_arg_t cv_cmulp_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_cmulp_thread_arg[i].loop = loop;
        cv_cmulp_thread_arg[i].L = L;
        cv_cmulp_thread_arg[i].batch = batch;
        cv_cmulp_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_thread_cmulp, &cv_cmulp_thread_arg[i]) != 0) {
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
