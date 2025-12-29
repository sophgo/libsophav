bmcv_image_jpeg_enc
-------------------

【描述】

该接口可以实现对多张 bm_image 的 JPEG 编码过程。

【语法】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_jpeg_enc(
            bm_handle_t handle,
            int         image_num,
            bm_image *  src,
            void *      p_jpeg_data[],
            size_t *    out_size,
            int         quality_factor = 85
    );


【参数】

.. list-table:: bmcv_image_jpeg_enc 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - image_num
      - 输入
      - 输入图片数量，最多支持 4。
    * - \*src
      - 输入
      - 输入 bm_image的指针。每个 bm_image 需要外部调用 bmcv_image_create 创建，image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存,或者使用 bmcv_image_attach 来 attach 已有的内存。
    * - \*p_jpeg_data[]
      - 输出
      - 编码后图片的数据指针，由于该接口支持对多张图片的编码，因此为指针数组，数组的大小即为 image_num。用户可以选择不为其申请空间（即数组每个元素均为NULL），在 api 内部会根据编码后数据的大小自动分配空间，但当不再使用时需要用户手动释放该空间。当然用户也可以选择自己申请足够的空间。
    * - \*out_size
      - 输入
      - 完成编码后各张图片的大小（以 byte 为单位）存放在该指针中。
    * - quality_factor
      - 输入
      - 编码后图片的质量因子。取值 0～100 之间，值越大表示图片质量越高，但数据量也就越大，反之值越小图片质量越低，数据量也就越少。该参数为可选参数，默认值为85。

【返回值】

该函数成功调用时, 返回 BM_SUCCESS。


.. note::

    目前编码支持的图片格式包括以下几种：
     | FORMAT_YUV420P
     | FORMAT_YUV422P
     | FORMAT_YUV444P
     | FORMAT_NV12
     | FORMAT_NV21
     | FORMAT_NV16
     | FORMAT_NV61
     | FORMAT_GRAY

    目前编码支持的数据格式如下：
     | DATA_TYPE_EXT_1N_BYTE



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
            void* in_ptr[4] = {(void *)input_data,
                                (void *)((unsigned char*)input_data + image_byte_size[0]),
                                (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1]),
                                (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
            bm_image_copy_host_to_device(src[0], in_ptr);
            void* jpeg_data[4] = {NULL, NULL, NULL, NULL};
            size_t* size = (size_t*)malloc(image_n * sizeof(size_t));
            ret = bmcv_image_jpeg_enc(handle[j], image_n, src, jpeg_data, size, 95);
            if (ret != BM_SUCCESS) exit(-1);

            for (int i = 0; i < image_n; i++) {
                free(jpeg_data[i]);
                bm_image_destroy(&src[i]);
            }
            free(input_data);
            free(size);
            free(stride);
        }
        for (int i = 0; i < dev_cnt; i++) {
            bm_dev_free(handle[i]);
        }
        return ret;
    }
