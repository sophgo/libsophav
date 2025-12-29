bmcv_image_pyramid_down
=======================

该接口实现图像高斯金字塔操作中的向下采样。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_image_pyramid_down(
                    bm_handle_t handle,
                    bm_image input,
                    bm_image output);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_image input

  输入参数。输入图像 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。bm_image 的内存可以使用 bm_image_alloc_dev_mem 或者bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach
  来 attach 已有的内存。

* bm_image output

  输出参数。输出图像 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。bm_image的内存可以使用 bm_image_alloc_dev_mem 或者bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach
  来 attach 已有的内存。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**格式支持：**

该接口目前支持以下 image_format 与 data_type:

+-----+------------------------+------------------------+
| num | image_format           | data_type              |
+=====+========================+========================+
| 1   | FORMAT_GRAY            | DATA_TYPE_EXT_1N_BYTE  |
+-----+------------------------+------------------------+


**注意事项：**

1. 目前支持图像的最小宽为3，最小高为3，最大宽为2043，最大高为4096。


**示例代码**

    .. code-block:: c

        #include <assert.h>
        #include "bmcv_api_ext_c.h"
        #include <math.h>
        #include <stdio.h>
        #include <stdlib.h>
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
            if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size) {
                printf("file size is less than %d required bytes\n", size);
            };

            fclose(fp_dst);
        }

        int main()
        {
            int height = 1080;
            int width = 1920;
            int ret = 0;
            char* src_name = "path/to/src";
            char* dst_name = "path/to/dst";
            bm_handle_t handle;
            ret = bm_dev_request(&handle, 0);

            int ow = width / 2;
            int oh = height / 2;
            int channel = 1;
            unsigned char* input_data = (unsigned char*)malloc(width * height * channel * sizeof(unsigned char));
            unsigned char* output_tpu = (unsigned char*)malloc(ow * oh * channel * sizeof(unsigned char));
            unsigned char* output_cpu = (unsigned char*)malloc(ow * oh * channel * sizeof(unsigned char));
            readBin(src_name, input_data, width * height * channel);
            memset(output_tpu, 0, ow * oh * channel * sizeof(unsigned char));

            bm_image_format_ext fmt = FORMAT_GRAY;
            bm_image img_i;
            bm_image img_o;

            ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &img_i, NULL);
            ret = bm_image_create(handle, oh, ow, fmt, DATA_TYPE_EXT_1N_BYTE, &img_o, NULL);

            ret = bm_image_alloc_dev_mem(img_i, BMCV_HEAP1_ID);
            ret = bm_image_alloc_dev_mem(img_o, BMCV_HEAP1_ID);

            ret = bm_image_copy_host_to_device(img_i, (void **)(&input_data));
            ret = bmcv_image_pyramid_down(handle, img_i, img_o);
            ret = bm_image_copy_device_to_host(img_o, (void **)(&output_tpu));

            bm_image_destroy(&img_i);
            bm_image_destroy(&img_o);

            writeBin(dst_name, output_tpu, ow * oh * channel);

            free(input_data);
            free(output_tpu);
            free(output_cpu);


            bm_dev_free(handle);
            return ret;
        }

