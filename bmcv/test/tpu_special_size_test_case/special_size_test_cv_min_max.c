
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

static int tpu_min_max(float* XHost, int L, float* min_tpu, float* max_tpu, bm_handle_t handle, long int *tpu_time)
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
    *tpu_time += TIME_COST_US(t1, t2);
    bm_free_device(handle, XDev);
    return 0;
}

static int cpu_min_max(float* XHost, int L, float* min_cpu, float* max_cpu)
{
    int ret = 0;
    float min_value = 0;
    float max_value = 0;

    if (XHost == NULL) {
        printf("the param Xhost or min_cpu or nax_cpu is null\n");
        return -1;
    }

    if (min_cpu == NULL && max_cpu == NULL) {
        printf("Do Not Calculate Min & Max.\n");
        return ret;
    }

    if (min_cpu != NULL) {
        min_value = XHost[0];
        for (int i = 1; i < L; ++i) {
            min_value = min_value < XHost[i] ? min_value : XHost[i];
        }
        *min_cpu = min_value;
    }

    if (max_cpu != NULL) {
        max_value = XHost[0];
        for (int i = 1; i < L; ++i) {
            max_value = max_value > XHost[i] ? max_value : XHost[i];
        }
        *max_cpu = max_value;
    }

    return ret;
}

static int test_min_max(int len, bm_handle_t handle, long int *cpu_time, long int *tpu_time)
{
    pthread_mutex_lock(&lock);
    int L = len;
    float min_tpu = 0, max_tpu = 0, min_cpu = 0, max_cpu = 0;
    int ret = 0;
    struct timeval t1, t2;
    float* Xhost = (float*)malloc(L * sizeof(float));
    int i;

    for (i = 0; i < L; ++i)
        Xhost[i] = (float)((rand() % 2 ? 1 : -1) * (rand() % 1000 + (rand() % 100000) * 0.01));
    ret = tpu_min_max(Xhost, L, &min_tpu, &max_tpu, handle, tpu_time);
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
    *cpu_time += TIME_COST_US(t1, t2);
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

int main(int argc, char* args[]) {
    printf("log will be saved in special_size_test_cv_min_max.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_min_max.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);

    int len_num[11] = {50, 50, 100, 1000, 5000, 10000, 50000, 100000, 150000, 200000, 260144};
    int ret = 0;

    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    for(int j = 0; j < 11; j++) {
        int len = len_num[j];
        printf("len: %d\n", len);
        long int cputime = 0;
        long int tputime = 0;
        long int *cpu_time = &cputime;
        long int *tpu_time = &tputime;
        for(int loop = 0; loop < 3; loop++) {
            int ret = test_min_max(len, handle, cpu_time, tpu_time);
            if (ret) {
                printf("------Test Min_Max Failed!------\n");
                return -1;
            }
        }
        printf("------average CPU time = %ldus------\n", cputime / 3);
        printf("------average TPU time = %ldus------\n", tputime / 3);
    }

    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}