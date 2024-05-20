#include <stdio.h>
#include "bmcv_api_ext_c.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <float.h>
#include <sys/time.h>
#include <pthread.h>

pthread_mutex_t lock;
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop_num;
    int height;
    int width;
    int use_real_img;
    char* input_path;
    char* output_path;
    bm_handle_t handle;
} cv_quantify_thread_arg_t;

static void read_bin(const char *input_path, float *input_data, int width, int height)
{
    FILE *fp_src = fopen(input_path, "rb");
    if (fp_src == NULL)
    {
        printf("无法打开输出文件 %s\n", input_path);
        return;
    }
    if(fread(input_data, sizeof(float), width * height, fp_src) != 0)
        printf("read image success\n");
    fclose(fp_src);
}

static void write_bin(const char *output_path, unsigned char *output_data, int width, int height)
{
    FILE *fp_dst = fopen(output_path, "wb");
    if (fp_dst == NULL)
    {
        printf("无法打开输出文件 %s\n", output_path);
        return;
    }
    fwrite(output_data, sizeof(int), width * height, fp_dst);
    fclose(fp_dst);
}

float random_float(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

static void fill(
        float* input,
        int channel,
        int width,
        int height) {
    for (int i = 0; i < channel; i++) {
        for (int j = 0; j < height; j++) {
            for(int k = 0; k < width; k++){
                float num = random_float(-100.0f, 300.0f);
                input[i * width * height + j * width + k] = num;
            }
        }
    }
}

static int quantify_cpu(
        float* input,
        unsigned char* output,
        int height,
        int width) {
    for(int i = 0; i < width * height * 3; i++) {
        if (input[i] < 0.0f) {
            output[i] = 0;
        } else if(input[i] > 255.0f) {
            output[i] = 255;
        } else {
            output[i] = (unsigned char)input[i];
        }
    }
    return 0;
}

static int quantify_tpu(
        float* input,
        unsigned char* output,
        int height,
        int width,
        bm_handle_t handle,
        long int *tpu_time) {

    bm_image input_img;
    bm_image output_img;
    struct timeval t1, t2;
    pthread_mutex_lock(&lock);
    bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_RGB_PLANAR, DATA_TYPE_EXT_FLOAT32, &input_img, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input_img, 1);
    bm_image_alloc_dev_mem(output_img, 1);
    float* in_ptr[1] = {input};
    bm_image_copy_host_to_device(input_img, (void **)in_ptr);
    gettimeofday(&t1, NULL);
    bmcv_image_quantify(handle, input_img, output_img);
    gettimeofday(&t2, NULL);
    printf("Quantify TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);
    unsigned char* out_ptr[1] = {output};
    bm_image_copy_device_to_host(output_img, (void **)out_ptr);
    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);
    pthread_mutex_unlock(&lock);
    return 0;
}

static int cmp(
    unsigned char *got,
    unsigned char *exp,
    int len) {
    for (int i = 0; i < len; i++) {
        if (got[i] != exp[i]) {
            printf("cmp error: idx=%d  exp=%d  got=%d\n", i, exp[i], got[i]);
            return -1;
        }
    }
    return 0;
}

static int test_quantify_random(
        int height,
        int width,
        int use_real_img,
        char *input_path,
        char *output_path,
        bm_handle_t handle,
        long int *cpu_time,
        long int *tpu_time) {
    int ret;
    struct timeval t1, t2;

    float* input_data = (float*)malloc(width * height * 3 * sizeof(float));
    unsigned char* output_cpu = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
    unsigned char* output_tpu = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
    if(use_real_img == 1){
        read_bin(input_path, input_data, width, height);
    } else {
        fill(input_data, 3, width, height);
    }
    gettimeofday(&t1, NULL);
    ret = quantify_cpu(input_data, output_cpu, height, width);
    gettimeofday(&t2, NULL);
    printf("Quantify CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *cpu_time += TIME_COST_US(t1, t2);
    if(ret != 0){
        free(input_data);
        free(output_cpu);
        free(output_tpu);
        return ret;
    }

    ret = quantify_tpu(input_data, output_tpu, height, width, handle, tpu_time);
    if(ret != 0){
        free(input_data);
        free(output_cpu);
        free(output_tpu);
        return ret;
    }
    ret = cmp(output_tpu, output_cpu, width * height * 3);
    if (ret == 0) {
        if (use_real_img == 1) {
            write_bin(output_path, output_tpu, width, height);
        }
    } else {
        printf("cpu and tpu failed to compare \n");
    }
    free(input_data);
    free(output_cpu);
    free(output_tpu);
    return ret;
}

int main(int argc, char* args[]) {
    printf("log will be saved in special_size_test_cv_quantify.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_quantify.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    unsigned int seed = tp.tv_nsec;
    srand(seed);
    int use_real_img = 0;
    int special_height[9] = {1, 100, 400, 800, 720, 1080, 1440, 2160, 4096};
    int special_width[9] = {1, 100, 400, 800, 1280, 1920, 2560, 3840, 4096};
    char *input_path = NULL;
    char *output_path = NULL;
    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    for(int i = 0; i < 9; i++) {
        int width = special_width[i];
        int height = special_height[i];
        printf("width: %d , height: %d\n", width, height);
        long int cputime = 0;
        long int tputime = 0;
        long int *cpu_time = &cputime;
        long int *tpu_time = &tputime;
        for(int loop = 0; loop < 3; loop++) {
            if (0 != test_quantify_random(height, width, use_real_img, input_path, output_path, handle, cpu_time, tpu_time)){
                printf("------TEST CV_QUANTIFY FAILED------\n");
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
