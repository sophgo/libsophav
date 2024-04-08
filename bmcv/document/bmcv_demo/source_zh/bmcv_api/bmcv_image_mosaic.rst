bmcv_image_mosaic
-----------------

**描述：**

该接口用于在图像上打一个或多个马赛克。

**语法：**

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_mosaic(
                bm_handle_t handle,
                int mosaic_num,
                bm_image input,
                bmcv_rect_t * mosaic_rect,
                int is_expand)

**参数：**

.. list-table:: bmcv_image_mosaic 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - mosaic_num
      - 输入
      - 马赛克数量，指 mosaic_rect 指针中所包含的 bmcv_rect_t 对象个数。
    * - input
      - 输入
      - 需要打马赛克的 bm_image 对象。
    * - \*mosaic_rect
      - 输入
      - 马赛克对象指针，包含每个马赛克起始点和宽高。具体内容参考下面的数据类型说明。
    * - is_expand
      - 输入
      - 是否扩列。值为0时表示不扩列, 值为1时表示在原马赛克周围扩列一个宏块(8个像素)。

**数据类型说明：**

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_rect {
        int start_x;
        int start_y;
        int crop_w;
        int crop_h;
    } bmcv_rect_t;

* start_x 描述了马赛克在原图中所在的起始横坐标。自左而右从 0 开始，取值范围 [0, width)。
* start_y 描述了马赛克在原图中所在的起始纵坐标。自上而下从 0 开始，取值范围 [0, height)。
* crop_w 描述的马赛克的宽度。
* crop_h 描述的马赛克的高度。

**返回值：**

该函数成功调用时, 返回 BM_SUCCESS。

**格式支持：**

1. 输入和输出的数据类型

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+

2. 输入和输出的色彩格式

+-----+-------------------------------+
| num | image_format                  |
+=====+===============================+
|  1  | FORMAT_YUV420P                |
+-----+-------------------------------+
|  2  | FORMAT_YUV444P                |
+-----+-------------------------------+
|  3  | FORMAT_NV12                   |
+-----+-------------------------------+
|  4  | FORMAT_NV21                   |
+-----+-------------------------------+
|  5  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  6  | FORMAT_BGR_PLANAR             |
+-----+-------------------------------+
|  7  | FORMAT_RGB_PACKED             |
+-----+-------------------------------+
|  8  | FORMAT_BGR_PACKED             |
+-----+-------------------------------+
|  9  | FORMAT_RGBP_SEPARATE          |
+-----+-------------------------------+
|  10 | FORMAT_BGRP_SEPARATE          |
+-----+-------------------------------+
|  11 | FORMAT_GRAY                   |
+-----+-------------------------------+

**注意：**

1. 输入输出所有 bm_image 结构必须提前创建，否则返回失败。
#. 如果马赛克宽高非8对齐，则会自动向上8对齐，若在边缘区域，则8对齐时会往非边缘方向延展。
#. 如果马赛克区域超出原图宽高，超出部分会自动贴到原图边缘。
#. 仅支持8x8以上的马赛克尺寸。
