bmcv_image_quantify
====================

将float类型数据转化成int类型(舍入模式为小数点后直接截断)，并将小于0的数变为0，大于255的数变为255。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_image_quantify(
                    bm_handle_t handle,
                    bm_image input,
                    bm_image output);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_image input

  输入参数。输入图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* bm_image output

  输出参数。输出 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。如果不主动分配将在 api 内部进行自行分配。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他:失败


**格式支持：**

该接口目前支持以下 image_format:

+-----+------------------------+------------------------+
| num | input image_format     | output image_format    |
+=====+========================+========================+
| 1   | FORMAT_RGB_PLANAR      | FORMAT_RGB_PLANAR      |
+-----+------------------------+------------------------+
| 2   | FORMAT_BGR_PLANAR      | FORMAT_BGR_PLANAR      |
+-----+------------------------+------------------------+


输入数据目前支持以下 data_type:

+-----+--------------------------------+
| num | data_type                      |
+=====+================================+
| 1   | DATA_TYPE_EXT_FLOAT32          |
+-----+--------------------------------+

输出数据目前支持以下 data_type:

+-----+--------------------------------+
| num | data_type                      |
+=====+================================+
| 1   | DATA_TYPE_EXT_1N_BYTE          |
+-----+--------------------------------+


**注意事项：**

1. 在调用该接口之前必须确保输入的 image 内存已经申请。

2. 如调用该接口的程序为多线程程序，需要在创建bm_image前和销毁bm_image后加线程锁。

3. 该接口支持图像宽高范围为1x1～4096x4096。

4. 该接口可通过设置环境变量启用双核计算，运行程序前：export TPU_CORES=2或export TPU_CORES=both即可。

**代码示例：**

    .. code-block:: c

        #include <stdio.h>
        #include "bmcv_api_ext_c.h"
        #include "stdlib.h"
        #include "string.h"
        #include <assert.h>
        #include <float.h>

        static void read_bin(const char *input_path, float *input_data, int width, int height)
        {
            FILE *fp_src = fopen(input_path, "rb");
            if (fp_src == NULL)
            {
                printf("无法打开输出文件 %s\n", input_path);
                return;
            }
            if(fread(input_data, sizeof(float), width * height, fp_src) != 0)
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
            fwrite(output_data, sizeof(int), width * height, fp_dst);
            fclose(fp_dst);
        }


        int main() {
            int height = 1080;
            int width = 1920;
            char *input_path = "path/to/input";
            char *output_path = "path/to/output";
            bm_handle_t handle;
            bm_status_t ret = bm_dev_request(&handle, 0);

            float* input_data = (float*)malloc(width * height * 3 * sizeof(float));
            unsigned char* output_tpu = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
            read_bin(input_path, input_data, width, height);

            bm_image input_img;
            bm_image output_img;
            bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_RGB_PLANAR, DATA_TYPE_EXT_FLOAT32, &input_img, NULL);
            bm_image_create(handle, height, width, (bm_image_format_ext)FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);
            bm_image_alloc_dev_mem(input_img, 1);
            bm_image_alloc_dev_mem(output_img, 1);
            float* in_ptr[1] = {input_data};

            bm_image_copy_host_to_device(input_img, (void **)in_ptr);
            bmcv_image_quantify(handle, input_img, output_img);
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
