bmcv_image_gaussian_blur
------------------------------

**描述：**

该接口用于对图像进行高斯滤波操作。

**语法：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_gaussian_blur(
        bm_handle_t handle,
        bm_image input,
        bm_image output,
        int kw,
        int kh,
        float sigmaX,
        float sigmaY = 0);

**参数：**

.. list-table:: bmcv_image_gaussian_blur 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用bm_dev_request获取。
    * - input
      - 输入
      - 输入图像的bm_image，bm_image需要外部调用bmcv_image_create创建。image内存可以使用bm_image_alloc_dev_mem或者bm_image_copy_host_to_device来开辟新的内存，或者使用bmcv_image_attach来attach已有的内存。
    * - output
      - 输出
      - 输出图像的bm_image，bm_image需要外部调用bmcv_image_create创建。image内存可以通过bm_image_alloc_dev_mem来开辟新的内存，或者使用bmcv_image_attach来attach已有的内存。如果不主动分配将在api内部进行自行分配。
    * - kw
      - 输入
      - kernel宽的大小。
    * - kh
      - 输入
      - kernel高的大小。
    * - sigmaX
      - 输入
      - X方向上的高斯核标准差，取值范围为0-4.0。
    * - sigmaY = 0
      - 输入
      - Y方向上的高斯核标准差，取值范围为0-4.0。如果为0则表示与X方向上的高斯核标准差相同。

**返回值：**

该函数成功调用时, 返回BM_SUCCESS。

**格式支持：**

该接口目前支持以下图像格式:

+-----+------------------------+
| num | image_format           |
+=====+========================+
| 1   | FORMAT_BGR_PLANAR      |
+-----+------------------------+
| 2   | FORMAT_RGB_PLANAR      |
+-----+------------------------+
| 3   | FORMAT_RGBP_SEPARATE   |
+-----+------------------------+
| 4   | FORMAT_BGRP_SEPARATE   |
+-----+------------------------+
| 5   | FORMAT_GRAY            |
+-----+------------------------+
| 6   | FORMAT_YUV420P         |
+-----+------------------------+
| 7   | FORMAT_YUV422P         |
+-----+------------------------+
| 8   | FORMAT_YUV444P         |
+-----+------------------------+

该接口目前支持的数据格式：

+-----+------------------------+
| num | data_type              |
+=====+========================+
| 1   | DATA_TYPE_EXT_1N_BYTE  |
+-----+------------------------+

**注意事项：**

1. 在调用该接口之前必须确保输入图像的内存已经申请。

2. 输入输出图像的数据格式，图像格式必须相同。

3. 目前支持图像的最大宽为4096，最大高为8192。

4. 目前卷积核支持的大小有3*3, 5*5， 7*7,当卷积核大小为3时，支持的宽高范围为8*8～4096*8192，核大小为5时支持的宽高范围为8*8～2048*8192，核大小为7时支持的宽高范围为8*8～1500*8192。

**代码示例：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    #include <stdio.h>
    #include "bmcv_api_ext_c.h"
    #include "stdlib.h"
    #include "string.h"
    #include <assert.h>
    #include <float.h>
    #include <math.h>

    static void read_bin(const char *input_path, unsigned char *input_data, int width, int height) {
        FILE *fp_src = fopen(input_path, "rb");
        if (fp_src == NULL) {
        printf("无法打开输出文件 %s\n", input_path);
        return;
        }
        if(fread(input_data, sizeof(char), width * height, fp_src) != 0) {
        printf("read image success\n");
        }
        fclose(fp_src);
    }

    static void write_bin(const char *output_path, unsigned char *output_data, int width, int height) {
        FILE *fp_dst = fopen(output_path, "wb");
        if (fp_dst == NULL) {
        printf("无法打开输出文件 %s\n", output_path);
        return;
        }
        fwrite(output_data, sizeof(char), width * height, fp_dst);
        fclose(fp_dst);
    }




    int main() {
        int width = 1920;
        int height = 1080;
        int format = FORMAT_GRAY;
        float sigmaX = (float)rand() / RAND_MAX * 5.0f;
        float sigmaY = (float)rand() / RAND_MAX * 5.0f;
        int ret = 0;
        char *input_path = "path/to/input";
        char *output_path = "path/to/output";
        bm_handle_t handle;
        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("bm_dev_request failed. ret = %d\n", ret);
            return -1;
        }

        int kw = 3, kh = 3;

        unsigned char *input_data = (unsigned char*)malloc(width * height);
        unsigned char *output_tpu = (unsigned char*)malloc(width * height);

        read_bin(input_path, input_data, width, height);

        bm_image img_i;
        bm_image img_o;

        bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &img_i, NULL);
        bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &img_o, NULL);
        bm_image_alloc_dev_mem(img_i, 2);
        bm_image_alloc_dev_mem(img_o, 2);

        unsigned char *input_addr[3] = {input_data, input_data + height * width, input_data + 2 * height * width};
        bm_image_copy_host_to_device(img_i, (void **)(input_addr));

        ret = bmcv_image_gaussian_blur(handle, img_i, img_o, kw, kh, sigmaX, sigmaY);
        unsigned char *output_addr[3] = {output_tpu, output_tpu + height * width, output_tpu + 2 * height * width};
        bm_image_copy_device_to_host(img_o, (void **)output_addr);

        bm_image_destroy(&img_i);
        bm_image_destroy(&img_o);


        write_bin(output_path, output_tpu, width, height);
        free(input_data);
        free(output_tpu);

        bm_dev_free(handle);
        return ret;
    }