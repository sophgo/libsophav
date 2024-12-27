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

2. 支持图像的分辨率为32x32~4096x4096，且要求32对齐。

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
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>

    #define YUV_8BIT(y, u, v) ((((y)&0xff) << 16) | (((u)&0xff) << 8) | ((v)&0xff))

    int main() {
      int src_h = 1024, src_w = 1024, dst_w = 1280, dst_h = 720, dev_id = 0;
      int yuv_8bit_y = 0, yuv_8bit_u = 0, yuv_8bit_v = 0;
      bm_image_format_ext fmt = FORMAT_YUV420P;
      char *src_name = "path/to/src", *dst_name = "path/to/dst";
      bm_handle_t handle = NULL;
      bmcv_fisheye_attr_s fisheye_attr = {0};
      dst_w = 1280;
      dst_h = 720;
      fmt = FORMAT_RGB_PLANAR;
      fisheye_attr.bEnable = 1;
      fisheye_attr.bBgColor = 1;
      yuv_8bit_y = 0;
      yuv_8bit_u = 128;
      yuv_8bit_v = 128;
      fisheye_attr.u32BgColor = YUV_8BIT(yuv_8bit_y, yuv_8bit_u, yuv_8bit_v);
      fisheye_attr.s32HorOffset = src_w / 2;
      fisheye_attr.s32VerOffset = src_h / 2;
      fisheye_attr.u32TrapezoidCoef = 0;
      fisheye_attr.s32FanStrength = 0;
      fisheye_attr.enMountMode = 0;
      fisheye_attr.enUseMode = 1;
      fisheye_attr.enViewMode = 0;
      fisheye_attr.u32RegionNum = 1;
      fisheye_attr.grid_info.u.system.system_addr = NULL;
      fisheye_attr.grid_info.size = 0;
      dst_name = "dwa_fisheye_output_rand.yuv";
      // rand_mode = 1;
      int ret = (int)bm_dev_request(&handle, dev_id);
      if (ret != 0) {
          printf("Create bm handle failed. ret = %d\n", ret);
          exit(-1);
      }
      bm_image src, dst;

      bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
      bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

      ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
      ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

      int image_byte_size[4] = {0};
      bm_image_get_byte_size(src, image_byte_size);
      int byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
      unsigned char *input_data = (unsigned char *)malloc(byte_size);
      FILE *fp_src = fopen(src_name, "rb");
      if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
        printf("file size is less than required bytes%d\n", byte_size);
      };
      fclose(fp_src);
      void* in_ptr[4] = {(void *)input_data,
                          (void *)((unsigned char*)input_data + image_byte_size[0]),
                          (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1]),
                          (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
      bm_image_copy_host_to_device(src, in_ptr);

      bmcv_dwa_fisheye(handle, src, dst, fisheye_attr);

      bm_image_get_byte_size(src, image_byte_size);
      byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
      unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
      void* out_ptr[4] = {(void*)output_ptr,
                          (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                          (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                          (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
      bm_image_copy_device_to_host(src, (void **)out_ptr);

      FILE *fp_dst = fopen(dst_name, "wb");
      if (fwrite((void *)input_data, 1, byte_size, fp_dst) < (unsigned int)byte_size){
          printf("file size is less than %d required bytes\n", byte_size);
      };
      fclose(fp_dst);

      free(input_data);
      free(output_ptr);
      bm_image_destroy(&src);
      bm_image_destroy(&dst);

      bm_dev_free(handle);

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
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>

    int main() {
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle = NULL;
        int dev_id = 0;

        char *src_name = "path/to/src", *dst_name = "path/to/dst", *grid_name = "path/to/grid_info_dat";

        int src_h = 2240, src_w = 2240;
        int dst_w = 2240, dst_h = 2240;
        bm_image src, dst;
        bm_image_format_ext fmt = FORMAT_YUV420P;
        ret = (int)bm_dev_request(&handle, dev_id);
        bmcv_fisheye_attr_s fisheye_attr = {0};
        fisheye_attr.grid_info.size = 446496;     // 注意：用户需根据实际的Grid_Info文件大小（字节数）进行设置
        fisheye_attr.bEnable = true;

        // create bm image
        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        // read image data from input files
        int image_byte_size[4] = {0};
        bm_image_get_byte_size(src, image_byte_size);
        int byte_size = src_w * src_h * 3 / 2;
        unsigned char *input_data = (unsigned char *)malloc(byte_size);
        FILE *fp_src = fopen(src_name, "rb");
        if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
          printf("file size is less than required bytes%d\n", byte_size);
        };
        fclose(fp_src);
        void* in_ptr[3] = {(void *)input_data, (void *)((unsigned char*)input_data + src_w * src_h), (void *)((unsigned char*)input_data + 5 / 4 * src_w * src_h)};
        bm_image_copy_host_to_device(src, in_ptr);
        // read grid_info data
        char *buffer = (char *)malloc(fisheye_attr.grid_info.size);
        memset(buffer, 0, fisheye_attr.grid_info.size);

        FILE *fp = fopen(grid_name, "rb");
        fread(buffer, 1, fisheye_attr.grid_info.size, fp);
        fclose(fp);
        fisheye_attr.grid_info.u.system.system_addr = (void *)buffer;

        bmcv_dwa_fisheye(handle, src, dst, fisheye_attr);
        bm_image_get_byte_size(dst, image_byte_size);
        unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
        void* out_ptr[3] = {(void*)output_ptr, (void*)((unsigned char*)output_ptr + dst_w * dst_h), (void*)((unsigned char*)output_ptr + 5 / 4 * dst_w * dst_h)};
        bm_image_copy_device_to_host(dst, (void **)out_ptr);

        FILE *fp_dst = fopen(dst_name, "wb");
        if (fwrite((void *)output_ptr, 1, byte_size, fp_dst) < (unsigned int)byte_size){
            printf("file size is less than %d required bytes\n", byte_size);
        };
        fclose(fp_dst);
        return ret;
    }
