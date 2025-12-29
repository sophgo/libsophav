#include <math.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <time.h>
#include "bmcv_api_ext_c.h"
#include <pthread.h>

#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#include "time.h"
#endif

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop_num;
    int bits_len;
    int input1_num;
    int input2_num;
    int random;
    bm_handle_t handle;
} hm_distance_thread_arg_t;

static int hammingDistance(int x, int y) {
    int cnt = 0;
    int z = x ^ y;
    while (z != 0) {
        cnt += z & 1;
        z = z >> 1;
    }
    return cnt;
}

static void native_cal_hammingDistance(int *input1, int *input2, int *output, int bits_len,
                                       int input1_num, int input2_num) {
    for (int i = 0; i < input1_num; i++) {
        for(int j = 0; j < input2_num; j++) {
                for(int d = 0; d < bits_len; d++) {
                    output[i * input2_num + j] +=
                    hammingDistance(input1[i * bits_len + d], input2[j * bits_len + d]);
            }
        }
    }
}

static int array_cmp_int32(int *p_exp, int *p_got, int len, const char *info_label, int delta) {
    for (int idx = 0; idx < len; idx++) {
        if ((int)fabs(p_exp[idx] - (int)p_got[ idx]) > delta) {
            printf("%s abs error at index %d exp %d got %d\n", info_label, idx, p_exp[idx], p_got[idx]);
            return -1;
        }
    }
    return 0;
}

static int hamming_distance_single_test(bm_handle_t handle, int bits_len, int input1_num, int input2_num, bool rdm,
                                        long int *cpu_time, long int *tpu_time) {
    int ret = 0;
    bm_device_mem_t input1_dev_mem;
    bm_device_mem_t input2_dev_mem;
    bm_device_mem_t output_dev_mem;
    struct timeval t1, t2, t3, t4;

    int* input1_data = (int*)malloc(input1_num * bits_len * sizeof(int));
    int* input2_data = (int*)malloc(input2_num * bits_len * sizeof(int));
    int* output_ref  = (int*)malloc(input1_num * input2_num * sizeof(int));
    int* output_tpu  = (int*)malloc(input1_num * input2_num * sizeof(int));

    memset(input1_data, 0, input1_num * bits_len * sizeof(int));
    memset(input2_data, 0, input2_num * bits_len * sizeof(int));
    memset(output_ref,  0,  input1_num * input2_num * sizeof(int));
    memset(output_tpu,  0,  input1_num * input2_num * sizeof(int));

    // fill data
    if (rdm) {
        for(int i = 0; i < input1_num * bits_len; i++) {
            input1_data[i] = rand() % 10;
        }
        for(int i = 0; i < input2_num * bits_len; i++) {
            input2_data[i] = rand() % 20 + 1;
        }
    } else {
        printf("enter input1 %d elements:\n", input1_num);
        for (int i = 0; i < input1_num * bits_len; i++) {
            scanf("%d", &input1_data[i]);
        }
        printf("enter inpout2 %d elements:\n", input2_num);
        for (int i = 0; i < input2_num * bits_len; i++) {
            scanf("%d", &input2_data[i]);
        }
    }
    // native cal
    gettimeofday(&t1, NULL);
    native_cal_hammingDistance(input1_data, input2_data, output_ref, bits_len, input1_num, input2_num);
    gettimeofday(&t2, NULL);
    printf("Hm_distance CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *cpu_time += TIME_COST_US(t1, t2);
    // tpu_cal
    bm_malloc_device_byte(handle, &input1_dev_mem, input1_num * bits_len * sizeof(int));
    bm_malloc_device_byte(handle, &input2_dev_mem, input2_num * bits_len * sizeof(int));
    bm_malloc_device_byte(handle, &output_dev_mem, input1_num * input2_num * sizeof(int));
    bm_memcpy_s2d(handle, input1_dev_mem, input1_data);
    bm_memcpy_s2d(handle, input2_dev_mem, input2_data);
    gettimeofday(&t3, NULL);
    bmcv_hamming_distance(handle, input1_dev_mem, input2_dev_mem, output_dev_mem, bits_len, input1_num, input2_num);
    gettimeofday(&t4, NULL);
    printf("Hm_distance TPU using time = %ld(us)\n", TIME_COST_US(t3, t4));
    *tpu_time += TIME_COST_US(t3, t4);
    bm_memcpy_d2s(handle, output_tpu, output_dev_mem);
    // cmp
    if (0 != array_cmp_int32(output_ref, output_tpu, input1_num * input2_num, "tpu_hamming_distance", 0)) {
        ret = -1;
    };

    free(input1_data);
    free(input2_data);
    free(output_ref);
    free(output_tpu);
    bm_free_device(handle, input1_dev_mem);
    bm_free_device(handle, input2_dev_mem);
    bm_free_device(handle, output_dev_mem);
    return ret;
}

int main(int argc, char *argv[]) {
    printf("log will be saved in special_size_test_cv_hm_distance.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_hm_distance.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    unsigned int seed = tp.tv_nsec;
    srand(seed);
    printf("seed = %d\n", seed);
    int random = 1;
    int bits_len = 8;
    int sepcial_input1_num[] = {1, 10, 100};
    int sepcial_input2_num[] = {1, 10, 100, 500, 1000, 1500, 2000, 2500};
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }
    for(int i = 0; i < 3; i++) {
        int input1_num = sepcial_input1_num[i];
        for(int j = 0; j < 8; j++) {
            int input2_num = sepcial_input2_num[j];
            printf("input1_num: %d , input2_num: %d\n", input1_num, input2_num);
            long int cputime = 0;
            long int tputime = 0;
            long int *cpu_time = &cputime;
            long int *tpu_time = &tputime;
            for(int loop = 0; loop < 3; loop++) {
                if (0 != hamming_distance_single_test(handle, bits_len, input1_num, input2_num, random, cpu_time, tpu_time)) {
                    printf("------TEST HM_DISTANCE FAILED------\n");
                    return -1;
                }
            }
            printf("------average CPU time = %ldus------\n", cputime / 3);
            printf("------average TPU time = %ldus------\n", tputime / 3);
        }
    }
    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}