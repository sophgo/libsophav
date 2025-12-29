bmcv_image_laplacian
====================

梯度计算laplacian算子。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_image_laplacian(
                    bm_handle_t handle,
                    bm_image input,
                    bm_image output,
                    unsigned int ksize);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_image input

  输入参数。输入图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* bm_image output

  输出参数。输出 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。如果不主动分配将在 api 内部进行自行分配。

* int ksize = 1 / 3

  Laplacian核的大小，必须是1或3。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**格式支持：**

1. 该接口目前支持以下 image_format:

+-----+------------------------+------------------------+
| num | input image_format     | output image_format    |
+=====+========================+========================+
| 1   | FORMAT_GRAY            | FORMAT_GRAY            |
+-----+------------------------+------------------------+

2. 目前支持以下 data_type:

+-----+--------------------------------+
| num | data_type                      |
+=====+================================+
| 1   | DATA_TYPE_EXT_1N_BYTE          |
+-----+--------------------------------+


**注意事项：**

1. 在调用该接口之前必须确保输入的 image 内存已经申请。

2. input output 的 data_type必须相同。

3. 目前支持图像宽高范围为2x2-4096x4096。



**示例代码**

    .. code-block:: c

        #include "bmcv_api_ext_c.h"
        #include <stdio.h>
        #include <stdlib.h>
        #include <math.h>
        #include <string.h>


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
            if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
                printf("file size is less than %d required bytes\n", size);
            };

            fclose(fp_dst);
        }

        int main()
        {
            int height = 1080;
            int width = 1920;
            unsigned int ksize = 3;
            bm_image_format_ext fmt = FORMAT_GRAY; /* 14 */
            int ret = 0;
            char* src_name = "path/to/src";
            char* dst_name = "path/to/dst";
            bm_handle_t handle;
            ret = bm_dev_request(&handle, 0);

            unsigned char* input = (unsigned char*)malloc(width * height * sizeof(unsigned char));
            unsigned char* tpu_out = (unsigned char*)malloc(width * height * sizeof(unsigned char));
            unsigned char* cpu_out = (unsigned char*)malloc(width * height * sizeof(unsigned char));
            int len;

            len = width * height;
            readBin(src_name, input, len);
            memset(tpu_out, 0, len * sizeof(unsigned char));


            bm_image_data_format_ext data_type = DATA_TYPE_EXT_1N_BYTE;
            bm_image input_img, output_img;

            ret = bm_image_create(handle, height, width, fmt, data_type, &input_img, NULL);
            ret = bm_image_create(handle, height, width, fmt, data_type, &output_img, NULL);
            ret = bm_image_alloc_dev_mem(input_img, 2);
            ret = bm_image_alloc_dev_mem(output_img, 2);

            ret = bm_image_copy_host_to_device(input_img, (void **)(&input));

            ret = bmcv_image_laplacian(handle, input_img, output_img, ksize);

            ret = bm_image_copy_device_to_host(output_img, (void **)(&tpu_out));

            bm_image_destroy(&input_img);
            bm_image_destroy(&output_img);

            writeBin(dst_name, tpu_out, len);
            free(input);
            free(tpu_out);
            free(cpu_out);

            bm_dev_free(handle);
            return ret;
        }