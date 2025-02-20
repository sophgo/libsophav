bmcv_ive_gradfg
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 根据背景图像和当前帧图像的梯度信息计算梯度前景图像。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_gradfg(
        bm_handle_t           handle,
        bm_image              input_bgdiff_fg,
        bm_image              input_curgrad,
        bm_image              intput_bggrad,
        bm_image              output_gradfg,
        bmcv_ive_gradfg_attr    gradfg_attr);

| 【参数】

.. list-table:: bmcv_ive_gradfg 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input_bgdiff_fg
      - 输入
      - 背景差分前景图像bm_image结构体, 不能为空。
    * - \input_curgrad
      - 输入
      - 当前帧梯度图像bm_image结构体, 不能为空。
    * - \intput_bggrad
      - 输入
      - 背景梯度图像bm_image结构体, 不能为空，宽、高同 input_curgrad。
    * - \output_gradfg
      - 输出
      - 梯度前景图像bm_image结构体, 不能为空，宽、高同 input_curgrad。
    * - \gradfg_attr
      - 输入
      - 计算梯度前景控制参数结构体，不能为空。

.. list-table::
    :widths: 35 25 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input_bgdiff_fg
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - input_curgrad
      - GRAY
      - DATA_TYPE_EXT_U16
      - 同 input
    * - intput_bggrad
      - GRAY
      - DATA_TYPE_EXT_U16
      - 同 input
    * - output_gradfg
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义梯度前景计算模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_gradfg_mode_e{
        GRAD_FG_MODE_USE_CUR_GRAD = 0x0,
        GRAD_FG_MODE_FIND_MIN_GRAD = 0x1,
    } bmcv_ive_gradfg_mode;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - GRAD_FG_MODE_USE_CUR_GRAD
      - 当前位置梯度计算模式。
    * - GRAD_FG_MODE_FIND_MIN_GRAD
      - 周边最小梯度计算模式。

【说明】定义计算梯度前景控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_gradfg_attr_s{
        bmcv_ive_gradfg_mode en_mode;
        unsigned short u16_edw_factor;
        unsigned char u8_crl_coef_thr;
        unsigned char u8_mag_crl_thr;
        unsigned char u8_min_mag_diff;
        unsigned char u8_noise_val;
        unsigned char u8_edw_dark;
    } bmcv_ive_gradfg_attr;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - 梯度前景计算模式。参考 bm_ive_gradfg_mode。
    * - u16_edw_factor
      - 边缘宽度调节因子。

        取值范围: [500, 2000], 参考取值: 100。
    * - u8_crl_coef_thr
      - 梯度向量相关系数阈值。

        取值范围: [50, 100], 参考取值: 80。
    * - u8_mag_crl_thr
      - 梯度幅度阈值。

        取值范围: [0, 20], 参考取值: 4。
    * - u8_min_mag_diff
      - 梯度幅值差值阈值。

        取值范围: [2, 8], 参考取值: 2。
    * - u8_noise_val
      - 梯度幅值差值阈值。

        取值范围: [1, 8], 参考取值: 1。
    * - u8_edw_dark
      - 黑像素使能标志。

        0 表示不开启, 1 表示开启，参考取值: 1。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 背景梯度图像和当前梯度图像的类型为 DATA_TYPE_EXT_U16, 水平和竖直方向梯度按照 :math:`[xyxyxyxy...]` 格式存储。


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
          bm_image_format_ext fmt = FORMAT_GRAY;
          bmcv_ive_gradfg_mode gradfg_mode = GRAD_FG_MODE_USE_CUR_GRAD;
          char *src_name = "path/to/src";
          char *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;
          /* 3 by 3*/
          signed char arr3by3[25] = { 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0, -2, 0,
                          2, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0 };
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bm_image stBgdiffFg;
          bm_image stCurGrad, stBgGrad, stGragFg;
          int u8Stride[4], u16Stride[4];

          // config setting

          // normGrad config
          bmcv_ive_normgrad_ctrl normGradAttr;
          normGradAttr.en_mode = BM_IVE_NORM_GRAD_OUT_COMBINE;
          normGradAttr.u8_norm = 8;
          memcpy(&normGradAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

          bmcv_ive_gradfg_attr gradFgAttr;
          gradFgAttr.en_mode = gradfg_mode;
          gradFgAttr.u16_edw_factor = 1000;
          gradFgAttr.u8_crl_coef_thr = 80;
          gradFgAttr.u8_mag_crl_thr = 4;
          gradFgAttr.u8_min_mag_diff = 2;
          gradFgAttr.u8_noise_val = 1;
          gradFgAttr.u8_edw_dark = 1;

          // calc ive image stride && create bm image struct
          int data_size = 1;
          u8Stride[0] = align_up(width, 16) * data_size;
          data_size = 2;
          u16Stride[0] = align_up(width, 16) * data_size;


          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stBgdiffFg, u8Stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &stCurGrad, u16Stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &stBgGrad, u16Stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stGragFg, u8Stride);

          ret = bm_image_alloc_dev_mem(stBgdiffFg, BMCV_HEAP1_ID);

          int image_byte_size[4] = {0};
          bm_image_get_byte_size(stBgdiffFg, image_byte_size);
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
          bm_image_copy_host_to_device(stBgdiffFg, in_ptr);


          ret = bm_image_alloc_dev_mem(stCurGrad, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(stBgGrad, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(stGragFg, BMCV_HEAP1_ID);

          // Create Fake Data.
          ret = bmcv_ive_norm_grad(handle, &stBgdiffFg, NULL, NULL, &stCurGrad, normGradAttr);

          normGradAttr.u8_norm = 2;
          ret = bmcv_ive_norm_grad(handle, &stBgdiffFg, NULL, NULL, &stBgGrad, normGradAttr);
          ret = bmcv_ive_gradfg(handle, stBgdiffFg, stCurGrad, stBgGrad, stGragFg, gradFgAttr);

          unsigned char *gradFg_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
          memset(gradFg_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(stGragFg, (void**)&gradFg_res);

          FILE *ive_fp = fopen(dst_name, "wb");
          fwrite((void *)gradFg_res, 1, width * height, ive_fp);
          fclose(ive_fp);
          free(gradFg_res);

          bm_image_destroy(&stBgdiffFg);
          bm_image_destroy(&stCurGrad);
          bm_image_destroy(&stBgGrad);
          bm_image_destroy(&stGragFg);

          bm_dev_free(handle);
          return 0;
      }