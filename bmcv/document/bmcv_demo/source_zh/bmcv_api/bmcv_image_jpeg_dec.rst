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

    size_t size = 0;
    // read input from picture
    FILE *fp = fopen(filename, "rb+");
    assert(fp != NULL);
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    u8* jpeg_data = (u8*)malloc(*size);
    fseek(fp, 0, SEEK_SET);
    fread(jpeg_data, *size, 1, fp);
    fclose(fp);

    // create bm_image used to save output
    bm_image dst;
    memset((char*)&dst, 0, sizeof(bm_image));
    // if you not create dst bm_image it will create automatically inside.
    // you can also create dst bm_image here, like this:
    // bm_image_create(handle, IMAGE_H, IMAGE_W, FORMAT_YUV420P,
    //         DATA_TYPE_EXT_1N_BYTE, &dst);

    // decode input
    int ret = bmcv_image_jpeg_dec(handle, (void**)&jpeg_data, &size, 1, &dst);
    free(jpeg_data);
    bm_image_destory(dst);



