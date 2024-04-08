bmcv_dwa_fisheye
------------------

| 【描述】
| 去畸变仿射(DWA)模块的鱼眼畸变校正功能，通过配置校正参数获取适当的校正模型来消除鱼眼镜头造成的图像畸变，从而使弯曲的图像呈现出人眼能够感受到的更真实的形式。
| 其中，提供两种校正的方式供用户选择，分别为：1. 用户根据鱼眼畸变的类型及校正模型输入配置参数列表对图像进行校正；2. 用户使用 Grid_Info(输入输出图像坐标映射关系描述)文件校正图像，以获得更好的图像校正效果。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_dwa_fisheye(bm_handle_t          handle,
                                 bm_image             input_image,
                                 bm_image             output_image,
                                 bmcv_fisheye_attr_s  fisheye_attr);

| 【参数】

.. list-table:: bmcv_dwa_fisheye 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取
    * - input_image
      - 输入
      - 输入的畸变图像，通过调用 bm_image_create 创建
    * - output_image
      - 输出
      - 输出的畸变校正后的图像，通过调用 bm_image_create 创建
    * - fisheye_attr
      - 输入
      - Fisheye功能的参数配置列表，包括 bEnable bBgColor u32BgColor s32HorOffset s32VerOffset u32TrapezoidCoef s32FanStrength enMountMode enUseMode u32RegionNum enViewMode \*grid_info

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【数据类型说明】

.. list-table:: bmcv_fisheye_attr_s 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - bEnable
      - 输入
      - 配置范围:bool，是否开启鱼眼校正功能
    * - bBgColor
      - 输入
      - 配置范围:bool，是否使用背景颜色功能
    * - u32BgColor
      - 输入
      - 配置范围:[0, 0xffffff]，背景颜色的 RGB888 值，bBgColor=1 时生效
    * - s32HorOffset
      - 输入
      - 图像中心点相对于物理中心点的水平偏移
    * - s32VerOffset
      - 输入
      - 图像中心点相对于物理中心点的垂直偏移
    * - u32TrapezoidCoef
      - 输入
      - 配置范围:[0, 32]，梯形校正强度系数，默认输入配置为0
    * - s32FanStrength
      - 输入
      - 配置范围:[-760, 760]，扇形校正强度系数，默认输入配置为0
    * - enMountMode
      - 输入
      - 配置范围:[0, 2]，鱼眼校正的安装模式，包括地装、顶装、和壁装三种安装模式
    * - enUseMode
      - 输入
      - 配置范围:[1, 9]，鱼眼校正的应用模式
    * - u32RegionNum
      - 输入
      - 配置范围:[1, 4]，鱼眼校正的区域数量
    * - enViewMode
      - 输入
      - 配置范围:[0, 3], 客户端的鱼眼视图模式，即不同的鱼眼摄像头观看模式
    * - grid_info
      - 输入
      - 用于存储 grid_info 的信息，包含 grid_info 的大小和数据

.. list-table:: 鱼眼校正的应用模式
    :header-rows: 1
    :widths: 50, 50

    * - 应用模式
      - 效果示例

    * - BMCV_MODE_PANORAMA_360
      -
        .. image:: ../_static/fisheye/image_fisheye_1.png
          :width: 3.00834in
          :height: 0.7in

    * - BMCV_MODE_PANORAMA_180
      -
        .. image:: ../_static/fisheye/image_fisheye_2.png
          :width: 3.00834in
          :height: 0.7in

    * - BMCV_MODE_01_1O
      -
        .. image:: ../_static/fisheye/image_fisheye_3.png
          :width: 3.00834in
          :height: 0.7in

    * - BMCV_MODE_02_1O4R
      -
        .. image:: ../_static/fisheye/image_fisheye_4.png
          :width: 3.00834in
          :height: 0.7in

    * - BMCV_MODE_03_4R
      -
        .. image:: ../_static/fisheye/image_fisheye_5.png
          :width: 3.00834in
          :height: 0.7in

    * - BMCV_MODE_04_1P2R
      -
        .. image:: ../_static/fisheye/image_fisheye_6.png
          :width: 3.00834in
          :height: 0.7in

    * - BMCV_MODE_05_1P2R
      -
        .. image:: ../_static/fisheye/image_fisheye_7.png
          :width: 3.00834in
          :height: 0.7in

    * - BMCV_MODE_06_1P
      -
        .. image:: ../_static/fisheye/image_fisheye_8.png
          :width: 3.00834in
          :height: 0.7in

    * - BMCV_MODE_07_2P
      -
        .. image:: ../_static/fisheye/image_fisheye_9.png
          :width: 3.00834in
          :height: 0.7in

注：BMCV_MODE_STEREO_FIT暂不支持。

.. list-table:: Fisheye Correction 客户端的鱼眼视图模式 enViewMode 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - BMCV_FISHEYE_VIEW_360_PANORAMA
      - 输入
      - 代表360度全景模式的鱼眼视图
    * - BMCV_FISHEYE_VIEW_180_PANORAMA
      - 输入
      - 代表180度全景模式的鱼眼视图
    * - BMCV_FISHEYE_VIEW_NORMAL
      - 输入
      - 代表普通视图模式
    * - BMCV_FISHEYE_NO_TRANSFORMATION
      - 输入
      - 代表没有任何变换的鱼眼视图模式
    * - BMCV_FISHEYE_VIEW_MODE_BUTT
      - 输入
      - 表示枚举的结束标记

| 【格式支持】

1. 输入和输出的数据类型：

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+

2. 输入和输出的色彩格式必须保持一致，可支持：

+-----+-------------------------------+
| num | image_format                  |
+=====+===============================+
|  1  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  2  | FORMAT_YUV420P                |
+-----+-------------------------------+
|  3  | FORMAT_YUV444P                |
+-----+-------------------------------+
|  4  | FORMAT_GRAY                   |
+-----+-------------------------------+

| 【注意】

1. 输入输出所有 bm_image 结构必须提前创建，否则返回失败。

2. 支持图像的分辨率为32x32~4096x4096，且要求32位对齐。

3. 若用户决定使用第一种方式进行图像校正，用户需根据鱼眼畸变的类型及校正模型自行输入配置参数列表 fisheye_attr，此时要将 grid_info 设置为空。

4. 若用户决定使用第二种方式进行图像校正，需提供 Grid_Info 文件，具体使用方式请参考下面的代码示例。

| 【代码示例】

| 1. 通过配置参数列表进行图像校正

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <pthread.h>
    #include <sys/time.h>
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>

    #define YUV_8BIT(y, u, v) ((((y)&0xff) << 16) | (((u)&0xff) << 8) | ((v)&0xff))

    extern void bm_read_bin(bm_image src, const char *input_name);
    extern void bm_write_bin(bm_image dst, const char *output_name);
    extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

    int main(int argc, char **argv) {
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle = NULL;
        int dev_id = 0;
        char *src_name = "fisheye_floor_1024x1024.yuv";
        char *dst_name = "out_fisheye_PANORAMA_360.yuv";
        int src_h = 1024, src_w = 1024;
        int dst_h = 720,  dst_w = 1280;
        bm_image src, dst;
        bm_image_format_ext fmt = FORMAT_YUV420P;
        int ret = (int)bm_dev_request(&handle, dev_id);
        bmcv_fisheye_attr_s fisheye_attr = {true, true, YUV_8BIT(0, 128, 128), 512, 512, 0, 0, 0, 1, 0, 1, NULL, };
        fisheye_attr.grid_info.u.system.system_addr = NULL;
        fisheye_attr.grid_info.size = 0;

        // create bm image
        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        // read image data from input files
        bm_read_bin(src, src_name);
        bmcv_dwa_fisheye(handle, src, dst, fisheye_attr);
        bm_write_bin(dst, dst_name);

        return 0;
    }

| 2. 通过 Grid_Info 文件进行图像校正

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <pthread.h>
    #include <sys/time.h>
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>

    typedef unsigned int u32;

    extern void bm_read_bin(bm_image src, const char *input_name);
    extern void bm_write_bin(bm_image dst, const char *output_name);
    extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

    int main(int argc, char **argv) {
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle = NULL;
        int dev_id = 0;
        char *src_name = "dc_src_2240x2240_L.yuv";
        char *dst_name = "out_fisheye_grid_info_L.yuv";
        char *grid_name = "L_grid_info_68_68_4624_70_70_dst_2240x2240_src_2240x2240.dat";
        int src_h = 2240, src_w = 2240;
        int dst_w = 2240, dst_h = 2240;
        bm_image src, dst;
        bm_image_format_ext fmt = FORMAT_YUV420P;
        int ret = (int)bm_dev_request(&handle, dev_id);
        bmcv_fisheye_attr_s fisheye_attr = {0};
        fisheye_attr.grid_info.size = 446496;     // 注意：用户需根据实际的Grid_Info文件大小（字节数）进行设置
        fisheye_attr.bEnable = true;

        // create bm image
        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        // read image data from input files
        bm_read_bin(src, src_name);

        // read grid_info data
        char *buffer = (char *)malloc(fisheye_attr.grid_info.size);
        if (buffer == NULL) {
            printf("malloc buffer for grid_info failed!\n");
            goto fail;
        }
        memset(buffer, 0, fisheye_attr.grid_info.size);

        FILE *fp = fopen(grid_name, "rb");
        if (!fp) {
            printf("open file:%s failed.\n", grid_name);
            goto fail;
        }
        fread(buffer, 1, fisheye_attr.grid_info.size, fp);
        fclose(fp);
        fisheye_attr.grid_info.u.system.system_addr = (void *)buffer;

        bmcv_dwa_fisheye(handle, src, dst, fisheye_attr);
        bm_write_bin(dst, dst_name);

        return 0;
    }