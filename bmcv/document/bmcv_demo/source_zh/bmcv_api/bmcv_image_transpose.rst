bmcv_image_transpose
====================

该接口可以实现图片宽和高的转置。


**接口形式:**

    .. code-block:: c

        bm_status_t bmcv_image_transpose(
                    bm_handle_t handle,
                    bm_image input,
                    bm_image output);


**参数说明:**

* bm_handle_t handle

  输入参数。设备环境句柄，通过调用 bm_dev_request 获取。

* bm_image input

  输入参数。输入图像的 bm_image 结构体。

* bm_image output

  输出参数。输出图像的 bm_image 结构体。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

        #include "bmcv_api_ext_c.h"
        #include <math.h>
        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>
        #include <sys/time.h>
        #include <time.h>
        #include <pthread.h>

        #define DTYPE_F32 4
        #define DTYPE_U8 1
        #define ALIGN(a, b) (((a) + (b) - 1) / (b) * (b))
        #define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))



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
            FILE *fp_dst = fopen(path, "wb+");
            if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size) {
                printf("file size is less than %d required bytes\n", size);
            };

            fclose(fp_dst);
        }


        int main()
        {
            int channel = 1;
            int height = 1080;
            int width = 1920;
            int stride = width;
            int dtype = 1;
            int ret = 0;
            char* src_name = "path/to/src";
            char* dst_name = "path/to/dst";
            bm_handle_t handle;
            ret = bm_dev_request(&handle, 0);
            unsigned char* input = (unsigned char*) malloc(channel * height * stride * sizeof(unsigned char));
            unsigned char* output = (unsigned char*) malloc(channel * height * width * sizeof(unsigned char));
            readBin(src_name, input, channel * height * stride);
            memset(output, 0, channel * height * width);

            int format;
            int type = 0;
            int stride_byte;
            bm_image input_img, output_img;

            format = (channel == 3) ? FORMAT_BGR_PLANAR : FORMAT_GRAY;

            if (dtype == DTYPE_F32) {
                type = DATA_TYPE_EXT_FLOAT32;
                stride_byte = stride * sizeof(float);
            } else if (dtype == DTYPE_U8) {
                type = DATA_TYPE_EXT_1N_BYTE;
                stride_byte = stride * sizeof(unsigned char);
            }

            ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, (bm_image_data_format_ext)type, &input_img, &stride_byte);
            ret = bm_image_alloc_dev_mem(input_img, 2);
            ret = bm_image_copy_host_to_device(input_img, (void**)&input);
            ret = bm_image_create(handle, width, height, (bm_image_format_ext)format, (bm_image_data_format_ext)type, &output_img, NULL);

            ret = bm_image_alloc_dev_mem(output_img, 2);

            ret = bmcv_image_transpose(handle, input_img, output_img);
            ret = bm_image_copy_device_to_host(output_img, (void**)&output);

            bm_image_destroy(&input_img);
            bm_image_destroy(&output_img);

            writeBin(dst_name, output, channel * height * width);

            free(input);
            free(output);

            bm_dev_free(handle);
            return ret;
        }


**注意事项:**

1. 该 API 要求输入和输出的 bm_image 图像格式相同，支持以下格式：

+-----+-------------------------------+
| num | image_format                  |
+=====+===============================+
|  1  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  2  | FORMAT_BGR_PLANAR             |
+-----+-------------------------------+
|  3  | FORMAT_GRAY                   |
+-----+-------------------------------+

2. 该 API 要求输入和输出的 bm_image 数据类型相同，支持以下类型：

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_FLOAT32         |
+-----+-------------------------------+
|  2  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+
|  3  | DATA_TYPE_EXT_4N_BYTE         |
+-----+-------------------------------+
|  4  | DATA_TYPE_EXT_1N_BYTE_SIGNED  |
+-----+-------------------------------+
|  5  | DATA_TYPE_EXT_4N_BYTE_SIGNED  |
+-----+-------------------------------+

3. 输出图像的 width 必须等于输入图像的 height，输出图像的 height 必须等于输入图像的 width ;

4. 输入图像支持带有 stride;

5. 输入输出 bm_image 结构必须提前创建，否则返回失败。

6. 输入 bm_image 必须 attach device memory，否则返回失败

7. 如果输出对象未 attach device memory，则会内部调用 bm_image_alloc_dev_mem 申请内部管理的 device memory，并将转置后的数据填充到 device memory 中。