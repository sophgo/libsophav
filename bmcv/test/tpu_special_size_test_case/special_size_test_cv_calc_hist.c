#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#define CHN_NUM_MAX 3
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

const int channels[CHN_NUM_MAX] = {0, 1, 2};
const int histSizes[CHN_NUM_MAX] = {32, 32, 32};
const float ranges[CHN_NUM_MAX * 2] = {0, 256, 0, 256, 0, 256};

struct frame_size {
    int height;
    int width;
};

struct hist_para {
    int chn_num; /*1 or 2 or 3 */
    int out_dim; /*if C=1, dim =1; if C=2, dim = 1 / 2; if C=3; dim = 1 / 2 / 3*/
    int data_type; /* 0 is float, 1 is uint8 */
    int op; /* 0 is no weighted, 1 is weighted */
};

typedef struct {
    int loop;
    int height;
    int width;
    int chn_num;
    int data_type;
    int out_dim;
    int op;
    bm_handle_t handle;
} cv_calc_hist_thread_arg_t;

// static int parameters_check(struct frame_size frame, struct hist_para para)
// {
//     int error = 0;
//     if (frame.height > 4096 || frame.width > 4096) {
//         printf("Unsupported size : size_max = 4096 x 4096 \n");
//         error = -1;
//     }
//     if (para.chn_num != 1 && para.chn_num != 2 && para.chn_num != 3) {
//         printf("parameters error : channel = 1 / 2 / 3 \n");
//         error = -1;
//     }
//     if (para.out_dim > para.chn_num) {
//         printf("parameters error : out_dim <= chn_num \n");
//         error = -1;
//     }
//     if (para.data_type != 0 && para.data_type != 1) {
//         printf("parameters error : data_type = 0/1, 0 is float, 1 is uint8 \n");
//         error = -1;
//     }
//     if (para.op != 0 && para.op != 1) {
//         printf("parameters error : op = 0/1, 0 is no weighted, 1 is weighted \n");
//         error = -1;
//     }
//     return error;
// }

static int cmp_result(float* out_tpu, float* out_cpu, int len)
{
    int ret = 0;
    int i;

    for (i = 0; i < len; ++i) {
        if (out_tpu[i] != out_cpu[i]) {
           printf("error index is %d, the tpu is%f, the cpu is%f, failed!\n", i, out_tpu[i], out_cpu[i]);
           return -1;
        }
    }
    return ret;
}

static int tpu_calc_hist(void* input_host, float* output_tpu, struct frame_size* frame,
                        struct hist_para* para, int totalHists, const float* weighted, bm_handle_t handle, long int* tpu_time)
{
    bm_device_mem_t input, output;
    int H = frame->height;
    int W = frame->width;
    int C = para->chn_num;
    int dim = para->out_dim;
    int data_type = para->data_type;
    int op = para->op;
    bm_status_t ret = BM_SUCCESS;
    struct timeval t1, t2;

    ret = bm_malloc_device_byte(handle, &output, totalHists * sizeof(float));
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte output float failed. ret = %d\n", ret);
        bm_dev_free(handle);
        return -1;
    }

    if (data_type == 0) {
        ret = bm_malloc_device_byte(handle, &input, C * H * W * sizeof(float));
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte input float failed. ret = %d\n", ret);
            bm_free_device(handle, output);
            bm_dev_free(handle);
            return -1;
        }
        ret = bm_memcpy_s2d(handle, input, input_host);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d float failed. ret = %d\n", ret);
            bm_free_device(handle, input);
            bm_free_device(handle, output);
            bm_dev_free(handle);
            return -1;
        }
        if (op == 0) {
            gettimeofday(&t1, NULL);
            ret = bmcv_calc_hist(handle, input, output, C, H, W, channels, dim, histSizes, ranges, data_type);
            if (ret != BM_SUCCESS) {
                printf("bmcv_calc_hist float failed. ret = %d\n", ret);
                bm_free_device(handle, input);
                bm_free_device(handle, output);
                bm_dev_free(handle);
                return -1;
            }
            gettimeofday(&t2, NULL);
            printf("Calc_hist TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
            *tpu_time += TIME_COST_US(t1, t2);
        } else {
            gettimeofday(&t1, NULL);
            ret = bmcv_calc_hist_with_weight(handle, input, output, weighted, C, H, W, \
                                            channels, dim, histSizes, ranges, data_type);
            if (ret != BM_SUCCESS) {
                printf("bmcv_calc_hist float failed. ret = %d\n", ret);
                bm_free_device(handle, input);
                bm_free_device(handle, output);
                bm_dev_free(handle);
                return -1;
            }
            gettimeofday(&t2, NULL);
            printf("Calc_hist TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
            *tpu_time += TIME_COST_US(t1, t2);
        }
    } else {
        ret = bm_malloc_device_byte(handle, &input, C * H * W * sizeof(uint8_t));
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte intput uint8_t failed. ret = %d\n", ret);
            bm_free_device(handle, output);
            bm_dev_free(handle);
            return -1;
        }
        ret = bm_memcpy_s2d(handle, input, input_host);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d uint8_t failed. ret = %d\n", ret);
            bm_free_device(handle, input);
            bm_free_device(handle, output);
            bm_dev_free(handle);
            return -1;
        }
        if (op == 0) {
            gettimeofday(&t1, NULL);
            ret = bmcv_calc_hist(handle, input, output, C, H, W, channels, dim, histSizes, ranges, data_type);
            if (ret != BM_SUCCESS) {
                printf("bmcv_calc_hist_weighted uint8_t failed. ret = %d\n", ret);
                bm_free_device(handle, input);
                bm_free_device(handle, output);
                bm_dev_free(handle);
                return -1;
            }
            gettimeofday(&t2, NULL);
            printf("Calc_hist TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
            *tpu_time += TIME_COST_US(t1, t2);
        } else {
            gettimeofday(&t1, NULL);
            ret = bmcv_calc_hist_with_weight(handle, input, output, weighted, C, H, W, \
                                            channels, dim, histSizes, ranges, data_type);
            if (ret != BM_SUCCESS) {
                printf("bmcv_calc_hist_weighted uint8_t failed. ret = %d\n", ret);
                bm_free_device(handle, input);
                bm_free_device(handle, output);
                bm_dev_free(handle);
                return -1;
            }
            gettimeofday(&t2, NULL);
            printf("Calc_hist TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
            *tpu_time += TIME_COST_US(t1, t2);
        }
    }

    ret = bm_memcpy_d2s(handle, output_tpu, output);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed. ret = %d\n", ret);
        bm_free_device(handle, input);
        bm_free_device(handle, output);
        bm_dev_free(handle);
        return -1;
    }

    bm_free_device(handle, input);
    bm_free_device(handle, output);
    return 0;
}

static int cpu_calc_Hist_1d(void* input_host, struct frame_size* frame, struct hist_para* para, \
                            float* out_put, const float* weighted)
{
    int H = frame->height;
    int W = frame->width;
    int data_type = para->data_type; /* 0 is fp32, 1 is u8 */
    int HistSize_1D = histSizes[0];
    float binX_len = (float)(HistSize_1D / (ranges[1] - ranges[0]));
    int op = para->op;
    int binX;
    int i;

    if (input_host == NULL || frame == NULL || para == NULL || weighted == NULL) {
        printf("cpu_calc_hist 1d param error!\n");
        return -1;
    }

    if (data_type == 0) {
        float* input = (float*)input_host;

        for (i = 0; i < W * H; ++i) {

            binX = (int)(input[i] * binX_len);

            if (binX >= 0 && binX < HistSize_1D ) {
                if (op == 0) {
                    out_put[binX]++;
                } else {
                    out_put[binX] += weighted[i];
                }

            }
        }
    } else {
        uint8_t* input = (uint8_t*)input_host;

        for (i = 0; i < W * H; ++i) {
            binX = (int)(input[i] * binX_len);

            if (binX >= 0 && binX < HistSize_1D ) {
                if (op == 0) {
                    out_put[binX]++;
                } else {
                    out_put[binX] += weighted[i];
                }
            }
        }
    }

    return 0;

}

static int cpu_calc_Hist_2d(void* input_host, struct frame_size* frame, struct hist_para* para,
                            float* out_put, const float* weighted)
{
    int H = frame->height;
    int W = frame->width;
    int data_type = para->data_type; /* 0 is fp32, 1 is u8 */
    int HistSize_1D = histSizes[0];
    int HistSize_2D = histSizes[1];
    float binX_len = (float)(HistSize_1D / (ranges[1] - ranges[0]));
    float binY_len = (float)(HistSize_2D / (ranges[3] - ranges[2]));
    int binX, binY;
    int op = para->op;
    int i;

    if (input_host == NULL || frame == NULL || para == NULL || weighted == NULL) {
        printf("cpu_calc_hist 2d param error!\n");
        return -1;
    }

    if (data_type == 0) {
        float* input = (float*)input_host;

        for (i = 0; i < W * H; ++i) {
            binX = (int)(input[i] * binX_len);
            binY = (int)(input[i + W * H] * binY_len);

            if (binX >= 0 && binX < HistSize_1D && binY >= 0 && binY < HistSize_2D) {
                if (op == 0) {
                    out_put[binX * HistSize_2D + binY]++;
                } else {
                    out_put[binX * HistSize_2D + binY] += weighted[i];
                }
            }
        }
    } else {
        uint8_t* input = (uint8_t*)input_host;

        for (i = 0; i < W * H; ++i) {
            binX = (int)(input[i] * binX_len);
            binY = (int)(input[i + W * H] * binY_len);
            if (binX >= 0 && binX < HistSize_1D && binY >= 0 && binY < HistSize_2D) {
                if (op == 0) {
                    out_put[binX * HistSize_2D + binY]++;
                } else {
                    out_put[binX * HistSize_2D + binY] += weighted[i];
                }
            }
        }
    }
    return 0;
}

static int cpu_calc_Hist_3d(void* input_host, struct frame_size* frame, struct hist_para* para,
                            float* out_put, const float* weighted)
{
    int H = frame->height;
    int W = frame->width;
    int data_type = para->data_type; /* 0 is fp32, 1 is u8 */
    int HistSize_1D = histSizes[0];
    int HistSize_2D = histSizes[1];
    int HistSize_3D = histSizes[2];
    float binX_len = (float)(HistSize_1D / (ranges[1] - ranges[0]));
    float binY_len = (float)(HistSize_2D / (ranges[3] - ranges[2]));
    float binZ_len = (float)(HistSize_3D / (ranges[5] - ranges[4]));
    int op = para->op;
    int binX, binY, binZ;
    int i;

    if (input_host == NULL || frame == NULL || para == NULL || weighted == NULL) {
        printf("cpu_calc_hist 3d param error!\n");
        return -1;
    }

    if (data_type == 0) {
        float* input = (float*)input_host;

        for (i = 0; i < W * H; ++i) {
            binX = (int)(input[i] * binX_len);
            binY = (int)(input[i + W * H] * binY_len);
            binZ = (int)(input[i + 2 * W * H] * binZ_len);

            if (binX >= 0 && binX < HistSize_1D && binY >= 0 &&
                binY < HistSize_2D && binZ >= 0 && binZ < HistSize_3D) {
                if (op == 0) {
                    out_put[binX * (HistSize_2D * HistSize_3D) + binY * HistSize_3D + binZ]++;
                } else {
                    out_put[binX * (HistSize_2D * HistSize_3D) + binY * HistSize_3D + binZ] += weighted[i];
                }
            }
        }
    } else {
        uint8_t* input = (uint8_t*)input_host;

        for (i = 0; i < W * H; ++i) {
            binX = (int)(input[i] * binX_len);
            binY = (int)(input[i + W * H] * binY_len);
            binZ = (int)(input[i + 2 * W * H] * binZ_len);
            if (binX >= 0 && binX < HistSize_1D && binY >= 0 &&
                binY < HistSize_2D && binZ >= 0 && binZ < HistSize_3D) {
                if (op == 0) {
                    out_put[binX * (HistSize_2D * HistSize_3D) + binY * HistSize_3D + binZ]++;
                } else {
                    out_put[binX * (HistSize_2D * HistSize_3D) + binY * HistSize_3D + binZ] += weighted[i];
                }
            }
        }
    }
    return 0;
}

static int cpu_calc_hist(void* input_host, float* output_cpu, struct frame_size* frame,
                        struct hist_para* para, const float* weighted)
{
    int dim = para->out_dim;
    int ret = 0;

    if (input_host == NULL || output_cpu == NULL || frame == NULL \
        || para == NULL || weighted == NULL) {
        printf("cpu_calc_hist param error!\n");
        return -1;
    }

    switch (dim) {
        case 1: {
            ret = cpu_calc_Hist_1d(input_host, frame, para, output_cpu, weighted);
            if (ret) {
                printf("cpu_calc_Hist_1d failed!\n");
                return -1;
            }
            break;
        }
        case 2: {
            ret = cpu_calc_Hist_2d(input_host, frame, para, output_cpu, weighted);
            if (ret) {
                printf("cpu_calc_Hist_2d failed!\n");
                return -1;
            }
            break;
        }
        case 3: {

            ret = cpu_calc_Hist_3d(input_host, frame, para, output_cpu, weighted);
            if (ret) {
                printf("cpu_calc_Hist_3d failed!\n");
                return -1;
            }
            break;
        }
    }

    return 0;
}

static int test_calc_hist_random(struct frame_size* frame, struct hist_para* para, bm_handle_t handle, long int* cpu_time, long int* tpu_time)
{
    int H = frame->height;
    int W = frame->width;
    int C = para->chn_num;
    int dim = para->out_dim; /*if C=1, dim =1; if C=2, dim = 1 / 2; if C=3; dim = 1 / 2 / 3*/
    int totalHists = 1;
    int data_type = para->data_type;
    int ret = 0;
    int i, j;
    int Num = 0;
    struct timeval t1, t2;

    for (i = 0; i < dim; ++i) {
        totalHists *= histSizes[i];
    }

    float* out_tpu = (float*)malloc(totalHists * sizeof(float));
    float* out_cpu = (float*)malloc(totalHists * sizeof(float));

    memset(out_tpu, 0, totalHists * sizeof(float));
    memset(out_cpu, 0, totalHists * sizeof(float));

    if (data_type == 0) {
        float* input_host = (float*)malloc(C * H * W * sizeof(float));
        float* weighted = (float*)malloc(C * H * W * sizeof(float));

        for (i = 0; i < C; ++i) {
            for (j = 0; j < H * W; ++j) {
                Num = (int)ranges[2 * C - 1];
                input_host[i * H * W + j] = (float)(rand() % Num); /* fill image */
            }
        }
        for (i = 0; i < H * W; ++i) {
            weighted[i] = (float)(rand() % 10); /* fill weighted */
        }

        ret = tpu_calc_hist((void*)input_host, out_tpu, frame, para, totalHists, weighted, handle, tpu_time);
        if (ret) {
            printf("tpu_calc_hist float failed!\n");
            free(input_host);
            free(weighted);
            goto exit;
        }

        gettimeofday(&t1, NULL);
        ret = cpu_calc_hist((void*)input_host, out_cpu, frame, para, weighted);
        if (ret) {
            printf("cpu_calc_hist float failed!\n");
            free(input_host);
            free(weighted);
            goto exit;
        }
        gettimeofday(&t2, NULL);
        printf("Calc_hist CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
        *cpu_time += TIME_COST_US(t1, t2);

        free(input_host);
        free(weighted);
    } else {
        uint8_t* input_host = (uint8_t*)malloc(C * H * W * sizeof(uint8_t));
        float* weighted = (float*)malloc(C * H * W * sizeof(float));

        for (i = 0; i < C; ++i) {
            for (j = 0; j < H * W; ++j) {
                Num = (int)ranges[2 * C - 1];
                input_host[i * H * W + j] = (uint8_t)(rand() % Num); /*fill image*/
            }
        }
        for (i = 0; i < H * W; ++i) {
            weighted[i] = (float)(rand() % 10); /* fill weighted */
        }

        ret = tpu_calc_hist((void*)input_host, out_tpu, frame, para, totalHists, weighted, handle, tpu_time);
        if (ret) {
            printf("tpu_calc_hist float failed!\n");
            free(input_host);
            free(weighted);
            goto exit;
        }

        gettimeofday(&t1, NULL);
        ret = cpu_calc_hist((void*)input_host, out_cpu, frame, para, weighted);
        if (ret) {
            printf("cpu_calc_hist float failed!\n");
            free(input_host);
            free(weighted);
            goto exit;
        }
        gettimeofday(&t2, NULL);
        printf("Calc_hist CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
        *cpu_time += TIME_COST_US(t1, t2);
        free(input_host);
        free(weighted);
    }

    ret = cmp_result(out_tpu, out_cpu, totalHists);
    if (ret) {
        printf("The result compare failed. ret = %d\n", ret);
        goto exit;
    }

exit:
    free(out_tpu);
    free(out_cpu);
    return ret;
}

// void* test_calc_hist(void* args) {
//     cv_calc_hist_thread_arg_t* cv_calc_hist_thread_arg = (cv_calc_hist_thread_arg_t*)args;
//     struct frame_size frame;
//     struct hist_para para;
//     int loop = cv_calc_hist_thread_arg->loop;
//     frame.height = cv_calc_hist_thread_arg->height;
//     frame.width = cv_calc_hist_thread_arg->width;
//     para.chn_num = cv_calc_hist_thread_arg->chn_num;
//     para.data_type = cv_calc_hist_thread_arg->data_type;
//     para.out_dim = cv_calc_hist_thread_arg->out_dim;
//     para.op = cv_calc_hist_thread_arg->op;
//     bm_handle_t handle = cv_calc_hist_thread_arg->handle;
//     for (int i = 0; i < loop; ++i) {
//         int ret = test_calc_hist_random(&frame, &para, handle);
//         if (ret) {
//             printf("------Test calc_hist Failed!------\n");
//             return (void*)-1;
//         }
//     }
//     printf("------Test calc_hist Successed!------\n");
//     return (void*)0;
// }

// int main(int argc, char* args[])
// {
//     struct timespec tp;
//     clock_gettime(0, &tp);
//     int seed = tp.tv_nsec;
//     srand(seed);
//     int thread_num = 1;
//     int loop = 1;
//     struct frame_size frame;
//     frame.height = 1 + rand() % 1024;
//     frame.width = 1 + rand() % 1024;
//     struct hist_para para;
//     para.chn_num = 1 + rand() % 3;
//     para.data_type = rand() % 2;
//     para.out_dim = 1 + rand() % para.chn_num;
//     para.op = rand() % 2;
//     int ret = 0;
//     int i;
//     int check = 0;
//     bm_handle_t handle;
//     ret = bm_dev_request(&handle, 0);
//     if (ret != BM_SUCCESS) {
//         printf("bm_dev_request failed. ret = %d\n", ret);
//         return -1;
//     }

//     if (argc == 2 && atoi(args[1]) == -1) {
//         printf("%s thread loop height width chn_num type out_dim op \n", args[0]);
//         printf("example:\n");
//         printf("%s \n", args[0]);
//         printf("%s 2\n", args[0]);
//         printf("%s 1 3\n", args[0]);
//         printf("%s 1 1 512 512 1 0 1 0 \n", args[0]);
//         return 0;
//     }

//     if (argc > 1) thread_num = atoi(args[1]);
//     if (argc > 2) loop = atoi(args[2]);
//     if (argc > 3) frame.height = atoi(args[3]);
//     if (argc > 4) frame.width = atoi(args[4]);
//     if (argc > 5) para.chn_num = atoi(args[5]);
//     if (argc > 6) para.data_type = atoi(args[6]);
//     if (argc > 7) para.out_dim = atoi(args[7]);
//     if (argc > 8) para.op = atoi(args[8]);
//     check = parameters_check(frame, para);
//     if (check) {
//         printf("Parameters Failed! \n");
//         return check;
//     }

//     printf("seed = %d\n", seed);
//     // test for multi-thread
//     pthread_t pid[thread_num];
//     cv_calc_hist_thread_arg_t cv_calc_hist_thread_arg[thread_num];
//     for (i = 0; i < thread_num; i++) {
//         cv_calc_hist_thread_arg[i].loop = loop;
//         cv_calc_hist_thread_arg[i].height = frame.height;
//         cv_calc_hist_thread_arg[i].width = frame.width;
//         cv_calc_hist_thread_arg[i].chn_num = para.chn_num;
//         cv_calc_hist_thread_arg[i].data_type = para.data_type;
//         cv_calc_hist_thread_arg[i].out_dim = para.out_dim;
//         cv_calc_hist_thread_arg[i].op = para.op;
//         cv_calc_hist_thread_arg[i].handle = handle;
//         if (pthread_create(&pid[i], NULL, test_calc_hist, &cv_calc_hist_thread_arg[i]) != 0) {
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

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_calc_hist.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_calc_hist.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return -1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    struct frame_size frame;
    struct hist_para para;
    int height[9] = {1, 256, 512, 1024, 720, 1080, 1440, 2160, 4096};
    int width[9] = {1, 256, 512, 1024, 1280, 1920, 2560, 3840, 4096};
    int chn_para[3] = {1, 2, 3};
    //int outdim_para[] = {1, 1, 2, 1, 2, 3};
    int data_type_para[2] = {0, 1};
    int op_para[2] = {0, 1};
    bm_handle_t handle;
    int loop = 0;
    int i, j, k, l, m;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    for (i = 0; i < 2; ++i) {
        para.op = op_para[i];
        for (j = 0; j < 2; ++j) {
            para.data_type = data_type_para[j];
            for (k = 0; k < 3; ++k) {
                para.chn_num = chn_para[k];
                for (l = 1; l <= chn_para[k]; ++l) {
                    para.out_dim = l;
                    for (m = 0; m < 9; ++m) {
                        frame.height = height[m];
                        frame.width = width[m];
                        printf("the input_hieght = %d, the input_width = %d, the op = %d, the data_type = %d, the chn_num = %d, the out_dim = %d\n", \
                                frame.height, frame.width, para.op, para.data_type, para.chn_num, para.out_dim);
                        long int cputime = 0;
                        long int tputime = 0;
                        for(loop = 0; loop < 10; loop++) {
                            ret = test_calc_hist_random(&frame, &para, handle, &cputime, &tputime);
                            if (ret) {
                                printf("------TEST CV_CALC_HIST FAILED------\n");
                                return -1;
                            }
                        }
                        printf("-----------average CPU time = %ldus----------\n", cputime / 10);
                        printf("-----------average TPU time = %ldus----------\n", tputime / 10);
                    }
                }
            }
        }
    }
    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}