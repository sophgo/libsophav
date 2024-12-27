bmcv_image_jpeg_dec
-------------------

【描述】

该接口可以实现对多张图片的 JPEG 解码过程。

【语法】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_jpeg_dec(
            bm_handle_t handle,
            void *      p_jpeg_data[],
            size_t *    in_size,
            int         image_num,
            bm_image *  dst
    );


【参数】

.. list-table:: bmcv_image_jpeg_dec 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*p_jpeg_data[]
      - 输入
      - 待解码的图片数据指针，由于该接口支持对多张图片的解码，因此为指针数组。
    * - \*in_size
      - 输入
      - 待解码各张图片的大小（以 byte 为单位）存放在该指针中，也就是上述 p_jpeg_data 每一维指针所指向空间的大小。
    * - image_num
      - 输入
      - 输入图片数量，最多支持 4。
    * - \*dst
      - 输出
      - 输出 bm_image的指针。每个 dst bm_image 用户可以选择自行调用 bm_image_create 创建，也可以选择不创建。如果用户只声明而不创建则由接口内部根据待解码图片信息自动创建，默认的 format 如表 :ref:`default_format` 所示, 当不再需要时仍然需要用户调用 bm_image_destory 来销毁。

.. _default_format:
.. list-table:: default_format
    :header-rows: 1
    :widths: auto

    * - 码流
      - 默认输出format
    * - YUV420
      - FORMAT_YUV420P
    * - YUV422
      - FORMAT_YUV422P
    * - YUV444
      - FORMAT_YUV444P
    * - YUV400
      - FORMAT_GRAY

【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 如果用户没有使用bmcv_image_create创建dst的bm_image，那么需要将参数传入指针所指向的空间置0。

2. 目前解码支持的图片格式及其输出格式对应如下，如果用户需要指定以下某一种输出格式，可通过使用 bmcv_image_create 自行创建 dst bm_image，从而实现将图片解码到以下对应的某一格式。

+------------------+------------------+
|     码 流        |   输 出 format   |
+==================+==================+
|                  |  FORMAT_YUV420P  |
+  YUV420          +------------------+
|                  |  FORMAT_NV12     |
+                  +------------------+
|                  |  FORMAT_NV21     |
+------------------+------------------+
|                  |  FORMAT_YUV422P  |
+  YUV422          +------------------+
|                  |  FORMAT_NV16     |
+                  +------------------+
|                  |  FORMAT_NV61     |
+------------------+------------------+
|  YUV444          |  FORMAT_YUV444P  |
+------------------+------------------+
|  YUV400          |  FORMAT_GRAY     |
+------------------+------------------+

【代码示例】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <memory.h>
    #include "bmcv_api_ext_c.h"
    #include <assert.h>
    #include <math.h>

    #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

    int main() {
        int dev_id = -1;
        int dev_cnt;
        int ret = 0;
        bm_dev_getcount(&dev_cnt);
        if (dev_id >= dev_cnt) {
            printf("[TEST JPEG] dev_id should less than device count, only detect %d devices\n", dev_cnt);
            exit(-1);
        }
        printf("device count = %d\n", dev_cnt);
        bm_handle_t handle[dev_cnt];

        for (int i = 0; i < dev_cnt; i++) {
            int id;
            if (dev_id != -1) {
                dev_cnt = 1;
                id = dev_id;
            } else {
                id = i;
            }
            bm_status_t req = bm_dev_request(handle + i, id);
            if (req != BM_SUCCESS) {
                printf("create bm handle for dev%d failed!\n", id);
                exit(-1);
            }
        }
        for (int j = 0; j < dev_cnt; j++) {
            char *dst_name = "path/to/dst";
            char *src_name = "path/to/src";
            int format = FORMAT_YUV420P;
            int image_n = 1;
            int image_h = 1080;
            int image_w = 1920;
            int* stride = (int*)malloc(3 * sizeof(int));
            stride[0] = align_up(image_w, 16);
            stride[1] = align_up(image_w / 2, 16);
            stride[2] = align_up(image_w / 2, 16);


            bm_image src[4];
            for (int i = 0; i < image_n; i++) {
                bm_image_create(handle[j], image_h, image_w, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, src + i, stride);
                bm_image_alloc_dev_mem(src[i], BMCV_HEAP1_ID);
            }
            int image_byte_size[4] = {0};
            bm_image_get_byte_size(src[0], image_byte_size);
            for (int i = 0; i < 4; i++) {
                printf("src_%d_byte_size: %d\n", i, image_byte_size[i]);
            }
            int byte_size = image_w * image_h * 3 / 2;
            unsigned char *input_data = (unsigned char *)malloc(byte_size);
            FILE *fp_src = fopen(src_name, "rb");
            if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
            printf("file size is less than required bytes%d\n", byte_size);
            };
            fclose(fp_src);
            void* in_ptr[3] = {(void *)input_data, (void *)((unsigned char*)input_data + image_w * image_h), (void *)((unsigned char*)input_data + 5 / 4 * image_w * image_h)};
            bm_image_copy_host_to_device(src[0], in_ptr);
            void* jpeg_data[4] = {NULL, NULL, NULL, NULL};
            size_t* size = (size_t*)malloc(image_n * sizeof(size_t));
            ret = bmcv_image_jpeg_enc(handle[j], image_n, src, jpeg_data, size, 95);
            if (ret != BM_SUCCESS) exit(-1);

            // test decode
            bm_image dst[4];
            // get dst format
            if (true) {  // create_dst_image
                int dst_format = FORMAT_NV12;
                stride[0] = align_up(image_w, 16);
                stride[1] = align_up(image_w, 16);
                for (int i = 0; i < image_n; i++) {
                    bm_image_create(handle[j], image_h, image_w, (bm_image_format_ext)dst_format, DATA_TYPE_EXT_1N_BYTE, dst + i, stride);
                }
            }
            ret = bmcv_image_jpeg_dec(handle[j], (void**)jpeg_data, size, image_n, dst);
            assert(ret == BM_SUCCESS);

            bm_image_get_byte_size(dst[0], image_byte_size);
            for (int i = 0; i < 4; i++) {
                printf("dst_bytesize[%d]: %d\n", i, image_byte_size[i]);
            }
            byte_size = image_h * image_w * 3;
            uint8_t* output_ptr = (uint8_t*)malloc(byte_size * image_n);
            for (int i = 0;i < image_n; i++) {
                void* out_ptr[3] = {(void *)output_ptr, (void *)((uint8_t*)output_ptr + image_w * image_h), (void *)((uint8_t*)output_ptr + 5 / 4 * image_w * image_h)};

                bm_image_copy_device_to_host(dst[i], out_ptr);
            }

            FILE *fp_dst = fopen(dst_name, "wb");
            if (fwrite((void *)output_ptr, 1, byte_size, fp_dst) < (unsigned int)byte_size){
                printf("file size is less than %d required bytes\n", byte_size);
            };
            fclose(fp_dst);

            for (int i = 0; i < image_n; i++) {
                bm_image_destroy(&dst[i]);
            }

            for (int i = 0; i < image_n; i++) {
                free(jpeg_data[i]);
                bm_image_destroy(&src[i]);
            }
            free(input_data);
            free(output_ptr);
            free(size);
            free(stride);
        }
        for (int i = 0; i < dev_cnt; i++) {
            bm_dev_free(handle[i]);
        }
        return ret;
    }
