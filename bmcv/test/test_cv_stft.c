#include "bmcv_api_ext_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#define ERR_MAX 1e-3
#define M_PI 3.14159265358979323846
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop_num;
    int length;
    int batch;
    bool real_input;
    int pad_mode;
    int win_mode;
    int n_fft;
    int hop_len;
    bool norm;
    char* src_name;
    char* dst_name;
    char* gold_name;
    bm_handle_t handle;
} cv_stft_thread_arg_t;

typedef struct {
    float real;
    float imag;
} Complex;

enum Win_mode {
    HANN = 0,
    HAMM = 1,
};

enum Pad_mode {
    CONSTANT = 0,
    REFLECT = 1,
};

extern int cpu_stft(float* XRHost, float* XIHost, float* YRHost_cpu, float* YIHost_cpu, int L,
            int batch, int n_fft, int hop_length, int num_frames, int pad_mode, int win_mode, bool norm);

static void readBin_4b(const char* path, float* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 4, size, fp_src) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

static void readBin_8b(const char* path, float* YRHost_tpu, float* YIHost_tpu, int size)
{
    FILE *fp_src = fopen(path, "rb");
    Complex* last_data = (Complex*)malloc(size * sizeof(Complex));
    int i;

    if (fread((void *)last_data, 8, size, fp_src) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
        free(last_data);
    };

    for (i = 0; i < size; ++i) {
        YRHost_tpu[i] = last_data[i].real;
        YIHost_tpu[i] = last_data[i].imag;
    }

    free(last_data);
    fclose(fp_src);
}

static void writeBin_8b(const char * path, float* input_XR, float* input_XI, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    Complex* last_data = (Complex*)malloc(size * sizeof(Complex));
    int i;

    for (i = 0; i < size; ++i) {
        last_data[i].real = input_XR[i];
        last_data[i].imag = input_XI[i];
    }

    if (fwrite((void *)last_data, 8, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    free(last_data);
    fclose(fp_dst);
}

static int cmp_res(float* YRHost_tpu, float* YIHost_tpu, float* YRHost_cpu,
                    float* YIHost_cpu, int batch, int row, int col)
{
    int i, j, k, index;

    for(i = 0; i < batch; ++i) {
        for (j = 0; j < row; ++j) {
            for (k = 0; k < col; ++k) {
                index = i * row * col + j * col + k;
                if (fabs(YRHost_cpu[index] - YRHost_tpu[index]) > ERR_MAX ||
                    fabs(YIHost_cpu[index] - YIHost_tpu[index]) > ERR_MAX) {
                        printf("the cpu_res[%d][%d][%d].real = %f, the cpu_res[%d][%d][%d].img = %f, \
                                the tpu_res[%d][%d][%d].real = %f, the tpu_res[%d][%d][%d].img = %f\n",
                                i, j, k, YRHost_cpu[index], i, j, k, YIHost_cpu[index],
                                i, j, k, YRHost_tpu[index], i, j, k, YIHost_tpu[index]);
                        return -1;
                }
            }
        }
    }
    return 0;
}

static bm_status_t tpu_stft(float* XRHost, float* XIHost, float* YRHost, float* YIHost, int L,
                            int batch, bool realInput, int pad_mode,
                            int win_mode, int n_fft, int hop_len, bool norm, bm_handle_t handle)
{
    bm_status_t ret = BM_SUCCESS;
    struct timeval t1, t2;

    gettimeofday(&t1, NULL);
    ret = bmcv_stft(handle, XRHost, XIHost, YRHost, YIHost, batch, L,
                    realInput, pad_mode, n_fft, win_mode, hop_len, norm);
    if (ret != BM_SUCCESS) {
        printf("bmcv_stft failed!\n");
        return ret;
    }
    gettimeofday(&t2, NULL);
    printf("STFT TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    return ret;
}

static int test_stft(int L, int batch, bool realInput, int pad_mode, int win_mode, bm_handle_t handle,
                    char* src_name, char* dst_name, char* gold_name, bool norm, int n_fft, int hop_length)
{
    int ret = 0;
    int i;
    struct timeval t1, t2;
    float* XRHost = (float*)malloc(L * batch * sizeof(float));
    float* XIHost = (float*)malloc(L * batch * sizeof(float));
    int num_frames = 1 + L / hop_length;
    int row_num = n_fft / 2 + 1;
    float* YRHost_cpu = (float*)malloc(batch * row_num * num_frames * sizeof(float));
    float* YIHost_cpu = (float*)malloc(batch * row_num * num_frames * sizeof(float));
    float* YRHost_tpu = (float*)malloc(batch * row_num * num_frames * sizeof(float));
    float* YIHost_tpu = (float*)malloc(batch * row_num * num_frames * sizeof(float));

    memset(XRHost, 0, L * batch * sizeof(float));
    memset(XIHost, 0, L * batch * sizeof(float));

    printf("the L=%d, the batch=%d, the n_fft=%d, the hop_len = %d, the realinput = %d, \
            the pad_mode=%d, the win_mode=%d, the noram = %d\n",
            L, batch, n_fft, hop_length, realInput, pad_mode, win_mode, norm);

    if (src_name != NULL) {
        readBin_4b(src_name, XRHost, L * batch);
    } else {
        for (i = 0; i < L * batch; i++) {
            XRHost[i] = (float)rand() / RAND_MAX;;
            XIHost[i] = realInput ? 0 : ((float)rand() / RAND_MAX);
        }
    }

    gettimeofday(&t1, NULL);
    ret = cpu_stft(XRHost, XIHost, YRHost_cpu, YIHost_cpu, L, batch, n_fft,
                    hop_length, num_frames, pad_mode, win_mode, norm);
    if (ret != 0) {
        printf("cpu_stft failed!\n");
        goto exit;
    }
    gettimeofday(&t2, NULL);
    printf("STFT CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    if (dst_name != NULL) {
        writeBin_8b(dst_name, YRHost_cpu, YIHost_cpu, batch * row_num * num_frames);
    }

    ret = tpu_stft(XRHost, XIHost, YRHost_tpu, YIHost_tpu, L, batch, realInput,
                    pad_mode, win_mode, n_fft, hop_length, norm, handle);
    if (ret != 0) {
        printf("tpu_stft failed!\n");
        goto exit;
    }

    if (gold_name != NULL) {
        printf("get the gold output!\n");
        readBin_8b(gold_name, YRHost_cpu, YIHost_cpu, batch * row_num * num_frames);
    }

    ret = cmp_res(YRHost_tpu, YIHost_tpu, YRHost_cpu, YIHost_cpu, batch, num_frames, row_num);
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

void* test_thread_stft(void* args) {
    cv_stft_thread_arg_t* cv_stft_thread_arg = (cv_stft_thread_arg_t*)args;
    int loop = cv_stft_thread_arg->loop_num;
    int length = cv_stft_thread_arg->length;
    int batch = cv_stft_thread_arg->batch;
    bool real_input = cv_stft_thread_arg->real_input;
    int pad_mode = cv_stft_thread_arg->pad_mode;
    int win_mode = cv_stft_thread_arg->win_mode;
    int n_fft = cv_stft_thread_arg->n_fft;
    int hop_len = cv_stft_thread_arg->hop_len;
    bool norm = cv_stft_thread_arg->norm;
    char *src_name = cv_stft_thread_arg->src_name;
    char *dst_name = cv_stft_thread_arg->dst_name;
    char *gold_name = cv_stft_thread_arg->gold_name;
    bm_handle_t handle = cv_stft_thread_arg->handle;
    int i, ret = 0;

    for (i = 0; i < loop; ++i) {
        ret = test_stft(length, batch, real_input, pad_mode, win_mode, handle, src_name, dst_name, gold_name, norm, n_fft, hop_len);
        if(ret) {
            printf("test_stft failed\n");
            exit(-1);
        }
    }
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
    int thread_num = 1;
    int length = 4096;
    int batch = 1;
    bool real_input = true;
    int loop = 1;
    int pad_mode = REFLECT;
    int win_mode = HANN;
    int n_fft = 4096;
    int hop_len = 1024;
    bool norm = true;
    char* src_name = NULL;
    char* dst_name = NULL;
    char* gold_name = NULL;

    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop length batch real_input pad_mode win_mode norm n_fft hop_len src_name dst_name gold_name) \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 2\n", args[0]);
        printf("%s 1 1 4096 1 1 1 1 1 4096 1024\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) length = atoi(args[3]);
    if (argc > 4) batch = atoi(args[4]);
    if (argc > 5) real_input = atoi(args[5]);
    if (argc > 6) pad_mode = atoi(args[6]);
    if (argc > 7) win_mode = atoi(args[7]);
    if (argc > 8) norm = atoi(args[8]);
    if (argc > 9) n_fft = atoi(args[9]);
    if (argc > 10) hop_len = atoi(args[10]);
    if (argc > 11) src_name = args[11];
    if (argc > 12) dst_name = args[12];
    if (argc > 13) gold_name = args[13];

    pthread_t pid[thread_num];
    cv_stft_thread_arg_t cv_stft_thread_arg[thread_num];

    for (i = 0; i < thread_num; i++) {
        cv_stft_thread_arg[i].loop_num = loop;
        cv_stft_thread_arg[i].length = length;
        cv_stft_thread_arg[i].batch = batch;
        cv_stft_thread_arg[i].real_input = real_input;
        cv_stft_thread_arg[i].pad_mode = pad_mode;
        cv_stft_thread_arg[i].win_mode = win_mode;
        cv_stft_thread_arg[i].norm = norm;
        cv_stft_thread_arg[i].n_fft = n_fft;
        cv_stft_thread_arg[i].hop_len = hop_len;
        cv_stft_thread_arg[i].src_name = src_name;
        cv_stft_thread_arg[i].dst_name = dst_name;
        cv_stft_thread_arg[i].gold_name = gold_name;
        cv_stft_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_thread_stft, &cv_stft_thread_arg[i]) != 0) {
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

    printf("------Test STFT All Success!------\n");
    bm_dev_free(handle);
    return ret;
}