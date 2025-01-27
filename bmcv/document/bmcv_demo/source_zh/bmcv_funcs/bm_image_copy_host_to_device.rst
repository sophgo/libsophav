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

其中，buffers[]的数组长度应等于创建 bm_image 结构时 image_format 对应的 plane 数。每个 plane 数据量的具体计算方法如下（以紧凑排列的默认stride为例）:

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    switch (image_format) {
        case FORMAT_YUV420P: {
            size[0] = width * height * data_size;
            size[1] = ALIGN(width, 2) * ALIGN(height, 2) / 4 * data_size;
            size[2] = ALIGN(width, 2) * ALIGN(height, 2) / 4 * data_size;
            break;
        }
        case FORMAT_YUV422P: {
            size[0] = width * height * data_size;
            size[1] = ALIGN(width, 2) * ALIGN(height, 2) / 2 * data_size;
            size[2] = ALIGN(width, 2) * ALIGN(height, 2) / 2 * data_size;
            break;
        }
        case FORMAT_YUV444P:
        case FORMAT_BGRP_SEPARATE:
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_HSV_PLANAR: {
            size[0] = width * height * data_size;
            size[1] = width * height * data_size;
            size[2] = width * height * data_size;
            break;
        }
        case FORMAT_NV24: {
            size[0] = width * height * data_size;
            size[1] = width * 2 * height * data_size;
            break;
        }
        case FORMAT_NV12:
        case FORMAT_NV21: {
            size[0] = width * height * data_size;
            size[1] = ALIGN(width, 2) * ALIGN(height, 2) / 2 * data_size;
            break;
        }
        case FORMAT_NV16:
        case FORMAT_NV61: {
            size[0] = width * height * data_size;
            size[1] = ALIGN(width, 2) * height * data_size;
            break;
        }
        case FORMAT_GRAY:
        case FORMAT_BAYER:
        case FORMAT_BAYER_RG8: {
            size[0] = width * height * data_size;
            break;
        }
        case FORMAT_COMPRESSED: {
            break;
        }
        case FORMAT_YUV444_PACKED:
        case FORMAT_YVU444_PACKED:
        case FORMAT_HSV180_PACKED:
        case FORMAT_HSV256_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGB_PACKED: {
            size[0] = width * 3 * height * data_size;
            break;
        }
        case FORMAT_ABGR_PACKED:
        case FORMAT_ARGB_PACKED: {
            size[0] = width * 4 * height * data_size;
            break;
        }
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PLANAR: {
            size[0] = width * height * 3 * data_size;
            break;
        }
        case FORMAT_RGBYP_PLANAR: {
            size[0] = width * height * data_size;
            size[1] = width * height * data_size;
            size[2] = width * height * data_size;
            size[3] = width * height * data_size;
            break;
        }
        case FORMAT_YUV422_YUYV:
        case FORMAT_YUV422_YVYU:
        case FORMAT_YUV422_UYVY:
        case FORMAT_YUV422_VYUY:
        case FORMAT_ARGB4444_PACKED:
        case FORMAT_ABGR4444_PACKED:
        case FORMAT_ARGB1555_PACKED:
        case FORMAT_ABGR1555_PACKED: {
            size[0] = width * 2 * height * data_size;
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
