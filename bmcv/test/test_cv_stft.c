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
    int batch;
    int fs;
    int duration;
    int window;
    int nperseg;
    int noverlap;
    int nfft;
    bool return_onesided;
    int boundary;
    bool padded;
    bool real_input;
    bm_handle_t handle;
} cv_stft_thread_arg_t;

enum bdry{
    zeros = 0,
    constant,
    even,
    odd,
    none,
};

enum win{
    HANN = 0,
    HAMM = 1,
};

static int check_param(int fs, int duration, int window, int nperseg, int noverlap, int nfft, int boundary) {
    if (nperseg > fs * duration){
        printf("nperseg should not larger than signal length!\n");
        return -1;
    }
    if (noverlap >= nperseg) {
        printf("noverlap should less than nperseg!\n");
        return -1;
    }
    if (nfft < nperseg) {
        printf("nfft should not less than nperseg!\n");
        return -1;
    }
    if (window != HAMM && window != HANN) {
        printf("Unsupported window! Only support HAMM and HANN\n");
        return -1;
    }
    if (boundary != zeros && boundary != constant && boundary != even && boundary != odd && boundary != none) {
        printf("Unsupported boundary!\n");
        return -1;
    }
    return 0;
}

static void apply_window(float* window, int n, int win_mode)
{
    int i;

    if (win_mode == HANN) {
        for (i = 0; i < n; i++) {
            window[i] *= 0.5 * (1 - cos(2 * M_PI * (float)i / (n - 1)));
        }
    } else if (win_mode == HAMM) {
        for (i = 0; i < n; i++) {
            window[i] *= 0.54 - 0.46 * cos(2 * M_PI * (float)i / (n - 1));
        }
    }
}

static void dft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int length, bool forward)
{
    int i, j;
    float angle;

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
void transpose(int rows, int cols, float* input, float* output)
{
    int i, j;

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            output[j * rows + i] = input[i * cols + j];
        }
    }
}

static int cpu_stft(float* XRHost, float* XIHost, int batch, int xlen, int fs, int window, int nperseg, int noverlap,
                    int nfft, bool return_onesided, int boundary, bool padded, float* YRHost, float* YIHost, float *f, float *t)
{
    int hop = nperseg - noverlap;
    int xlen_ = 0;
    if (boundary == zeros || boundary == constant || boundary == even || boundary == odd) {
        xlen_ = xlen + 2 * hop;
    } else {
        xlen_ = xlen;
    }
    int padded_len = 0;
    if (padded) {
        padded_len = (xlen_ - nperseg) % hop == 0 ? 0 : hop - (xlen_ - nperseg) % hop;
    }
    xlen_ += padded_len;
    int L = 1 + (xlen_ - nperseg) / hop;
    int row_num = return_onesided ? nfft / 2 + 1 : nfft;

    float* input_XR = (float*)malloc(xlen_ * sizeof(float));
    float* input_XI = (float*)malloc(xlen_ * sizeof(float));
    float* window_XR = (float*)malloc(nperseg * sizeof(float));
    float* window_XI = (float*)malloc(nperseg * sizeof(float));
    float* wxr_padded = (float*)calloc(nfft, sizeof(float));
    float* wxi_padded = (float*)calloc(nfft, sizeof(float));
    float* YR = (float*)malloc(nfft * sizeof(float));
    float* YI = (float*)malloc(nfft * sizeof(float));
    float* YR_cpu= (float*)malloc(nfft * L * sizeof(float));
    float* YI_cpu = (float*)malloc(nfft * L * sizeof(float));

    for (int i = 0; i < row_num; i++) {
        f[i] = i * ((float)fs / nfft);
    }

    for (int l = 0; l < L; l++) {
        t[l] = (nperseg / 2.0 + l * hop) / fs;
    }

    for (int j = 0; j < batch; ++j) {
        /* Initialize the input signal */
        memset(input_XR, 0, xlen_ * sizeof(float));
        memset(input_XI, 0, xlen_ * sizeof(float));

        switch (boundary) {
        case zeros:
            // Zero-padding at both ends
            for (int i = 0; i < hop; i++) {
                input_XR[i] = 0.0f;                     // Left zero-padding
                input_XR[xlen_ - padded_len - hop + i] = 0.0f; // Right zero-padding
                input_XI[i] = 0.0f;                     // Left zero-padding
                input_XI[xlen_ - padded_len - hop + i] = 0.0f; // Right zero-padding
            }
            // Copy original signal to the center
            memcpy(input_XR + hop, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI + hop, XIHost + j * xlen, xlen * sizeof(float));
            break;
        case constant:
            for (int i = 0; i < hop; i++) {
                input_XR[i] = *(XRHost + j * xlen + hop);
                input_XR[xlen_ - padded_len - hop + i] = *(XRHost + j * xlen + xlen - 1);
                input_XI[i] = *(XIHost + j * xlen + hop);
                input_XI[xlen_ - padded_len - hop + i] = *(XIHost + j * xlen + xlen - 1);
            }
            memcpy(input_XR + hop, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI + hop, XIHost + j * xlen, xlen * sizeof(float));
            break;
        case even:
            // Even symmetry padding (mirroring with repetition of last element)
            for (int i = 0; i < hop; i++) {
                // Left padding: mirror from start
                input_XR[i] = *(XRHost + j * xlen + hop - 1 - i);
                input_XI[i] = *(XIHost + j * xlen + hop - 1 - i);
                // Right padding: mirror from end
                input_XR[xlen_ - padded_len - hop + i] = *(XRHost + j * xlen + xlen - 1 - i);
                input_XI[xlen_ - padded_len - hop + i] = *(XIHost + j * xlen + xlen - 1 - i);
            }
            // Copy original signal to the center
            memcpy(input_XR + hop, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI + hop, XIHost + j * xlen, xlen * sizeof(float));
            break;

        case odd:
            // Odd symmetry padding (mirroring with sign flip)
            for (int i = 0; i < hop; i++) {
                // Left padding: mirror with sign flip
                input_XR[i] = -*(XRHost + j * xlen + hop - i);  // Note: different indexing for odd symmetry
                input_XI[i] = -*(XIHost + j * xlen + hop - i);
                // Right padding: mirror with sign flip
                input_XR[xlen_ - padded_len - hop + i] = -*(XRHost + j * xlen + xlen - 2 - i);
                input_XI[xlen_ - padded_len - hop + i] = -*(XIHost + j * xlen + xlen - 2 - i);
            }
            // Copy original signal to the center
            memcpy(input_XR + hop, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI + hop, XIHost + j * xlen, xlen * sizeof(float));
            break;
        case none:
            memcpy(input_XR, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI, XIHost + j * xlen, xlen * sizeof(float));
            break;
        default:
            printf("Unsupported boundary value! Use original signal.\n");
            memcpy(input_XR, XRHost + j * xlen, xlen * sizeof(float));
            memcpy(input_XI, XIHost + j * xlen, xlen * sizeof(float));
            break;
        }

        for (int i = 0; i < padded_len; i++) {
            input_XR[xlen_ - padded_len + i] = 0;
            input_XI[xlen_ - padded_len + i] = 0;
        }
        /* calculate window & dft */
        for (int l = 0; l < L; l++) {
            memcpy(window_XR, input_XR + hop * l, nperseg * sizeof(float));
            memcpy(window_XI, input_XI + hop * l, nperseg * sizeof(float));
            apply_window(window_XR, nperseg, window);
            apply_window(window_XI, nperseg, window);

            memcpy(wxr_padded, window_XR, nperseg * sizeof(float));
            memcpy(wxi_padded, window_XI, nperseg * sizeof(float));

            dft(wxr_padded, wxi_padded, YR, YI, nfft, true);

            memcpy(&(YR_cpu[l * row_num]), YR, row_num * sizeof(float));
            memcpy(&(YI_cpu[l * row_num]), YI, row_num * sizeof(float));
        }
        transpose(L, row_num, YR_cpu, YRHost + j * row_num * L);
        transpose(L, row_num, YI_cpu, YIHost + j * row_num * L);
    }

    free(input_XR);
    free(input_XI);
    free(window_XR);
    free(window_XI);
    free(wxr_padded);
    free(wxi_padded);
    free(YR);
    free(YI);
    free(YR_cpu);
    free(YI_cpu);
    return 0;
}

static int cmp_res(float* YRHost_tpu, float* YIHost_tpu, float* YRHost_cpu,
                    float* YIHost_cpu, int batch, int row, int col)
{
    int i, j, k, index;
    int count = 0;

    for(i = 0; i < batch; ++i) {
        for (j = 0; j < row; ++j) {
            for (k = 0; k < col; ++k) {
                index = i * row * col + j * col + k;
                if ((fabs(YRHost_cpu[index] - YRHost_tpu[index]) > 0.01 && fabs((YRHost_cpu[index] - YRHost_tpu[index])/YRHost_cpu[index]) > 0.01) ||
                    (fabs((YIHost_cpu[index] - YIHost_tpu[index])/YIHost_cpu[index]) > 0.01 && fabs(YIHost_cpu[index] - YIHost_tpu[index]) > 0.01)) {
                    count++;
                }
            }
        }
    }
    if ((float)count / (2 * batch * row * col) > 0.01) {
        return count;
    }
    return 0;
}

static bm_status_t tpu_stft(float* XRHost, float* XIHost, int batch, int xlen, int fs, int window, int nperseg, int noverlap, int nfft,
                    bool return_onesided, int boundary, bool padded, bool real_input, float* YRHost, float* YIHost, float *f, float *t, bm_handle_t handle)
{
    bm_status_t ret = BM_SUCCESS;
    struct timeval t1, t2;
    ret = bmcv_stft(handle, XRHost, XIHost, batch, xlen, fs, window, nperseg, noverlap,
                    nfft, return_onesided, boundary, padded, real_input, YRHost, YIHost, f, t);
    if (ret != BM_SUCCESS) {
        printf("bmcv_stft failed!\n");
        return ret;
    }
    gettimeofday(&t1, NULL);
    ret = bmcv_stft(handle, XRHost, XIHost, batch, xlen, fs, window, nperseg, noverlap,
                    nfft, return_onesided, boundary, padded, real_input, YRHost, YIHost, f, t);
    if (ret != BM_SUCCESS) {
        printf("bmcv_stft failed!\n");
        return ret;
    }
    gettimeofday(&t2, NULL);
    printf("STFT TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    return ret;
}

static int test_stft(bm_handle_t handle, const int batch, const int fs, const int duration, const int window, const int nperseg, const int noverlap,
                     const int nfft, bool return_onesided, const int boundary, bool padded, bool real_input)
{
    int ret = 0;
    struct timeval t1, t2;
    int xlen = fs * duration;
    float* XRHost = (float*)malloc(batch * xlen * sizeof(float));
    float* XIHost = (float*)malloc(batch * xlen * sizeof(float));

    // Generate signal
    memset(XRHost, 0, xlen * batch * sizeof(float));
    memset(XIHost, 0, xlen * batch * sizeof(float));

    int hop = nperseg - noverlap;
    int xlen_ = 0;
    if (boundary == zeros || boundary == constant || boundary == even || boundary == odd) {
        xlen_ = xlen + 2 * hop;
    } else {
        xlen_ = xlen;
    }
    int padded_len = 0;
    if (padded) {
        padded_len = (xlen_ - nperseg) % hop == 0 ? 0 : hop - (xlen_ - nperseg) % hop;
    }
    xlen_ += padded_len;

    int L = 1 + (xlen_ - nperseg) / hop;
    int row_num = return_onesided ? nfft / 2 + 1 : nfft;
    float* YRHost_cpu = (float*)malloc(batch * L * row_num * sizeof(float));
    float* YIHost_cpu = (float*)malloc(batch * L * row_num * sizeof(float));
    float* YRHost_tpu = (float*)malloc(batch * L * row_num * sizeof(float));
    float* YIHost_tpu = (float*)malloc(batch * L * row_num * sizeof(float));

    printf("the xlen = %d, the batch=%d, the L=%d, the nfft=%d, the realinput = %d, the boundary=%d, the window=%d\n",
            xlen, batch, L, nfft, real_input, boundary, window);

    float *f = (float*)malloc(nfft * sizeof(float));
    float *t_sec = (float*)malloc(L * sizeof(float));

    gettimeofday(&t1, NULL);
    ret = cpu_stft(XRHost, XIHost, batch, xlen, fs, window, nperseg, noverlap, nfft, return_onesided, boundary, padded, YRHost_cpu, YIHost_cpu, f, t_sec);
    if (ret != 0) {
        printf("cpu_stft failed!\n");
        goto exit;
    }
    gettimeofday(&t2, NULL);
    printf("STFT CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    // tpu stft
    ret = tpu_stft(XRHost, XIHost, batch, xlen, fs, window, nperseg, noverlap, nfft, return_onesided, boundary, padded, real_input,
                   YRHost_tpu, YIHost_tpu, f, t_sec, handle);
    if (ret != 0) {
        printf("tpu_stft failed!\n");
        goto exit;
    }

    ret = cmp_res(YRHost_tpu, YIHost_tpu, YRHost_cpu, YIHost_cpu, batch, row_num, L);
    if (ret != 0) {
        printf("cmp_res fail!\n");
        printf("diff count = %d\n", ret);
        goto exit;
    }

exit:
    free(f);
    free(t_sec);
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
    const int loop = cv_stft_thread_arg->loop_num;
    int batch = cv_stft_thread_arg->batch;
    int fs = cv_stft_thread_arg->fs;
    int duration = cv_stft_thread_arg->duration;
    int window = cv_stft_thread_arg->window;
    int nperseg = cv_stft_thread_arg->nperseg;
    int noverlap = cv_stft_thread_arg->noverlap;
    int nfft = cv_stft_thread_arg->nfft;
    bool return_onesided = cv_stft_thread_arg->return_onesided;
    int boundary = cv_stft_thread_arg->boundary;
    bool padded = cv_stft_thread_arg->padded;
    bool real_input = cv_stft_thread_arg->real_input;
    bm_handle_t handle = cv_stft_thread_arg->handle;
    int i, ret = 0;

    for (i = 0; i < loop; ++i) {
        ret = test_stft(handle, batch, fs, duration, window, nperseg, noverlap, nfft, return_onesided, boundary, padded, real_input);
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
    int thread_num = 1;
    int batch = 1;
    int loop = 1;

    int fs = 1024;
    int duration = 10;
    int window = HAMM;
    int nperseg = 1024;
    int noverlap = 512;
    int nfft = 1215;
    bool return_onesided = true;
    int boundary = even;
    bool padded = true;
    bool real_input = true;
    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop batch fs duration window nperseg noverlap nfft return_onesided boundary padded real_input\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 2\n", args[0]);
        printf("%s 1 1 1 200 10 0 256 128 256 0 0 0 1\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) batch = atoi(args[3]);
    if (argc > 4) fs = atoi(args[4]);
    if (argc > 5) duration = atoi(args[5]);
    if (argc > 6) window = atoi(args[6]);
    if (argc > 7) nperseg = atoi(args[7]);
    if (argc > 8) noverlap = atoi(args[8]);
    if (argc > 9) nfft = atoi(args[9]);
    if (argc > 10) return_onesided = (atoi(args[10]) != 0);
    if (argc > 11) boundary = atoi(args[11]);
    if (argc > 12) padded = (atoi(args[12]) != 0);
    if (argc > 13) real_input = (atoi(args[13]) != 0);
    int check = check_param(fs, duration, window, nperseg, noverlap, nfft, boundary);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }

    pthread_t pid[thread_num];
    cv_stft_thread_arg_t cv_stft_thread_arg[thread_num];

    for (int i = 0; i < thread_num; i++) {
        cv_stft_thread_arg[i].loop_num = loop;
        cv_stft_thread_arg[i].batch = batch;
        cv_stft_thread_arg[i].fs = fs;
        cv_stft_thread_arg[i].duration = duration;
        cv_stft_thread_arg[i].window = window;
        cv_stft_thread_arg[i].nperseg = nperseg;
        cv_stft_thread_arg[i].noverlap = noverlap;
        cv_stft_thread_arg[i].nfft = nfft;
        cv_stft_thread_arg[i].return_onesided = return_onesided;
        cv_stft_thread_arg[i].boundary = boundary;
        cv_stft_thread_arg[i].padded = padded;
        cv_stft_thread_arg[i].real_input = real_input;
        cv_stft_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_thread_stft, &cv_stft_thread_arg[i]) != 0) {
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

    printf("------Test STFT All Success!------\n");
    bm_dev_free(handle);
    return ret;
}