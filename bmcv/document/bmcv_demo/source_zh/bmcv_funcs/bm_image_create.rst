bm_image_create
---------------

| 【描述】
| 创建一个bm_image结构。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :name: a label for hyperlink (creat)
    :force:

    bm_status_t bm_image_create(
        bm_handle_t handle,
        int img_h,
        int img_w,
        bmcv_image_format_ext image_format,
        bmcv_data_format_ext data_type,
        bm_image *image,
        int* stride);

| 【参数】

.. list-table:: bm_image_create 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 输入参数。设备环境句柄，通过调用 bm_dev_request 获取。
    * - img_h
      - 输入
      - 图片高度。
    * - img_w
      - 输入
      - 图片宽度。
    * - image_format
      - 输入
      - 所需创建 bm_image 图片格式。
    * - data_type
      - 输入
      - 所需创建 bm_image 数据格式。
    * - \*image
      - 输出
      - 输出填充的 bm_image 结构指针。
    * - \*stride
      - 输入
      - 所创建 bm_image 将要关联的 device memory 内存布局。在每个plane 的 width stride 值，以 byte 计数。在不填写时候默认为和一行的数据宽度相同(以 byte 计数)。


| 【返回值】

bmcv_image_create 成功调用将返回 BM_SUCCESS，并填充输出的 image 指针结构。这个结构中记录了图片的大小，以及相关格式。但此时并没有与任何 device memory 关联，也没有申请数据对应的 device memory。

| 【注意事项】

1. 以下图片格式的宽可以是奇数，接口内部会调整到偶数再完成相应功能。但建议尽量使用偶数的宽，这样可以发挥最大的效率。

    - FORMAT_YUV420P
    - FORMAT_YUV422P
    - FORMAT_NV12
    - FORMAT_NV21
    - FORMAT_NV16
    - FORMAT_NV61

#. 以下图片格式的高可以是奇数，接口内部会调整到偶数再完成相应功能。但建议尽量使用偶数的高，这样可以发挥最大的效率。

    - FORMAT_YUV420P
    - FORMAT_NV12
    - FORMAT_NV21

#. FORMAT_COMPRESSED 图片格式的图片创建时需要将 stride 参数的数组设置为各通道的数据长度。
#. 若 stride 参数设置为 NULL，此时默认各个 plane 的数据是 compact 排列，没有 stride。
#. 如果 stride 非 NULL，则会检测 stride 中的 width stride 值是否合法。所谓的合法，即 image_format 对应的所有 plane 的 stride 大于默认 stride。默认 stride 值的计算方法如下：

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :name: a label for hyperlink (stride)
    :force:

    int data_size = 1;
    switch (data_type) {
        case DATA_TYPE_EXT_FLOAT32:
            data_size = 4;
            break;
        case DATA_TYPE_EXT_FP16:
        case DATA_TYPE_EXT_BF16:
        case DATA_TYPE_EXT_U16:
        case DATA_TYPE_EXT_S16:
            data_size = 2;
            break;
        default:
            data_size = 1;
            break;
    }
    int default_stride[3] = {0};
    switch (image_format) {
        case FORMAT_YUV420P: {
            image_private->plane_num = 3;
            default_stride[0] = width * data_size;
            default_stride[1] = (ALIGN(width, 2) >> 1) * data_size;
            default_stride[2] = default_stride[1];
            break;
        }
        case FORMAT_YUV422P: {
            default_stride[0] = res->width * data_size;
            default_stride[1] = (ALIGN(res->width, 2) >> 1) * data_size;
            default_stride[2] = default_stride[1];
            break;
        }
        case FORMAT_YUV444P: {
            default_stride[0] = res->width * data_size;
            default_stride[1] = res->width * data_size;
            default_stride[2] = default_stride[1];
            break;
        }
        case FORMAT_NV12:
        case FORMAT_NV21: {
            image_private->plane_num = 2;
            default_stride[0] = width * data_size;
            default_stride[1] = ALIGN(res->width, 2) * data_size;
            break;
        }
        case FORMAT_NV16:
        case FORMAT_NV61: {
            image_private->plane_num = 2;
            default_stride[0] = res->width * data_size;
            default_stride[1] = ALIGN(res->width, 2) * data_size;
            break;
        }
        case FORMAT_GRAY: {
            image_private->plane_num = 1;
            default_stride[0] = res->width * data_size;
            break;
        }
        case FORMAT_COMPRESSED: {
            image_private->plane_num = 4;
            break;
        }
        case FORMAT_BGR_PACKED:
        case FORMAT_RGB_PACKED: {
            image_private->plane_num = 1;
            default_stride[0] = res->width * 3 * data_size;
            break;
        }
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PLANAR: {
            image_private->plane_num = 1;
            default_stride[0] = res->width * data_size;
            break;
        }
        case FORMAT_BGRP_SEPARATE:
        case FORMAT_RGBP_SEPARATE: {
            image_private->plane_num = 3;
            default_stride[0] = res->width * data_size;
            default_stride[1] = res->width * data_size;
            default_stride[2] = res->width * data_size;
            break;
        }
        case FORMAT_BAYER:
            image_private->plane_num = 1;
            default_stride[0] = res->width * 2 * data_size;
            break;
    }
