bmcv_image_draw_point
=========================

该接口用于在图像上填充一个或者多个point。


**接口形式：**
    .. code-block:: c

        bm_status_t bmcv_image_draw_point(
                    bm_handle_t handle,
                    bm_image image,
                    int point_num,
                    bmcv_point_t* coord,
                    int length,
                    unsigned char r,
                    unsigned char g,
                    unsigned char b);


**传入参数说明:**

* bm_handle_t handle

  输入参数。设备环境句柄，通过调用 bm_dev_request 获取。

* bm_image image

  输入参数。需要在其上填充 point 的 bm_image 对象。

* int point_num

  输入参数。需填充 point 的数量，指 coord 指针中所包含的 bmcv_point_t 对象个数。

* bmcv_point_t\* rect

  输入参数。point 位置指针。具体内容参考下面的数据类型说明。

* int length

  输入参数。point 的边长。

* unsigned char r

  输入参数。矩形填充颜色的r分量。

* unsigned char g

  输入参数。矩形填充颜色的g分量。

* unsigned char b

  输入参数。矩形填充颜色的b分量。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**数据类型说明：**

    .. code-block:: c

        typedef struct {
            int x;
            int y;
        } bmcv_point_t;


* x 描述了 point 在原图中所在的起始横坐标。自左而右从 0 开始，取值范围 [0, width)。

* y 描述了 point 在原图中所在的起始纵坐标。自上而下从 0 开始，取值范围 [0, height)。


**注意事项:**

1. 该接口底图的格式与尺寸限制于 bmcv_image_vpp_basic 接口相同。

2. 所有输入point对象区域须在图像以内。


**代码示例：**

    .. code-block:: c

        #include "bmcv_api_ext_c.h"
        #include <stdio.h>
        #include <stdlib.h>

        int main()
        {
            int src_w = 1920, src_h = 1080, dev_id = 0;
            bmcv_point_t center = {128, 128};
            unsigned char r = 0, g = 255, b = 128;
            int length = 2;
            bm_image_format_ext src_fmt  = FORMAT_YUV420P;
            bm_handle_t handle = NULL;
            bm_image src;
            char *src_name = "/opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin";
            char *dst_name = "dst.bin";
            bmcv_point_t coord[9] = {{center.x - 60, center.y - 40}, {center.x, center.y - 40}, {center.x + 60, center.y - 40},
                        {center.x - 60, center.y}, {center.x, center.y}, {center.x + 60, center.y},
                        {center.x - 60, center.y + 40}, {center.x, center.y + 40}, {center.x + 60, center.y + 40}};
            bm_dev_request(&handle, dev_id);
            bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
            bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
            bm_read_bin(src, src_name);

            bmcv_image_draw_point(handle, src, 9, coord, length, r, g, b);

            bm_write_bin(src, dst_name);
            bm_image_destroy(&src);
            bm_dev_free(handle);
            return 0;
        }