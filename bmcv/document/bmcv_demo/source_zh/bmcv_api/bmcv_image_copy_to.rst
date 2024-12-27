bmcv_image_copy_to
------------------

**描述：**

该接口实现将一幅图像拷贝到目的图像的对应内存区域。

**语法：**

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_copy_to(
            bm_handle_t handle,
            bmcv_copy_to_atrr_t copy_to_attr,
            bm_image            input,
            bm_image            output
    );

**参数：**

.. list-table:: bmcv_image_copy_to 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - copy_to_attr
      - 输入
      - api 所对应的属性配置。
    * - input
      - 输入
      - 输入 bm_image。
    * - output
      - 输出
      - 输出 bm_image。

**返回值：**

该函数成功调用时, 返回 BM_SUCCESS。

**数据类型说明：**

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_copy_to_atrr_s {
        int           start_x;
        int           start_y;
        unsigned char padding_r;
        unsigned char padding_g;
        unsigned char padding_b;
        int if_padding;
    } bmcv_copy_to_atrr_t;

* padding_b 表示当 input 的图像要小于输出图像的情况下，多出来的图像 b 通道上被填充的值。
* padding_r 表示当 input 的图像要小于输出图像的情况下，多出来的图像 r 通道上被填充的值。
* padding_g 表示当 input 的图像要小于输出图像的情况下，多出来的图像 g 通道上被填充的值。
* start_x 描述了 copy_to 拷贝到输出图像所在的起始横坐标。
* start_y 描述了 copy_to 拷贝到输出图像所在的起始纵坐标。
* if_padding 表示当 input 的图像要小于输出图像的情况下，是否需要对多余的图像区域填充特定颜色，0表示不需要，1表示需要。当该值填0时，padding_r，padding_g，padding_b 的设置将无效

**格式支持：**

1. 输入和输出的数据类型

+-----+-------------------------------+-------------------------------+
| num | input data type               | output data type              |
+=====+===============================+===============================+
|  1  |                               | DATA_TYPE_EXT_FLOAT32         |
+-----+                               +-------------------------------+
|  2  |                               | DATA_TYPE_EXT_1N_BYTE         |
+-----+                               +-------------------------------+
|  3  | DATA_TYPE_EXT_1N_BYTE         | DATA_TYPE_EXT_1N_BYTE_SIGNED  |
+-----+                               +-------------------------------+
|  4  |                               | DATA_TYPE_EXT_FP16            |
+-----+                               +-------------------------------+
|  5  |                               | DATA_TYPE_EXT_BF16            |
+-----+-------------------------------+-------------------------------+
|  6  | DATA_TYPE_EXT_FLOAT32         | DATA_TYPE_EXT_FLOAT32         |
+-----+-------------------------------+-------------------------------+
|  7  | DATA_TYPE_EXT_1N_BYTE_SIGNED  | DATA_TYPE_EXT_1N_BYTE_SIGNED  |
+-----+-------------------------------+-------------------------------+

2. 输入和输出支持的色彩格式

+-----+-------------------------------+
| num | image_format                  |
+=====+===============================+
|  1  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  2  | FORMAT_BGR_PLANAR             |
+-----+-------------------------------+
|  3  | FORMAT_RGB_PACKED             |
+-----+-------------------------------+
|  4  | FORMAT_BGR_PACKED             |
+-----+-------------------------------+
|  5  | FORMAT_GRAY                   |
+-----+-------------------------------+

**注意：**

1. 在调用 bmcv_image_copy_to()之前必须确保输入的 image 内存已经申请。
#. 为了避免内存越界，输入图像width + start_x 必须小于等于输出图像width stride。

**代码示例：**

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <assert.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "bmcv_api_ext_c.h"

    typedef enum { COPY_TO_GRAY = 0, COPY_TO_BGR, COPY_TO_RGB } padding_corlor_e;
    typedef enum { PLANNER = 0, PACKED } padding_format_e;

    static int writeBin(const char* path, void* output_data, int size)
    {
        int len = 0;
        FILE* fp_dst = fopen(path, "wb+");

        if (fp_dst == NULL) {
            perror("Error opening file\n");
            return -1;
        }

        len = fwrite((void*)output_data, 1, size, fp_dst);
        if (len < size) {
            printf("file size = %d is less than required bytes = %d\n", len, size);
            return -1;
        }

        fclose(fp_dst);
        return 0;
    }


    int main() {
        int         dev_id = 0;
        bm_handle_t handle;
        bm_status_t ret = bm_dev_request(&handle, dev_id);

        int in_w    = 400;
        int in_h    = 400;
        int out_w   = 800;
        int out_h   = 800;
        int start_x = 200;
        int start_y = 200;

        int image_format = FORMAT_RGB_PLANAR;
        int data_type = DATA_TYPE_EXT_FLOAT32;
        int channel   = 3;

        int image_n = 1;

        float* src_data = (float *)malloc(image_n * channel * in_w * in_h * sizeof(float));
        float* res_data = (float *)malloc(image_n * channel * out_w * out_h * sizeof(float));

        for (int i = 0; i < image_n * channel * in_w * in_h; i++) {
            src_data[i] = rand() % 255;
        }
        // calculate res
        bmcv_copy_to_atrr_t copy_to_attr;
        copy_to_attr.start_x    = start_x;
        copy_to_attr.start_y    = start_y;
        copy_to_attr.padding_r  = 0;
        copy_to_attr.padding_g  = 0;
        copy_to_attr.padding_b  = 0;
        copy_to_attr.if_padding = 1;
        bm_image input, output;
        bm_image_create(handle,
                        in_h,
                        in_w,
                        (bm_image_format_ext)image_format,
                        (bm_image_data_format_ext)data_type,
                        &input, NULL);
        bm_image_alloc_dev_mem(input, BMCV_HEAP1_ID);
        bm_image_copy_host_to_device(input, (void **)&src_data);
        bm_image_create(handle,
                        out_h,
                        out_w,
                        (bm_image_format_ext)image_format,
                        (bm_image_data_format_ext)data_type,
                        &output, NULL);
        bm_image_alloc_dev_mem(output, BMCV_HEAP1_ID);
        ret = bmcv_image_copy_to(handle, copy_to_attr, input, output);

        if (BM_SUCCESS != ret) {
            printf("bmcv_copy_to error 1 !!!\n");
            bm_image_destroy(&input);
            bm_image_destroy(&output);
            free(src_data);
            free(res_data);
            return -1;
        }
        bm_image_copy_device_to_host(output, (void **)&res_data);

        char *dst_name = "path/to/dst";
        writeBin(dst_name, res_data, out_w * out_h * 3);
        writeBin("path/to/src", src_data, in_h * in_w * 3);

        bm_image_destroy(&input);
        bm_image_destroy(&output);
        free(src_data);
        free(res_data);

        return ret;
    }
