bmcv_image_overlay
----------------------

| 【描述】

| 该 API 实现在图片上添加带透明通道的水印图。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_overlay(
        bm_handle_t         handle,
        bm_image            image,
        int                 overlay_num,
        bmcv_rect_t*        overlay_info,
        bm_image*           overlay_image);

| 【参数】

.. list-table:: bmcv_image_overlay 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - image
      - 输入/输出
      - 输入/输出 bm_image 对象。
    * - overlay_num
      - 输入
      - 水印图的个数。
    * - \* overlay_info
      - 输入
      - 输入 水印图 在输入图片上区域信息。
    * - \* overlay_image
      - 输入
      - 输入 overlay_image 对象。

| 【数据类型说明】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_rect {
        unsigned int start_x;
        unsigned int start_y;
        unsigned int crop_w;
        unsigned int crop_h;
    } bmcv_rect_t;

start_x、start_y、crop_w、crop_h 分别表示输入水印图对象在输入图像上信息，包括起始点x坐标、起始点y坐标、水印图像的宽度以及水印图像的高度。图像左上顶点作为坐标原点。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。


| 【注意】

1. 水印图色彩格式为：

+-----+-------------------------------+
| num | input image_format            |
+=====+===============================+
|  1  | FORMAT_ARGB_PACKED            |
+-----+-------------------------------+
|  2  | FORMAT_ARGB4444_PACKED        |
+-----+-------------------------------+
|  3  | FORMAT_ARGB1555_PACKED        |
+-----+-------------------------------+

2. 水印图的 stride 需要 16 对齐。

3. 该 API 所需要满足的底图格式以及尺寸要求与 bmcv_image_vpp_basic 一致。

4. 输入必须关联 device memory，否则返回失败。


| 【代码示例】

.. code-block:: c++

    #include <limits.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <pthread.h>
    #include <time.h>
    #include <sys/time.h>

    #include "bmcv_api_ext_c.h"

    int main(int argc, char *argv[]) {
        int dev_id = 0;
        int test_loop_times  = 1;
        int test_threads_num = 1;
        bm_image_format_ext image_fmt = FORMAT_RGB_PACKED;
        bm_image_format_ext overlay_fmt = FORMAT_ARGB1555_PACKED;
        int img_w = 1920, img_h = 1080;
        int overlay_w = 80, overlay_h = 60;
        int overlay_num = 1;
        int x = 200, y = 200;
        bool bWrite = false;
        char *image_name = "./res/car_rgb888.rgb";
        char *overlay_name = "./res/dog_s_80x60_pngto1555.bin";
        char *md5 = "a858701cb01131f1356f80c76e7813c0";    // argb1555
        bm_handle_t handle = NULL;
        bm_status_t ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            return -1;
        }
        bmcv_rect_t overlay_info;
        memset(&overlay_info, 0, sizeof(bmcv_rect_t));

        overlay_info.start_x = x;
        overlay_info.start_y = y;
        overlay_info.crop_h = overlay_h;
        overlay_info.crop_w = overlay_w;

        bm_image_create(handle, img_h, img_w, image_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);

        ret = bm_image_alloc_dev_mem(image, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("image bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_read_bin(image, image_name);
        for(int idx = 0; idx < overlay_num; idx++){
            bm_image_create(handle, overlay_h, overlay_w, overlay_fmt,
                                DATA_TYPE_EXT_1N_BYTE, &overlay_image[idx], NULL);

            ret = bm_image_alloc_dev_mem(overlay_image[idx], BMCV_HEAP_ANY);
            if(ret != BM_SUCCESS){
                printf("image bm_image_alloc_dev_mem failed. ret = %d\n", ret);
                exit(-1);
            }

            bm_read_bin(overlay_image[idx], overlay_name);
        }
        ret = bmcv_image_overlay(handle, image, overlay_num, &overlay_info, overlay_image);
        bm_write_bin(image, "./bmcv_image_overlay_res.bin");

        bm_image_destroy(&image);
        for (int i=0;i<overlay_num;i++){
            bm_image_destroy(&overlay_image[i]);
        }
        return 0;
    }
