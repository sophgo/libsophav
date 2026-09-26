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
      - X方向上的高斯核标准差，取值范围为0-5.0。
    * - sigmaY = 0
      - 输入
      - Y方向上的高斯核标准差，取值范围为0-5.0。如果为0则表示与X方向上的高斯核标准差相同。

**返回值：**

该函数成功调用时, 返回BM_SUCCESS。

**格式支持：**

该接口目前支持以下图像格式:

+-------------------+------------------------+
| Enumeration value | image_format           |
+===================+========================+
| 2                 | FORMAT_YUV444P         |
+-------------------+------------------------+
| 3                 | FORMAT_NV12            |
+-------------------+------------------------+
| 4                 | FORMAT_NV21            |
+-------------------+------------------------+
| 5                 | FORMAT_NV16            |
+-------------------+------------------------+
| 6                 | FORMAT_NV61            |
+-------------------+------------------------+
| 8                 | FORMAT_RGB_PLANAR      |
+-------------------+------------------------+
| 9                 | FORMAT_BGR_PLANAR      |
+-------------------+------------------------+
| 10                | FORMAT_RGB_PACKED      |
+-------------------+------------------------+
| 11                | FORMAT_BGR_PACKED      |
+-------------------+------------------------+
| 12                | FORMAT_RGBP_SEPARATE   |
+-------------------+------------------------+
| 13                | FORMAT_BGRP_SEPARATE   |
+-------------------+------------------------+
| 14                | FORMAT_GRAY            |
+-------------------+------------------------+

该接口目前支持的数据格式：

+-------------------+------------------------+
| Enumeration value | data_type              |
+===================+========================+
| 1                 | DATA_TYPE_EXT_1N_BYTE  |
+-------------------+------------------------+

**注意事项：**

1. 在调用该接口之前必须确保输入图像的内存已经申请。

2. 输入输出图像的数据格式，图像格式必须相同。

3. 目前卷积核支持的大小有3*3、5*5、7*7、9*9，支持的宽高范围均为8*8～8192*8192。

4. 本算子所有图像格式均支持双核模式处理，但当图像格式为 NV12/NV21/NV16/NV61 且环境变量 ``TPU_CORES`` 设置为双核模式（``2`` 或 ``both``）时，受双核拆分实现限制，最大支持尺寸为 1920×1080；超出该尺寸将返回 ``BM_NOT_SUPPORTED``，如需处理更大尺寸请使用单核模式（``TPU_CORES=0`` 或 ``1``）。

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