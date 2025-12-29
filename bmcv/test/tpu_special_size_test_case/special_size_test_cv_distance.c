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

static int cpu_distance_fp32(float* XHost, float* cpu_out, int L, int dim, float pnt32[])
{
    int i, j;

    if (XHost == NULL || cpu_out == NULL || pnt32 == NULL) {
        printf("cpu_distance the param is null!\n");
        return -1;
    }

    for (i = 0; i < L; ++i) {
        cpu_out[i] = 0.f;
        for (j = 0; j < dim; ++j) {
            cpu_out[i] += (XHost[i * dim + j] - pnt32[j]) * (XHost[i * dim + j] - pnt32[j]);
        }
        cpu_out[i] = sqrt(cpu_out[i]);
    }

    return 0;
}

static int cpu_distance_fp16(struct fp16* XHost16, struct fp16* cpu_out_fp16, int L,
                            int dim, struct fp16 pnt16[])
{
    int i, j;
    float pnt32[DIM_MAX] = {0};
    float* XHost = (float*)malloc(L * dim * sizeof(float));
    float* YHost = (float*)malloc(L * sizeof(float));

    if (XHost16 == NULL || cpu_out_fp16 == NULL || pnt16 == NULL) {
        printf("cpu_distance the param is null!\n");
        return -1;
    }

    for (i = 0; i < L; ++i) {
        YHost[i] = 0.f;
        for (j = 0; j < dim; ++j) {
            XHost[i * dim + j] = fp16tofp32(XHost16[i * dim + j]);
            pnt32[j] = fp16tofp32(pnt16[j]);
            YHost[i] += (XHost[i * dim + j] - pnt32[j]) * (XHost[i * dim + j] - pnt32[j]);
        }
        YHost[i] = sqrt(YHost[i]);
        cpu_out_fp16[i] = fp32tofp16(YHost[i], 1);
    }

    free(XHost);
    free(YHost);

    return 0;
}

static int tpu_distance_fp32(float* XHost, float* YHost, int L,int dim, float pnt32[], bm_handle_t handle, long int *tpu_time)
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
    *tpu_time += TIME_COST_US(t1, t2);
    ret = bm_memcpy_d2s(handle, YHost, YDev);
    if (ret != BM_SUCCESS) {
        printf("bmcv memcpy d2s failed. ret = %d\n", ret);
        return -1;
    }

    bm_free_device(handle, XDev);
    bm_free_device(handle, YDev);


    return 0;
}

static int tpu_distance_fp16(struct fp16* XHost16, struct fp16* YHost16, int L,
                            int dim, struct fp16 pnt16[], bm_handle_t handle, long int *tpu_time)
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
    *tpu_time += TIME_COST_US(t1, t2);
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

static int test_distance(int dim, int L, int op, bm_handle_t handle, long int *cpu_time, long int *tpu_time)
{
    float pnt32[DIM_MAX] = {0};
    float* XHost = (float*)malloc(L * dim * sizeof(float));
    float* tpu_out = (float*)malloc(L * sizeof(float));
    float* cpu_out = (float*)malloc(L * sizeof(float));
    struct fp16* XHost16 = (struct fp16*)malloc(L * dim * sizeof(struct fp16));
    struct fp16* tpu_out_fp16 = (struct fp16*)malloc(L * sizeof(struct fp16));
    struct fp16* cpu_out_fp16 = (struct fp16*)malloc(L * sizeof(struct fp16));
    struct fp16 pnt16[DIM_MAX] = {0};
    int round = 1;
    int ret = 0;
    int i;
    struct timeval t1, t2;
    for (i = 0; i < dim; ++i) {
        pnt32[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
        pnt16[i] = fp32tofp16(pnt32[i], round);
    }

    for (i = 0; i < L * dim; ++i) {
        XHost[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
        XHost16[i] = fp32tofp16(XHost[i], round);
    }

    if (op == FP32) {
        ret = tpu_distance_fp32(XHost, tpu_out, L, dim, pnt32, handle, tpu_time);
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
        *cpu_time += TIME_COST_US(t1, t2);
        ret = cmp_result(tpu_out, cpu_out, L);
        if (ret) {
            printf("the tpu_distance_fp32 & cpu_distance_fp32 cmp failed, ret = %d\n", ret);
            return -1;
        }

    } else {
        ret = tpu_distance_fp16(XHost16, tpu_out_fp16, L, dim, pnt16, handle, tpu_time);
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
        *cpu_time += TIME_COST_US(t1, t2);
        for (i = 0; i < L; ++i) {
            tpu_out[i] = fp16tofp32(tpu_out_fp16[i]);
            cpu_out[i] = fp16tofp32(cpu_out_fp16[i]);
        }

        ret = cmp_result(tpu_out, cpu_out, L);
        if (ret) {
            printf("the tpu_distance_fp16 & cpu_distance_fp16 cmp failed, ret = %d\n", ret);
            return -1;
        }
    }

    free(XHost16);
    free(tpu_out_fp16);
    free(cpu_out_fp16);
    free(XHost);
    free(tpu_out);
    free(cpu_out);

    return ret;
}

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_distance.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_distance.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int len_num[] = {1, 1, 10, 100, 1000, 10000, 20000, 30000, 40960};
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    for(enum op op = 0; op < 2; op++) {
        for(int dim = 1; dim < 9; dim++) {
            for(int j = 0; j < 9; j++) {
                int len = len_num[j];
                printf("op: %d , dim: %d, len: %d\n", op, dim, len);
                long int cputime = 0;
                long int tputime = 0;
                long int *cpu_time = &cputime;
                long int *tpu_time = &tputime;
                for(int loop = 0; loop < 3; loop++) {
                    int ret = test_distance(dim, len, op, handle, cpu_time, tpu_time);
                    if (ret) {
                        printf("------Test Distance Failed!------\n");
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