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

static int parameters_check(int height, int width)
{
    if (height > 4096 || width > 2043){
        printf("Unsupported size : size_max = 2043 x 4096 \n");
        return -1;
    }
    return 0;
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
                            int height, int width, int oh, int ow, bm_handle_t handle)
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

static int test_pyramid_down(int if_use_img, int height, int width, char* src_name, char* dst_name, bm_handle_t handle)
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
        printf("test random data !\n");
        fill_img(input_data, channel, width, height);
    }
    printf("width = %d, height = %d\n", width, height);
    memset(output_tpu, 0, ow * oh * channel * sizeof(unsigned char));
    memset(output_cpu, 0, ow * oh * channel * sizeof(unsigned char));

    ret = tpu_pyramid_down(input_data, output_tpu, height, width, oh, ow, handle);
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

void* test_thread_pyramid(void* args) {
    cv_pyramid_thread_arg_t* cv_pyramid_thread_arg = (cv_pyramid_thread_arg_t*)args;
    int loop = cv_pyramid_thread_arg->loop;
    int if_use_img = cv_pyramid_thread_arg->if_use_img;
    int height = cv_pyramid_thread_arg->height;
    int width = cv_pyramid_thread_arg->width;
    char* src_name = cv_pyramid_thread_arg->src_name;
    char* dst_name = cv_pyramid_thread_arg->dst_name;
    bm_handle_t handle = cv_pyramid_thread_arg->handle;

    for (int i = 0; i < loop; ++i) {
        // if (loop > 1) {
        //     height = 3 + rand() % 4094;
        //     width = 3 + rand() % 2041;
        // }
        int ret = test_pyramid_down(if_use_img, height, width, src_name, dst_name, handle);
        if (ret) {
            printf("------Test Pyramid Failed!------\n");
            exit(-1);
        }
        printf("------Test Pyramid Successed!------\n");
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
    int if_use_img = 0;
    int loop = 1;
    int height = 3 + rand() % 4094;
    int width = 3 + rand() % 2041;
    int ret = 0;
    int check = 0;
    char* src_name = NULL;
    char* dst_name = NULL;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop use_real_img height width src_name dst_name(when use_real_img = 1,need to set src_name and dst_name) \n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 2\n", args[0]);
        printf("%s 1 1 0 512 512 \n", args[0]);
        printf("%s 1 1 1 1024 1024 input.bin pyramid_output.bin \n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) if_use_img = atoi(args[3]);
    if (argc > 4) height = atoi(args[4]);
    if (argc > 5) width = atoi(args[5]);
    if (argc > 6) src_name = args[6];
    if (argc > 7) dst_name = args[7];

    check = parameters_check(height, width);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }
    printf("seed = %d\n", seed);
    // test for multi-thread
    pthread_t pid[thread_num];
    cv_pyramid_thread_arg_t cv_pyramid_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_pyramid_thread_arg[i].loop = loop;
        cv_pyramid_thread_arg[i].if_use_img = if_use_img;
        cv_pyramid_thread_arg[i].height = height;
        cv_pyramid_thread_arg[i].width = width;
        cv_pyramid_thread_arg[i].src_name = src_name;
        cv_pyramid_thread_arg[i].dst_name = dst_name;
        cv_pyramid_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_thread_pyramid, &cv_pyramid_thread_arg[i]) != 0) {
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

