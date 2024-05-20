#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    int loop;
    int if_use_img;
    int height;
    int width;
    int fmt;
    int ksize;
    char* src_name;
    char* dst_name;
    bm_handle_t handle;
} cv_laplace_thread_arg_t;

#define KERNEL_LENGTH 9
#define ERR_MAX 1.0e-4
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void readBin(const char* path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_src);
}

static void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

static void fill_img(unsigned char* input, int img_size)
{
    int i;

    for (i = 0; i < img_size; ++i) {
        input[i] = rand() % 256;
    }
}

static int compare_result(unsigned char* tpu_out, unsigned char* cpu_out, int length)
{
    int i;

    if (tpu_out == NULL || cpu_out == NULL) {
        printf("tpu_out or cpu_out is null!\n");
        return -1;
    }

    for (i = 0; i< length; ++i) {
        if (abs((double)(tpu_out[i] - cpu_out[i])) > ERR_MAX) {
            printf("Error:index:%d, tpu_out:%hhu, cpu_out:%hhu\n", i, tpu_out[i], cpu_out[i]);
            return -1;
        }
    }

    return 0;
}

static int cpu_lap(unsigned char* src, unsigned char* dst, int width, int height, int ksize)
{
    const int kernel_size = 3;
    int new_width = width + kernel_size - 1;
    int new_height = height + kernel_size - 1;
    unsigned char* new_input = (unsigned char*)malloc(new_width * new_height * \
                                sizeof(unsigned char));
    float kernel[KERNEL_LENGTH] = {2.f, 0.f, 2.f, 0.f, -8.f, 0.f, 2.f, 0.f, 2.f};
    int i, j, m, n;

    if (src == NULL || dst == NULL) {
        printf("the cpu param is err!\n");
        return -1;
    }

    if (ksize == 1) {
        float kernel_tmp[KERNEL_LENGTH] = {0.f, 1.f, 0.f, 1.f, -4.f, 1.f, 0.f, 1.f, 0.f};
        memcpy(kernel, kernel_tmp, sizeof(float) * KERNEL_LENGTH);
    }

    for (i = 0; i < new_height; ++i) { /* padding the img round*/
        if (i != 0 && i != new_height - 1) {
            for (j = 0; j < new_width; ++j) {
                if (j == 0) {
                    new_input[i * new_width + j] = src[(i - 1) * width + 1];
                } else if (j == new_width - 1) {
                    new_input[i * new_width + j] = src[(i - 1) * width + (width - 1 - 1)];
                } else {
                    new_input[i * new_width + j] = src[(i - 1) * width + (j - 1)];
                }
            }
        }

        if (i == new_height - 1) {
            for (j = 0; j < new_width; ++j) {
                new_input[i * new_width + j] = new_input[(i - 2) * new_width + j];
            }

            for (j = 0; j < new_width; ++j) {
                new_input[j] = new_input[2 * new_width + j];
            }
        }
    }

    for (i = 1; i < new_height - 1; ++i) {
        for (j = 1; j < new_width - 1; ++j) {
            float value = 0.f;
            int count = 0;
            for (m = -1; m <= 1; ++m) {
                for (n = -1; n <= 1; ++n) {
                    value += new_input[(i + m) * new_width + (j + n)] * kernel[count++];
                }
            }

            if (value < 0.f) {
                dst[(i - 1) * width + (j - 1)] = 0;
            } else if (value > 255.) {
                dst[(i - 1) * width + (j - 1)] = 255;
            } else {
                dst[(i - 1) * width + (j - 1)] = (unsigned char)(value);
            }
        }
    }

    free(new_input);
    return 0;
}

static int tpu_lap(unsigned char* src, unsigned char* dst, int width, int height,
                    bm_image_format_ext fmt, int ksize, bm_handle_t handle, long int *tpu_time)
{
    bm_status_t ret = BM_SUCCESS;

    bm_image_data_format_ext data_type = DATA_TYPE_EXT_1N_BYTE;
    bm_image input_img, output_img;
    struct timeval t1, t2;

    ret = bm_image_create(handle, height, width, fmt, data_type, &input_img, NULL);
    if (ret != BM_SUCCESS) {
        printf("Create bm input image failed. ret = %d\n", ret);
        bm_dev_free(handle);
        return -1;
    }

    ret = bm_image_alloc_dev_mem(input_img, 2);
    if (ret != BM_SUCCESS) {
        printf("Alloc bm dev input mem failed. ret = %d\n", ret);
        bm_image_destroy(&input_img);
        bm_dev_free(handle);
        return -1;
    }

    ret = bm_image_create(handle, height, width, fmt, data_type, &output_img, NULL);
    if (ret != BM_SUCCESS) {
        printf("Create bm output image failed. ret = %d\n", ret);
        bm_image_destroy(&input_img);
        bm_dev_free(handle);
        return -1;
    }

    ret = bm_image_alloc_dev_mem(output_img, 2);
    if (ret != BM_SUCCESS) {
        printf("Alloc bm dev output mem failed. ret = %d\n", ret);
        bm_image_destroy(&input_img);
        bm_image_destroy(&output_img);
        bm_dev_free(handle);
        return -1;
    }

    pthread_mutex_lock(&mutex);
    ret = bm_image_copy_host_to_device(input_img, (void **)(&src));
    if (ret != BM_SUCCESS) {
        printf("Copy bm image h2d src failed. ret = %d\n", ret);
        bm_image_destroy(&input_img);
        bm_image_destroy(&output_img);
        bm_dev_free(handle);
        return -1;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_image_laplacian(handle, input_img, output_img, ksize);
    if (ret != BM_SUCCESS) {
        printf("Laplace bmcv failed. ret = %d\n", ret);
        bm_image_destroy(&input_img);
        bm_image_destroy(&output_img);
        bm_dev_free(handle);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("Laplace TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);
    pthread_mutex_unlock(&mutex);

    ret = bm_image_copy_device_to_host(output_img, (void **)(&dst));
    if (ret != BM_SUCCESS) {
        printf("Copy bm image d2h src failed. ret = %d\n", ret);
        bm_image_destroy(&input_img);
        bm_image_destroy(&output_img);
        bm_dev_free(handle);
        return -1;
    }

    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);
    return 0;
}

static int test_lap(int if_use_image, int width, int height, bm_image_format_ext fmt, int ksize,
                    char* src_name, char* dst_name, bm_handle_t handle, long int *cpu_time, long int *tpu_time)
{
    unsigned char* input = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    unsigned char* tpu_out = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    unsigned char* cpu_out = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    int ret = 0;
    int len;
    struct timeval t1, t2;

    len = width * height;
    if (if_use_image) {
        printf("test real img \n");
        readBin(src_name, input, len);
    } else {
        fill_img(input, len);
    }
    memset(tpu_out, 0, len * sizeof(unsigned char));
    memset(cpu_out, 0, len * sizeof(unsigned char));

    gettimeofday(&t1, NULL);
    ret = cpu_lap(input, cpu_out, width, height, ksize);
    if (ret) {
        printf("cpu_lap failed, ret = %d\n", ret);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("Laplace CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *cpu_time += TIME_COST_US(t1, t2);

    ret = tpu_lap(input, tpu_out, width, height, fmt, ksize, handle, tpu_time);
    if (ret) {
        printf("tpu_lap failed, ret = %d\n", ret);
        return -1;
    }

    ret = compare_result(tpu_out, cpu_out, len);
    if (ret) {
        printf("compare result failed\n");
        return -1;
    }
    if(if_use_image){
        printf("output img by tpu\n");
        writeBin(dst_name, tpu_out, len);
    }

    free(input);
    free(tpu_out);
    free(cpu_out);

    return ret;
}

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_laplace.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_laplace.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int if_use_img = 0;
    int special_height[9] = {2, 100, 400, 800, 720, 1080, 1440, 2160, 4096};
    int special_width[9] = {2, 100, 400, 800, 1280, 1920, 2560, 3840, 4096};
    unsigned int ksize = 3;
    bm_image_format_ext fmt = FORMAT_GRAY; /* 14 */
    int ret = 0;
    char* src_name = NULL;
    char* dst_name = NULL;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
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
            int ret = test_lap(if_use_img, width, height, fmt, ksize, src_name, dst_name ,handle, cpu_time, tpu_time);
            if (ret) {
                printf("------Test Laplace Failed------\n");
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