bmcv_image_absdiff
===================

两张大小相同的图片对应像素值相减并取绝对值。


**接口形式：**

.. code-block:: c

    bm_status_t bmcv_image_absdiff(
                bm_handle_t handle,
                bm_image input1,
                bm_image input2,
                bm_image output);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 设备环境句柄，通过调用 bm_dev_request 获取。

* bm_image input1

  输入参数。 输入第一张图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* bm_image input2

  输入参数。 输入第二张图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* bm_image output

  输出参数。 输出 bm_image。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。如果不主动分配将在 api 内部进行自行分配。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**格式支持:**

1. 输入和输出的数据类型

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+

2. 输入和输出的色彩格式

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


**注意事项：**

1. 在调用 bmcv_image_absdiff()之前必须确保输入的 image 内存已经申请。

2. 输入、输出的 data_type，image_format必须相同。


**示例代码**

    .. code-block:: c

        #include "bmcv_api_ext_c.h"
        #include "stdio.h"
        #include "stdlib.h"
        #include "string.h"
        #include <assert.h>

        void readBin(const char * path, unsigned char* input_data, int size)
        {
            FILE *fp_src = fopen(path, "rb");
            if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size){
                printf("file size is less than %d required bytes\n", size);
            };

            fclose(fp_src);
        }

        void writeBin(const char * path, unsigned char* input_data, int size)
        {
            FILE *fp_dst = fopen(path, "wb");
            if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
                printf("file size is less than %d required bytes\n", size);
            };

            fclose(fp_dst);
        }


        int main() {
            int height = 1080;
            int width = 1920;
            int format = 8;
            char *src1_name = "path/to/src1";
            char *src2_name = "path/to/src2";
            char *dst_name = "path/to/dst";
            bm_handle_t handle;
            bm_status_t ret = bm_dev_request(&handle, 0);
            if (ret != BM_SUCCESS) {
                printf("Create bm handle failed. ret = %d\n", ret);
                return -1;
            }
            int img_size = width * height * 3;
            unsigned char* input1_data = malloc(width * height * 3);
            unsigned char* input2_data = malloc(width * height * 3);
            unsigned char* output_tpu = malloc(width * height * 3);
            readBin(src1_name, input1_data, img_size);
            readBin(src2_name, input2_data, img_size);
            memset(output_tpu, 0, width * height * 3);
            bm_image input1_img;
            bm_image input2_img;
            bm_image output_img;

            bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input1_img, NULL);
            bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input2_img, NULL);
            bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
            bm_image_alloc_dev_mem(input1_img, 2);
            bm_image_alloc_dev_mem(input2_img, 2);
            bm_image_alloc_dev_mem(output_img, 2);
            unsigned char *in1_ptr[3] = {input1_data, input1_data + height * width, input1_data + 2 * height * width};
            unsigned char *in2_ptr[3] = {input2_data, input2_data + width * height, input2_data + 2 * height * width};
            bm_image_copy_host_to_device(input1_img, (void **)(in1_ptr));
            bm_image_copy_host_to_device(input2_img, (void **)(in2_ptr));

            ret = bmcv_image_absdiff(handle, input1_img, input2_img, output_img);
            if (BM_SUCCESS != ret) {
                bm_image_destroy(&input1_img);
                bm_image_destroy(&input2_img);
                bm_image_destroy(&output_img);
                return -1;
            }
            unsigned char *out_ptr[3] = {output_tpu, output_tpu + height * width, output_tpu + 2 * height * width};
            bm_image_copy_device_to_host(output_img, (void **)out_ptr);

            bm_image_destroy(&input1_img);
            bm_image_destroy(&input2_img);
            bm_image_destroy(&output_img);

            writeBin(dst_name, output_tpu, img_size);
            free(input1_data);
            free(input2_data);
            free(output_tpu);
            bm_dev_free(handle);
            return ret;
        }
