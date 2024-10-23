.. raw:: latex

   \newpage

BMCV基础结构体
================

本章节主要用于介绍BMCV使用的基础结构体；我们不保证此文档更改后能及时告知用户，因此请使用最新版 released SDK 里面的文档。


bm_image
-----------

bm_image 结构体定义如下:

    ::

        struct bm_image {
            int width;
            int height;
            bm_image_format_ext image_format;
            bm_data_format_ext data_type;
            struct bm_image_private* image_private;
        };

.. list-table:: bm_image 结构成员
    :widths: 15 35 35

    * - **序号**
      - **术语**
      - **定义说明**
    * - 1
      - width
      - 图像的宽
    * - 2
      - height
      - 图像的高
    * - 3
      - image_format
      - 图像的色彩格式
    * - 4
      - data_type
      - 图像的数据格式
    * - 5
      - image_private
      - 图像的私有数据

bm_image_format_ext
-----------------------

bm_image_format_ext 有以下枚举类型。

    .. code-block:: cpp
      :force:

        typedef enum bm_image_format_ext_{
            FORMAT_YUV420P,
            FORMAT_YUV422P,
            FORMAT_YUV444P,
            FORMAT_NV12,
            FORMAT_NV21,
            FORMAT_NV16,
            FORMAT_NV61,
            FORMAT_NV24,
            FORMAT_RGB_PLANAR,
            FORMAT_BGR_PLANAR,
            FORMAT_RGB_PACKED,
            FORMAT_BGR_PACKED,
            FORMAT_RGBP_SEPARATE,
            FORMAT_BGRP_SEPARATE,
            FORMAT_GRAY,
            FORMAT_COMPRESSED,
            FORMAT_HSV_PLANAR,
            FORMAT_ARGB_PACKED,
            FORMAT_ABGR_PACKED,
            FORMAT_YUV444_PACKED,
            FORMAT_YVU444_PACKED,
            FORMAT_YUV422_YUYV,
            FORMAT_YUV422_YVYU,
            FORMAT_YUV422_UYVY,
            FORMAT_YUV422_VYUY,
            FORMAT_RGBYP_PLANAR,
            FORMAT_HSV180_PACKED,
            FORMAT_HSV256_PACKED,
            FORMAT_BAYER,
            FORMAT_BAYER_RG8,
            FORMAT_ARGB4444_PACKED,
            FORMAT_ABGR4444_PACKED,
            FORMAT_ARGB1555_PACKED,
            FORMAT_ABGR1555_PACKED,
        } bm_image_format_ext;


.. list-table:: 各个格式说明
   :widths: 10 40 50
   :header-rows: 1
   :class: longtable
   :stub-columns: 1

   * - **序号**
     - **格式名称**
     - **说明**
   * - 1
     - FORMAT_YUV420P
     - 表示预创建一个 YUV420 格式的图片，有三个plane。
   * - 2
     - FORMAT_YUV422P
     - 表示预创建一个 YUV422 格式的图片，有三个plane。
   * - 3
     - FORMAT_YUV444P
     - 表示预创建一个 YUV444 格式的图片，有三个plane。
   * - 4
     - FORMAT_NV12
     - 表示预创建一个 NV12 格式的图片，有两个plane。
   * - 5
     - FORMAT_NV21
     - 表示预创建一个 NV21 格式的图片，有两个plane。
   * - 6
     - FORMAT_NV16
     - 表示预创建一个 NV16 格式的图片，有两个plane。
   * - 7
     - FORMAT_NV61
     - 表示预创建一个 NV61 格式的图片，有两个plane。
   * - 8
     - FORMAT_NV24
     - 表示预创建一个 NV24 格式的图片，有两个plane。
   * - 9
     - FORMAT_RGB_PLANAR
     - 表示预创建一个 RGB 格式的图片，RGB 分开排列,有一个 plane。
   * - 10
     - FORMAT_BGR_PLANAR
     - 表示预创建一个 BGR 格式的图片，BGR 分开排列,有一个 plane。
   * - 11
     - FORMAT_RGB_PACKED
     - 表示预创建一个 RGB 格式的图片，RGB 交错排列,有一个 plane。
   * - 12
     - FORMAT_BGR_PACKED
     - 表示预创建一个 BGR 格式的图片，BGR 交错排列,有一个 plane。
   * - 13
     - FORMAT_RGBP_SEPARATE
     - 表示预创建一个 RGB planar 格式的图片，RGB 分开排列并各占一个 plane，共有 3 个 plane。
   * - 14
     - FORMAT_BGRP_SEPARATE
     - 表示预创建一个 BGR planar 格式的图片，BGR 分开排列并各占一个 plane，共有 3 个 plane。
   * - 15
     - FORMAT_GRAY
     - 表示预创建一个灰度图格式的图片，有一个 plane。
   * - 16
     - FORMAT_COMPRESSED
     - 表示预创建一个 VPU 内部压缩格式的图片，共有四个 plane，分别存放内容如下：plane0: Y 压缩表、plane1: Y 压缩数据、plane2: CbCr 压缩表、plane3: CbCr 压缩数据。
   * - 17
     - FORMAT_HSV_PLANAR
     - 表示预创建一个HSV planar格式的图片，H 的范围为 0~180，有三个 plane。
   * - 18
     - FORMAT_ARGB_PACKED
     - 表示预创建一个ARGB 格式的图片，该图片仅有一个 plane，并且像素值以 RGBA 顺序交错连续排列，即 RGBARGBA。
   * - 19
     - FORMAT_ABGR_PACKED
     - 表示预创建一个ABGR 格式的图片，该图片仅有一个 plane，并且像素值以 BGRA 顺序交错连续排列，即 BGRABGRA。
   * - 20
     - FORMAT_YUV444_PACKED
     - 表示预创建一个YUV444 格式的图片，YUV 交错排列，有一个 plane。
   * - 21
     - FORMAT_YVU444_PACKED
     - 表示预创建一个YVU444 格式的图片，YVU 交错排列，有一个 plane。
   * - 22
     - FORMAT_YUV422_YUYV
     - 表示预创建一个YUV422 格式的图片，YUYV 交错排列，有一个 plane。
   * - 23
     - FORMAT_YUV422_YVYU
     - 表示预创建一个YUV422 格式的图片，YVYU 交错排列，有一个 plane。
   * - 24
     - FORMAT_YUV422_UYVY
     - 表示预创建一个YUV422 格式的图片，UYVY 交错排列，有一个 plane。
   * - 25
     - FORMAT_YUV422_VYUY
     - 表示预创建一个YUV422 格式的图片，VYUY 交错排列，有一个 plane。
   * - 26
     - FORMAT_RGBYP_PLANAR
     - 表示预创建一个RGBY planar格式的图片，有四个 plane。
   * - 27
     - FORMAT_HSV180_PACKED
     - 表示预创建一个HSV 格式的图片，H 的范围为 0~180，HSV 交错排列，有一个 plane。
   * - 28
     - FORMAT_HSV256_PACKED
     - 表示预创建一个HSV 格式的图片，H 的范围为 0~255，HSV 交错排列，有一个 plane。
   * - 29
     - FORMAT_BAYER
     - 表示预创建一个bayer 格式的图片，有一个 plane，像素排列方式是BGGR，且宽高需要是偶数。
   * - 30
     - FORMAT_BAYER_RG8
     - 表示预创建一个bayer 格式的图片，有一个 plane，像素排列方式是RGGB，且宽高需要是偶数。
   * - 31
     - FORMAT_ARGB4444_PACKED
     - 表示预创建一个ARGB 格式的图片，包含四个通道，即A：透明度（Alpha）、R：红色（Red）、G：绿色（Green）、B：蓝色（Blue），每个通道占4位，共16位，有一个 plane。
   * - 32
     - FORMAT_ABGR4444_PACKED
     - 表示预创建一个ABGR 格式的图片，包含四个通道，即A：透明度（Alpha）、B：蓝色（Blue）、G：绿色（Green）、R：红色（Red），每个通道占4位，共16位，有一个 plane。
   * - 33
     - FORMAT_ARGB1555_PACKED
     - 表示预创建一个ARGB 格式的图片，包含四个通道，即A：透明度（Alpha）、R：红色（Red）、G：绿色（Green）、B：蓝色（Blue），各个通道分别占1、5、5、5位，共16位，有一个 plane。
   * - 34
     - FORMAT_ABGR1555_PACKED
     - 表示预创建一个ABGR 格式的图片，包含四个通道，即A：透明度（Alpha）、B：蓝色（Blue）、G：绿色（Green）、R：红色（Red），各个通道分别占1、5、5、5位，共16位，有一个 plane。


bm_data_format_ext
-----------------------

bm_data_format_ext 有以下枚举类型。

    .. code-block:: cpp
      :force:

        typedef enum bm_image_data_format_ext_{
            DATA_TYPE_EXT_FLOAT32,
            DATA_TYPE_EXT_1N_BYTE,
            DATA_TYPE_EXT_1N_BYTE_SIGNED,
            DATA_TYPE_EXT_FP16,
            DATA_TYPE_EXT_BF16,
            DATA_TYPE_EXT_U16,
            DATA_TYPE_EXT_S16,
            DATA_TYPE_EXT_U32,
            DATA_TYPE_EXT_4N_BYTE = 254,
            DATA_TYPE_EXT_4N_BYTE_SIGNED,
        }bm_image_data_format_ext;

.. list-table:: 各个格式说明
   :widths: 8 51 41

   * - **序号**
     - **格式名称**
     - **说明**
   * - 1
     - DATA_TYPE_EXT_FLOAT32
     - 表示所创建的图片数据格式为单精度浮点数。
   * - 2
     - DATA_TYPE_EXT_1N_BYTE
     - 表示所创建图片数据格式为普通无符号UINT8。
   * - 3
     - DATA_TYPE_EXT_1N_BYTE_SIGNED
     - 表示所创建图片数据格式为普通有符号INT8。
   * - 4
     - DATA_TYPE_EXT_FP16
     - 表示所创建的图片数据格式为半精度浮点数，5bit表示指数，10bit表示小数。
   * - 5
     - DATA_TYPE_EXT_BF16
     - 表示所创建的图片数据格式为16bit浮点数，实际是对FLOAT32单精度浮点数截断数据，即用8bit表示指数，7bit表示小数。
   * - 6
     - DATA_TYPE_EXT_U16
     - 表示所创建图片数据格式为普通无符号UINT16。
   * - 7
     - DATA_TYPE_EXT_S16
     - 表示所创建图片数据格式为普通有符号INT16。
   * - 8
     - DATA_TYPE_EXT_U32
     - 表示所创建图片数据格式为普通无符号UINT32。
   * - 9
     - DATA_TYPE_EXT_4N_BYTE
     - 表示所创建图片数据格式为 4N UINT8，即四张无符号 UINT8 图片数据交错排列，一个 bm_image 对象其实含有四张属性相同的图片。
   * - 10
     - DATA_TYPE_EXT_4N_BYTE_SIGNED
     - 表示所创建图片数据格式为 4N INT8，即四张有符号 INT8 图片数据交错排列。

.. raw:: latex

   \newpage



