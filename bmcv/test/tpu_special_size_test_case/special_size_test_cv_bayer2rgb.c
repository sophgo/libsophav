#include "bmcv_api_ext_c.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t lock;
#define NPU_NUM 32
#define KERNEL_SIZE 3 * 3 * 3 * 4 * 64
#define CONVD_MATRIX 12 * 9
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))


typedef struct {
    int loop_num;
    int height;
    int width;
    int dst_fmt;
    int src_type;
    bool use_real_img;
    char* input_path;
    char* output_path;
    bm_handle_t handle;
} bayer2rgb_thread_arg_t;

void sleep_(unsigned long _t){
    #ifdef __linux__
      usleep(_t);
    #else
      Sleep(_t);
    #endif
}

const unsigned char convd_kernel_bg8[CONVD_MATRIX] = {1, 0, 1, 0, 0, 0, 1, 0, 1, //Rb
                                                    0, 0, 2, 0, 0, 0, 0, 0, 2, //Rg1
                                                    0, 0, 0, 0, 0, 0, 2, 0, 2, //Rg2
                                                    0, 0, 0, 0, 0, 0, 0, 0, 4, //Rr
                                                    4, 0, 0, 0, 0, 0, 0, 0, 0, //Bb
                                                    2, 0, 2, 0, 0, 0, 0, 0, 0, //Bg1
                                                    2, 0, 0, 0, 0, 0, 2, 0, 0, //Bg2
                                                    1, 0, 1, 0, 0, 0, 1, 0, 1, //Br
                                                    0, 1, 0, 1, 0, 1, 0, 1, 0, //Gb
                                                    0, 0, 0, 0, 0, 4, 0, 0, 0, //Gg1
                                                    0, 0, 0, 0, 0, 0, 0, 4, 0, //Gg2
                                                    0, 1, 0, 1, 0, 1, 0, 1, 0};//Gr
const unsigned char convd_kernel_rg8[CONVD_MATRIX] = {4, 0, 0, 0, 0, 0, 0, 0, 0, //Rr
                                                    2, 0, 2, 0, 0, 0, 0, 0, 0, //Rg1
                                                    2, 0, 0, 0, 0, 0, 2, 0, 0, //Rg2
                                                    1, 0, 1, 0, 0, 0, 1, 0, 1, //Rb
                                                    1, 0, 1, 0, 0, 0, 1, 0, 1, //Br
                                                    0, 0, 2, 0, 0, 0, 0, 0, 2, //Bg1
                                                    0, 0, 0, 2, 0, 2, 0, 0, 0, //Bg2
                                                    0, 0, 0, 0, 0, 0, 0, 0, 4, //Bb
                                                    1, 0, 1, 0, 0, 0, 1, 0, 1, //Gr
                                                    0, 0, 0, 0, 0, 4, 0, 0, 0, //Gg1
                                                    0, 0, 0, 0, 0, 0, 0, 4, 0, //Gg2
                                                    0, 1, 0, 1, 0, 1, 0, 1, 0};//Gb
void readBin(const char *path, unsigned char* input_data, int size) {
    FILE *fp_src = fopen(path, "rb+");
    if (fp_src == NULL) {
        printf("无法打开输出文件 %s\n", path);
        return;
    }
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("file size is less than %d required bytes\n", size);
    };
    fclose(fp_src);
}

void ReflectionPad2d(unsigned char* srcImg, unsigned char* srcImg_padding, int h, int w) {
    int pad_w = w + 2;
    int pad_h = h + 2;
    // Copy srcImg (center)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            srcImg_padding[(y + 1) * pad_w + (x + 1)] = srcImg[y * w + x];
        }
    }
    // Top and bottom padding
    for (int x = 0; x < pad_w; x++) {
        srcImg_padding[x] = srcImg_padding[2 * pad_w + x]; // Top padding
        srcImg_padding[(pad_h - 1) * pad_w + x] = srcImg_padding[(pad_h - 3) * pad_w + x]; // Bottom padding
    }
    // Left and right padding
    for (int y = 0; y < pad_h; y++) {
        srcImg_padding[y * pad_w] = srcImg_padding[y * pad_w + 2]; // Left padding
        srcImg_padding[y * pad_w + (pad_w - 1)] = srcImg_padding[y * pad_w + (pad_w - 3)]; // Right padding
    }
}

// Function to generate the final matrix
void gen_matrix_final(float** vector_r, float** vector_g1, float** vector_g2, float** vector_b,
                      int rows, int cols, float** vector_temp3) {
    float** vector_temp1 = (float**)malloc(rows * sizeof(float*));
    float** vector_temp2 = (float**)malloc(rows * sizeof(float*));

    for (int i = 0; i < rows; ++i) {
        vector_temp1[i] = (float*)malloc(2 * cols * sizeof(float));
        vector_temp2[i] = (float*)malloc(2 * cols * sizeof(float));
    }
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < 2 * cols; ++j) {
            vector_temp1[i][j] = 0.0;
            vector_temp2[i][j] = 0.0;
        }
    }
    // Fill vector_temp1
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            vector_temp1[i][2 * j] = vector_r[i][j];
            vector_temp1[i][2 * j + 1] = vector_g1[i][j];
        }
    }

    // Fill vector_temp2
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            vector_temp2[i][2 * j] = vector_g2[i][j];
            vector_temp2[i][2 * j + 1] = vector_b[i][j];
        }
    }

    // Fill vector_temp3
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < 2 * cols; ++j) {
            vector_temp3[2 * i][j] =  vector_temp1[i][j];
            vector_temp3[2 * i + 1][j] = vector_temp2[i][j];
        }
    }
    // Free memory for temporary matrices
    for (int i = 0; i < rows; ++i) {
        free(vector_temp1[i]);
        free(vector_temp2[i]);
    }

    free(vector_temp1);
    free(vector_temp2);
}

void convolution_g(const unsigned char* input, unsigned char* dstImgCpu_g, float* kernel, int stride, int offset, int pad_h, int pad_w) {
    // Calculate output size
    int outputRows = (pad_h - 3) / stride + 1;
    int outputCols = (pad_w - 3) / stride + 1;

    // Define dimensions for input and output matrices
    int rows = pad_h - 2;
    int cols = pad_w - 2;

    // Allocate memory for input matrices and output matrix
    float** input_float = (float**)malloc(pad_h * sizeof(float*));
    float** output_float = (float**)malloc(rows * sizeof(float*));
    float** output_gr = (float**)malloc(outputRows * sizeof(float*));
    float** output_gg1 = (float**)malloc(outputRows * sizeof(float*));
    float** output_gg2 = (float**)malloc(outputRows * sizeof(float*));
    float** output_gb = (float**)malloc(outputRows * sizeof(float*));

    for (int i = 0; i < pad_h; ++i) {
        input_float[i] = (float*)malloc(pad_w * sizeof(float));
    }
    for (int i = 0; i < rows; ++i) {
        output_float[i] = (float*)malloc(cols * sizeof(float));
    }
    for (int i = 0; i < outputRows; ++i) {
        output_gr[i] = (float*)malloc(outputCols * sizeof(float));
        output_gg1[i] = (float*)malloc(outputCols * sizeof(float));
        output_gg2[i] = (float*)malloc(outputCols * sizeof(float));
        output_gb[i] = (float*)malloc(outputCols * sizeof(float));
    }
    for (int i = 0; i < outputRows; i++) {
        for (int j = 0; j < outputCols; j++) {
            output_gr[i][j] = 0.0;
            output_gg1[i][j] = 0.0;
            output_gg2[i][j] = 0.0;
            output_gb[i][j] = 0.0;
        }
    }
    // uchar2float
    for (int i = 0; i < pad_h; ++i) {
        for (int j = 0; j < pad_w; ++j) {
            input_float[i][j] = (float)input[i * pad_w + j];
        }
    }

    // r based on B
    for (int i = 0; i < outputRows; i++) {
        for (int j = 0; j < outputCols; j++) {
            for (int k = 0; k < 3; k++) {
                for (int l = 0; l < 3; l++) {
                    output_gr[i][j] += (input_float[i * stride + k + offset][j * stride + l + offset] * kernel[3 * k + l]);
                    output_gg1[i][j] += (input_float[i * stride + k + offset][j * stride + l + offset] * kernel[9 + 3 * k + l]);
                    output_gg2[i][j] += (input_float[i * stride + k + offset][j * stride + l + offset] * kernel[18 + 3 * k + l]);
                    output_gb[i][j] += (input_float[i * stride + k + 1][j * stride + l + 1] * kernel[27 + 3 * k + l]);
                }
            }
        }
    }

    // debayer
    gen_matrix_final(output_gr, output_gg1, output_gg2, output_gb, outputRows, outputCols, output_float);

    // float2uchar
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            dstImgCpu_g[i * cols + j] = (unsigned char)output_float[i][j];
        }
    }

    // Free memory for input and output matrices
    for (int i = 0; i < pad_h; ++i) {
        free(input_float[i]);
    }
    for (int i = 0; i < rows; ++i) {
        free(output_float[i]);
    }
    for (int i = 0; i < outputRows; ++i) {
        free(output_gr[i]);
        free(output_gg1[i]);
        free(output_gg2[i]);
        free(output_gb[i]);
    }
    free(input_float);
    free(output_float);
    free(output_gr);
    free(output_gg1);
    free(output_gg2);
    free(output_gb);
}

void convolution_rb(const unsigned char* input, unsigned char* dstImgCpu_r, float* kernel, int stride, int offset, int pad_h, int pad_w) {
    int outputRows = (pad_h - 3) / stride + 1;
    int outputCols = (pad_w - 3) / stride + 1;

    // Define dimensions for input and output matrices
    int rows = pad_h - 2;
    int cols = pad_w - 2;
    // Allocate memory for input matrices and output matrix
    float** input_float = (float**)malloc(pad_h * sizeof(float*));
    float** output_float = (float**)malloc(rows * sizeof(float*));
    float** output_rr = (float**)malloc(outputRows * sizeof(float*));
    float** output_rg1 = (float**)malloc(outputRows * sizeof(float*));
    float** output_rg2 = (float**)malloc(outputRows * sizeof(float*));
    float** output_rb = (float**)malloc(outputRows * sizeof(float*));
    for (int i = 0; i < pad_h; ++i) {
        input_float[i] = (float*)malloc(pad_w * sizeof(float));
    }
    for (int i = 0; i < rows; ++i) {
        output_float[i] = (float*)malloc(cols * sizeof(float));
    }
    for (int i = 0; i < outputRows; ++i) {
        output_rr[i] = (float*)malloc(outputCols * sizeof(float));
        output_rg1[i] = (float*)malloc(outputCols * sizeof(float));
        output_rg2[i] = (float*)malloc(outputCols * sizeof(float));
        output_rb[i] = (float*)malloc(outputCols * sizeof(float));
    }
    for (int i = 0; i < outputRows; i++) {
        for (int j = 0; j < outputCols; j++) {
            output_rr[i][j] = 0.0;
            output_rg1[i][j] = 0.0;
            output_rg2[i][j] = 0.0;
            output_rb[i][j] = 0.0;
        }
    }
    // uchar2float
    for (int i = 0; i < pad_h; ++i) {
        for (int j = 0; j < pad_w; ++j) {
            input_float[i][j] = (float)input[i * pad_w + j];
        }
    }
    // r based on B
    for (int i = 0; i < outputRows; i++) {
        for (int j = 0; j < outputCols; j++) {
            for (int k = 0; k < 3; k++) {
                for (int l = 0; l < 3; l++) {
                    output_rr[i][j] += (input_float[i * stride + k + offset][j * stride + l + offset] * kernel[3 * k + l]);
                    output_rg1[i][j] += (input_float[i * stride + k + offset][j * stride + l + offset] * kernel[9 + 3 * k + l]);
                    output_rg2[i][j] += (input_float[i * stride + k + offset][j * stride + l + offset] * kernel[18 + 3 * k + l]);
                    output_rb[i][j] += (input_float[i * stride + k + offset][j * stride + l + offset] * kernel[27 + 3 * k + l]);
                }
            }
        }
    }
    // debayer
    gen_matrix_final(output_rr, output_rg1, output_rg2, output_rb, outputRows, outputCols, output_float);
    // float2uchar
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            dstImgCpu_r[i * cols + j] = (unsigned char)output_float[i][j];
        }
    }
    // Free memory for input and output matrices
    for (int i = 0; i < pad_h; ++i) {
        free(input_float[i]);
    }
    for (int i = 0; i < rows; ++i) {
        free(output_float[i]);
    }
    for (int i = 0; i < outputRows; ++i) {
        free(output_rr[i]);
        free(output_rg1[i]);
        free(output_rg2[i]);
        free(output_rb[i]);
    }
    free(input_float);
    free(output_float);
    free(output_rr);
    free(output_rg1);
    free(output_rg2);
    free(output_rb);
}

void bayer2rgb_cpu(unsigned char* dstImgCpu_r, unsigned char* dstImgCpu_g, unsigned char* dstImgCpu_b,
                   unsigned char* srcImg, int height, int width, int src_type) {
    int pad_w = width + 2;
    int pad_h = height + 2;
    unsigned char* srcImg_padding = (unsigned char*)malloc(pad_h * pad_w);
    ReflectionPad2d(srcImg, srcImg_padding, height, width);
    if(src_type == 0) {
        float array_kernel_r[36] = {0.25, 0, 0.25, 0, 0, 0, 0.25, 0, 0.25,
                                    0, 0, 0.5, 0, 0, 0, 0, 0, 0.5,
                                    0, 0, 0, 0, 0, 0, 0.5, 0, 0.5,
                                    0, 0, 0, 0, 0, 0, 0, 0, 1};
        float array_kernel_g[36] = {0, 0.25, 0, 0.25, 0, 0.25, 0, 0.25, 0,
                                    0, 0, 0, 0, 0, 1, 0, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 1, 0,
                                    0, 0.25, 0, 0.25, 0, 0.25, 0, 0.25, 0};
        float array_kernel_b[36] = {1, 0, 0, 0, 0, 0, 0, 0, 0,
                                    0.5, 0, 0.5, 0, 0, 0, 0, 0, 0,
                                    0.5, 0, 0, 0, 0, 0, 0.5, 0, 0,
                                    0.25, 0, 0.25, 0, 0, 0, 0.25, 0, 0.25};
        convolution_rb(srcImg_padding, dstImgCpu_r, array_kernel_r, 2, 0, pad_h, pad_w);
        convolution_g(srcImg_padding, dstImgCpu_g, array_kernel_g, 2, 0, pad_h, pad_w);
        convolution_rb(srcImg_padding, dstImgCpu_b, array_kernel_b, 2, 1, pad_h, pad_w);
    } else {
        float array_kernel_r[36] = {1, 0, 0, 0, 0, 0, 0, 0, 0,
                                    0.5, 0, 0.5, 0, 0, 0, 0, 0, 0,
                                    0.5, 0, 0, 0, 0, 0, 0.5, 0, 0,
                                    0.25, 0, 0.25, 0, 0, 0, 0.25, 0, 0.25};
        float array_kernel_g[36] = {0.25, 0, 0.25, 0, 0, 0, 0.25, 0, 0.25,
                                    0, 0, 0, 0, 0, 1, 0, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 1, 0,
                                    0, 0.25, 0, 0.25, 0, 0.25, 0, 0.25, 0};
        float array_kernel_b[36] = {0.25, 0, 0.25, 0, 0, 0, 0.25, 0, 0.25,
                                    0, 0, 0.5, 0, 0, 0, 0, 0, 0.5,
                                    0, 0, 0, 0.5, 0, 0.5, 0, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 0, 1};
        convolution_rb(srcImg_padding, dstImgCpu_r, array_kernel_r, 2, 1, pad_h, pad_w);
        convolution_g(srcImg_padding, dstImgCpu_g, array_kernel_g, 2, 0, pad_h, pad_w);
        convolution_rb(srcImg_padding, dstImgCpu_b, array_kernel_b, 2, 0, pad_h, pad_w);
    }
    free(srcImg_padding);
}

static int bayer2rgb_tpu(bm_handle_t handle, unsigned char* input, unsigned char* output,
                         unsigned char* convd_kernel, int height, int width, int src_type, long int *tpu_time) {
    struct timeval t1, t2;
    bm_image input_img;
    bm_image output_img;
    bm_status_t ret;
    pthread_mutex_lock(&lock);
    if (src_type == 0) {
        ret = bm_image_create(handle, height, width, FORMAT_BAYER, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    } else {
        ret = bm_image_create(handle, height, width, FORMAT_BAYER_RG8, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
    }
    if(ret != BM_SUCCESS){
        return -1;
    }
    bm_image_create(handle, height, width, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
    bm_image_alloc_dev_mem(input_img, BMCV_HEAP1_ID);
    bm_image_alloc_dev_mem(output_img, BMCV_HEAP1_ID);
    bm_image_copy_host_to_device(input_img, (void **)(&input));
    gettimeofday(&t1,NULL);
    bmcv_image_bayer2rgb(handle, convd_kernel, input_img, output_img);
    gettimeofday(&t2,NULL);
    printf("Bayer2rgb TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);
    bm_image_copy_device_to_host(output_img, (void **)(&output));
    bm_image_destroy(&input_img);
    bm_image_destroy(&output_img);
    pthread_mutex_unlock(&lock);
    return 0;
}

int resultCompare2(unsigned char* output_data, unsigned char* dstImgCpu_r, unsigned char* dstImgCpu_g, unsigned char* dstImgCpu_b, int width, int height) {

    for (int i = 0;i < height;i++)
        for (int j = 0;j < width;j++) {
            if (abs(dstImgCpu_r[i * width + j] - output_data[i * width + j]) > 1) {
                printf(" dstImgCpu_r , i = %d, j = %d\n", i, j);
                printf(" dstImgCpu_r = %u, output_tpu = %u\n", dstImgCpu_r[i * width + j], output_data[i * width + j]);
                return -1;
            }

            if (abs(dstImgCpu_g[i * width + j] - output_data[height * width + i * width + j]) > 1) {
                printf(" dstImgCpu_g , i = %d, j = %d\n", i, j);
                printf(" dstImgCpu_g = %u, output_tpu = %u\n", dstImgCpu_g[i * width + j], output_data[height * width + i * width + j]);
             return -1;
            }

            if (abs(dstImgCpu_b[i * width + j] - output_data[height * width * 2 + i * width + j]) > 1) {
                printf(" dstImgCpu_b , i = %d, j = %d\n", i, j);
                printf(" dstImgCpu_b = %u, output_tpu = %u\n", dstImgCpu_b[i * width + j], output_data[height * width * 2 + i * width + j]);
                return -1;
            }
        }

    return 0;
}

int test_bayer2rgb_random(bm_handle_t handle, int height, int width, bool use_real_img,
                          const char *src_name, const char *output_path, int src_type, long int *cpu_time, long int *tpu_time) {
    int ret = 0;
    struct timeval t1, t2;

    unsigned char* output_tpu = (unsigned char*)malloc(width * height * 3);
    unsigned char* input_data = (unsigned char*)malloc(width * height);
    unsigned char* dstImgCpu_r = (unsigned char*)malloc(width * height * 3);
    unsigned char* dstImgCpu_g = (unsigned char*)malloc(width * height * 3);
    unsigned char* dstImgCpu_b = (unsigned char*)malloc(width * height * 3);
    unsigned char kernel_data[KERNEL_SIZE] = {0};

	// reconstructing kernel data
	for (int i = 0;i < 12;i++) {
        for (int j = 0;j < 9;j++) {
            if (src_type == 0) {
                kernel_data[i * 9 * 32 + 32 * j] = convd_kernel_bg8[i * 9 + j];
            } else {
                kernel_data[i * 9 * 32 + 32 * j] = convd_kernel_rg8[i * 9 + j];
            }
        }
    }
    // constructing input data
    if (use_real_img == true) {
        printf("test real image !\n");
        readBin(src_name, input_data, height * width);
    } else {
        for (int i = 0;i < height;i++) {
            for (int j = 0;j < width;j++) {
                input_data[i * width + j] = rand() % 255;
            }
        }
    }
    gettimeofday(&t1,NULL);
    bayer2rgb_cpu(dstImgCpu_r, dstImgCpu_g, dstImgCpu_b, input_data, height, width, src_type);
    gettimeofday(&t2,NULL);

    printf("Bayer2rgb CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *cpu_time += TIME_COST_US(t1, t2);
    ret = bayer2rgb_tpu(handle, input_data, output_tpu, kernel_data, height, width, src_type, tpu_time);

    if (ret != 0) {
        free(output_tpu);
        free(input_data);
        free(dstImgCpu_r);
        free(dstImgCpu_g);
        free(dstImgCpu_b);
        return ret;
    }
    ret = resultCompare2(output_tpu, dstImgCpu_r, dstImgCpu_g, dstImgCpu_b, width, height);
    if(ret != 0){
        printf(" compare failed! \n");
    }
    // ret == 0 ? printf(" compare successful! \n") :
    if (use_real_img == true) {
        FILE *fp_dst = fopen(output_path, "wb");
        if (fp_dst == NULL) {
            printf("无法打开输出文件 %s\n", output_path);
            ret = -1;
        } else {
            fwrite((void *)output_tpu, 1, height * width * 3, fp_dst);
        }
        fclose(fp_dst);
    }
    free(output_tpu);
    free(input_data);
    free(dstImgCpu_r);
    free(dstImgCpu_g);
    free(dstImgCpu_b);
    return ret;
}

int main() {
    printf("log will be saved in special_size_test_cv_bayer2rgb.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_bayer2rgb.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return 1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int use_real_img = 0;
    int special_height[6] = {2, 720, 1080, 1440, 2160, 4096};
    int special_width[6] = {2, 1280, 1920, 2560, 3840, 4096};
    int s_src_type[2] = {0, 1};
    char *input_path = NULL;
    char *output_path = NULL;

    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }
    for(int j = 0;j < 2;j++){
        int src_type = s_src_type[j];
        for(int i = 0;i < 6;i++){
            int width = special_width[i];
            int height = special_height[i];
            long int cputime = 0;
            long int tputime = 0;
            long int *cpu_time = &cputime;
            long int *tpu_time = &tputime;
            printf("width = %d, height = %d, src_type = %d\n", width, height, src_type);
            for(int loop = 0; loop < 3; loop++) {
                if(0 != test_bayer2rgb_random(handle, height, width, use_real_img, input_path, output_path, src_type, cpu_time, tpu_time)) {
                    printf("------TEST BAYER2RGB FAILED!------\n");
                    return -1;
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
