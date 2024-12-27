bmcv_image_overlay
----------------------

| 【描述】

| 该 API 实现在图片上叠加带透明通道的水印图。该接口可搭配bmcv_gen_text_bitmap接口实现绘制中英文的功能，参照代码示例2。

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

5. 不支持对单底图进行位置重叠的多图叠加。


| 【代码示例1】

.. code-block:: c++

    #include <limits.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "stb_image.h"

    #include "bmcv_api_ext_c.h"

    void readBin(const char * path, unsigned char* input_data, int size)
    {
        FILE *fp_src = fopen(path, "rb");
        if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size){
            printf("read_Bin: file size is less than %d required bytes\n", size);
        };

        fclose(fp_src);
    }

    void writeBin(const char * path, unsigned char* input_data, int size)
    {
        FILE *fp_dst = fopen(path, "wb");
        if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
            printf("write_Bin: file size is less than %d required bytes\n", size);
        };

        fclose(fp_dst);
    }


    int main(){
        bm_image_format_ext image_fmt = FORMAT_RGB_PACKED;
        bm_image_format_ext overlay_fmt = FORMAT_ARGB_PACKED;

        int img_w = 1920, img_h = 1080;
        int overlay_w = 300, overlay_h = 300;
        int overlay_num = 1;
        int x = 500, y = 500;
        char *image_name = "path/to/image";
        char *overlay_name = "path/to/overlay";
        char *output_image = "path/to/output";
        bm_handle_t handle = NULL;
        bm_status_t ret = bm_dev_request(&handle, 0);

        // config setting
        bmcv_rect_t overlay_info;
        memset(&overlay_info, 0, sizeof(bmcv_rect_t));

        overlay_info.start_x = x;
        overlay_info.start_y = y;
        overlay_info.crop_h = overlay_h;
        overlay_info.crop_w = overlay_w;

        unsigned char* img = malloc(img_h * img_w * 3);
        unsigned char* output_tpu = malloc(img_h * img_w * 3);
        readBin(image_name, img, img_h * img_w * 3);
        memset(output_tpu, 0, img_h * img_w * 3);

        // create bm image struct & alloc dev mem
        bm_image image;
        bm_image_create(handle, img_h, img_w, image_fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);
        ret = bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
        unsigned char *in1_ptr[1] = {img};
        bm_image_copy_host_to_device(image, (void **)(in1_ptr));

        unsigned char* overlay_ptr = malloc(overlay_w * overlay_h * 4);
        readBin(overlay_name, overlay_ptr, overlay_w * overlay_h * 4);

        bm_image overlay_image[overlay_num];
        bm_image_create(handle, overlay_h, overlay_w, overlay_fmt, DATA_TYPE_EXT_1N_BYTE, overlay_image, NULL);
        ret = bm_image_alloc_dev_mem(overlay_image[0], BMCV_HEAP1_ID);
        unsigned char* in_overlay[4] = {overlay_ptr, overlay_ptr + overlay_h * overlay_w, overlay_ptr + 2 * overlay_w * overlay_h, overlay_ptr + overlay_w * overlay_h * 3};
        bm_image_copy_host_to_device(overlay_image[0], (void **)(in_overlay));

        ret = bmcv_image_overlay(handle, image, overlay_num, &overlay_info, overlay_image);

        unsigned char *out_ptr[3] = {output_tpu, output_tpu + img_h * img_w, output_tpu + 2 * img_h * img_w};
        bm_image_copy_device_to_host(image, (void **)out_ptr);

        writeBin(output_image, output_tpu, img_h * img_w * 3);

        free(img);
        free(output_tpu);
        bm_dev_free(handle);
        return ret;
    }

| 【代码示例2】

.. code-block:: c++

    #include <stdio.h>
    #include <string.h>
    #include <math.h>
    #include <stdbool.h>
    #include <stdlib.h>
    #include <wchar.h>
    #include <locale.h>
    #include <bmcv_api_ext_c.h>

    int main(int argc, char* args[]){

        setlocale(LC_ALL, "");
        bm_status_t ret = BM_SUCCESS;
        wchar_t hexcode[256];
        int r = 255, g = 255, b = 0;
        unsigned char fontScale = 2;
        char* output_path = "out.bmp";
        mbstowcs(hexcode, "北京beijing" sizeof(hexcode) / sizeof(wchar_t)); //usigned
        printf("Received wide character string: %ls\n", hexcode);
        printf("output path: %s\n", output_path);

        bm_image image;
        bm_handle_t handle = NULL;
        bm_dev_request(&handle, 0);
        bm_image_create(handle, 1080, 1920, FORMAT_YUV420P, DATA_TYPE_EXT_1N_BYTE, &image, NULL);
        bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
        bm_read_bin(image,"/opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin");
        bmcv_point_t org = {.x = 0, .y = 500};
        bmcv_color_t color = {.r = r, .g = g, .b = b};

        bm_image bitmap;
        ret = bmcv_gen_text_bitmap(handle, &bitmap, hexcode, color, fontScale);
        if (ret != BM_SUCCESS) {
            printf("bmcv_gen_text_bitmap fail\n");
            goto fail1;
        }

        bmcv_rect_t rect = {.start_x = org.x, .start_y = org.y, .crop_w = bitmap.width, .crop_h = bitmap.height};
        ret = bmcv_image_overlay(handle, image, 1, &rect, &bitmap);
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_overlay fail\n");
            goto fail2;
        }
        bm_image_write_to_bmp(image, output_path);

    fail2:
        bm_image_destroy(&bitmap);
    fail1:
        bm_image_destroy(&image);
        bm_dev_free(handle);
        return ret;
    }