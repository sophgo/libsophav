#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include "test_misc.h"
#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"
#include <time.h>
#include <sys/time.h>
#include <float.h>
#include <assert.h>
#include "bmcv_a2_tpu_cmodel.h"

#ifdef __linux__
#include <sys/ioctl.h>
#endif
#define KERNEL_SIZE 25
pthread_mutex_t lock;
const int histSizes[CHN_NUM_MAX] = {32, 32, 32};
const float ranges[CHN_NUM_MAX * 2] = {0, 256, 0, 256, 0, 256};

/*
  test_cv_absdiff
*/
int absdiff_cmodel(unsigned char* input1,
                          unsigned char* input2,
                          unsigned char* output,
                          int img_size){
    for(int i = 0; i < img_size; i++)
        output[i] = abs(input1[i] - input2[i]);
    return 0;
}

/*
  test_cv_add_weight
*/
int cpu_add_weighted(unsigned char* input1, unsigned char* input2, unsigned char* output,
                            int img_size,  float alpha, float beta, float gamma)
{
    double roundNum = 0;
    int i;

    for (i = 0; i < img_size; i++) {
        roundNum = round(input1[i] * alpha + input2[i] * beta + gamma);
        if (roundNum > UINT8_MAX) {
            roundNum = UINT8_MAX; /*If exceed the max value, result is UINT8_MAX*/
        } else if (roundNum < 0) {
            roundNum = 0; /*If less than the min value, result is 0*/
        }
        output[i] = (uint8_t)roundNum;
    }
    return 0;
}

/*
  test_cv_as_strided
*/
int cpu_as_strided(float* input, float* output, struct row_col_para row_col)
{
    int input_row = row_col.row.input_row;
    int output_row = row_col.row.output_row;
    int row_stride = row_col.row.row_stride;
    int input_col = row_col.col.input_col;
    int output_col = row_col.col.output_col;
    int col_stride = row_col.col.col_stride;
    int i, j;
    int input_size = input_row * input_col;
    int start = 0;
    int cur = start;

    if (input == NULL || output == NULL) {
        printf("the cpu input or output is null!\n");
        return -1;
    }

    for(i = 0; i < output_row; i++) {
        cur = start;
        for(j = 0; j < output_col; j++) {
            output[i * output_col + j] = input[cur];
            cur += col_stride;
            cur %= input_size;
        }
        start += row_stride;
        start %= input_size;
    }

    return 0;
}

/*
  test_cv_batch_topk
*/
void merge_ascend(Pair ref_res[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    Pair L[n1], R[n2];

    for (int i = 0; i < n1; i++) {
        L[i] = ref_res[left + i];
    }
    for (int j = 0; j < n2; j++) {
        R[j] = ref_res[mid + 1 + j];
    }

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].second <= R[j].second) {
            ref_res[k] = L[i];
            i++;
        } else {
        ref_res[k] = R[j];
        j++;
        }
        k++;
    }
    while (i < n1) {
        ref_res[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        ref_res[k] = R[j];
        j++;
        k++;
    }
}

void merge_descend(Pair ref_res[], int left, int mid, int right) {
  int n1 = mid - left + 1;
  int n2 = right - mid;
  Pair L[n1], R[n2];

  for (int i = 0; i < n1; i++) {
    L[i] = ref_res[left + i];
  }
  for (int j = 0; j < n2; j++) {
    R[j] = ref_res[mid + 1 + j];
  }

  int i = 0, j = 0, k = left;
  while (i < n1 && j < n2) {
    if (L[i].second >= R[j].second) {
      ref_res[k] = L[i];
      i++;
    } else {
      ref_res[k] = R[j];
      j++;
    }
    k++;
  }
  while (i < n1) {
    ref_res[k] = L[i];
    i++;
    k++;
  }
  while (j < n2) {
    ref_res[k] = R[j];
    j++;
    k++;
  }
}

void merge_sort(Pair ref_res[], int left, int right, bool is_ascend) {
  if (left < right) {
    int mid = left + (right - left) / 2;
    merge_sort(ref_res, left, mid, is_ascend);
    merge_sort(ref_res, mid + 1, right, is_ascend);
    if (is_ascend) {
      merge_ascend(ref_res, left, mid, right);
    }else{
      merge_descend(ref_res, left, mid, right);
    }
  }
}

void topk_reference(int k, int batch, int batch_num, int batch_stride, int descending, bool bottom_index_valid,
                    float* bottom_data, int* bottom_index, float* top_data_ref, int* top_index_ref) {
  for (int b = 0; b < batch; b++) {
    Pair* bottom_vec = (Pair*)malloc(batch_num * sizeof(Pair));
    for (int i = 0; i < batch_num; i++) {
      int offset = b * batch_stride + i;
      float data = bottom_data[offset];
      if (bottom_index_valid) {
        bottom_vec[i].first = bottom_index[offset];
        bottom_vec[i].second = data;
      } else {
        bottom_vec[i].first = i;
        bottom_vec[i].second = data;
      }
    }

    if (descending) {
      merge_sort(bottom_vec, 0, batch_num - 1, 0);
    } else {
      merge_sort(bottom_vec, 0, batch_num - 1, 1);
    }

    for (int i = 0; i < k; i++) {
      top_index_ref[b * k + i] = bottom_vec[i].first;
      top_data_ref[b * k + i] = bottom_vec[i].second;
    }
    free(bottom_vec);
  }
}

/*
  test_cv_bayer2rgb
*/
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

/*
  test_cv_bitwise
*/
int cpu_bitwise(unsigned char* input1, unsigned char* input2, unsigned char* output,
                        int len, int op)
{
    int ret = 0;

    switch (op) {
        case AND:
            for (int i = 0; i < len; ++i) {
                output[i] = input1[i] & input2[i];
            }
            break;
        case OR:
            for (int i = 0; i < len; ++i) {
                output[i] = input1[i] | input2[i];
            }
            break;
        case XOR:
            for (int i = 0; i < len; ++i) {
                output[i] = input1[i] ^ input2[i];
            }
            break;
    }

    return ret;
}

/*
  test_cv_calc_hist
*/
int cpu_calc_Hist_1d(void* input_host, struct frame_size* frame, struct hist_para* para, \
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

int cpu_calc_Hist_2d(void* input_host, struct frame_size* frame, struct hist_para* para,
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

int cpu_calc_Hist_3d(void* input_host, struct frame_size* frame, struct hist_para* para,
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

int cpu_calc_hist(void* input_host, float* output_cpu, struct frame_size* frame,
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

/*
  test_cv_cmulp
*/
int cpu_cmul(float* XRHost, float* XIHost, float* PRHost, float* PIHost,
                    float* cpu_YR, float* cpu_YI, int L, int batch)
{
    int ret = 0;
    int i, j;

    if (XRHost == NULL || XIHost == NULL|| PRHost == NULL ||
        PIHost == NULL || cpu_YR == NULL || cpu_YI == NULL) {
        printf("the param is null!\n");
        return -1;
    }

    for (i = 0; i < batch; ++i) {
        for (j = 0; j < L; ++j) {
            cpu_YR[i * L + j] = XRHost[i * L + j] * PRHost[j] - \
                                XIHost[i * L + j] * PIHost[j]; /* x = a * c - b * d; */
            cpu_YI[i * L + j] = XRHost[i * L + j] * PIHost[j] + \
                                XIHost[i * L + j] * PRHost[j]; /* y = a * d + b * c; */
        }
    }

    return ret;
}

/*
  test_cv_distance
*/
int cpu_distance_fp32(float* XHost, float* cpu_out, int L, int dim, float pnt32[])
{
    int i, j;

    if (XHost == NULL || cpu_out == NULL || pnt32 == NULL) {
        printf("cpu_distance the param is null!\n");
        return -1;
    }

    for (i = 0; i < L; ++i) {
        cpu_out[i] = 0.f;
        for (j = 0; j < dim; ++j) {
            cpu_out[i] += (XHost[i * dim + j] - pnt32[j]) * (XHost[i * dim + j] - pnt32[j]);
        }
        cpu_out[i] = sqrt(cpu_out[i]);
    }

    return 0;
}

int cpu_distance_fp16(fp16* XHost16, fp16* cpu_out_fp16, int L,
                            int dim, fp16 pnt16[])
{
    int i, j;
    float pnt32[DIM_MAX] = {0};
    float* XHost = (float*)malloc(L * dim * sizeof(float));
    float* YHost = (float*)malloc(L * sizeof(float));

    if (XHost16 == NULL || cpu_out_fp16 == NULL || pnt16 == NULL) {
        printf("cpu_distance the param is null!\n");
        return -1;
    }

    for (i = 0; i < L; ++i) {
        YHost[i] = 0.f;
        for (j = 0; j < dim; ++j) {
            XHost[i * dim + j] = fp16tofp32(XHost16[i * dim + j]);
            pnt32[j] = fp16tofp32(pnt16[j]);
            YHost[i] += (XHost[i * dim + j] - pnt32[j]) * (XHost[i * dim + j] - pnt32[j]);
        }
        YHost[i] = sqrt(YHost[i]);
        cpu_out_fp16[i] = fp32tofp16(YHost[i], 1);
    }

    free(XHost);
    free(YHost);

    return 0;
}

/*
  test_cv_draw_lines
*/
int get_image_offset(int format, int width, int height, int* offset_list)
{
    int ret = 0;

    switch (format) {
        case FORMAT_YUV420P:
            offset_list[0] = width * height;
            offset_list[1] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;
            offset_list[2] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;
            break;
        case FORMAT_YUV422P:
            offset_list[0] = width * height;
            offset_list[1] = ALIGN(width, 2) * height >> 1;
            offset_list[2] = ALIGN(width, 2) * height >> 1;
            break;
        case FORMAT_YUV444P:
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE:
            offset_list[0] = width * height;
            offset_list[1] = width * height;
            offset_list[2] = width * height;
            break;
        case FORMAT_NV12:
        case FORMAT_NV21:
            offset_list[0] = width * height;
            offset_list[1] = ALIGN(width, 2) * ALIGN(height, 2) >> 1;
            break;
        case FORMAT_NV16:
        case FORMAT_NV61:
        case FORMAT_NV24:
            offset_list[0] = width * height;
            offset_list[1] = ALIGN(width, 2) * height;
            break;
        case FORMAT_GRAY:
            offset_list[0] = width * height;
            break;
        default:
            printf("image format error!\n");
            ret = -1;
            break;
    }

    return ret;
}

void swap(int* a, int* b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}
int get_image_default_step(int format, int width, int* step)
{
    int ret = 0;

    switch (format) {
        case FORMAT_GRAY:
            step[0] = width;
            break;
        case FORMAT_YUV420P:
        case FORMAT_YUV422P:
            step[0] = width;
            step[1] = ALIGN(width, 2) >> 1;
            step[2] = ALIGN(width, 2) >> 1;
            break;
        case FORMAT_YUV444P:
            step[0] = width;
            step[1] = width;
            step[2] = width;
            break;
        case FORMAT_NV12:
        case FORMAT_NV21:
        case FORMAT_NV16:
        case FORMAT_NV61:
            step[0] = width;
            step[1] = ALIGN(width, 2);
            break;
        default:
            printf("not support format!\n");
            ret = -1;
            break;
    }

    return ret;
}

int draw_line_cpu(unsigned char* input, int height, int width, int line_num, int format,
                        const bmcv_point_t* start, const bmcv_point_t* end,
                        unsigned char color[3], int thickness)
{

    bmcv_point_t* sp = (bmcv_point_t*)malloc(line_num * sizeof(bmcv_point_t));
    bmcv_point_t* ep = (bmcv_point_t*)malloc(line_num * sizeof(bmcv_point_t));
    bmcv_color_t rgb = {color[0], color[1], color[2]};
    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
    int step[IMAGE_CHN_NUM_MAX];
    int ret = 0;
    int i;
    bmMat mat;
    struct timeval t1, t2;

    for (i = 0; i < line_num; ++i) {
        sp[i].x = SATURATE(start[i].x, 0, width - 1);
        sp[i].y = SATURATE(start[i].y, 0, height - 1);
        ep[i].x = SATURATE(end[i].x, 0, width - 1);
        ep[i].y = SATURATE(end[i].y, 0, height - 1);
        // because only support start_y < end_y
        if (sp[i].y > ep[i].y) {
            swap(&(sp[i].x), &(ep[i].x));
            swap(&(sp[i].y), &(ep[i].y));
        }
    }

    ret = get_image_offset(format, width, height, offset_list);
    if (ret != 0) {
        printf("get_image_offset failed!\n");
        goto exit;
    }

    in_ptr[0] = input;
    in_ptr[1] = input + offset_list[0];
    in_ptr[2] = input + offset_list[0] + offset_list[1];

    ret = get_image_default_step(format, width, step);
    if (ret != 0) {
        printf("get_image_default_step failed!\n");
        goto exit;
    }

    mat.width = width;
    mat.height = height;
    mat.format = (bm_image_format_ext)format;
    mat.step = step;
    mat.data = (void**)in_ptr;

    gettimeofday(&t1, NULL);
    for (i = 0; i < line_num; ++i) {
        draw_line(&mat, sp[i], ep[i], rgb, thickness);
    }
    gettimeofday(&t2, NULL);
    printf("Draw_lines CPU using time: %ld(us)\n", TIME_COST_US(t1, t2));

exit:
    free(sp);
    free(ep);
    return ret;
}

/*
  test_cv_feature_match
*/
float calc_sqrt(const float* feature, int feature_size)
{
    float a = 0.f;
    int i;

    for (i = 0; i < feature_size; ++i) {
        a += feature[i] * feature[i];
    }

    return 1.f / sqrt(a);
}

int calc_sqrt_transposed(float** feature, int rows, int cols, float* db_feature)
{
    int i, j;
    float tmp;
    float result;

    for (i = 0; i < cols; ++i) {
        tmp = 0.f;
        for (j = 0; j < rows; ++j) {
            tmp += feature[j][i] * feature[j][i];
        }
        result = 1.f / sqrt(tmp);
        db_feature[i] = result;
    }

    return 0;
}

int transpose_feature_match(float** origin, int rows, int cols, float** trans)
{
    if (rows == 0 || cols == 0) {
        return BM_ERR_FAILURE;
    }

    int i, j;

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            trans[j][i] = origin[i][j];
        }
    }

    return 0;
}

int compare_float(const void *a, const void *b)
{
    const float epsilon = CMP_ERR_THRESHOLD;
    float diff = *(float *)b - *(float *)a;
    return (diff > epsilon) ? 1 : ((diff < -epsilon) ? -1 : 0);
}

void sort_cpu_float(float** match_res, int batch_size, int db_size)
{
    int i;

    for (i = 0; i < batch_size; ++i) {
        qsort(match_res[i], db_size, sizeof(float), compare_float);
    }
}

int cpu_feature_match(float** input_data, int batch_size, int feature_size, float** db_data,
                            int db_size, float** match_res)
{
    int i, j, k;
    float a, b;
    int ret = 0;
    float** trans_vec = (float**)malloc(db_size * sizeof(float*));
    for (i = 0; i < db_size; i++) {
        trans_vec[i] = (float*)malloc(feature_size * sizeof(float));
    }

    ret = transpose_feature_match(db_data, feature_size, db_size, trans_vec); /*trans_vec row = db_size col = feature_size*/

    for (i = 0; i < batch_size; ++i) {
        for (j = 0; j < db_size; ++j) {
            a = 0.f;
            for (k = 0; k < feature_size; ++k) {
                a += input_data[i][k] * trans_vec[j][k];
            }
            b = calc_sqrt(input_data[i], feature_size) * calc_sqrt(trans_vec[j], feature_size);
            match_res[i][j] = a * b;
        }
    }

    sort_cpu_float(match_res, batch_size, db_size);

    for (i = 0; i < db_size; ++i) {
        free(trans_vec[i]);
    }
    free(trans_vec);

    return ret;
}

int compare_short(const void *a, const void *b) {
    return (*(short*)b - *(short*)a);
}

void sort_cpu_fix8b(short** match_res, int batch_size, int db_size) {
    int i;

    for (i = 0; i < batch_size; ++i) {
        qsort(match_res[i], db_size, sizeof(short), compare_short);
    }
}

int cpu_feature_match_fix8b(int8_t** input_data, int batch_size, int feature_size,
                                    int8_t** db_data, int db_size, int rshiftbits,
                                    short** match_res)
{
    int tmp = 0;
    int i, j, k;

    for (i = 0; i < batch_size; ++i) {
        for (j = 0; j < db_size; ++j) {
            tmp = 0;
            for (k = 0; k < feature_size; ++k) {
                tmp += db_data[k][j] * input_data[i][k];
            }
            tmp = tmp > 0x7fff ? 0x7fff : tmp;
            tmp = tmp < -0x8000 ? -0x8000 : tmp;
            if (rshiftbits != 0) {
                tmp = ((tmp >> (rshiftbits - 1)) + 1) >> 1;
            }
            match_res[i][j] = (short)tmp;
        }
    }
    sort_cpu_fix8b(match_res, batch_size, db_size);

    return 0;
}

/*
  test_cv_gaussian_blur
*/
int get_gaussian_sep_kernel(int n, float sigma, float *k_sep) {
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

void create_gaussian_kernel(float* kernel, int kw, int kh, float sigma1, float sigma2) {
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

void borderfill(int width, int height, int ksize_w, int ksize_h,unsigned char *input_data,
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

void conv(int width, int height, int kw, int kh, float *kernel,
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

int gaussian_blur_ref_single_channel(unsigned char *input, unsigned char *output, int height,
                                            int width, int kw, float *kernel) {
    int half_k = kw / 2;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0;
            for (int i = -half_k; i <= half_k; i++) {
                for (int j = -half_k; j <= half_k; j++) {  // Boundary reflection fill
                    int yy = y + i;
                    int xx = x + j;
                    if (yy < 0) yy = -yy;
                    if (yy >= height) yy = 2 * height - yy - 2;
                    if (xx < 0) xx = -xx;
                    if (xx >= width) xx = 2 * width - xx - 2;
                    int pixel_index = yy * width + xx;
                    int pixel = input[pixel_index];
                    sum += pixel * kernel[(i + half_k) * kw + j + half_k];
                }
            }
            if (sum > 255) sum = 255;
            if (sum < 0) sum = 0;
            int output_index = y * width + x;
            output[output_index] = (unsigned char)sum;
        }
    }

    return 0;
}

int gaussian_blur_cpu_ref(unsigned char *input, unsigned char *output, int channel, bool is_packed,
                                 int height, int width, int kw, int kh, float sigmaX, float sigmaY) {
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
                                             height, width, kw, kernel);
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
                                             height, width, kw, kernel);
        }
    }
    free(kernel);
    return 0;
}

/*
  test_cv_gemm
*/
void gemm_trans(float* src, int row_num, int col_num)
{
    int i, j;

    float* dst = (float*)malloc(row_num * col_num * sizeof(float));

    for (i = 0; i < row_num; ++i) {
        for (j = 0; j < col_num; j++) {
            dst[j * row_num + i] = src[i * col_num + j];
        }
    }
    memcpy(src, dst, col_num * row_num * sizeof(float));
    free(dst);
}

int cpu_gemm(bool if_A_trans, bool if_B_trans, int M, int N, int K, float alpha, float* src_A,
                    int lda, float* src_B, int ldb, float beta, float* src_C)
{
    float* A = (float*)malloc(M * K * sizeof(float));
    float* B = (float*)malloc(N * K * sizeof(float));
    float* C = (float*)malloc(M * N * sizeof(float));
    int i, j, k;

    memcpy(A, src_A, sizeof(float) * M * K);
    memcpy(B, src_B, sizeof(float) * N * K);
    memset(C, 0.f, sizeof(float) * M * N);

    if (if_A_trans) {
        gemm_trans(A, K, lda);
    }
    if (if_B_trans) {
        gemm_trans(B, N, ldb);
    }

    for (i = 0; i < M; ++i) {
        for (j = 0; j < N; ++j) {
            for (k = 0; k < K; ++k) {
                C[i * N + j] += alpha * A[i * K + k] * B[k * N + j];
            }
        }
    }

    for (i = 0; i < M * N; ++i) {
        C[i] = C[i] + src_C[i] * beta;
    }

    memcpy(src_C, C, sizeof(float) * M * N);

    free(A);
    free(B);
    free(C);

    return 0;
}

/*
  test_cv_hist_balance
*/
float get_cdf_min(float* cdf, int len)
{
    int i;

    for(i = 0; i < len; ++i) {
        if (cdf[i] != 0) {
            return cdf[i];
        }
    }
    return 0.f;
}

int cpu_hist_balance(uint8_t* input_host, uint8_t* output_cpu, int height, int width)
{
    int H = height;
    int W = width;
    int binX;
    int i;
    float gray_tmp;
    uint8_t gray_index_tmp;
    float cdf_min;
    float cdf_max;
    float* cpu_cdf;

    if (input_host == NULL || output_cpu == NULL) {
        printf("cpu_calc_hist param error!\n");
        return -1;
    }

    cpu_cdf = (float*)malloc(GRAY_SERIES * sizeof(float));
    memset(cpu_cdf, 0.f, GRAY_SERIES * sizeof(float));

    for (i = 0; i < W * H; ++i) {
        binX = input_host[i];
        if (binX >= 0 && binX < GRAY_SERIES) {
            cpu_cdf[binX]++;
        }
    }

    for (i = 1; i < GRAY_SERIES ; ++i) {
        cpu_cdf[i] += cpu_cdf[i - 1];
    }

    cdf_min = get_cdf_min(cpu_cdf, GRAY_SERIES);
    cdf_max = W * H;

    for(i = 0; i < H * W; ++i) {
        gray_index_tmp = input_host[i];
        gray_tmp = round((cpu_cdf[gray_index_tmp] - cdf_min) * (GRAY_SERIES - 1) / (cdf_max - cdf_min));
        output_cpu[i] = (uint8_t)gray_tmp;
    }

    free(cpu_cdf);
    return 0;
}

/*
  test_cv_laplace
*/
int cpu_lap(unsigned char* src, unsigned char* dst, int width, int height, int ksize)
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

/*
  test_cv_matmul
*/
void local_transpose(void* src, int row_num, int col_num, int sign)
{
    int i, j;

    if (sign == 1) {
        signed char* dst = (signed char*)malloc(row_num * col_num * sizeof(signed char));
        for (i = 0; i < row_num; i++) {
            for (j = 0; j < col_num ; j++) {
                dst[j * row_num + i] = *((signed char*)src + i * col_num + j);;
            }
        }
        memcpy(src, dst, col_num * row_num * sizeof(signed char));
    } else {
        unsigned char* dst = (unsigned char*)malloc(row_num * col_num * sizeof(unsigned char));
        for (i = 0; i < row_num; i++) {
            for (j = 0; j < col_num ; j++) {
                dst[j * row_num + i] = *((unsigned char*)src + i * col_num + j);;
            }
        }
        memcpy(src, dst, col_num * row_num * sizeof(unsigned char));
    }
}

int cpu_matrix_mul(bool trans_A, bool trans_B, int M, int N, int K, void* src_A, int lda,
                            void* src_B, int ldb, int* src_C, int signA, int signB)
{
    int* pC;
    int i, j, k;
    signed char* pA_signed;
    unsigned char* pA_unsigned;
    signed char* pB_signed;
    unsigned char* pB_unsigned;

    if (src_A == NULL || src_B == NULL || src_C == NULL) {
        printf("the cpu_matrix_mul param error!\n");
        return -1;
    }

    if (signA == 1) {
        pA_signed = (signed char*)malloc(M * K * sizeof(signed char));
        memcpy(pA_signed, (signed char*)src_A, M * K * sizeof(signed char));
    } else {
        pA_unsigned = (unsigned char*)malloc(M * K * sizeof(unsigned char));
        memcpy(pA_unsigned, (unsigned char*)src_A, M * K * sizeof(unsigned char));
    }

    if (signB == 1) {
        pB_signed = (signed char*)malloc(K * N * sizeof(signed char));
        memcpy(pB_signed, (signed char*)src_B, K * N * sizeof(signed char));
    } else {
        pB_unsigned = (unsigned char*)malloc(K * N * sizeof(unsigned char));
        memcpy(pB_unsigned, (unsigned char*)src_B, K * N * sizeof(unsigned char));
    }

    pC = (int*)malloc(M * N * sizeof(int));
    memset(pC, 0, M * N * sizeof(int));

    if (trans_A) {
        if (signA == 1) {
            local_transpose((void*)pA_signed, K, lda, signA);
        } else {
            local_transpose((void*)pA_unsigned, K, lda, signA);
        }
    }

    if (trans_B) {
        if (signB == 1) {
            local_transpose((void*)pB_signed, N, ldb, signB);
        } else {
            local_transpose((void*)pB_unsigned, N, ldb, signB);
        }
    }

    for (i = 0; i < M; i++) { /* matrix_multiply */
        for (j = 0; j < N; j++) {
            for (k = 0; k < K; k++) {
                if (signA == 1) {
                    if (signB == 1) {
                        pC[i * N + j] += pA_signed[i * K + k] * pB_signed[k * N + j];
                    } else {
                        pC[i * N + j] += pA_signed[i * K + k] * pB_unsigned[k * N + j];
                    }
                } else {
                    if (signB == 1) {
                        pC[i * N + j] += pA_unsigned[i * K + k] * pB_signed[k * N + j];
                    } else {
                        pC[i * N + j] += pA_unsigned[i * K + k] * pB_unsigned[k * N + j];
                    }
                }
            }
        }
    }

    memcpy(src_C, pC, M * N * sizeof(int));

    if (signA == 1) {
        free(pA_signed);
    } else {
        free(pA_unsigned);
    }

    if (signB == 1) {
        free(pB_signed);
    } else {
        free(pB_unsigned);
    }
    free(pC);

    return 0;
}

void rightshift_int(const int *srcData, void *dstData, int shiftNum, int length, int is_16bit)
{
    int tmpData = 0;
    int half_data = 1 << (shiftNum - 1);
    const int maxDataforInt = is_16bit ? INT16_MAX : INT8_MAX;
    const int minDataforInt = is_16bit ? INT16_MIN : INT8_MIN;
    int i;
    int dst_data;

    half_data = shiftNum == 0 ? 0 : half_data;

    for (i = 0; i < length; i++) {

        tmpData = (srcData[i] + half_data) >> shiftNum; /* right shift */
        dst_data = 0; /*saturate to int8 */
        if (tmpData >= maxDataforInt) {
            dst_data = maxDataforInt;
        } else if (tmpData <= minDataforInt) {
            dst_data = minDataforInt;
        } else {
            dst_data = tmpData;
        }
        if (is_16bit) {
            *((signed short*)dstData + i) = (signed short)dst_data;
        } else {
            *((signed char*)dstData + i) = (signed char)dst_data;
        }
    }
}

void rightshift_unint(const int *srcData, void *dstData, int shiftNum, int length, int is_16bit)
{
    int tmpData = 0;
    long half_data = 1 << (shiftNum - 1);
    const int maxDataforUnInt = is_16bit ? UINT16_MAX : UINT8_MAX;
    int i;
    int dst_data;

    half_data = shiftNum == 0 ? 0 : half_data;

    for (i = 0; i < length; i++) {
        tmpData = (srcData[i] + half_data) >> shiftNum; /* right shift */
        dst_data = 0; /*saturate to int8 */
        if (tmpData >= maxDataforUnInt) {
            dst_data = maxDataforUnInt;
        } else if (tmpData <= UINT_MIN) {
            dst_data = UINT_MIN;
        } else {
            dst_data = tmpData;
        }
        if (is_16bit) {
            *((unsigned short*)dstData + i) = (unsigned short)dst_data;
        } else {
            *((unsigned char*)dstData + i) = (unsigned char)dst_data;
        }
    }
}

int cpu_matmul(signed char* src_A, signed char* src_B, void* output, struct Matpara para)
{
    int M = para.M;
    int K = para.K;
    int N = para.N;
    int trans_A = para.trans_A;
    int trans_B = para.trans_B;
    int A_sign = para.signA;
    int B_sign = para.signB;
    int right_shift_bit = para.right_shift_bit;
    int result_type = para.result_type;
    float alpha = para.alpha;
    float beta = para.beta;
    int* output_dst = (int*)malloc(M * N * sizeof(int));
    int i;
    int len;
    int ret = 0;

    if (src_A == NULL || src_B == NULL || output == NULL) {
        printf("the cpu_matmul param is error!\n");
        free(output_dst);
        return -1;
    }

    ret = cpu_matrix_mul(trans_A, trans_B, M, N, K, (void*)src_A, trans_A ? M:K, (void*)src_B, \
                        trans_B ? K:N, output_dst, A_sign, B_sign);
    if (ret) {
        printf("the cpy_matrix_mul failed!\n");
        free(output_dst);
        return ret;
    }

    if (result_type != 2) {
        len = M * N;
        if (A_sign == 1 || B_sign == 1) {
            rightshift_int(output_dst, output, right_shift_bit, len, result_type);
        } else {
            rightshift_unint(output_dst, output, right_shift_bit, len, result_type);
        }
    } else {
        float* outputC = (float*)output;
        for (i = 0; i < M * N; i++) {
            outputC[i] = alpha * output_dst[i] + beta;
        }
    }

    free(output_dst);

    return ret;
}

/*
  test_cv_min_max
*/
int cpu_min_max(float* XHost, int L, float* min_cpu, float* max_cpu)
{
    int ret = 0;
    float min_value = 0;
    float max_value = 0;

    if (XHost == NULL) {
        printf("the param Xhost or min_cpu or nax_cpu is null\n");
        return -1;
    }

    if (min_cpu == NULL && max_cpu == NULL) {
        printf("Do Not Calculate Min & Max.\n");
        return ret;
    }

    if (min_cpu != NULL) {
        min_value = XHost[0];
        for (int i = 1; i < L; ++i) {
            min_value = min_value < XHost[i] ? min_value : XHost[i];
        }
        *min_cpu = min_value;
    }

    if (max_cpu != NULL) {
        max_value = XHost[0];
        for (int i = 1; i < L; ++i) {
            max_value = max_value > XHost[i] ? max_value : XHost[i];
        }
        *max_cpu = max_value;
    }

    return ret;
}

/*
  test_cv_nms
*/
int compareBBox(const void* a, const void* b)
{
    const face_rect_t* rectA = (const face_rect_t*)a;
    const face_rect_t* rectB = (const face_rect_t*)b;

    return (rectB->score > rectA->score) - (rectB->score < rectA->score);
}

float iou(face_rect_t a, face_rect_t b)
{
    float x1 = bm_max(a.x1, b.x1);
    float y1 = bm_max(a.y1, b.y1);
    float x2 = bm_min(a.x2, b.x2);
    float y2 = bm_min(a.y2, b.y2);

    if (x2 < x1 || y2 < y1) return 0;

    float a_width = a.x2 - a.x1 + 1;
    float a_height = a.y2 - a.y1 + 1;
    float b_width = b.x2 - b.x1 + 1;
    float b_height = b.y2 - b.y1 + 1;

    float inter_area = (x2 - x1 + 1) * (y2 - y1 + 1);
    float iou = inter_area / ((a_width * a_height) + (b_width * b_height) - inter_area);

    return iou;
}

int cpu_nms(face_rect_t* proposals, const float nms_threshold, face_rect_t* nmsProposals,
                    int size, int* result_size)
{
    bool* keep = (bool*)malloc(size * sizeof(bool));
    face_rect_t* bboxes = (face_rect_t*)malloc(size * sizeof(face_rect_t));
    *result_size = 0;
    int i, j;

    memset(keep, true, size * sizeof(bool));
    memcpy(bboxes, proposals, size * sizeof(face_rect_t));
    qsort(bboxes, size, sizeof(bboxes[0]), compareBBox);

    for (i = 0; i < size; ++i) {
        if (keep[i]) {
            for(j = i + 1; j < size; j++) {
                if (keep[j] && (iou(bboxes[i], bboxes[j]) > nms_threshold)) {
                    keep[j] = false;
                }
            }
        }
    }

    j = 0;
    for (i = 0; i < size; ++i) {
        if (keep[i]) {
            (*result_size)++;
            nmsProposals[j++] = bboxes[i];
        }
    }

    free(keep);
    free(bboxes);
    return 0;
}

/*
  test_cv_put_text
*/
int put_text_cpu(unsigned char* input, int height, int width, const char* text,
                        bmcv_point_t org, int fontFace, float fontScale, int format,
                        unsigned char color[3], int thickness)
{
    int offset_list[IMAGE_CHN_NUM_MAX] = {0};
    unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
    bmcv_color_t rgb = {color[0], color[1], color[2]};
    int step[IMAGE_CHN_NUM_MAX];
    int ret = 0;
    struct timeval t1, t2;
    bmMat mat;

    ret = get_image_offset(format, width, height, offset_list);
    if (ret != 0) {
        printf("get_image_offset failed!\n");
        return ret;
    }

    in_ptr[0] = input;
    in_ptr[1] = input + offset_list[0];
    in_ptr[2] = input + offset_list[0] + offset_list[1];

    ret = get_image_default_step(format, width, step);
    if (ret != 0) {
        printf("get_image_default_step failed!\n");
        return ret;
    }

    mat.width = width;
    mat.height = height;
    mat.format = (bm_image_format_ext)format;
    mat.step = step;
    mat.data = (void**)in_ptr;

    gettimeofday(&t1, NULL);
    put_text(mat, text, org, fontFace, fontScale, rgb, thickness);
    gettimeofday(&t2, NULL);
    printf("Put_text CPU using time: %ld(us)\n", TIME_COST_US(t1, t2));

    return ret;
}

/*
  test_cv_pyramid
*/
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

unsigned char customRound(float num)
{
    float decimalPart = num - floor(num);
    if (decimalPart < 0.5) {
        return (unsigned char)floor(num);
    } else {
        return (unsigned char)ceil(num);
    }
}

int cpu_pyramid_down(unsigned char* input, unsigned char* output,
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

/*
  test_cv_quantify
*/
int quantify_cpu(
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

/*
  test_cv_threshold
*/
int threshold_cpu(
        unsigned char* input,
        unsigned char* output,
        int height,
        int width,
        unsigned char threshold,
        unsigned char max_val,
        int type) {
    switch (type) {
        case 0:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = max_val;
                } else {
                    output[i] = 0;
                }
            }
            break;
        case 1:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = 0;
                } else {
                    output[i] = max_val;
                }
            }
            break;
        case 2:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = threshold;
                } else {
                    output[i] = input[i];
                }
            }
            break;
        case 3:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = input[i];
                } else {
                    output[i] = 0;
                }
            }
            break;
        case 4:
            for(int i = 0; i < width * height; i++){
                if (input[i] > threshold) {
                    output[i] = 0;
                } else {
                    output[i] = input[i];
                }
            }
            break;
        default:
            printf("threshold type is illegal!\n");
            return -1;
    }
    return 0;
}

/*
  test_cv_transpose
*/
int cpu_transpose(void* input, void* output, int channel, int height,
                        int width, int stride, int dtype)
{
    int i, j, k;

    if (input == NULL || output == NULL) {
        printf("the input or the output is null!\n");
        return -1;
    }
    if (dtype == DTYPE_F32) {
        float* input_tmp = (float*)input;
        float* output_tmp = (float*)output;

        for (i = 0; i < channel; ++i) {
            for (j = 0; j < width; ++j) {
                for (k = 0; k < height; ++k) {
                    output_tmp[i * height * width + j * height + k] = \
                                                    input_tmp[i * height * stride + k * stride + j];
                }
            }
        }
    } else if (dtype == DTYPE_U8) {
        unsigned char* input_tmp = (unsigned char*)input;
        unsigned char* output_tmp = (unsigned char*)output;

        for (i = 0; i < channel; ++i) {
            for (j = 0; j < width; ++j) {
                for (k = 0; k < height; ++k) {
                    output_tmp[i * height * width + j * height + k] = \
                                                    input_tmp[i * height * stride + k * stride + j];
                }
            }
        }
    }

    return 0;
}

/*
  test_cv_warp_affine
*/
float det(float arcs[3][3],int n){
    if(n == 1){
        return arcs[0][0];
    }
    float ans = 0;
    float temp[3][3];
    int i,j,k;
    for(i = 0;i < n;i++){
        for(j = 0;j < n - 1;j++){
            for(k = 0;k < n - 1;k++){
                temp[j][k] = arcs[j+1][(k >= i) ? k+1 : k];
            }
        }
        float t = det(temp, n-1);
        if(i%2 == 0){
            ans += arcs[0][i] * t;
        }
        else{
            ans -=  arcs[0][i] * t;
        }
    }
    return ans;
}

int get_source_idx(int idx, int *matrix, int image_n) {
    for (int i = 0; i < image_n; i++) {
        if (idx < matrix[i]) {
            return i;
        }
    }
    printf("%s:%d[%s] Error:get source idx error\n",__FILE__, __LINE__, __FUNCTION__);
    exit(-1);
}

void inverse_matrix_1(float matrix[3][3], float matrix_inv[2][3]){
    float det_value = det(matrix, 3);
    matrix_inv[0][0] = matrix[1][1] / det_value;
    matrix_inv[0][1] = -matrix[0][1] / det_value;
    matrix_inv[0][2] = (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) / det_value;
    matrix_inv[1][0] = - matrix[1][0] / det_value;
    matrix_inv[1][1] = matrix[0][0] / det_value;
    matrix_inv[1][2] = (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) / det_value;
}

unsigned char fetch_pixel_2(float x_idx, float y_idx, int c_idx, int width, int height, unsigned char* image)
{
    x_idx = BM_MIN(width - 1, x_idx);
    x_idx = BM_MAX(0, x_idx);

    y_idx = BM_MIN(height - 1, y_idx);
    y_idx = BM_MAX(0, y_idx);

    int ceil_x  = ceil(x_idx);
    int ceil_y  = ceil(y_idx);
    int floor_x = floor(x_idx);
    int floor_y = floor(y_idx);

    unsigned char pixel_lu = image[width * height * c_idx + ceil_y * width + floor_x];
    unsigned char pixel_ld = image[width * height * c_idx + floor_y * width + floor_x];
    unsigned char pixel_ru = image[width * height * c_idx + ceil_y * width + ceil_x];
    unsigned char pixel_rd = image[width * height * c_idx + floor_y * width + ceil_x];

    float coef_x_ceil = x_idx - floor_x;
    float coef_y_ceil = y_idx - floor_y;

    float coef_x_floor = 1.0 - coef_x_ceil;
    float coef_y_floor = 1.0 - coef_y_ceil;

    unsigned char mix = (unsigned char)(coef_x_floor * coef_y_ceil * pixel_lu +
                coef_x_floor * coef_y_floor * pixel_ld +
                coef_x_ceil * coef_y_ceil * pixel_ru +
                coef_x_ceil * coef_y_floor * pixel_rd + 0.5f);
    return mix;
};

int bmcv_affine_nearest_1n_ref(
                                    // input
                                    unsigned char *src_image,
                                    float *        trans_mat,
                                    int            image_c,
                                    int            image_sh,
                                    int            image_sw,
                                    int            image_dh,
                                    int            image_dw,
                                    // output
                                    int *          map,
                                    unsigned char *dst_image,
                                    bool use_opencv) {

    UNUSED(image_c);
    float* tensor_S = (float*)malloc(image_dh * image_dw * 2 * sizeof(float));
    float *tensor_SX    = tensor_S;
    float *tensor_SY    = tensor_SX + image_dh * image_dw;
    int *tensor_DX      = map;
    int *tensor_DY      = tensor_DX + image_dh * image_dw;
    int             dst_w     = image_dw;
    int             dst_h     = image_dh;
    int             src_w     = image_sw;
    int             src_h     = image_sh;

    if (use_opencv){
        float mat_array[6];
        float matrix_tem[3][3];
        float matrix_tem_inv[2][3];
        memset(matrix_tem, 0, sizeof(matrix_tem));
        memset(matrix_tem_inv, 0, sizeof(matrix_tem_inv));
        for(int a = 0;a < 6;a++){
            matrix_tem[(a/3)][(a%3)] = trans_mat[a];
        }
        matrix_tem[2][0] = 0;
        matrix_tem[2][1] = 0;
        matrix_tem[2][2] = 1;
        inverse_matrix_1(matrix_tem, matrix_tem_inv);
        for(int a = 0;a < 6;a++){
            mat_array[a] = matrix_tem_inv[(a/3)][(a%3)];
        }
        float           m0        = mat_array[0];
        float           m1        = mat_array[1];
        float           m2        = mat_array[2];
        float           m3        = mat_array[3];
        float           m4        = mat_array[4];
        float           m5        = mat_array[5];

        // generate the input for calculate coordinate map.
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                tensor_SX[y * dst_w + x] = (float)x;
                tensor_SY[y * dst_w + x] = (float)y;
            }
        }

        // calculate coordinate map.
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                float dx = tensor_SX[y * dst_w + x] * m0 +
                        tensor_SY[y * dst_w + x] * m1 + m2;
                float dy = tensor_SX[y * dst_w + x] * m3 +
                        tensor_SY[y * dst_w + x] * m4 + m5;
                // tensor_DX[y * dst_w + x] = (unsigned short)(dx + 0.5f);
                // tensor_DY[y * dst_w + x] = (unsigned short)(dy + 0.5f);
                tensor_DX[y * dst_w + x] = (int)(dx + 0.5f);
                tensor_DY[y * dst_w + x] = (int)(dy + 0.5f);
                tensor_DX[y * dst_w + x] =
                    BM_MIN(tensor_DX[y * dst_w + x], image_sw - 1);
                tensor_DX[y * dst_w + x] =
                    BM_MAX(tensor_DX[y * dst_w + x], 0);
                tensor_DY[y * dst_w + x] =
                    BM_MIN(tensor_DY[y * dst_w + x], image_sh - 1);
                tensor_DY[y * dst_w + x] =
                    BM_MAX(tensor_DY[y * dst_w + x], 0);
            }
        }
    }else{
        float           m0        = trans_mat[0];
        float           m1        = trans_mat[1];
        float           m2        = trans_mat[2];
        float           m3        = trans_mat[3];
        float           m4        = trans_mat[4];
        float           m5        = trans_mat[5];
        // generate the input for calculate coordinate map.
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                tensor_SX[y * dst_w + x] = (float)x;
                tensor_SY[y * dst_w + x] = (float)y;
            }
        }
        // calculate coordinate map.
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                float dx = tensor_SX[y * dst_w + x] * m0 +
                        tensor_SY[y * dst_w + x] * m1 + m2;
                float dy = tensor_SX[y * dst_w + x] * m3 +
                        tensor_SY[y * dst_w + x] * m4 + m5;
                // tensor_DX[y * dst_w + x] = (unsigned short)(dx + 0.5f);
                // tensor_DY[y * dst_w + x] = (unsigned short)(dy + 0.5f);
                tensor_DX[y * dst_w + x] = (int)(dx + 0.5f);
                tensor_DY[y * dst_w + x] = (int)(dy + 0.5f);
                tensor_DX[y * dst_w + x] =
                    BM_MIN(tensor_DX[y * dst_w + x], image_sw - 1);
                tensor_DX[y * dst_w + x] =
                    BM_MAX(tensor_DX[y * dst_w + x], 0);
                tensor_DY[y * dst_w + x] =
                    BM_MIN(tensor_DY[y * dst_w + x], image_sh - 1);
                tensor_DY[y * dst_w + x] =
                    BM_MAX(tensor_DY[y * dst_w + x], 0);
            }
        }

    }

    int w_stride = 1;
    int src_w_stride = src_w * w_stride;
    int dst_w_stride = dst_w * w_stride;

    // warp in source image directly.
    if (image_c == 1) {
        unsigned char *sb = src_image;
        unsigned char *db = dst_image;

        tensor_DX         = map;
        tensor_DY         = tensor_DX + dst_h * dst_w;

        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                unsigned short sx = tensor_DX[y * dst_w + x];
                unsigned short sy = tensor_DY[y * dst_w + x];
                db[y * dst_w_stride + x * w_stride] =
                    sb[sy * src_w_stride + sx * w_stride];
            }
        }
    }
    else if (image_c == 3) {
        unsigned char *sb = src_image;
        unsigned char *sg = sb + src_w_stride * src_h;
        unsigned char *sr = sg + src_w_stride * src_h;
        unsigned char *db = dst_image;
        unsigned char *dg = db + dst_w_stride * dst_h;
        unsigned char *dr = dg + dst_w_stride * dst_h;

        tensor_DX         = map;
        tensor_DY         = tensor_DX + dst_h * dst_w;

        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                unsigned short sx = tensor_DX[y * dst_w + x];
                unsigned short sy = tensor_DY[y * dst_w + x];
                db[y * dst_w_stride + x * w_stride] =
                    sb[sy * src_w_stride + sx * w_stride];
                dg[y * dst_w_stride + x * w_stride] =
                    sg[sy * src_w_stride + sx * w_stride];
                dr[y * dst_w_stride + x * w_stride] =
                    sr[sy * src_w_stride + sx * w_stride];
            }
        }
    }
    free(tensor_S);
    return 0;
}

int bmcv_affine_bilinear_1n_ref(
                                    // input
                                    unsigned char *src_image,
                                    float *        trans_mat,
                                    int            image_c,
                                    int            image_sh,
                                    int            image_sw,
                                    int            image_dh,
                                    int            image_dw,
                                    // output
                                    float *        map_index,
                                    unsigned char *dst_image,
                                    bool opencv_mode) {
    UNUSED(image_c);
    float* tensor_S = (float*)malloc(image_dh *image_dw * 2 * sizeof(float));
    float *tensor_SX = tensor_S;
    float *tensor_SY = tensor_SX + image_dh * image_dw;
    float *tensor_DX = map_index;
    float *tensor_DY = tensor_DX + image_dh * image_dw;
    int             dst_w     = image_dw;
    int             dst_h     = image_dh;
    int             src_w     = image_sw;
    int             src_h     = image_sh;

    if (opencv_mode){
        float mat_array[6];
        float matrix_tem[3][3];
        float matrix_tem_inv[2][3];
        memset(matrix_tem, 0, sizeof(matrix_tem));
        memset(matrix_tem_inv, 0, sizeof(matrix_tem_inv));
        for(int a = 0;a < 6;a++){
            matrix_tem[(a/3)][(a%3)] = trans_mat[a];
        }
        matrix_tem[2][0] = 0;
        matrix_tem[2][1] = 0;
        matrix_tem[2][2] = 1;
        inverse_matrix_1(matrix_tem, matrix_tem_inv);
        for(int a = 0;a < 6;a++){
            mat_array[a] = matrix_tem_inv[(a/3)][(a%3)];
        }
        float           m0        = mat_array[0];
        float           m1        = mat_array[1];
        float           m2        = mat_array[2];
        float           m3        = mat_array[3];
        float           m4        = mat_array[4];
        float           m5        = mat_array[5];

        // generate the input for calculate coordinate map.
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                tensor_SX[y * dst_w + x] = (float)x;
                tensor_SY[y * dst_w + x] = (float)y;
            }
        }
        // calculate coordinate map.
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                float dx = tensor_SX[y * dst_w + x] * m0 +
                        tensor_SY[y * dst_w + x] * m1 + m2;
                float dy = tensor_SX[y * dst_w + x] * m3 +
                        tensor_SY[y * dst_w + x] * m4 + m5;
                tensor_DX[y * dst_w + x] = dx;
                tensor_DY[y * dst_w + x] = dy;
            }
        }
    } else {
        float           m0        = trans_mat[0];
        float           m1        = trans_mat[1];
        float           m2        = trans_mat[2];
        float           m3        = trans_mat[3];
        float           m4        = trans_mat[4];
        float           m5        = trans_mat[5];

        // generate the input for calculate coordinate map.
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                tensor_SX[y * dst_w + x] = (float)x;
                tensor_SY[y * dst_w + x] = (float)y;
            }
        }
        // calculate coordinate map.
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                float dx = tensor_SX[y * dst_w + x] * m0 +
                        tensor_SY[y * dst_w + x] * m1 + m2;
                float dy = tensor_SX[y * dst_w + x] * m3 +
                        tensor_SY[y * dst_w + x] * m4 + m5;
                tensor_DX[y * dst_w + x] = dx;
                tensor_DY[y * dst_w + x] = dy;
            }
        }
    }

    int w_stride = 1;
    int src_w_stride = src_w * w_stride;
    int dst_w_stride = dst_w * w_stride;

    if (image_c == 3) {
        // warp in source image directly.
        unsigned char *sb = src_image;
        unsigned char *sg = sb + src_w_stride * src_h;
        unsigned char *sr = sg + src_w_stride * src_h;
        unsigned char *db = dst_image;
        unsigned char *dg = db + dst_w_stride * dst_h;
        unsigned char *dr = dg + dst_w_stride * dst_h;
        tensor_DX         = map_index;
        tensor_DY         = tensor_DX + dst_h * dst_w;
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                float sx = tensor_DX[y * dst_w + x];
                float sy = tensor_DY[y * dst_w + x];
                db[y * dst_w_stride + x * w_stride] =
                    fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sb);
                dg[y * dst_w_stride + x * w_stride] =
                    fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sg);
                dr[y * dst_w_stride + x * w_stride] =
                    fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sr);
            }
        }
    }else {
        // warp in source image directly.
        unsigned char *sb = src_image;
        unsigned char *db = dst_image;
        tensor_DX         = map_index;
        tensor_DY         = tensor_DX + dst_h * dst_w;
        for (int y = 0; y < dst_h; y++) {
            for (int x = 0; x < dst_w; x++) {
                float sx = tensor_DX[y * dst_w + x];
                float sy = tensor_DY[y * dst_w + x];
                db[y * dst_w_stride + x * w_stride] =
                    fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sb);
            }
        }
    }
    free(tensor_S);
    return 0;
}

int bmcv_warp_ref(
    // input
    unsigned char* src_image,
    float* trans_mat,
    int matrix_num[4],
    int image_n,
    int image_c,
    int image_sh,
    int image_sw,
    int image_dh,
    int image_dw,
    int is_bilinear,
    // output
    int* map,
    unsigned char* dst_image,
    bool use_opencv)
{
    int output_num      = 0;
    int matrix_sigma[4] = {0};
    bm_status_t ret = BM_SUCCESS;
    for (int i = 0; i < image_n; i++) {
        output_num += matrix_num[i];
        matrix_sigma[i] = output_num;
    }
    for (int i = 0; i < output_num; i++) {
        int s_idx = get_source_idx(i, matrix_sigma, image_n);
        int src_offset = s_idx * image_c * image_sh * image_sw;
        int dst_offset = i * image_c * image_dh * image_dw;
        if (is_bilinear)
            ret = bmcv_affine_bilinear_1n_ref(
                src_image + src_offset,
                trans_mat + i * 6,
                image_c,
                image_sh,
                image_sw,
                image_dh,
                image_dw,
                (float *)map + i * image_dh * image_dw * 2,
                dst_image + dst_offset,
                use_opencv);
        else
            ret = bmcv_affine_nearest_1n_ref(
                src_image + src_offset,
                trans_mat + i * 6,
                image_c,
                image_sh,
                image_sw,
                image_dh,
                image_dw,
                (int *)map + i * image_dh * image_dw * 2,
                dst_image + dst_offset,
                use_opencv);
        if (ret != BM_SUCCESS)
            return ret;
    }
    return ret;
}

/*
  test_cv_warp_perspective
*/
int bmcv_perspective_bilinear_1n_ref(
                                    // input
                                    unsigned char *src_image,
                                    float *        trans_mat,
                                    int            image_c,
                                    int            image_sh,
                                    int            image_sw,
                                    int            image_dh,
                                    int            image_dw,
                                    // output
                                    float          *map,
                                    unsigned char  *dst_image,
                                    float*     tensor_S) {
    UNUSED(image_c);
    float* tensor_SX = tensor_S;
    float* tensor_SY = tensor_SX + image_dh * image_dw;
    float *tensor_DX = map;
    float *tensor_DY = tensor_DX + image_dh * image_dw;
    int dst_w = image_dw;
    int dst_h = image_dh;
    int src_w = image_sw;
    int src_h = image_sh;
    float m0 = trans_mat[0];
    float m1 = trans_mat[1];
    float m2 = trans_mat[2];
    float m3 = trans_mat[3];
    float m4 = trans_mat[4];
    float m5 = trans_mat[5];
    float m6 = trans_mat[6];
    float m7 = trans_mat[7];
    float m8 = trans_mat[8];

    // generate the input for calculate coordinate map.
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            tensor_SX[y * dst_w + x] = (float)x;
            tensor_SY[y * dst_w + x] = (float)y;
        }
    }
    // calculate coordinate map.
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float dx = tensor_SX[y * dst_w + x] * m0 +
                       tensor_SY[y * dst_w + x] * m1 + m2;
            float dy = tensor_SX[y * dst_w + x] * m3 +
                       tensor_SY[y * dst_w + x] * m4 + m5;
            float dz = tensor_SX[y * dst_w + x] * m6 +
                       tensor_SY[y * dst_w + x] * m7 + m8;
            dx = dx / dz;
            dy = dy / dz;

            tensor_DX[y * dst_w + x] = dx;
            tensor_DY[y * dst_w + x] = dy;
        }
    }
    int w_stride = 1;
    int src_w_stride = src_w * w_stride;
    int dst_w_stride = dst_w * w_stride;

    // warp in source image directly.
    unsigned char *sb = src_image;
    unsigned char *sg = sb + src_w_stride * src_h;
    unsigned char *sr = sg + src_w_stride * src_h;
    unsigned char *db = dst_image;
    unsigned char *dg = db + dst_w_stride * dst_h;
    unsigned char *dr = dg + dst_w_stride * dst_h;
    tensor_DX         = map;
    tensor_DY         = tensor_DX + dst_h * dst_w;
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float sx = tensor_DX[y * dst_w + x];
            float sy = tensor_DY[y * dst_w + x];
            db[y * dst_w_stride + x * w_stride] =
                fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sb);
            dg[y * dst_w_stride + x * w_stride] =
                fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sg);
            dr[y * dst_w_stride + x * w_stride] =
                fetch_pixel_2(sx, sy, 0, image_sw, image_sh, sr);
        }
    }
    return 0;
}

int bmcv_perspective_nearest_1n_ref(
                                    // input
                                    unsigned char *src_image,
                                    float *        trans_mat,
                                    int            image_c,
                                    int            image_sh,
                                    int            image_sw,
                                    int            image_dh,
                                    int            image_dw,
                                    // output
                                    int *map,
                                    unsigned char * dst_image) {
    UNUSED(image_c);
    float* tensor_S = (float*)malloc(image_dh *image_dw * 2 * sizeof(float));
    float*     tensor_SX  = tensor_S;
    float*     tensor_SY  = tensor_SX + image_dh * image_dw;
    int        *tensor_DX = map;
    int        *tensor_DY = tensor_DX + image_dh * image_dw;
    int        dst_w      = image_dw;
    int        dst_h      = image_dh;
    int        src_w      = image_sw;
    int        src_h      = image_sh;
    float      m0         = trans_mat[0];
    float      m1         = trans_mat[1];
    float      m2         = trans_mat[2];
    float      m3         = trans_mat[3];
    float      m4         = trans_mat[4];
    float      m5         = trans_mat[5];
    float      m6         = trans_mat[6];
    float      m7         = trans_mat[7];
    float      m8         = trans_mat[8];

    // generate the input for calculate coordinate map.
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            tensor_SX[y * dst_w + x] = (float)x;
            tensor_SY[y * dst_w + x] = (float)y;
        }
    }
    // calculate coordinate map.
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float dx = tensor_SX[y * dst_w + x] * m0 +
                       tensor_SY[y * dst_w + x] * m1 + m2;
            float dy = tensor_SX[y * dst_w + x] * m3 +
                       tensor_SY[y * dst_w + x] * m4 + m5;
            float dz = tensor_SX[y * dst_w + x] * m6 +
                       tensor_SY[y * dst_w + x] * m7 + m8;

            dx = dx / dz;
            dy = dy / dz;

            tensor_DX[y * dst_w + x] = (int)(dx + 0.5f);
            tensor_DY[y * dst_w + x] = (int)(dy + 0.5f);
            tensor_DX[y * dst_w + x] =
                BM_MIN(tensor_DX[y * dst_w + x], image_sw - 1);
            tensor_DX[y * dst_w + x] =
                BM_MAX(tensor_DX[y * dst_w + x], 0);
            tensor_DY[y * dst_w + x] =
                BM_MIN(tensor_DY[y * dst_w + x], image_sh - 1);
            tensor_DY[y * dst_w + x] =
                BM_MAX(tensor_DY[y * dst_w + x], 0);
        }
    }

    int w_stride = 1;
    int src_w_stride = src_w * w_stride;
    int dst_w_stride = dst_w * w_stride;

    // warp in source image directly.
    unsigned char *sb = src_image;
    unsigned char *sg = sb + src_w_stride * src_h;
    unsigned char *sr = sg + src_w_stride * src_h;
    unsigned char *db = dst_image;
    unsigned char *dg = db + dst_w_stride * dst_h;
    unsigned char *dr = dg + dst_w_stride * dst_h;
    tensor_DX         = map;
    tensor_DY         = tensor_DX + dst_h * dst_w;

    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            unsigned short  sx = tensor_DX[y * dst_w + x];
            unsigned short  sy = tensor_DY[y * dst_w + x];
            db[y * dst_w_stride + x * w_stride] =
                sb[sy * src_w_stride + sx * w_stride];
            dg[y * dst_w_stride + x * w_stride] =
                sg[sy * src_w_stride + sx * w_stride];
            dr[y * dst_w_stride + x * w_stride] =
                sr[sy * src_w_stride + sx * w_stride];
        }
    }
    free(tensor_S);
    return 0;
}

int bmcv_warp_perspective_ref(
    // input
    unsigned char* src_image,
    float* trans_mat,
    int matrix_num[4],
    int image_n,
    int image_c,
    int image_sh,
    int image_sw,
    int image_dh,
    int image_dw,
    int use_bilinear,
    // output
    int* map,
    unsigned char* dst_image,
    float* tensor_S)
{
    int output_num = 0;
    int matrix_sigma[4] = {0};
    bm_status_t ret = BM_SUCCESS;
    for (int i = 0; i < image_n; i++) {
        output_num += matrix_num[i];
        matrix_sigma[i] = output_num;
    }
    for (int i = 0; i < output_num; i++) {
        int s_idx = get_source_idx(i, matrix_sigma, image_n);
        int src_offset = s_idx * image_c * image_sh * image_sw;
        int dst_offset = i * image_c * image_dh * image_dw;
        if (use_bilinear)
            ret = bmcv_perspective_bilinear_1n_ref(
                    src_image + src_offset,
                    trans_mat + i * 9,
                    image_c,
                    image_sh,
                    image_sw,
                    image_dh,
                    image_dw,
                    (float*)map + i * image_dh * image_dw * 2,
                    dst_image + dst_offset,
                    tensor_S);
        else
            ret = bmcv_perspective_nearest_1n_ref(
                    src_image + src_offset,
                    trans_mat + i * 9,
                    image_c,
                    image_sh,
                    image_sw,
                    image_dh,
                    image_dw,
                    map + i * image_dh * image_dw * 2,
                    dst_image + dst_offset);

        if (ret != BM_SUCCESS)
            return ret;
    }
    return ret;
}

/*
  test_cv_base64
*/
int base64_enc_core(void *dst, void *src, char *box) {
    // convert 3x8bits to 4x6bits
    unsigned char *d, *s;
        d = (unsigned char*)dst;
        s = (unsigned char*)src;
    d[0] = box[(s[0] >> 2) & 0x3f];
    d[1] = box[((s[0] & 0x3) << 4) | (s[1] >> 4)];
    d[2] = box[((s[1] & 0xf) << 2) | (s[2] >> 6)];
    d[3] = box[(s[2] & 0x3f)];
    return 0;
}

int base64_enc_left(void *dst, void *src, int size, char *box) {
    // convert 3x8bits to 4x6bits
    unsigned char *d, s[3];
    memset(s, 0x00, sizeof(s));
    memcpy(s, src, size);
        d = (unsigned char *)dst;
    d[0] = box[(s[0] >> 2) & 0x3f];
    d[1] = box[((s[0] & 0x3) << 4) | (s[1] >> 4)];
    d[2] = box[((s[1] & 0xf) << 2) | (s[2] >> 6)];
    d[3] = box[64];
    if ( size == 1 )
        d[2] = d[3];
    return 0;
}

int base64_encode(void *dst, void *src, int size, char *box) {
    unsigned long block = size / 3;
    unsigned long left = size % 3;
        unsigned char *d = (unsigned char *)dst;
        unsigned char *s = (unsigned char *)src;
        unsigned i;
    for (i = 0; i < block; ++i) {
        base64_enc_core(d, s, box);
        d += 4;
        s += 3;
    }
    if (left)
        base64_enc_left(d, s, left, box);
    return 0;
}

int base64_dec_core(void *dst, void *src, unsigned char *box) {
    unsigned char *d, *s;
    unsigned char t[4];
        d = (unsigned char *)dst;
        s = (unsigned char *)src;
    t[0] = box[s[0]];
    t[1] = box[s[1]];
    t[2] = box[s[2]];
    t[3] = box[s[3]];

    d[0] = (t[0] << 2) | (t[1] >> 4);
    d[1] = (t[1] << 4) | (t[2] >> 2);
    d[2] = (t[2] << 6) | (t[3]);

    return 0;
}

int base64_box_t(unsigned char box_t[256], char *box) {
        int i;

        memset(box_t, 0x00, 256);
        for (i = 0; i < 64; ++i) {
            box_t[(unsigned int)box[i]] = i;
        }
        box_t[64] = 0;
        return 0;
}

int base64_decode(void *dst, void *src, unsigned long size, char *box) {
        unsigned char *d = (unsigned char *)dst;
        char *s = (char *)src;
    assert(size % 4 == 0);
    unsigned char box_t[256];
    base64_box_t(box_t, box);
    unsigned long block = size / 4;
    unsigned long i;
    for (i = 0; i < block; ++i) {
        base64_dec_core(d, s, box_t);
        d += 3;
        s += 4;
    }
    return 0;
}

/*
  test_cv_convert_to
*/
int32_t convert_to_ref_float( float            *src,
                              float            *dst,
                              convert_to_arg_t *convert_to_arg,
                              int               image_num,
                              int               image_channel,
                              int               image_h,
                              int               image_w_stride,
                              int               image_w,
                              int               convert_format){
    int image_len = image_num * image_channel * image_w_stride * image_h;
    float *temp_old_buf = malloc(image_len * sizeof(float));
    memcpy(temp_old_buf, src, sizeof(float) * image_len);

    for(int n_idx = 0; n_idx < image_num; n_idx++){
        for(int c_idx = 0; c_idx < image_channel; c_idx++){
            for(int y = 0; y < image_h; y++){
                for(int x = 0; x < image_w; x++){
                    int check_idx = (n_idx * image_channel + c_idx) * image_w * image_h +
                                     y * image_w + x;
                    int src_check_idx = (n_idx * image_channel + c_idx) *
                                         image_w_stride * image_h +
                                         y * image_w_stride + x;
                    float temp = 0.0;
                    temp = (temp_old_buf[src_check_idx]) * convert_to_arg[c_idx].alpha +
                            convert_to_arg[c_idx].beta;
                    dst[check_idx] = (float) round (temp);
                }

            }
        }
    }

    free(temp_old_buf);
    return 0;
}

int32_t convert_to_ref_int8( signed char            *src,
                             signed char            *dst,
                             convert_to_arg_t *convert_to_arg,
                             int               image_num,
                             int               image_channel,
                             int               image_h,
                             int               image_w_stride,
                             int               image_w,
                             int               convert_format){
    int image_len = image_num * image_channel * image_w_stride * image_h;
    signed char *temp_old_buf = malloc(image_len * sizeof(signed char));
    memcpy(temp_old_buf, src, sizeof(signed char) * image_len);

    for(int n_idx = 0; n_idx < image_num; n_idx++){
        for(int c_idx = 0; c_idx < image_channel; c_idx++){
            for(int y = 0; y < image_h; y++){
                for(int x = 0; x < image_w; x++){
                    int check_idx = (n_idx * image_channel + c_idx) * image_w * image_h +
                                     y * image_w + x;
                    int src_check_idx = (n_idx * image_channel + c_idx) *
                                         image_w_stride * image_h +
                                         y * image_w_stride + x;
                    float temp = 0.0;
                    temp = (temp_old_buf[src_check_idx]) * convert_to_arg[c_idx].alpha +
                            convert_to_arg[c_idx].beta;
                    temp = (temp > 127) ? (127)
                                            : ((temp < -128) ? (-128) : (temp));
                    dst[check_idx] = (signed char) round (temp);
                }
            }
        }
    }

    free(temp_old_buf);
    return 0;
}

/*
  test_cv_copy_to
*/
int packedToplanar_float(float * packed, int channel, int width, int height){
    float * planar = (float *)malloc(channel * width * height * sizeof(float));
    float * p_img = packed;
    float * p_r = planar;
    float * p_g = planar + height * width;
    float * p_b = planar + height * width * 2;
    for(int i = 0; i < height * width; i++){
        * p_r++ = * p_img++;
        * p_g++ = * p_img++;
        * p_b++ = * p_img++;
    }
    memcpy(packed, planar, sizeof(float) * channel * width * height);
    free(planar);
    return 0;
}

typedef enum { COPY_TO_GRAY = 0, COPY_TO_BGR, COPY_TO_RGB } padding_corlor_e;
typedef enum { PLANNER = 0, PACKED } padding_format_e;

int copy_to_ref_v2_float(float * src,
                                float * dst,
                                int channel,
                                int in_h,
                                int in_w,
                                int out_h,
                                int out_w,
                                int start_x,
                                int start_y,
                                int ispacked){
    if(ispacked){
        packedToplanar_float(src, channel, in_w, in_h);
    }
    for(int c = 0; c < channel; c++){
        for(int i = 0; i < in_h; i++){
            for(int j = 0; j < in_w; j++){
                int index = c * out_w * out_h + (start_y + i) * out_w + start_x + j;
                    dst[index] = src[(c * in_h * in_w) + i * in_w + j];
            }
        }
    }
    return 0;
}

/*
  test_cv_hm_distance
*/
int hammingDistance(int x, int y) {
    int cnt = 0;
    int z = x ^ y;
    while (z != 0) {
        cnt += z & 1;
        z = z >> 1;
    }
    return cnt;
}

void native_cal_hammingDistance(int *input1, int *input2, int *output, int bits_len,
                                       int input1_num, int input2_num) {
    for (int i = 0; i < input1_num; i++) {
        for(int j = 0; j < input2_num; j++) {
                for(int d = 0; d < bits_len; d++) {
                    output[i * input2_num + j] +=
                    hammingDistance(input1[i * bits_len + d], input2[j * bits_len + d]);
            }
        }
    }
}

/*
  test_cv_sort
*/
void merge_ascend_sort(sort_t ref_res[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    sort_t L[n1], R[n2];

    for (int i = 0; i < n1; i++) {
        L[i] = ref_res[left + i];
    }
    for (int j = 0; j < n2; j++) {
        R[j] = ref_res[mid + 1 + j];
    }

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].val <= R[j].val) {
            ref_res[k] = L[i];
            i++;
        } else {
        ref_res[k] = R[j];
        j++;
        }
        k++;
    }
    while (i < n1) {
        ref_res[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        ref_res[k] = R[j];
        j++;
        k++;
    }
}

void merge_descend_sort(sort_t ref_res[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    sort_t L[n1], R[n2];

    for (int i = 0; i < n1; i++) {
        L[i] = ref_res[left + i];
    }
    for (int j = 0; j < n2; j++) {
        R[j] = ref_res[mid + 1 + j];
    }

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].val >= R[j].val) {
            ref_res[k] = L[i];
            i++;
        } else {
            ref_res[k] = R[j];
            j++;
        }
        k++;
    }
    while (i < n1) {
        ref_res[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        ref_res[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(sort_t ref_res[], int left, int right, bool is_ascend) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(ref_res, left, mid, is_ascend);
        mergeSort(ref_res, mid + 1, right, is_ascend);
        if (is_ascend) {
            merge_ascend_sort(ref_res, left, mid, right);//merge_ascend_sort
        }else{
            merge_descend_sort(ref_res, left, mid, right);//merge_descend_sort
        }
    }
}

/*
  test_cv_isft
*/
void fft(float* in_real, float* in_imag, float* output_real, float* output_imag, int length, int step, bool forward)
{
    int i;

    if (length == 1) {
        output_real[0] = in_real[0];
        output_imag[0] = in_imag[0];
        return;
    }

    fft(in_real, in_imag, output_real, output_imag, length / 2, 2 * step, forward);
    fft(in_real + step, in_imag + step, output_real + length / 2, output_imag + length / 2, length / 2, 2 * step, forward);

    for (i = 0; i < length / 2; ++i) {
        double angle = forward ? -2 * M_PI * i / length : 2 * M_PI * i / length;
        double wr = cos(angle);
        double wi = sin(angle);

        float tr = wr * output_real[i + length / 2] - wi * output_imag[i + length / 2];
        float ti = wr * output_imag[i + length / 2] + wi * output_real[i + length / 2];

        output_real[i + length / 2] = output_real[i] - tr;
        output_imag[i + length / 2] = output_imag[i] - ti;
        output_real[i] += tr;
        output_imag[i] += ti;
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

void apply_window(float* window, int n, int win_mode)
{
    int i;

    if (win_mode == HANN) {
        for (i = 0; i < n; i++) {
            window[i] *= 0.5 * (1 - cos(2 * M_PI * (float)i / n));
        }
    } else if (win_mode == HAMM) {
        for (i = 0; i < n; i++) {
            window[i] *= 0.54 - 0.46 * cos(2 * M_PI * (float)i / n);
        }
    }
}

void normalize_stft(float* res_XR, float* res_XI, int frm_len, int num)
{
    float norm_fac;
    int i;

    norm_fac = 1.0f / sqrtf((float)frm_len);

    for (i = 0; i < num; ++i) {
        res_XR[i] *= norm_fac;
        res_XI[i] *= norm_fac;
    }
}

int cpu_istft(float* input_R, float* input_I, float* out_R, float* out_I, int L,
                    int batch, int n_fft, bool real_input, int win_mode, int hop_length, bool norm)
{
    int hop, i, j;
    int pad_length = n_fft / 2;
    int pad_signal_len = L + 2 * pad_length;
    int row_num = n_fft / 2 + 1;
    // int hop_length = n_fft / 4;
    int num_frames = 1 + L / hop_length;
    float* input_XR = (float*)malloc(n_fft * sizeof(float));
    float* input_XI = (float*)malloc(n_fft * sizeof(float));
    float* output_XR = (float*)malloc(n_fft * sizeof(float));
    float* output_XI = (float*)malloc(n_fft * sizeof(float));
    float* output_XR_tmp = (float*)malloc(batch * pad_signal_len * sizeof(float));
    float* output_XI_tmp = (float*)malloc(batch * pad_signal_len * sizeof(float));
    float* win_squares = (float*)malloc(n_fft * sizeof(float));
    float* cumul_win_squares = (float*)malloc(pad_signal_len * sizeof(float));
    float* input_r = (float*)malloc(batch * num_frames * row_num * sizeof(float));
    float* input_i = (float*)malloc(batch * num_frames * row_num * sizeof(float));

    memset(cumul_win_squares, 0, pad_signal_len * sizeof(float));
    memset(output_XR_tmp, 0, batch * pad_signal_len * sizeof(float));
    if (!real_input) {
        memset(output_XI_tmp, 0, batch * pad_signal_len * sizeof(float));
    }

    /* Compute the window squares */
    if (win_mode == HANN) {
        for (i = 0; i < n_fft; i++) {
            win_squares[i] = 0.5 * (1 - cos(2 * M_PI * (float)i / n_fft));
            win_squares[i] = win_squares[i] * win_squares[i];
        }
    } else if (win_mode == HAMM) {
        for (i = 0; i < n_fft; i++) {
            win_squares[i] = 0.54 - 0.46 * cos(2 * M_PI * (float)i / n_fft);
            win_squares[i] = win_squares[i] * win_squares[i];
        }
    }

    for (hop = 0; hop < num_frames; ++hop) {
        for (i = 0; i < n_fft; ++i) {
            cumul_win_squares[hop * hop_length + i] += win_squares[i];
        }
    }

    for (j = 0; j < batch; j++) {
        transpose(row_num, num_frames, &input_R[j * row_num * num_frames], &input_r[j * row_num * num_frames]);
        transpose(row_num, num_frames, &input_I[j * row_num * num_frames], &input_i[j * row_num * num_frames]);

        /* calculate window & idft */
        for (hop = 0; hop < num_frames; ++hop) {
            memcpy(input_XR, &input_r[j * row_num * num_frames + hop * row_num], row_num * sizeof(float));
            memcpy(input_XI, &input_i[j * row_num * num_frames + hop * row_num], row_num * sizeof(float));

            /* Expand single-side spectrum to full spectrum */
            for (i = 1; i < row_num - 1; ++i) {
                input_XR[n_fft - i] = input_r[j * row_num * num_frames + hop * row_num + i];
                input_XI[n_fft - i] = -1 * input_i[j * row_num * num_frames + hop * row_num + i];
            }

            fft(input_XR, input_XI, output_XR, output_XI, n_fft, 1, false);

            if (norm) {
                normalize_stft(output_XR, output_XI, n_fft, n_fft);
            } else {
                normalize_stft(output_XR, output_XI, n_fft, n_fft);
                normalize_stft(output_XR, output_XI, n_fft, n_fft);
            }

            apply_window(output_XR, n_fft, win_mode);
            if (!real_input) {
                apply_window(output_XI, n_fft, win_mode);
            }

            for (i = 0; i < n_fft; ++i) {
                if (!real_input) {
                    output_XI_tmp[j * pad_signal_len + hop * hop_length + i] += output_XI[i];
                }
                output_XR_tmp[j * pad_signal_len + hop * hop_length + i] += output_XR[i];
            }
        }

        for (i = 0; i < pad_signal_len; ++i) {
            output_XR_tmp[j * pad_signal_len + i] = output_XR_tmp[j * pad_signal_len + i] / cumul_win_squares[i];
            if (!real_input) {
                output_XI_tmp[j * pad_signal_len + i] = output_XI_tmp[j * pad_signal_len + i] / cumul_win_squares[i];
            }
        }

        memcpy(&out_R[j * L], &output_XR_tmp[j * pad_signal_len + n_fft / 2], L * sizeof(float));
        if (!real_input) {
            memcpy(&out_I[j * L], &output_XI_tmp[j * pad_signal_len + n_fft / 2], L * sizeof(float));
        }
    }

    free(input_XR);
    free(input_XI);
    free(output_XR);
    free(output_XI);
    free(output_XR_tmp);
    free(output_XI_tmp);
    free(win_squares);
    free(cumul_win_squares);
    free(input_r);
    free(input_i);
    return 0;
}

/*
  test_cv_fft
*/
void dft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int length, bool forward)
{
    int i, j;
    double angle;

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

void normalize_fft(float* res_XR, float* res_XI, int frm_len, int num)
{
    float norm_fac;
    int i;

    norm_fac = 1.0f / frm_len;

    for (i = 0; i < num; ++i) {
        res_XR[i] *= norm_fac;
        res_XI[i] *= norm_fac;
    }
}

int cpu_fft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int length, int batch, bool forward)
{
    int i;

    for (i = 0; i < batch; ++i) {
        dft(&(in_real[i * length]), &(in_imag[i * length]), &(output_real[i * length]),
            &(output_imag[i * length]), length, forward);
    }

    if (!forward) {
        normalize_fft(output_real, output_imag, length, batch * length);
    }

    return 0;
}

int cpu_fft_2d(float *in_real, float *in_imag, float *out_real,
                        float *out_imag, int M, int N, bool forward)
{
    float *row_in_real = (float*)malloc(N * sizeof(float));
    float *row_in_imag = (float*)malloc(N * sizeof(float));
    float *row_out_real = (float*)malloc(N * sizeof(float));
    float *row_out_imag = (float*)malloc(N * sizeof(float));
    float *col_in_real = (float*)malloc(M * sizeof(float));
    float *col_in_imag = (float*)malloc(M * sizeof(float));
    float *col_out_real =(float*)malloc(M * sizeof(float));
    float *col_out_imag =(float*)malloc(M * sizeof(float));
    int i, j;

    /* 逐行计算一维 DFT */
    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            row_in_real[j] = in_real[i * N + j];
            row_in_imag[j] = in_imag[i * N + j];
        }
        dft(row_in_real, row_in_imag, row_out_real, row_out_imag, N, forward);
        if (!forward) {
            normalize_fft(row_out_real, row_out_imag, N, N);
        }
        for (j = 0; j < N; j++) {
            out_real[i * N + j] = row_out_real[j];
            out_imag[i * N + j] = row_out_imag[j];
        }
    }

    /* 逐列计算一维 DFT */
    for (j = 0; j < N; j++) {
        for (i = 0; i < M; i++) {
            col_in_real[i] = out_real[i * N + j];
            col_in_imag[i] = out_imag[i * N + j];
        }
        dft(col_in_real, col_in_imag, col_out_real, col_out_imag, M, forward);
        if (!forward) {
            normalize_fft(col_out_real, col_out_imag, M, M);
        }
        for (i = 0; i < M; i++) {
            out_real[i * N + j] = col_out_real[i];
            out_imag[i * N + j] = col_out_imag[i];
        }
    }

    free(row_in_real);
    free(row_in_imag);
    free(row_out_real);
    free(row_out_imag);
    free(col_in_real);
    free(col_in_imag);
    free(col_out_real);
    free(col_out_imag);
    return 0;
}

/*
  test_cv_stft
*/
int cpu_stft(float* XRHost, float* XIHost, float* YRHost_cpu, float* YIHost_cpu, int L,
            int batch, int n_fft, int hop_length, int num_frames, int pad_mode, int win_mode, bool norm)
{
    int win_len = n_fft;
    int hop, i, j;
    int pad_length = n_fft / 2;
    int pad_signal_len = L + 2 * pad_length;
    int row_num = n_fft / 2 + 1;
    float* input_XR = (float*)malloc(pad_signal_len * sizeof(float));
    float* input_XI = (float*)malloc(pad_signal_len * sizeof(float));
    float* window_XR = (float*)malloc(win_len * sizeof(float));
    float* window_XI = (float*)malloc(win_len * sizeof(float));
    float* YR = (float*)malloc(n_fft * sizeof(float));
    float* YI = (float*)malloc(n_fft * sizeof(float));
    float* YR_cpu= (float*)malloc(row_num * num_frames * sizeof(float));
    float* YI_cpu = (float*)malloc(row_num * num_frames * sizeof(float));

    /* pad the input signal */
    memset(input_XR, 0, pad_signal_len * sizeof(float));
    memset(input_XI, 0, pad_signal_len * sizeof(float));

    for (j = 0; j < batch; ++j) {
        if (pad_mode == REFLECT) {
            memcpy(input_XR + pad_length, XRHost + j * L, L * sizeof(float));
            memcpy(input_XI + pad_length, XIHost + j * L, L * sizeof(float));

            for (i = 0; i < pad_length; ++i) {
                input_XR[pad_length - i - 1] = XRHost[j * L + i];
                input_XR[pad_signal_len - pad_length + i] = XRHost[j * L + L - i - 1];
                input_XI[pad_length - i - 1] = XIHost[j * L + i];
                input_XI[pad_signal_len - pad_length + i] = XIHost[j * L + L - i - 1];
            }
        } else if (pad_mode == CONSTANT) {
            memcpy(input_XR + pad_length, XRHost + j * L, L * sizeof(float));
            memcpy(input_XI + pad_length, XIHost + j * L, L * sizeof(float));
        }

        /* calculate window & dft */
        for (hop = 0; hop < num_frames; ++hop) {
            memcpy(window_XR, input_XR + hop * hop_length, win_len * sizeof(float));
            memcpy(window_XI, input_XI + hop * hop_length, win_len * sizeof(float));

            apply_window(window_XR, win_len, win_mode);
            apply_window(window_XI, win_len, win_mode);

            fft(window_XR, window_XI, YR, YI, win_len, 1, true);

            memcpy(&(YR_cpu[hop * row_num]), YR, row_num * sizeof(float));
            memcpy(&(YI_cpu[hop * row_num]), YI, row_num * sizeof(float));
        }

        transpose(num_frames, row_num, YR_cpu, YRHost_cpu + j * row_num * num_frames);
        transpose(num_frames, row_num, YI_cpu, YIHost_cpu + j * row_num * num_frames);
    }

    if (norm) {
        normalize_stft(YRHost_cpu, YIHost_cpu, n_fft, batch * num_frames * row_num);
    }

    free(window_XI);
    free(window_XR);
    free(input_XR);
    free(input_XI);
    free(YR);
    free(YI);
    free(YR_cpu);
    free(YI_cpu);
    return 0;
}