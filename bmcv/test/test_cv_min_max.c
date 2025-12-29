
#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

pthread_mutex_t lock;

typedef struct {
    int loop;
    int len;
    bm_handle_t handle;
} cv_min_max_thread_arg_t;

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

extern int cpu_min_max(float* XHost, int L, float* min_cpu, float* max_cpu);

static int parameters_check(int len)
{
    if (len < 50 || len > 260144){
        printf("Unsupported size! len_size range : 50 ~ 260144 \n");
        return -1;
    }
    return 0;
}

static int tpu_min_max(float* XHost, int L, float* min_tpu, float* max_tpu, bm_handle_t handle)
{

    bm_device_mem_t XDev;
    bm_status_t ret = BM_SUCCESS;
    struct timeval t1, t2;

    if (XHost == NULL) {
        printf("the param Xhost or mi_tpu or nax_tpu is null\n");
        return -1;
    }
    ret = bm_malloc_device_byte(handle, &XDev, L * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("Allocate bm device memory failed. ret = %d\n", ret);
        return -1;
    }
    ret = bm_memcpy_s2d(handle, XDev, XHost);
    if (ret != BM_SUCCESS) {
        printf("Memory copy sys to device failed. ret = %d\n", ret);
        return -1;
    }
    gettimeofday(&t1, NULL);
    ret = bmcv_min_max(handle, XDev, min_tpu, max_tpu, L);
    if (ret != BM_SUCCESS) {
        printf("Calculate bm min_max failed. ret = %d\n", ret);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("Min_max TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    bm_free_device(handle, XDev);
    return 0;

}

static int test_min_max(int len, bm_handle_t handle)
{
    pthread_mutex_lock(&lock);
    int L = len;
    float min_tpu = 0, max_tpu = 0, min_cpu = 0, max_cpu = 0;
    int ret = 0;
    struct timeval t1, t2;
    float* Xhost = (float*)malloc(L * sizeof(float));
    int i;

    printf("len = %d\n", len);
    for (i = 0; i < L; ++i)
        Xhost[i] = (float)((rand() % 2 ? 1 : -1) * (rand() % 1000 + (rand() % 100000) * 0.01));
    ret = tpu_min_max(Xhost, L, &min_tpu, &max_tpu, handle);
    if (ret) {
        printf("the tpu_min_max is failed, ret = %d\n", ret);
    }
    gettimeofday(&t1, NULL);
    ret = cpu_min_max(Xhost, L, &min_cpu, &max_cpu);
    if (ret) {
        printf("the tpu_min_max is failed, ret = %d\n", ret);
    }
    gettimeofday(&t2, NULL);
    printf("Min_max CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    if ((min_tpu != min_cpu) || (max_tpu != max_cpu)) {
        printf("the min_tpu is %f, the max_tpu is %f\n", min_tpu, max_tpu);
        printf("the min_cpu is %f, the max_cpu is %f\n", min_cpu, max_cpu);
        printf("the cmp tpu & cpu is failed!\n");
        return -1;
    }
    free(Xhost);
    Xhost = NULL;
    pthread_mutex_unlock(&lock);
    return ret;
}

void* test_thread_min_max(void* args) {
    cv_min_max_thread_arg_t* cv_cmulp_thread_arg = (cv_min_max_thread_arg_t*)args;
    int loop = cv_cmulp_thread_arg->loop;
    int len = cv_cmulp_thread_arg->len;
    bm_handle_t handle = cv_cmulp_thread_arg->handle;
    for (int i = 0; i < loop; ++i) {
        int ret = test_min_max(len, handle);
        if (ret) {
            printf("------Test Min_Max Failed!------\n");
            exit(-1);
        }
        printf("------Test Min_Max Successed!------\n");
    }
    return (void*)0;
}

int main(int argc, char* args[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int thread_num = 1;
    int loop = 1;
    int len = 50 + rand() % 260095;
    int ret = 0;
    int check = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }
    if (argc == 2 && atoi(args[1]) == -1) {
        printf("uasge: %d\n", argc);
        printf("%s thread_num loop len\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2 1 512\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) len = atoi(args[3]);

    check = parameters_check(len);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }
    // test for multi-thread
    pthread_t pid[thread_num];
    cv_min_max_thread_arg_t cv_cmulp_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_cmulp_thread_arg[i].loop = loop;
        cv_cmulp_thread_arg[i].len = len;
        cv_cmulp_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_thread_min_max, &cv_cmulp_thread_arg[i]) != 0) {
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