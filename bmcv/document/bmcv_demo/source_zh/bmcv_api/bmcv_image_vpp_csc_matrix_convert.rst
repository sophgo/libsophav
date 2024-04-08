bmcv_image_vpp_csc_matrix_convert
---------------------------------

| 【描述】

| 默认情况下，bmcv_image_vpp_convert使用的是BT_601标准进行色域转换。有些情况下需要使用其他标准，或者用户自定义csc参数。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_vpp_csc_matrix_convert(
        bm_handle_t  handle,
        int output_num,
        bm_image input,
        bm_image *output,
        csc_type_t csc,
        csc_matrix_t * matrix = NULL,
        bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR,
        bmcv_rect_t *crop_rect = NULL);

| 【参数】

.. list-table:: bmcv_image_vpp_csc_matrix_convert 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - image_num
      - 输入
      - 输入 bm_image 数量。
    * - input
      - 输入
      - 输入 bm_image 对象。
    * - \*output
      - 输出
      - 输出 bm_image 对象指针。
    * - csc
      - 输入
      - 色域转换枚举类型，目前可选类型见表： :ref:`csc_type`
    * - \*matrix
      - 输入
      - 色域转换自定义矩阵，当且仅当csc为CSC_USER_DEFINED_MATRIX时这个值才生效。
    * - algorithm
      - 输入
      - resize 算法选择，包括 BMCV_INTER_NEAREST、BMCV_INTER_LINEAR 和 BMCV_INTER_BICUBIC三种，默认情况下是双线性差值。

| 【数据类型说明】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum csc_type {
        CSC_YCbCr2RGB_BT601 = 0,
        CSC_YPbPr2RGB_BT601,
        CSC_RGB2YCbCr_BT601,
        CSC_YCbCr2RGB_BT709,
        CSC_RGB2YCbCr_BT709,
        CSC_RGB2YPbPr_BT601,
        CSC_YPbPr2RGB_BT709,
        CSC_RGB2YPbPr_BT709,
        CSC_FANCY_PbPr_BT601 = 100,
        CSC_FANCY_PbPr_BT709,
        CSC_USER_DEFINED_MATRIX = 1000,
        CSC_MAX_ENUM
    } csc_type_t;

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct {
        short csc_coe00;
        short csc_coe01;
        short csc_coe02;
        unsigned char csc_add0;
        unsigned char csc_sub0;
        short csc_coe10;
        short csc_coe11;
        short csc_coe12;
        unsigned char csc_add1;
        unsigned char csc_sub1;
        short csc_coe20;
        short csc_coe21;
        short csc_coe22;
        unsigned char csc_add2;
        unsigned char csc_sub2;
    } csc_matrix_t;

其中，矩阵变换关系如下：

.. math::

    \left\{
    \begin{array}{c}
    dst_0=(coe_{00} * (src_0-sub_0)+coe_{01} * (src_1-sub_1)+coe_{02} * (src_2-sub_2))>>10 + add_0 \\
    dst_1=(coe_{10} * (src_0-sub_0)+coe_{11} * (src_1-sub_1)+coe_{12} * (src_2-sub_2))>>10 + add_1 \\
    dst_2=(coe_{20} * (src_0-sub_0)+coe_{21} * (src_1-sub_1)+coe_{22} * (src_2-sub_2))>>10 + add_2 \\
    \end{array}
    \right.

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 该 API 所需要满足的格式以及部分要求与vpp_convert一致

2. 如果色域转换枚举类型与input和output格式不对应，如csc == CSC_YCbCr2RGB_BT601,而input image_format为RGB格式，则返回失败。

3. 如果csc == CSC_USER_DEFINED_MATRIX而matrix为nullptr，则返回失败。

【代码示例】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <iostream>
    #include <vector>
    #include "bmcv_api_ext.h"
    #include "bmlib_runtime.h"
    #include "common.h"
    #include <memory>
    #include "stdio.h"
    #include "stdlib.h"
    #include <stdio.h>
    #include <stdlib.h>

    int main(int argc, char *argv[]) {
        bm_handle_t handle;
        int            image_h     = 1080;
        int            image_w     = 1920;
        bm_image       src, dst[4];
        bm_dev_request(&handle, 0);
        bm_image_create(handle, image_h, image_w, FORMAT_NV12,
                DATA_TYPE_EXT_1N_BYTE, &src);
        bm_image_alloc_dev_mem(src, 1);
        for (int i = 0; i < 4; i++) {
            bm_image_create(handle,
                image_h / 2,
                image_w / 2,
                FORMAT_BGR_PACKED,
                DATA_TYPE_EXT_1N_BYTE,
                dst + i);
            bm_image_alloc_dev_mem(dst[i]);
        }
        std::unique_ptr<u8 []> y_ptr(new u8[image_h * image_w]);
        std::unique_ptr<u8 []> uv_ptr(new u8[image_h * image_w / 2]);
        memset((void *)(y_ptr.get()), 148, image_h * image_w);
        memset((void *)(uv_ptr.get()), 158, image_h * image_w / 2);
        u8 *host_ptr[] = {y_ptr.get(), uv_ptr.get()};
        bm_image_copy_host_to_device(src, (void **)host_ptr);

        bmcv_rect_t rect[] = {{0, 0, image_w / 2, image_h / 2},
                {0, image_h / 2, image_w / 2, image_h / 2},
                {image_w / 2, 0, image_w / 2, image_h / 2},
                {image_w / 2, image_h / 2, image_w / 2, image_h / 2}};

        bmcv_image_vpp_csc_matrix_convert(handle, 4, src, dst, CSC_YCbCr2RGB_BT601);

        for (int i = 0; i < 4; i++) {
            bm_image_destroy(dst + i);
        }

        bm_image_destroy(&src);
        bm_dev_free(handle);
        return 0;
    }


