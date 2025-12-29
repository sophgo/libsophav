bmcv_image_threshold
------------------------------

**描述：**

该接口用于对图像进行像素阈值化操作。

**语法：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_threshold(
        bm_handle_t handle,
        bm_image input,
        bm_image output,
        unsigned char thresh,
        unsigned char max_value,
        bm_thresh_type_t type);

其中thresh类型如下：

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum {
        BM_THRESH_BINARY = 0,
        BM_THRESH_BINARY_INV,
        BM_THRESH_TRUNC,
        BM_THRESH_TOZERO,
        BM_THRESH_TOZERO_INV,
        BM_THRESH_TYPE_MAX
    } bm_thresh_type_t;

各个类型对应的具体公式如下：

.. image:: ../_static/thresh_type.jpg
   :width: 899px
   :height: 454px
   :align: center
   :alt: 图片加载失败

**参数：**

.. list-table:: bmcv_image_threshold 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用bm_dev_request获取。
    * - input
      - 输入
      - 输入参数。输入图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。
    * - output
      - 输出
      - 输出参数。输出 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。如果不主动分配将在 api 内部进行自行分配。
    * - thresh
      - 输入
      - 像素阈值，取值范围为0-255。
    * - max_value
      - 输入
      - 阈值化操作后的像素最大值，取值范围为0-255。
    * - type
      - 输入
      - 阈值化类型，取值范围为0-4。

**返回值：**

该函数成功调用时, 返回BM_SUCCESS。

**格式支持：**

该接口目前支持以下图像格式:

+-----+------------------------+
| num | image_format           |
+=====+========================+
| 1   | FORMAT_BGR_PACKED      |
+-----+------------------------+
| 2   | FORMAT_BGR_PLANAR      |
+-----+------------------------+
| 3   | FORMAT_RGB_PACKED      |
+-----+------------------------+
| 4   | FORMAT_RGB_PLANAR      |
+-----+------------------------+
| 5   | FORMAT_RGBP_SEPARATE   |
+-----+------------------------+
| 6   | FORMAT_BGRP_SEPARATE   |
+-----+------------------------+
| 7   | FORMAT_GRAY            |
+-----+------------------------+
| 8   | FORMAT_YUV420P         |
+-----+------------------------+
| 9   | FORMAT_YUV422P         |
+-----+------------------------+
| 10  | FORMAT_YUV444P         |
+-----+------------------------+
| 11  | FORMAT_NV12            |
+-----+------------------------+
| 12  | FORMAT_NV21            |
+-----+------------------------+
| 13  | FORMAT_NV16            |
+-----+------------------------+
| 14  | FORMAT_NV61            |
+-----+------------------------+
| 15  | FORMAT_NV24            |
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

3. 目前支持的图像最大宽和高为4096。

**代码示例：**

.. code-block:: c
    :linenos:
    :lineno-start: 1

    #include <stdio.h>
    #include "bmcv_api_ext_c.h"
    #include "test_misc.h"
    #include "stdlib.h"
    #include "string.h"
    #include <assert.h>
    #include <float.h>

    static void read_bin(const char *input_path, unsigned char *input_data, int width, int height)
    {
        FILE *fp_src = fopen(input_path, "rb");
        if (fp_src == NULL)
        {
            printf("无法打开输出文件 %s\n", input_path);
            return;
        }
        if(fread(input_data, sizeof(char), width * height, fp_src) != 0)
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
        fwrite(output_data, sizeof(char), width * height, fp_dst);
        fclose(fp_dst);
    }

    int main() {
        int height = 1080;
        int width = 1920;
        int type = rand() % 5;
        char *input_path = "path/to/input";
        char *output_path = "path/to/output";
        bm_handle_t handle;
        bm_status_t ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("Create bm handle failed. ret = %d\n", ret);
            return -1;
        }
        int channel = 1;

        unsigned char threshold = 50;
        unsigned char max_value = 228;
        printf("type: %d\n", type);
        printf("threshold: %d , max_value: %d\n", threshold, max_value);
        printf("width: %d , height: %d , channel: %d \n", width, height, channel);

        unsigned char* input_data = (unsigned char*)malloc(width * height);
        unsigned char* output_tpu = (unsigned char*)malloc(width * height);
        read_bin(input_path, input_data, width, height);

        bm_image input_img;
        bm_image output_img;

        bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
        bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
        bm_image_alloc_dev_mem(input_img, 1);
        bm_image_alloc_dev_mem(output_img, 1);

        unsigned char* in_ptr[1] = {input_data};
        bm_image_copy_host_to_device(input_img, (void **)in_ptr);
        bmcv_image_threshold(handle, input_img, output_img, threshold, max_value, (bm_thresh_type_t)type);
        unsigned char* out_ptr[1] = {output_tpu};
        bm_image_copy_device_to_host(output_img, (void **)out_ptr);

        bm_image_destroy(&input_img);
        bm_image_destroy(&output_img);

        write_bin(output_path, output_tpu, width, height);

        free(input_data);
        free(output_tpu);
        bm_dev_free(handle);
        return ret;
    }
