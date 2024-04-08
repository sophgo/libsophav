bm_image_copy_host_to_device
----------------------------

| 【描述】
| 该 API 将 host 端数据拷贝到 bm_image 结构对应的 device memory 中。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :name: a label for hyperlink (destroy)
    :force:

    bm_status_t bm_image_copy_host_to_device(
        bm_image image,
        void* buffers[]
    );

| 【参数】

.. list-table:: bm_image_copy_host_to_device 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 待填充 device memory 数据的 bm_image 对象。
    * - \* buffers[]
      - 输入
      - host 端指针，buffers 为指向不同 plane 数据的指针。

其中，buffers[]的数组长度由创建 bm_image 结构时 image_format 对应的 plane 数所决定。每个 plane 的数据量会由创建 bm_image 时的图片宽高、stride、image_format、data_type 决定。具体的计算方法如下：

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    switch (res->image_format) {
        case FORMAT_YUV420P: {
            width[0]  = res->width;
            width[1]  = ALIGN(res->width, 2) / 2;
            width[2]  = width[1];
            height[0] = res->height;
            height[1] = ALIGN(res->height, 2) / 2;
            height[2] = height[1];
            break;
        }
        case FORMAT_YUV422P: {
            width[0]  = res->width;
            width[1]  = ALIGN(res->width, 2) / 2;
            width[2]  = width[1];
            height[0] = res->height;
            height[1] = height[0];
            height[2] = height[1];
            break;
        }
        case FORMAT_YUV444P: {
            width[0]  = res->width;
            width[1]  = width[0];
            width[2]  = width[1];
            height[0] = res->height;
            height[1] = height[0];
            height[2] = height[1];
            break;
        }
        case FORMAT_NV12:
        case FORMAT_NV21: {
            width[0]  = res->width;
            width[1]  = ALIGN(res->width, 2);
            height[0] = res->height;
            height[1] = ALIGN(res->height, 2) / 2;
            break;
        }
        case FORMAT_NV16:
        case FORMAT_NV61: {
            width[0]  = res->width;
            width[1]  = ALIGN(res->width, 2);
            height[0] = res->height;
            height[1] = res->height;
            break;
        }
        case FORMAT_GRAY: {
            width[0]  = res->width;
            height[0] = res->height;
            break;
        }
        case FORMAT_COMPRESSED: {
            width[0]  = res->width;
            height[0] = res->height;
            break;
        }
        case FORMAT_BGR_PACKED:
        case FORMAT_RGB_PACKED: {
            width[0]  = res->width * 3;
            height[0] = res->height;
            break;
        }
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PLANAR: {
            width[0]  = res->width;
            height[0] = res->height * 3;
            break;
        }
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE: {
            width[0]  = res->width;
            width[1]  = width[0];
            width[2]  = width[1];
            height[0] = res->height;
            height[1] = height[0];
            height[2] = height[1];
            break;
        }
        case FORMAT_BAYER: {
            width[0]  = res->width * 2;
            height[0] = res->height * 2;
            break;
        }
    }

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

.. note::

    1. 如果 bm_image 未由 bm_image_create 创建，则返回失败。

    2. 如果所传入的 bm_image 对象还没有与 device memory 相关联的话，会自动为每个 plane 申请对应 image_private->plane_byte_size 大小的 device memory，并将 host 端数据拷贝到申请的 device memory 中。如果申请 device memory 失败，则该 API 调用失败。

    3. 如果所传入的 bm_image 对象图片格式为 FORMAT_COMPRESSED ，且该 bm_image 在 create 时没有设置 stride 参数，则直接返回失败。

    4. 如果拷贝失败,则该 API 调用失败。
