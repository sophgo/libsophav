#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bmcv_api_ext_c.h"
#include <pthread.h>
#ifdef __linux__
#include <sys/time.h>
#include "time.h"
#else
#include <windows.h>
#include "time.h"
#endif

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))
#define GRAY_SERIES 256
#define ERR_MAX 1

struct gray_para {
    uint8_t gray_val;
    float cdf_val;
};

struct cv_hb_thread_arg_t {
    int loop;
    int height;
    int width;
    bm_handle_t handle;
};

static float get_cdf_min(float* cdf, int len)
{
    int i;

    for(i = 0; i < len; ++i) {
        if (cdf[i] != 0) {
            return cdf[i];
        }
    }
    return 0.f;
}

static int cpu_hist_balance(uint8_t* input_host, uint8_t* output_cpu, int height, int width)
{
    int H = height;
    int W = width;
    int binX;
    int i;
    float gray_tmp;
    uint8_t gray_index_tmp;
    float cdf_min;
    float cdf_max;
    float* cpu_cdf;

    if (input_host == NULL || output_cpu == NULL) {
        printf("cpu_calc_hist param error!\n");
        return -1;
    }

    cpu_cdf = (float*)malloc(GRAY_SERIES * sizeof(float));
    memset(cpu_cdf, 0.f, GRAY_SERIES * sizeof(float));

    for (i = 0; i < W * H; ++i) {
        binX = input_host[i];
        if (binX >= 0 && binX < GRAY_SERIES) {
            cpu_cdf[binX]++;
        }
    }

    for (i = 1; i < GRAY_SERIES ; ++i) {
        cpu_cdf[i] += cpu_cdf[i - 1];
    }

    cdf_min = get_cdf_min(cpu_cdf, GRAY_SERIES);
    cdf_max = W * H;

    for(i = 0; i < H * W; ++i) {
        gray_index_tmp = input_host[i];
        gray_tmp = round((cpu_cdf[gray_index_tmp] - cdf_min) * (GRAY_SERIES - 1) / (cdf_max - cdf_min));
        output_cpu[i] = (uint8_t)gray_tmp;
    }

    free(cpu_cdf);
    return 0;
}

static int tpu_hist_balance(uint8_t* input_host, uint8_t* output_tpu, int height, int width, bm_handle_t handle)
{
    bm_device_mem_t input, output;
    int H = height;
    int W = width;
    bm_status_t ret = BM_SUCCESS;
    struct timeval t1, t2;

    ret = bm_malloc_device_byte(handle, &output, H * W * sizeof(uint8_t));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte output float failed. ret = %d\n", ret);
        return ret;
    }

    ret = bm_malloc_device_byte(handle, &input, H * W * sizeof(uint8_t));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte intput uint8_t failed. ret = %d\n", ret);
        bm_free_device(handle, output);
        return ret;
    }

    ret = bm_memcpy_s2d(handle, input, input_host);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d uint8_t failed. ret = %d\n", ret);
        bm_free_device(handle, input);
        bm_free_device(handle, output);
        return ret;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_hist_balance(handle, input, output, H, W);
    if (ret != BM_SUCCESS) {
        printf("bmcv_calc_hist_weighted uint8_t failed. ret = %d\n", ret);
        bm_free_device(handle, input);
        bm_free_device(handle, output);
        return ret;
    }
    gettimeofday(&t2, NULL);
    printf("bmcv_hist_balance TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = bm_memcpy_d2s(handle, output_tpu, output);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed. ret = %d\n", ret);
        bm_free_device(handle, input);
        bm_free_device(handle, output);
        return ret;
    }

    bm_free_device(handle, input);
    bm_free_device(handle, output);
    return 0;
}

static int cmp_result(uint8_t* out_cpu, uint8_t* out_tpu, int len)
{
    int i;

    for (i = 0; i < len; ++i) {
        if (abs(out_tpu[i] - out_cpu[i]) > ERR_MAX) {
            printf("out_tpu[%d] = %u, out_cpu[%d] = %u\n", i, out_tpu[i], i, out_cpu[i]);
            return -1;
        }
    }

    return 0;
}

static int test_hist_balance(bm_handle_t handle, int height, int width)
{
    int len = height * width;
    int ret = 0;
    int i;
    struct timeval t1, t2;

    printf("the height = %d, the width = %d\n", height, width);

    uint8_t* inputHost = (uint8_t*)malloc(len * sizeof(uint8_t));
    uint8_t* output_cpu = (uint8_t*)malloc(len * sizeof(uint8_t));
    uint8_t* output_tpu = (uint8_t*)malloc(len * sizeof(uint8_t));

    for (i = 0; i < len; ++i) {
        inputHost[i] = (uint8_t)(rand() % 256);
    }

    gettimeofday(&t1, NULL);
    ret = cpu_hist_balance(inputHost, output_cpu, height, width);
    gettimeofday(&t2, NULL);
    printf("cpu_hist_balance CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    if (ret) {
        printf("cpu_hist_balance failed!\n");
        goto exit;
    }

    ret = tpu_hist_balance(inputHost, output_tpu, height, width, handle);
    if (ret) {
        printf("tpu_hist_balance failed!\n");
        goto exit;
    }

    ret = cmp_result(output_cpu, output_tpu, len);
    if (ret) {
        printf("cmp_result failed!\n");
        goto exit;
    }

exit:
    free(inputHost);
    free(output_cpu);
    free(output_tpu);
    return ret;
}

void* test_hist_balance_all(void* args)
{
    struct cv_hb_thread_arg_t* cv_hb_thread_arg = (struct cv_hb_thread_arg_t*)args;
    int loop = cv_hb_thread_arg->loop;
    int height = cv_hb_thread_arg->height;
    int width = cv_hb_thread_arg->width;
    bm_handle_t handle = cv_hb_thread_arg->handle;
    int i;
    int ret = 0;

    for (i = 0; i < loop; ++i) {
        ret = test_hist_balance(handle, height, width);
        if (ret) {
            printf("test_hist_balance failed\n");
            exit(-1);
        }
    }
    printf("Compare TPU result with CPU successfully!\n");
    return (void*)0;
}

int main(int argc, char* args[])
{
    struct timespec tp;
    clock_gettime(0, &tp);
    srand(tp.tv_nsec);
    int thread_num = 1;
    int loop = 1;
    int height = rand() % 8192 + 1;
    int width = rand() % 8192 + 1;
    int ret = 0;
    bm_handle_t handle;
    int i;

    ret = bm_dev_request(&handle, 0);
    if (ret) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return ret;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("%s thread loop height width\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 1 3\n", args[0]);
        printf("%s 1 1 800 800\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) height = atoi(args[3]);
    if (argc > 4) width = atoi(args[4]);

    pthread_t pid[thread_num];
    struct cv_hb_thread_arg_t cv_hb_thread_arg[thread_num];
    for (i = 0; i < thread_num; i++) {
        cv_hb_thread_arg[i].loop = loop;
        cv_hb_thread_arg[i].height = height;
        cv_hb_thread_arg[i].width = width;
        cv_hb_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_hist_balance_all, &cv_hb_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }

    for (i = 0; i < thread_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }

    bm_dev_free(handle);
    return ret;
}