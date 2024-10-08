#include <stdio.h>
#include "bmcv_api_ext_c.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct {
    int loop_num;
    int use_real_img;
    int channel;
    int height;
    int width;
    int format;
    float sigmaX;
    float sigmaY;
    bool is_packed;
    char* input_path;
    char* output_path;
    bm_handle_t handle;
} gaussian_blur_thread_arg_t;

#if 0 // Use opencv for comparison to prove that the cpu side's own implementation of ref is correct.
#include "opencv2/opencv.hpp"
using namespace cv;
#endif

#if 0
static int gaussian_blur_opencv(unsigned char *input, unsigned char *output, int channel, bool is_packed,
                                int height, int width, int kw, int kh, float sigmaX, float sigmaY, int format) {
    int type = is_packed ? CV_8UC3 : CV_8UC1;
    channel = is_packed ? 1 : channel;
    if (format == 0) {
        unsigned char *input_addr[3] = {input, input + width * height, input + width * height * 5 / 4};
        unsigned char *output_addr[3] = {output, output + width * height, output + width * height * 5 / 4};
        for (int i = 0; i < channel; i++){
            Mat mat_i = Mat(height, width, type, input_addr[i]);
            Mat mat_o = Mat(height, width, type, output_addr[i]);
            GaussianBlur(mat_i, mat_o, Size(kw, kh), sigmaX, sigmaY);
        }
    } else if (format == 1) {
        unsigned char *input_addr[3] = {input, input + width * height, input + width * height * 3 / 2};
        unsigned char *output_addr[3] = {output, output + width * height, output + width * height * 3 / 2};
        for (int i = 0; i < channel; i++) {
            Mat mat_i = Mat(height, width, type, input_addr[i]);
            Mat mat_o = Mat(height, width, type, output_addr[i]);
            GaussianBlur(mat_i, mat_o, Size(kw, kh), sigmaX, sigmaY);
        }
    } else {
        for (int i = 0; i < channel; i++) {
            Mat mat_i = Mat(height, width, type, input + i * height * width);
            Mat mat_o = Mat(height, width, type, output + i * height * width);
            GaussianBlur(mat_i, mat_o, Size(kw, kh), sigmaX, sigmaY);
        }
    }
    return 0;
}
#endif

static int get_format_size(int format,int width, int height) {
    switch (format) {
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE:
            return width * height * 3;
        case FORMAT_GRAY:
            return width * height;
            break;
        default:
            printf("format error\n");
            return 0;
    }
}

static void fill(unsigned char *input, int channel, int width, int height, int is_packed) {
    for (int i = 0; i < (is_packed ? 3 : channel); i++) {
        int num = 10;
        for (int j = 0; j < height; j++) {
            for (int k = 0; k < width; k++) {
                input[i * width * height + j * width + k] = num % 128;
                num++;
            }
        }
    }
}

static int get_gaussian_sep_kernel(int n, float sigma, float *k_sep) {
    const int SMALL_GAUSSIAN_SIZE = 3;
    static const float small_gaussian_tab[3] = {0.25f, 0.5f, 0.25f};
    const float* fixed_kernel = n % 2 == 1 && n <= SMALL_GAUSSIAN_SIZE && sigma <= 0 ? small_gaussian_tab : 0;
    float sigmaX = sigma > 0 ? sigma : ((n - 1) * 0.5 - 1) * 0.3 + 0.8;
    float scale2X = -0.5 / (sigmaX * sigmaX);
    float sum = 0;
    int i;

    for (i = 0; i < n; i++) {
        float x = i - (n - 1) * 0.5;
        float t = fixed_kernel ? fixed_kernel[i] : exp(scale2X * x * x);
        k_sep[i] = t;
        sum += k_sep[i];
    }
    sum = 1./sum;
    for (i = 0; i < n; i++) {
        k_sep[i] = k_sep[i] * sum;
    }
    return 0;
}

static void create_gaussian_kernel(float* kernel, int kw, int kh, float sigma1, float sigma2) {
    float* k_sep_x = (float* )malloc(sizeof(float) * kw);
    float* k_sep_y = (float* )malloc(sizeof(float) * kh);

    if(sigma2 <= 0) sigma2 = sigma1;
    // automatic detection of kernel size from sigma
    if (kw <= 0 && sigma1 > 0 ) kw = (int)round(sigma1 * 3 * 2 + 1) | 1;
    if (kh <= 0 && sigma2 > 0 ) kh = (int)round(sigma2 * 3 * 2 + 1) | 1;
    sigma1 = sigma1 < 0 ? 0 : sigma1;
    sigma2 = sigma2 < 0 ? 0 : sigma2;
    get_gaussian_sep_kernel(kw, sigma1, k_sep_x);
    if (kh == kw && abs(sigma1 - sigma2) < DBL_EPSILON) {
        get_gaussian_sep_kernel(kw, sigma1, k_sep_y);
    } else {
        get_gaussian_sep_kernel(kh, sigma2, k_sep_y);
    }
    for (int i = 0; i < kh; i++) {
        for (int j = 0; j < kw; j++) {
            kernel[i * kw + j] = k_sep_y[i] * k_sep_x[j];
        }
    }
    free(k_sep_x);
    free(k_sep_y);
}

static void borderfill(int width, int height, int ksize_w, int ksize_h,unsigned char *input_data,
                       unsigned char* input_data_border) {
    int border_w = ksize_w / 2;
    int border_h = ksize_h / 2;
    int col = 0;
    int row = 0;
    int temp = 0;
    // first fill left and right
    for (int i = border_h; i < border_h + height; i++) {
        temp = border_w;
        for (int j = 0; j < border_w; j++) {
            input_data_border[i * (width + 2 * border_w) + j] = input_data[row * width + temp];
            temp--;
        }
        for (int j = border_w; j < border_w + width; j++) {
            input_data_border[i * (width + 2 * border_w) + j] = input_data[row * width + col];
            col++;
        }
        temp = width - 2;
        for (int j = border_w + width; j < 2 * border_w + width; j++) {
            input_data_border[i * (width + 2 * border_w) + j] = input_data[row * width + temp];
            temp--;
        }
        row++;
        col = 0;
    }
    // fill top and bottom
    temp = 2 * border_h;
    for (int i = 0; i < border_h; i++) {
        for (int j = 0; j < 2 * border_w + width; j++) {
            input_data_border[i * (2 * border_w + width) + j] =
            input_data_border[(i + temp) * (2 * border_w + width) + j];
        }
        temp -= 2;
    }
    temp = 2;
    for (int i = border_h + height; i < 2 * border_h + height; i++) {
        for (int j = 0; j < 2 * border_w + width; j++) {
            input_data_border[i * (2 * border_w + width) + j] =
            input_data_border[(i - temp) * (2 * border_w + width) + j];
        }
        temp += 2;
    }
}

static void conv(int width, int height, int kw, int kh, float *kernel,
                 unsigned char *input_data_border, unsigned char *output) {
    // The cpu side does the convolution on the input_img
    float sum_ = 0;
    for (int i = 0; i < (kh / 2 + height); i++) {
        for (int j = 0; j < (kw / 2 + width); j++) {
            for (int g = 0; g < kh; g++) {
                for (int k = 0; k < kw; k++) {
                    sum_ += input_data_border[(i + g) * (2 * (kw / 2) + width) + j + k] * kernel[g * kw + k];
                }
            }
            if (sum_ < 0) {sum_ = 0;}
            if (sum_ > 255) {sum_ = 255;}
            if ((i < height) && (j < width)) {
                output[i * width + j] = (unsigned char)(sum_ + 0.5); // opencv是四舍五入
            }
            sum_ = 0;
        }
    }
}

static int gaussian_blur_ref_single_channel(unsigned char *input, unsigned char *output, int height,
                                            int width, int kw, int kh, float *kernel) {
    // 1.get kernel   --get from last param
    // 2.border fill  --different from tpu,but the result is the same.
    int border_w = kw / 2;
    int border_h = kh / 2;
    unsigned char* input_data_border = (unsigned char*)malloc((width + 2 * border_w) * (height + 2 * border_h + 1));
    memset(input_data_border, 0, (width + 2 * border_w) * (height + 2 * border_h));
    borderfill(width, height, kw, kh, input, input_data_border);
#if 0 // check borderfill
            for(int i = 0; i < 10; i++){
                for(int j = 0; j < 10; j++){
                    printf("%3d ",input_data_border[i * 10 + j]);
                }
                printf("\n");
            }
#endif
    // 3.convolution
    conv(width, height, kw, kh, kernel, input_data_border, output);
    free(input_data_border);
    return 0;
}

static int gaussian_blur_cpu_ref(unsigned char *input, unsigned char *output, int channel, bool is_packed,
                                 int height, int width, int kw, int kh, float sigmaX, float sigmaY, int format) {
    // create kernel
    float *kernel = (float*)malloc(kw * kh * sizeof(float));
    memset(kernel, 0, kw * kh * sizeof(float));
    create_gaussian_kernel(kernel, kw, kh, sigmaX, sigmaY);
    if(is_packed) {
        unsigned char *input_temp = (unsigned char *)malloc(width * height * 3);
        unsigned char *output_temp = (unsigned char *)malloc(width * height * 3);
        // Adjusting the order of rgb alignment
        for (int i = 0; i < 3; i++) {
            int temp = 0;
            for (int j = 0; j < width * height; j++) {
                input_temp[i * width * height + j] = input[i + temp];
                temp += 3;
            }
        }
        for (int i = 0; i < 3; i++) {
            gaussian_blur_ref_single_channel(input_temp + i * width * height, output_temp + i * width * height,
                                             height, width, kw, kh, kernel);
        }
        for (int i = 0; i < 3; i++) {
            int tep = 0;
            for (int j = 0; j < width * height; j++) {
                output[i + tep] = output_temp[i * width * height + j];
                tep += 3;
            }
        }
        free(input_temp);
        free(output_temp);
    } else {
        for (int i = 0; i < channel; i++) {
            gaussian_blur_ref_single_channel(input + i * width * height, output + i * width * height,
                                             height, width, kw, kh, kernel);
        }
    }
    free(kernel);
    return 0;
}

static int gaussian_blur_tpu(unsigned char *input, unsigned char *output, int height, int width, int kw,
                             int kh, float sigmaX, float sigmaY, int format, bm_handle_t handle, long int *tpu_time) {
    bm_image img_i;
    bm_image img_o;
    struct timeval t1, t2;

    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &img_i, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &img_o, NULL);
    bm_image_alloc_dev_mem(img_i, 2);
    bm_image_alloc_dev_mem(img_o, 2);

    if(format == 14){
        unsigned char *input_addr[1] = {input};
        bm_image_copy_host_to_device(img_i, (void **)(input_addr));
    }else{
        unsigned char *input_addr[3] = {input, input + height * width, input + 2 * height * width};
        bm_image_copy_host_to_device(img_i, (void **)(input_addr));
    }

    gettimeofday(&t1, NULL);
    if(BM_SUCCESS != bmcv_image_gaussian_blur(handle, img_i, img_o, kw, kh, sigmaX, sigmaY)){
        bm_image_destroy(&img_i);
        bm_image_destroy(&img_o);
        bm_dev_free(handle);
        return -1;
    }
    gettimeofday(&t2, NULL);
    printf("Gaussian_blur TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);
    if (format == 14){
        unsigned char *output_addr[1] = {output};
        bm_image_copy_device_to_host(img_o, (void **)output_addr);
    } else {
        unsigned char *output_addr[3] = {output, output + height * width, output + 2 * height * width};
        bm_image_copy_device_to_host(img_o, (void **)output_addr);
    }
    bm_image_destroy(&img_i);
    bm_image_destroy(&img_o);
    return 0;
}

static int cmp(unsigned char *got, unsigned char *exp, int len) {
    for (int i = 0; i < len; i++) {
        if (abs(got[i] - exp[i]) > 2) {
            printf("cmp error: idx=%d  exp=%d  got=%d\n", i, exp[i], got[i]);
            return -1;
        }
    }
    return 0;
}

static void read_bin(const char *input_path, unsigned char *input_data, int width, int height, int format) {
    FILE *fp_src = fopen(input_path, "rb");
    if (fp_src == NULL) {
        printf("无法打开输出文件 %s\n", input_path);
        return;
    }
    if(fread(input_data, sizeof(char), get_format_size(format, width, height), fp_src) != 0) {
        printf("read image success\n");
    }
    fclose(fp_src);
}

static void write_bin(const char *output_path, unsigned char *output_data, int width, int height, int format) {
    FILE *fp_dst = fopen(output_path, "wb");
    if (fp_dst == NULL) {
        printf("无法打开输出文件 %s\n", output_path);
        return;
    }
    fwrite(output_data, sizeof(char), get_format_size(format, width, height), fp_dst);
    fclose(fp_dst);
}

static int test_gaussian_blur_random(int use_real_img, int channel, bool is_packed, int height, int width, int format,
                                     float sgmX, float sgmY, char *input_path, char *output_path, bm_handle_t handle,
                                     long int *cpu_time, long int *tpu_time) {
    struct timeval t1, t2;
    int kw = 3, kh = 3;
    float sigmaX = sgmX;
    float sigmaY = sgmY;
    unsigned char *input_data = (unsigned char*)malloc(width * height * (is_packed ? 3 : channel));
    unsigned char *output_tpu = (unsigned char*)malloc(width * height * (is_packed ? 3 : channel));
    unsigned char *output_cpu = (unsigned char*)malloc(width * height * (is_packed ? 3 : channel));
    // 2.fill input_img
    if (use_real_img) {
        // Input file test, can be written as image observation
        read_bin(input_path, input_data, width, height, format);
    } else {
        fill(input_data, (is_packed ? 3 : channel), width, height, is_packed);
    }
    // 3.gaussian_blur --cpu reference
    gettimeofday(&t1, NULL);
    gaussian_blur_cpu_ref(input_data, output_cpu, channel, is_packed, height,
                          width, kw, kh, sigmaX, sigmaY, format);
    gettimeofday(&t2, NULL);
    printf("Gaussian_blur CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *cpu_time += TIME_COST_US(t1, t2);
#if 0 // Use opencv for comparison to prove that the cpu side's own implementation of ref is correct.
    unsigned char *output_opencv = (unsigned char*)malloc(width * height * (is_packed ? 3 : channel));
    gaussian_blur_opencv(input_data, output_opencv, channel, is_packed,
                         height, width, kw, kh, sigmaX, sigmaY, format);
    // cmp opencv & cpu reference
    if (0 != cmp(output_cpu, output_opencv, get_format_size(format, width, height))) {
        printf("opencv and cpu failed to compare \n");
    } else {
        printf("Compare CPU result with OpenCV successfully!\n");
    }
    free(output_opencv);
#endif
    if(0 != gaussian_blur_tpu(input_data, output_tpu, height, width, kw, kh, sigmaX, sigmaY, format, handle, tpu_time)){
        free(input_data);
        free(output_tpu);
        free(output_cpu);
        return -1;
    }
    int ret = cmp(output_tpu, output_cpu, get_format_size(format, width, height));
    if (ret == 0) {
        if (use_real_img) {
            write_bin(output_path, output_tpu, width, height, format);
        }
    } else {
        printf("cpu and tpu failed to compare \n");
    }
    free(input_data);
    free(output_tpu);
    free(output_cpu);
    return ret;
}

int main(int argc, char *args[]) {
    printf("log will be saved in special_size_test_cv_gaussian_blur.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_gaussian_blur.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    int use_real_img = 0;
    int special_height[8] = {2, 720, 1080, 1440, 2160, 4096, 6000, 8192};
    int special_width[8] = {2, 1280, 1920, 2560, 3840, 4096, 4096, 4096};
    int format_num[] = {8,9,10,11,12,13,14};
    float sigmaX = 3;
    float sigmaY = 3;
    int channel;
    int is_packed;
    int ret = 0;
    char *input_path = NULL;
    char *output_path = NULL;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }
    for(int j = 0;j < 7;j++) {
        int format = format_num[j];
        channel = ((format == 10 || format == 11 || format == 14) ? 1 : 3);
        is_packed = ((format == 10 || format == 11) ? 1 : 0);
        for(int i = 0;i < 8;i++) {
            int width = special_width[i];
            int height = special_height[i];
            printf("width: %d , height: %d, format: %d\n", width, height, format);
            long int cputime = 0;
            long int tputime = 0;
            long int *cpu_time = &cputime;
            long int *tpu_time = &tputime;
            for(int loop = 0; loop < 3; loop++) {
                if (0 != test_gaussian_blur_random(use_real_img,
                                                    channel,
                                                    is_packed,
                                                    height,
                                                    width,
                                                    format,
                                                    sigmaX,
                                                    sigmaY,
                                                    input_path,
                                                    output_path,
                                                    handle,
                                                    cpu_time,
                                                    tpu_time)) {
                    printf("------TEST GAUSSIAN_BLUR FAILED------\n");
                    exit(-1);
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