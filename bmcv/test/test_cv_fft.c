#include "bmcv_api_ext_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#define ERR_MAX 1e-1
#define M_PI 3.14159265358979323846
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

struct cv_fft_thread_arg_t {
    int loop_num;
    int L;
    int batch;
    bool forward;
    float real_input;
    bm_handle_t handle;
};

static void dft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int length, bool forward)
{
    int i, j;
    double angle;

    for (i = 0; i < length; ++i) {
        output_real[i] = 0.f;
        output_imag[i] = 0.f;
        for (j = 0; j < length; ++j) {
            angle = (forward ? -2.0 : 2.0) * M_PI * i * j / length;
            output_real[i] += in_real[j] * cos(angle) - in_imag[j] * sin(angle);
            output_imag[i] += in_real[j] * sin(angle) + in_imag[j] * cos(angle);
        }
    }
}

static int cmp_res(float* YRHost_tpu, float* YIHost_tpu, float* YRHost_cpu,
                    float* YIHost_cpu, int L, int batch)
{
    int i, j, index;

    for(i = 0; i < batch; ++i) {
        for (j = 0; j < L; ++j) {
            index = i * L + j;
            if (abs(YRHost_cpu[index] - YRHost_tpu[index]) > ERR_MAX ||
                abs(YIHost_cpu[index] - YIHost_tpu[index]) > ERR_MAX) {
                    printf("the cpu_res[%d].real = %f, the cpu_res[%d].img = %f, \
                            the tpu_res[%d].real = %f, the tpu_res[%d].img = %f\n",
                            index, YRHost_cpu[index], index, YIHost_cpu[index], index,
                            YRHost_tpu[index], index, YIHost_tpu[index]);
                    return -1;
            }
        }
    }
    return 0;
}

static void normalize_fft(float* res_XR, float* res_XI, int frm_len, int num)
{
    float norm_fac;
    int i;

    norm_fac = 1.0f / frm_len;

    for (i = 0; i < num; ++i) {
        res_XR[i] *= norm_fac;
        res_XI[i] *= norm_fac;
    }
}

static int cpu_fft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int length, int batch, bool forward)
{
    int i;

    for (i = 0; i < batch; ++i) {
        dft(&(in_real[i * length]), &(in_imag[i * length]), &(output_real[i * length]),
            &(output_imag[i * length]), length, forward);
    }

    if (!forward) {
        normalize_fft(output_real, output_imag, length, batch * length);
    }

    return 0;
}

static int tpu_fft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int L, int batch, bool forward,
                bool realInput, bm_handle_t handle)
{
    bm_device_mem_t XRDev, XIDev, YRDev, YIDev;
    bm_status_t ret = BM_SUCCESS;
    void* plan = NULL;
    struct timeval t1, t2;

    ret = bm_malloc_device_byte(handle, &XRDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto err0;
    }

    ret = bm_malloc_device_byte(handle, &XIDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto err1;
    }

    ret = bm_malloc_device_byte(handle, &YRDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto err2;
    }

    ret = bm_malloc_device_byte(handle, &YIDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto err3;
    }

    ret = bm_memcpy_s2d(handle, XRDev, in_real);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d failed!\n");
        goto err4;
    }

    ret = bm_memcpy_s2d(handle, XIDev, in_imag);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d failed!\n");
        goto err4;
    }

    ret = bmcv_fft_1d_create_plan(handle, batch, L, forward, &plan);
    if (ret != BM_SUCCESS) {
        printf("bmcv_fft_1d_create_plan failed!\n");
        goto err5;
    }

    gettimeofday(&t1, NULL);
    if (realInput) {
        ret = bmcv_fft_execute_real_input(handle, XRDev, YRDev, YIDev, plan);
        if (ret != BM_SUCCESS) {
            printf("bmcv_fft_execute_real_input failed!\n");
            goto err5;
        }
    } else {
        ret = bmcv_fft_execute(handle, XRDev, XIDev, YRDev, YIDev, plan);
        if (ret != BM_SUCCESS) {
            printf("bmcv_fft_execute failed!\n");
            goto err5;
        }
    }
    gettimeofday(&t2, NULL);
    printf("FFT_1D TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = bm_memcpy_d2s(handle, (void*)output_real, YRDev);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed!\n");
        goto err5;
    }

    ret = bm_memcpy_d2s(handle, (void*)output_imag, YIDev);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed!\n");
        goto err5;
    }

err5:
    if (plan != NULL) {
        bmcv_fft_destroy_plan(handle, plan);
    }
err4:
    bm_free_device(handle, YIDev);
err3:
    bm_free_device(handle, YRDev);
err2:
    bm_free_device(handle, XIDev);
err1:
    bm_free_device(handle, XRDev);
err0:
    return ret;
}

static int test_fft_1d(int L, int batch, bool forward, bool realInput, bm_handle_t handle)
{
    float* XRHost = (float*)malloc(L * batch * sizeof(float));
    float* XIHost = (float*)malloc(L * batch * sizeof(float));
    float* YRHost_tpu = (float*)malloc(L * batch * sizeof(float));
    float* YIHost_tpu = (float*)malloc(L * batch * sizeof(float));
    float* YRHost_cpu = (float*)malloc(L * batch * sizeof(float));
    float* YIHost_cpu = (float*)malloc(L * batch * sizeof(float));
    int ret = 0;
    int i;
    struct timeval t1, t2;

    for (i = 0; i < L * batch; ++i) {
        XRHost[i] = (float)rand() / RAND_MAX;
        XIHost[i] = realInput ? 0 : ((float)rand() / RAND_MAX);
    }

    gettimeofday(&t1, NULL);
    ret = cpu_fft(XRHost, XIHost, YRHost_cpu, YIHost_cpu, L, batch, forward);
    if (ret != 0) {
        printf("cpu_fft failed!\n");
        goto exit;
    }
    gettimeofday(&t2, NULL);
    printf("FFT_1D CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = tpu_fft(XRHost, XIHost, YRHost_tpu, YIHost_tpu, L, batch, forward, realInput, handle);
    if (ret != 0) {
        printf("cpu_fft failed!\n");
        goto exit;
    }

    ret = cmp_res(YRHost_tpu, YIHost_tpu, YRHost_cpu, YIHost_cpu, L, batch);
    if (ret != 0) {
        printf("cmp_res fail!\n");
        goto exit;
    }

exit:

    free(XRHost);
    free(XIHost);
    free(YRHost_cpu);
    free(YIHost_cpu);
    free(YRHost_tpu);
    free(YIHost_tpu);
    return ret;
}

static int tpu_fft_2d(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int L, int batch, bool forward,
                bool realInput, bm_handle_t handle)
{
    bm_device_mem_t XRDev, XIDev, YRDev, YIDev;
    bm_status_t ret = BM_SUCCESS;
    void* plan = NULL;
    struct timeval t1, t2;

    ret = bm_malloc_device_byte(handle, &XRDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto err0;
    }

    ret = bm_malloc_device_byte(handle, &XIDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto err1;
    }

    ret = bm_malloc_device_byte(handle, &YRDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto err2;
    }

    ret = bm_malloc_device_byte(handle, &YIDev, L * batch * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed!\n");
        goto err3;
    }

    ret = bm_memcpy_s2d(handle, XRDev, in_real);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d failed!\n");
        goto err4;
    }

    ret = bm_memcpy_s2d(handle, XIDev, in_imag);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d failed!\n");
        goto err4;
    }

    ret = bmcv_fft_2d_create_plan(handle, L, batch, forward, &plan);
    if (ret != BM_SUCCESS) {
        printf("bmcv_fft_1d_create_plan failed!\n");
        goto err5;
    }

    gettimeofday(&t1, NULL);
    if (realInput) {
        ret = bmcv_fft_execute_real_input(handle, XRDev, YRDev, YIDev, plan);
        if (ret != BM_SUCCESS) {
            printf("bmcv_fft_execute_real_input failed!\n");
            goto err5;
        }
    } else {
        ret = bmcv_fft_execute(handle, XRDev, XIDev, YRDev, YIDev, plan);
        if (ret != BM_SUCCESS) {
            printf("bmcv_fft_execute failed!\n");
            goto err5;
        }
    }
    gettimeofday(&t2, NULL);
    printf("FFT_2D TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = bm_memcpy_d2s(handle, (void*)output_real, YRDev);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed!\n");
        goto err5;
    }

    ret = bm_memcpy_d2s(handle, (void*)output_imag, YIDev);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed!\n");
        goto err5;
    }

err5:
    if (plan != NULL) {
        bmcv_fft_destroy_plan(handle, plan);
    }
err4:
    bm_free_device(handle, YIDev);
err3:
    bm_free_device(handle, YRDev);
err2:
    bm_free_device(handle, XIDev);
err1:
    bm_free_device(handle, XRDev);
err0:
    return ret;
}

static int cpu_fft_2d(float *in_real, float *in_imag, float *out_real,
                        float *out_imag, int M, int N, bool forward)
{
    float *row_in_real = (float*)malloc(N * sizeof(float));
    float *row_in_imag = (float*)malloc(N * sizeof(float));
    float *row_out_real = (float*)malloc(N * sizeof(float));
    float *row_out_imag = (float*)malloc(N * sizeof(float));
    float *col_in_real = (float*)malloc(M * sizeof(float));
    float *col_in_imag = (float*)malloc(M * sizeof(float));
    float *col_out_real =(float*)malloc(M * sizeof(float));
    float *col_out_imag =(float*)malloc(M * sizeof(float));
    int i, j;

    /* 逐行计算一维 DFT */
    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            row_in_real[j] = in_real[i * N + j];
            row_in_imag[j] = in_imag[i * N + j];
        }
        dft(row_in_real, row_in_imag, row_out_real, row_out_imag, N, forward);
        if (!forward) {
            normalize_fft(row_out_real, row_out_imag, N, N);
        }
        for (j = 0; j < N; j++) {
            out_real[i * N + j] = row_out_real[j];
            out_imag[i * N + j] = row_out_imag[j];
        }
    }

    /* 逐列计算一维 DFT */
    for (j = 0; j < N; j++) {
        for (i = 0; i < M; i++) {
            col_in_real[i] = out_real[i * N + j];
            col_in_imag[i] = out_imag[i * N + j];
        }
        dft(col_in_real, col_in_imag, col_out_real, col_out_imag, M, forward);
        if (!forward) {
            normalize_fft(col_out_real, col_out_imag, M, M);
        }
        for (i = 0; i < M; i++) {
            out_real[i * N + j] = col_out_real[i];
            out_imag[i * N + j] = col_out_imag[i];
        }
    }

    free(row_in_real);
    free(row_in_imag);
    free(row_out_real);
    free(row_out_imag);
    free(col_in_real);
    free(col_in_imag);
    free(col_out_real);
    free(col_out_imag);
    return 0;
}

static int test_fft_2d(int L, int batch, bool forward, bool realInput, bm_handle_t handle)
{
    float* XRHost = (float*)malloc(L * batch * sizeof(float));
    float* XIHost = (float*)malloc(L * batch * sizeof(float));
    float* YRHost_tpu = (float*)malloc(L * batch * sizeof(float));
    float* YIHost_tpu = (float*)malloc(L * batch * sizeof(float));
    float* YRHost_cpu = (float*)malloc(L * batch * sizeof(float));
    float* YIHost_cpu = (float*)malloc(L * batch * sizeof(float));
    int ret = 0;
    int i;
    struct timeval t1, t2;

    for (i = 0; i < L * batch; ++i) {
        XRHost[i] = (float)rand() / RAND_MAX;
        XIHost[i] = realInput ? 0 : ((float)rand() / RAND_MAX);
    }

    gettimeofday(&t1, NULL);
    ret = cpu_fft_2d(XRHost, XIHost, YRHost_cpu, YIHost_cpu, L, batch, forward);
    if (ret != 0) {
        printf("cpu_fft failed!\n");
        goto exit;
    }
    gettimeofday(&t2, NULL);
    printf("FFT_2D CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = tpu_fft_2d(XRHost, XIHost, YRHost_tpu, YIHost_tpu, L, batch, forward, realInput, handle);
    if (ret != 0) {
        printf("cpu_fft failed!\n");
        goto exit;
    }

    ret = cmp_res(YRHost_tpu, YIHost_tpu, YRHost_cpu, YIHost_cpu, L, batch);
    if (ret != 0) {
        printf("cmp_res fail!\n");
        goto exit;
    }

exit:

    free(XRHost);
    free(XIHost);
    free(YRHost_cpu);
    free(YIHost_cpu);
    free(YRHost_tpu);
    free(YIHost_tpu);
    return ret;
}

void* test_fft_all(void* args)
{
    struct cv_fft_thread_arg_t* fft_thread_arg = (struct cv_fft_thread_arg_t*)args;

    int loop = fft_thread_arg->loop_num;
    int L = fft_thread_arg->L;
    int batch = fft_thread_arg->batch;
    bool forward = fft_thread_arg->forward;
    bool real_input = fft_thread_arg->real_input;
    bm_handle_t handle = fft_thread_arg->handle;
    int i;
    int ret = 0;

    for (i = 0; i < loop; ++i) {
        ret = test_fft_1d(L, batch, forward, real_input, handle);
        if(ret) {
            printf("test_fft_1d failed\n");
            exit(-1);
        }
        ret = test_fft_2d(L, batch, forward, real_input, handle);
        if(ret) {
            printf("test_fft_2d failed\n");
            exit(-1);
        }
    }
    printf("------Test FFT1D & FFT2D All Success!------\n");
    return (void*)0;
}

int main(int argc, char* args[])
{
    struct timespec tp;
    clock_gettime(0, &tp);
    srand(tp.tv_nsec);

    bm_handle_t handle;
    int ret = 0;
    int i;
    int length = 100;
    int batch = 100;
    bool forward = true;
    bool real_input = false;
    int thread_num = 1;
    int loop = 1;

    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("%s thread_num loop L batch forward real_input\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 1 100 100 1 0\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) length = atoi(args[3]);
    if (argc > 4) batch = atoi(args[4]);
    if (argc > 5) forward = atoi(args[5]);
    if (argc > 6) real_input = atoi(args[6]);

    pthread_t pid[thread_num];
    struct cv_fft_thread_arg_t fft_thread_arg[thread_num];
    for (i = 0; i < thread_num; i++) {
        fft_thread_arg[i].loop_num = loop;
        fft_thread_arg[i].L = length;
        fft_thread_arg[i].batch = batch;
        fft_thread_arg[i].forward = forward;
        fft_thread_arg[i].real_input = real_input;
        fft_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_fft_all, &fft_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (i = 0; i < thread_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }

    bm_dev_free(handle);
    return ret;
}