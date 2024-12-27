bmcv_ive_gmm
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 GMM 背景建模任务，支持灰度图、 RGB_PACKAGE 图像的 GMM 背景建模， 高斯模型个数为 3 或者 5。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_gmm(
        bm_handle_t         handle,
        bm_image            input,
        bm_image            output_fg,
        bm_image            output_bg,
        bm_device_mem_t     output_model,
        bmcv_ive_gmm_ctrl   gmm_attr);

| 【参数】

.. list-table:: bmcv_ive_gmm 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \output_fg
      - 输出
      - 输出 bm_image 对象结构体, 表示前景图像, 不能为空, 宽、高同 input。
    * - \output_bg
      - 输出
      - 输出 bm_image 对象结构体, 表示背景图像, 不能为空, 宽、高同 input。
    * - \output_model
      - 输入、输出
      - bm_device_mem_t 对象结构体, 对应GMM 模型参数, 不能为空。
    * - gmm_attr
      - 输入
      - 控制信息结构体，不能为空。

.. list-table::
    :widths: 25 42 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY

        RGB_PACKED
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output_fg
      - GRAY 二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input
    * - output_bg
      - 同 input
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义 GMM 背景建模的控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_gmm_ctrl_s{
        unsigned int u22q10_noise_var;
        unsigned int u22q10_max_var;
        unsigned int u22q10_min_var;
        unsigned short u0q16_learn_rate;
        unsigned short u0q16_bg_ratio;
        unsigned short u8q8_var_thr;
        unsigned short u0q16_init_weight;
        unsigned char u8_model_num;
    } bmcv_ive_gmm_ctrl;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - u22q10_noise_var
      - 初始噪声方差。

        取值范围：[0x1, 0xFFFFFF]。
    * - u22q10_max_var
      - 模型方差的最大值。

        取值范围：[0x1, 0xFFFFFF]。
    * - u22q10_min_var
      - 模型方差的最小值。

        取值范围：[0x1, u22q10MaxVar]。
    * - u0q16_learn_rate
      - 学习速率。

        取值范围: [1, 65535]。
    * - u0q16_bg_ratio
      - 背景比例阈值。

        取值范围: [1, 65535]。
    * - u8q8_var_thr
      - 方差阈值。

        取值范围: [1, 65535]。
    * - u0q16_init_weight
      - 初始权重。

        取值范围: [1, 65535]。
    * - u8_model_num
      - 模型个数。

        取值范围: {3, 5}。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. GMM 的实现方式参考了 OpenCV 中的 MOG 和 MOG2。

3. 源图像类型只能为 U8C1 或 U8C3_PACKAGE, 分别用于灰度图和 RGB 图的 GMM 背景建模。

4. 前景图像是二值图，类型只能为 U8C1; 背景图像与源图像类型一致。

5. 灰度图像 或 RGB 图像 GMM 采用 n 个(n=3 或 5)高斯模型。


**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <math.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

      int main(){
          int dev_id = 0;
          int height = 1080, width = 1920;
          bm_image_format_ext src_fmt = FORMAT_GRAY;
          char *input_name = "path/to/input";
          char *dstFg_name = "path/to/dst_Fg", *dstBg_name = "path/to/dst_Bg";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          bm_image src;
          bm_image dst_fg, dst_bg;
          bm_device_mem_t dst_model;
          int stride[4];

          unsigned int u32FrameNumMax = 32;
          unsigned int u32FrmCnt = 0;

          bmcv_ive_gmm_ctrl gmmAttr;
          gmmAttr.u0q16_bg_ratio = 45875;
          gmmAttr.u0q16_init_weight = 3277;
          gmmAttr.u22q10_noise_var = 225 * 1024;
          gmmAttr.u22q10_max_var = 2000 * 1024;
          gmmAttr.u22q10_min_var = 200 * 1024;
          gmmAttr.u8q8_var_thr = (unsigned short)(256 * 6.25);
          gmmAttr.u8_model_num = 3;

          int model_len = width * height * gmmAttr.u8_model_num * 8;

          unsigned char* inputData = malloc(width * height * u32FrameNumMax * sizeof(unsigned char));
          FILE *input_fp = fopen(input_name, "rb");
          fread((void *)inputData, 1, width * height * u32FrameNumMax * sizeof(unsigned char), input_fp);
          fclose(input_fp);

          unsigned char* srcData = malloc(width * height * sizeof(unsigned char));
          unsigned char* ive_fg_res = malloc(width * height * sizeof(unsigned char));
          unsigned char* ive_bg_res = malloc(width * height * sizeof(unsigned char));
          unsigned char* model_data = malloc(model_len * sizeof(unsigned char));

          memset(srcData, 0, width * height * sizeof(unsigned char));
          memset(ive_fg_res, 0, width * height * sizeof(unsigned char));
          memset(ive_bg_res, 0, width * height * sizeof(unsigned char));
          memset(model_data, 0, model_len * sizeof(unsigned char));

          // calc ive image stride && create bm image struct
          int data_size = 1;
          stride[0] = align_up(width, 16) * data_size;
          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
          bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_fg, stride);
          bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_bg, stride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst_fg, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst_bg, BMCV_HEAP1_ID);

          ret = bm_malloc_device_byte(handle, &dst_model, model_len);

          ret = bm_memcpy_s2d(handle, dst_model, model_data);
          for(u32FrmCnt = 0; u32FrmCnt < u32FrameNumMax; u32FrmCnt++){
              if(width > 480 ){
                  for(int j = 0; j < 288; j++){
                      memcpy(srcData + (j * width),
                              inputData + (u32FrmCnt * 352 * 288 + j * 352), 352);
                      memcpy(srcData + (j * width + 352),
                              inputData + (u32FrmCnt * 352 * 288 + j * 352), 352);
                  }
              } else {
                  for(int j = 0; j < 288; j++){
                      memcpy(srcData + j * stride[0],
                              inputData + u32FrmCnt * width * 288 + j * width, width);
                      int s = stride[0] - width;
                      memset(srcData + j * stride[0] + width, 0, s);
                  }
              }

              ret = bm_image_copy_host_to_device(src, (void**)&srcData);

              if(u32FrmCnt >= 500)
                  gmmAttr.u0q16_learn_rate = 131;  //0.02
              else
                  gmmAttr.u0q16_learn_rate = 65535 / (u32FrmCnt + 1);

              ret = bmcv_ive_gmm(handle, src, dst_fg, dst_bg, dst_model, gmmAttr);
          }

          ret = bm_image_copy_device_to_host(dst_fg, (void **)&ive_fg_res);
          ret = bm_image_copy_device_to_host(dst_bg, (void **)&ive_bg_res);

          FILE *fg_fp = fopen(dstFg_name, "wb");
          fwrite((void *)ive_fg_res, 1, width * height, fg_fp);
          fclose(fg_fp);

          FILE *bg_fp = fopen(dstBg_name, "wb");
          fwrite((void *)ive_bg_res, 1, width * height, bg_fp);
          fclose(bg_fp);

          free(inputData);
          free(ive_fg_res);
          free(srcData);
          free(ive_bg_res);
          free(model_data);
          bm_image_destroy(&src);
          bm_image_destroy(&dst_fg);
          bm_image_destroy(&dst_bg);
          bm_free_device(handle, dst_model);

          bm_dev_free(handle);
          return 0;
      }