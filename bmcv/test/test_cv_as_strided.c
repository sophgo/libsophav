#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#define ERR_MAX 1e-6
#define DIV_UP(a, b) ( ((a) + (b) - 1) / (b) )
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

struct row_para {
    int input_row;
    int output_row;
    int row_stride;
};

struct col_para {
    int input_col;
    int output_col;
    int col_stride;
};

struct row_col_para {
    struct row_para row;
    struct col_para col;
};

typedef struct {
    int loop_num;
    int input_row;
    int input_col;
    int output_row;
    int output_col;
    int row_stride;
    int col_stride;
    bm_handle_t handle;
} cv_as_strided_thread_arg_t;

static int parameters_check(struct row_col_para row_col)
{
    int input_row = row_col.row.input_row;
    int output_row = row_col.row.output_row;
    int input_col = row_col.col.input_col;
    int output_col = row_col.col.output_col;

    if (input_row > 4096 || input_col > 4096 || output_row > 4096 || output_col > 4096){
        printf("Unsupported size : size_max = 4096 x 4096 \n");
        return -1;
    }
    return 0;
}

static void fill_input_data(float* data, int len)
{
    int i;
    struct timespec tp;

    if (clock_gettime(0, &tp) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    srand(tp.tv_nsec);

    for (i = 0; i < len; i++) {
        data[i] = (float)rand() / (float)RAND_MAX * 100;
    }
}

static int concat_input(float* input, int input_row, int input_col, int n)
{
    int original_size = input_row * input_col;
    int new_size = original_size * n;
    float* temp = (float*)malloc(new_size * sizeof(float));
    int i;
    int original_index = 0;

    if (input == NULL || n <= 1) {
        printf("the para is error!\n");
        return -1;
    }

    for (i = 0; i < new_size; i++) {
        original_index = i % original_size;
        temp[i] = input[original_index];
    }

    for (i = 0; i < new_size; i++) {
        input[i] = temp[i];
    }

    free(temp);
    return 0;
}

static int cmp_result(float* tpu_out, float* cpu_out, int n_Rows, int n_Cols)
{
    int i, j;
    int index = 0;

    for (i = 0; i < n_Rows; ++i) {
        for (j = 0; j < n_Cols; ++j) {
            index = i * n_Cols + j;
            if (fabs(tpu_out[index] - cpu_out[index]) > ERR_MAX) {
                printf("error[%d]: the tpu_out = %f, cpu_out = %f\n",
                        index, tpu_out[index], cpu_out[index]);
                return -1;
            }
        }
    }

    return 0;
}

static int cpu_as_strided(float* input, float* output, struct row_col_para row_col)
{
    int input_row = row_col.row.input_row;
    int output_row = row_col.row.output_row;
    int row_stride = row_col.row.row_stride;
    int input_col = row_col.col.input_col;
    int output_col = row_col.col.output_col;
    int col_stride = row_col.col.col_stride;
    int i, j;
    int input_size = input_row * input_col;
    int start = 0;
    int cur = start;

    if (input == NULL || output == NULL) {
        printf("the cpu input or output is null!\n");
        return -1;
    }

    for(i = 0; i < output_row; i++) {
        cur = start;
        for(j = 0; j < output_col; j++) {
            output[i * output_col + j] = input[cur];
            cur += col_stride;
            cur %= input_size;
        }
        start += row_stride;
        start %= input_size;
    }

    return 0;
}

static int tpu_as_strided(float* input, float* output, struct row_col_para row_col, int crit_val, bm_handle_t handle)
{
    int input_row = row_col.row.input_row;
    int output_row = row_col.row.output_row;
    int row_stride = row_col.row.row_stride;
    int input_col = row_col.col.input_col;
    int output_col = row_col.col.output_col;
    int col_stride = row_col.col.col_stride;
    int input_size = input_row * input_col;
    int output_size = output_row * output_col;
    bm_status_t ret = BM_SUCCESS;
    // bm_handle_t handle;
    bm_device_mem_t input_dev_mem, output_dev_mem;
    struct timeval t1, t2;

    // ret = bm_dev_request(&handle, 0);
    // if (BM_SUCCESS != ret){
    //     printf("request dev handle failed\n");
    //     return BM_ERR_FAILURE;
    // }

    ret = bm_malloc_device_byte(handle, &input_dev_mem, input_size * crit_val * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm malloc input dev mem failed\n");
        return -1;
    }
    ret = bm_malloc_device_byte(handle, &output_dev_mem, output_size * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm malloc output dev mem failed\n");
        return -1;
    }
    ret = bm_memcpy_s2d(handle, input_dev_mem, input);
    if (ret != BM_SUCCESS) {
        printf("bm mem cpy s2d failed\n");
        return -1;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_as_strided(handle, input_dev_mem, output_dev_mem, input_row * crit_val, \
                        input_col, output_row, output_col, row_stride, col_stride);
    if (ret != BM_SUCCESS) {
        printf("tpu_as_strided failed. ret = %d\n", ret);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("As_strided TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = bm_memcpy_d2s(handle, output, output_dev_mem);
    if (ret != BM_SUCCESS) {
        printf("bm mem cpy s2d failed\n");
        return -1;
    }

    bm_free_device(handle, input_dev_mem);
    bm_free_device(handle, output_dev_mem);
    // bm_dev_free(handle);

    return ret;
}

static int test_as_strided_random(struct row_col_para row_col, bm_handle_t handle)
{
    int input_row = row_col.row.input_row;
    int output_row = row_col.row.output_row;
    int row_stride = row_col.row.row_stride;
    int input_col = row_col.col.input_col;
    int output_col = row_col.col.output_col;
    int col_stride = row_col.col.col_stride;
    int input_size = input_row * input_col;
    int output_size = output_row * output_col;
    int ret = 0;
    int crit_val;
    struct timeval t1, t2;

    crit_val = DIV_UP((1 + (output_col - 1) * col_stride + (output_row - 1) * row_stride),
                    input_row * input_col);

    float* input_data = (float*)malloc(input_size  * crit_val * sizeof(float));
    float* tpu_output = (float*)malloc(output_size * sizeof(float));
    float* cpu_output = (float*)malloc(output_size * sizeof(float));

    fill_input_data(input_data, input_size);
    memset(tpu_output, 0, output_size * sizeof(float));
    memset(cpu_output, 0, output_size * sizeof(float));

    if (crit_val > 1) {
        ret = concat_input(input_data, input_row, input_col, crit_val);
        if (ret) {
            printf("concat input failed!\n");
            free(input_data);
            free(tpu_output);
            free(cpu_output);
            return ret;
        }
    }

    ret = tpu_as_strided(input_data, tpu_output, row_col, crit_val, handle);
    if (ret) {
        printf("tpu_as_strided failed!\n");
        free(input_data);
        free(tpu_output);
        free(cpu_output);
        return ret;
    }

    gettimeofday(&t1, NULL);
    ret = cpu_as_strided(input_data, cpu_output, row_col);
    if (ret) {
        printf("cpu_as_strided failed!\n");
        free(input_data);
        free(tpu_output);
        free(cpu_output);
        return ret;
    }
    gettimeofday(&t2, NULL);
    printf("As_strided CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = cmp_result(tpu_output, cpu_output, output_row, output_col);
    if (ret) {
        printf("tpu & cpu cmp failed!\n");
        free(input_data);
        free(tpu_output);
        free(cpu_output);
        return ret;
    }

    free(input_data);
    free(tpu_output);
    free(cpu_output);
    return ret;
}

void* test_as_strided(void* args) {
    cv_as_strided_thread_arg_t* cv_as_strided_thread_arg = (cv_as_strided_thread_arg_t*)args;
    struct row_col_para row_col;
    int loop = cv_as_strided_thread_arg->loop_num;
    row_col.row.input_row = cv_as_strided_thread_arg->input_row;
    row_col.col.input_col = cv_as_strided_thread_arg->input_col;
    row_col.row.output_row = cv_as_strided_thread_arg->output_row;
    row_col.col.output_col = cv_as_strided_thread_arg->output_col;
    row_col.row.row_stride = cv_as_strided_thread_arg->row_stride;
    row_col.col.col_stride = cv_as_strided_thread_arg->col_stride;
    bm_handle_t handle = cv_as_strided_thread_arg->handle;

    for (int i = 0; i < loop; ++i) {
        int ret = test_as_strided_random(row_col, handle);
        if (ret) {
            printf("------Test as_strided Failed!------\n");
            exit(-1);
        }
        printf("------Test as_strided Successed!------\n");
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
    int loop = 1;
    struct row_col_para row_col;
    row_col.row.input_row = 1 + rand() % 4096;
    row_col.col.input_col = 1 + rand() % 4096;
    row_col.row.output_row = 1 + rand() % 4096;
    row_col.col.output_col = 1 + rand() % 4096;
    row_col.row.row_stride = 1 + rand() % 4096;
    row_col.col.col_stride = 1 + rand() % 4096;
    int ret = 0;
    int i;
    int check = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: \n");
        printf("%s thread_num loop input_row input_col output_row output_col row_stride col_stride \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 2 1\n", args[0]);
        printf("%s 1 1 5 5 3 3 2 1 \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) row_col.row.input_row = atoi(args[3]);
    if (argc > 4) row_col.col.input_col = atoi(args[4]);
    if (argc > 5) row_col.row.output_row = atoi(args[5]);
    if (argc > 6) row_col.col.output_col = atoi(args[6]);
    if (argc > 7) row_col.row.row_stride = atoi(args[7]);
    if (argc > 8) row_col.col.col_stride = atoi(args[8]);
    check = parameters_check(row_col);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    printf("seed = %d\n", seed);
    // test for multi-thread
    pthread_t pid[thread_num];
    cv_as_strided_thread_arg_t cv_as_strided_thread_arg[thread_num];
    for (i = 0; i < thread_num; i++) {
        cv_as_strided_thread_arg[i].loop_num = loop;
        cv_as_strided_thread_arg[i].input_row = row_col.row.input_row;
        cv_as_strided_thread_arg[i].input_col = row_col.col.input_col;
        cv_as_strided_thread_arg[i].output_row = row_col.row.output_row;
        cv_as_strided_thread_arg[i].output_col = row_col.col.output_col;
        cv_as_strided_thread_arg[i].row_stride = row_col.row.row_stride;
        cv_as_strided_thread_arg[i].col_stride = row_col.col.col_stride;
        cv_as_strided_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_as_strided, &cv_as_strided_thread_arg[i]) != 0) {
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
