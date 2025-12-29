#include "bmcv_api_ext_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

enum Pad_mode {
    CONSTANT = 0,
    REFLECT = 1,
};

enum Win_mode {
    HANN = 0,
    HAMM = 1,
};

static void readBin_4b(const char* path, float* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 4, size, fp_src) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

int main()
{
    bm_handle_t handle;
    int ret = 0;
    int i;
    int L = 4096;
    int batch = 1;
    bool realInput = true;
    int pad_mode = REFLECT;
    int win_mode = HANN;
    int n_fft = 4096;
    int hop_length = 1024;
    bool norm = true;
    char* src_name = NULL;

    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    float* XRHost = (float*)malloc(L * batch * sizeof(float));
    float* XIHost = (float*)malloc(L * batch * sizeof(float));
    int num_frames = 1 + L / hop_length;
    int row_num = n_fft / 2 + 1;
    float* YRHost_tpu = (float*)malloc(batch * row_num * num_frames * sizeof(float));
    float* YIHost_tpu = (float*)malloc(batch * row_num * num_frames * sizeof(float));

    memset(XRHost, 0, L * batch * sizeof(float));
    memset(XIHost, 0, L * batch * sizeof(float));

    if (src_name != NULL) {
        readBin_4b(src_name, XRHost, L * batch);
    } else {
        for (i = 0; i < L * batch; i++) {
            XRHost[i] = (float)rand() / RAND_MAX;;
            XIHost[i] = realInput ? 0 : ((float)rand() / RAND_MAX);
        }
    }
    ret = bmcv_stft(handle, XRHost, XIHost, YRHost_tpu, YIHost_tpu, batch, L,
                    realInput, pad_mode, n_fft, win_mode, hop_length, norm);



    free(XRHost);
    free(XIHost);
    free(YRHost_tpu);
    free(YIHost_tpu);

    bm_dev_free(handle);
    return ret;
}