#include <assert.h>
#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#define ERR_MAX 1
#define KERNEL_SIZE 25
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop;
    int if_use_img;
    int height;
    int width;
    char* src_name;
    char* dst_name;
    bm_handle_t handle;
} cv_pyramid_thread_arg_t;

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
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp_dst);
}

static void fill_img(unsigned char* input, int channel, int width, int height)
{
    int i, j;

    for (i = 0; i < height * channel; i++) {
        for (j = 0; j < width; j++) {
            input[i * width + j] = (unsigned char)(rand() % 256);
        }
    }
}

static int cmp_result(unsigned char* tpu, unsigned char* cpu, int len)
{
    int i;

    if (tpu == NULL || cpu == NULL) {
        printf("the cmp_result param is error!\n");
        return -1;
    }

    for (i = 0; i < len; ++i) {
        if (abs(tpu[i] - cpu[i]) >= ERR_MAX) {
            printf("cmp error: idx=%d, TPU=%d, CPU=%d\n", i, tpu[i], cpu[i]);
            return -1;
        }
    }

    return 0;
}

static int tpu_pyramid_down(unsigned char* input, unsigned char* output,
                            int height, int width, int oh, int ow, bm_handle_t handle, long int *tpu_time)
{
    bm_image_format_ext fmt = FORMAT_GRAY;
    bm_image img_i;
    bm_image img_o;
    bm_status_t ret;
    struct timeval t1, t2;

    ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &img_i, NULL);
    if (ret != BM_SUCCESS) {
        printf("Create bm input image failed. ret = %d\n", ret);
        bm_dev_free(handle);
        return -1;
    }

    ret = bm_image_create(handle, oh, ow, fmt, DATA_TYPE_EXT_1N_BYTE, &img_o, NULL);
    if (ret != BM_SUCCESS) {
        printf("Create bm output image failed. ret = %d\n", ret);
        bm_image_destroy(&img_i);
        bm_dev_free(handle);
        return -1;
    }

    ret = bm_image_alloc_dev_mem(img_i, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem input img failed. ret = %d\n", ret);
        bm_image_destroy(&img_i);
        bm_image_destroy(&img_o);
        bm_dev_free(handle);
        return -1;
    }

    ret = bm_image_alloc_dev_mem(img_o, BMCV_HEAP1_ID);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem output img failed. ret = %d\n", ret);
        bm_image_destroy(&img_i);
        bm_image_destroy(&img_o);
        bm_dev_free(handle);
        return -1;
    }

    ret = bm_image_copy_host_to_device(img_i, (void **)(&input));
    if (ret != BM_SUCCESS) {
        printf("bm_image_copy_host_to_device input img failed. ret = %d\n", ret);
        bm_image_destroy(&img_i);
        bm_image_destroy(&img_o);
        bm_dev_free(handle);
        return -1;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_image_pyramid_down(handle, img_i, img_o);
    gettimeofday(&t2, NULL);
    printf("Pyramid TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);
    if (ret != BM_SUCCESS) {
        printf("bmcv_image_pyramid_down failed. ret = %d\n", ret);
        bm_image_destroy(&img_i);
        bm_image_destroy(&img_o);
        bm_dev_free(handle);
        return -1;
    }

    ret = bm_image_copy_device_to_host(img_o, (void **)(&output));
    if (ret != BM_SUCCESS) {
        printf("bm_image_copy_device_to_host output img failed. ret = %d\n", ret);
        bm_image_destroy(&img_i);
        bm_image_destroy(&img_o);
        bm_dev_free(handle);
        return -1;
    }

    bm_image_destroy(&img_i);
    bm_image_destroy(&img_o);

    return 0;
}

void cpu_padding_img(unsigned char* padded_img, unsigned char* input_img, int padded_width,
                    int padded_height, int input_width, int input_height)
{
    int i, j;
    int originalX, originalY;

    for (i = 2; i < padded_height - 2; i++) {
        for (j = 2; j < padded_width - 2; j++) {
            originalX = j - 2;
            originalY = i - 2;
            padded_img[i * padded_width + j] = input_img[originalY * input_width + originalX];
        }
    }

    for (j = 2; j < padded_width - 2; j++) {
        for (i = 0; i < 2; i++) {
            padded_img[i * padded_width + j] = input_img[(2 - i) * input_width + (j - 2)];
        }

        for (i = padded_height - 2; i < padded_height; i++) {
            padded_img[i * padded_width + j] = input_img[(2 * input_height- i) * \
                                            input_width + (j - 2)];
        }
    }

    for (i = 2; i < padded_height - 2; i++) {
        for (j = 0; j < 2; j++) {
            padded_img[i * padded_width + j] = input_img[(i - 2) * input_width + (2 - j)];
        }

        for (j = padded_width - 2; j < padded_width; j++) {
            padded_img[i * padded_width + j] = input_img[(i - 2) * input_width + \
                                            (2 * input_width- j)];
        }
    }

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            padded_img[i * padded_width + j] = padded_img[i * padded_width + (2 * 2 - j)];
        }
        for (j = padded_width - 2; j < padded_width; j++) {
            padded_img[i * padded_width + j] = padded_img[i * padded_width + \
                                            (2 * (padded_width - 2 - 1) - j)];
        }
    }

    for (i = padded_height - 2; i < padded_height; i++) {
        for (j = 0; j < 2; j++) {
            padded_img[i * padded_width + j] = padded_img[i * padded_width + (2 * 2 - j)];
        }
        for (j = padded_width - 2; j < padded_width; j++) {
            padded_img[i * padded_width + j] = padded_img[i * padded_width + \
                                            (2 * (padded_width - 2 - 1) - j)];
        }
    }
}

static unsigned char customRound(float num)
{
    float decimalPart = num - floor(num);
    if (decimalPart < 0.5) {
        return (unsigned char)floor(num);
    } else {
        return (unsigned char)ceil(num);
    }
}

static int cpu_pyramid_down(unsigned char* input, unsigned char* output,
                            int height, int width, int oh, int ow)
{
    int i, j, y, x;
    int inputX, inputY;
    int kw = 5, kh = 5;
    int new_height = height + kh - 1;
    int new_width = width + kw - 1;
    float sum;
    float kernel[KERNEL_SIZE] = {1, 4, 6, 4, 1, 4, 16, 24, 16, 4, 6, 24, 36, 24, 6, \
                                4, 16, 24, 16, 4, 1, 4, 6, 4, 1};
    unsigned char* new_img = (unsigned char*)malloc(new_height * new_width * sizeof(unsigned char));


    if(input == NULL || output == NULL) {
        printf("the cpu_pyramid param is error!\n");
        free(new_img);
        return -1;
    }

    cpu_padding_img(new_img, input, new_width, new_height, width, height);

    for (y = 0; y < oh; y++) {
        for (x = 0; x < ow; x++) {
            sum = 0.0f;
            for (i = 0; i < kh; i++) {
                for (j = 0; j < kw; j++) {
                    inputX = 2 * x + j;
                    inputY = 2 * y + i;
                    sum += kernel[i * kw + j] * new_img[inputY * new_width + inputX];
                }
            }
            sum = sum / 256;
            output[y * ow + x] = customRound(sum);
        }
    }

    free(new_img);
    return 0;
}

static int test_pyramid_down(int if_use_img, int height, int width, char* src_name, char* dst_name, bm_handle_t handle,
                            long int *cpu_time, long int *tpu_time)
{
    int ow = width / 2;
    int oh = height / 2;
    int channel = 1;
    int ret = 0;
    unsigned char* input_data = (unsigned char*)malloc(width * height * channel * sizeof(unsigned char));
    unsigned char* output_tpu = (unsigned char*)malloc(ow * oh * channel * sizeof(unsigned char));
    unsigned char* output_cpu = (unsigned char*)malloc(ow * oh * channel * sizeof(unsigned char));
    struct timeval t1, t2;
    if (if_use_img) {
        printf("test real img \n");
        readBin(src_name, input_data, width * height * channel);
    } else {
        fill_img(input_data, channel, width, height);
    }
    memset(output_tpu, 0, ow * oh * channel * sizeof(unsigned char));
    memset(output_cpu, 0, ow * oh * channel * sizeof(unsigned char));

    ret = tpu_pyramid_down(input_data, output_tpu, height, width, oh, ow, handle, tpu_time);
    if (ret) {
        printf("the tpu_pyramid_down failed!\n");
        goto exit;
    }
    gettimeofday(&t1, NULL);
    ret = cpu_pyramid_down(input_data, output_cpu, height, width, oh, ow);
    gettimeofday(&t2, NULL);
    if (ret) {
        printf("the cpu_pyramid_down failed!\n");
        goto exit;
    }
    printf("Pyramid CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *cpu_time += TIME_COST_US(t1, t2);
    ret = cmp_result(output_tpu, output_cpu, ow * oh * channel);
    if (ret) {
        printf("the cpu and tpu compared failed!\n");
        goto exit;
    }
    if(if_use_img){
        printf("output img by tpu\n");
        writeBin(dst_name, output_tpu, ow * oh * channel);
    }

exit:
    free(input_data);
    free(output_tpu);
    free(output_cpu);

    return ret;
}

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_pyramid.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_pyramid.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }

    int special_height[6] = {3, 720, 1080, 1440, 2160, 4096};
    int special_width[6] = {3, 1280, 1920, 2043, 2043, 2043};
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int if_use_img = 0;
    int ret = 0;
    char* src_name = NULL;
    char* dst_name = NULL;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    for(int i = 0; i < 6; i++) {
        int width = special_width[i];
        int height = special_height[i];
        printf("width: %d , height: %d\n", width, height);
        long int cputime = 0;
        long int tputime = 0;
        long int *cpu_time = &cputime;
        long int *tpu_time = &tputime;
        for(int loop = 0; loop < 3; loop++) {
            int ret = test_pyramid_down(if_use_img, height, width, src_name, dst_name, handle, cpu_time, tpu_time);
            if (ret) {
                printf("------Test Pyramid_down Failed!------\n");
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

