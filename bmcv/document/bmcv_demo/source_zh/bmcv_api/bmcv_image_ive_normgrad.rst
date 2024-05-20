bmcv_ive_normgrad
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建归一化梯度计算任务，梯度分量均归一化到 S8。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_normgrad(
        bm_handle_t             handle,
        bm_image  *             input,
        bm_image  *             output_h,
        bm_image  *             output_v,
        bm_image  *             output_hv,
        bmcv_ive_normgrad_ctrl    normgrad_attr);

| 【参数】

.. list-table:: bmcv_ive_normgrad 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*input
      - 输入
      - 输入 bm_image 对象数据指针, 不能为空。
    * - \*output_h
      - 输出
      - 输出 bm_image 对象数据指针, 由模板直接滤波并归一到 s8 后得到的梯度分量图像 (H) 指针, 根据 normgrad_attr.en_mode, 若需要输出，则不能为空。
    * - \*output_v
      - 输出
      - 输出 bm_image 对象数据指针, 由模板直接滤波并归一到 s8 后得到的梯度分量图像 (V) 指针, 根据 normgrad_attr.en_mode, 若需要输出则不能为空。
    * - \*output_hv
      - 输出
      - 输出 bm_image 对象数据指针, 由模板和转置后的模板直接滤波, 并归一到 s8 后采用 package 格式存储的图像指针。根据 normgrad_attr.en_mode, 若需要输出则不为空。
    * - \normgrad_attr
      - 输入
      - 归一化梯度信息计算参数, 不能为空。

.. list-table::
    :widths: 21 18 72 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output_h
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE_SIGNED
      - 64x64~1920x1080
    * - output_v
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE_SIGNED
      - 64x64~1920x1080
    * - output_hv
      - GRAY
      - DATA_TYPE_EXT_U16
      - 64x64~1920x1080


【数据类型说明】

【说明】定义归一化梯度信息计算任务输出控制枚举类型。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_normgrad_outmode_e{
        BM_IVE_NORM_GRAD_OUT_HOR_AND_VER = 0x0,
        BM_IVE_NORM_GRAD_OUT_HOR         = 0x1,
        BM_IVE_NORM_GRAD_OUT_VER         = 0x2,
        BM_IVE_NORM_GRAD_OUT_COMBINE     = 0x3,
    } bmcv_ive_normgrad_outmode;

.. list-table::
    :widths: 125 80

    * - **成员名称**
      - **描述**
    * - BM_IVE_NORM_GRAD_OUT_HOR_AND_VER
      - 同时输出梯度信息的H、V 分量图。
    * - BM_IVE_NORM_GRAD_OUT_HOR
      - 仅输出梯度信息的 H 分量图。
    * - BM_IVE_NORM_GRAD_OUT_VER
      - 仅输出梯度信息的 V 分量图。
    * - BM_IVE_NORM_GRAD_OUT_COMBINE
      - 输出梯度信息以 package 存储的 HV 图。

【说明】定义归一化梯度信息计算任务输出控制枚举类型。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_normgrad_ctrl_s{
        bmcv_ive_normgrad_outmode en_mode;
        signed char as8_mask[25];
        unsigned char u8_norm;
    } bmcv_ive_normgrad_ctrl;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - 梯度信息输出控制模式。
    * - as8_mask
      - as8Mask 计算梯度需要的模板。
    * - u8_norm
      - 归一化参数, 取值范围: [1, 13]。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 控制参数中输出模式如下：
   - BM_IVE_NORM_GRAD_OUT_HOR_AND_VER 时, output_h 和 output_v 指针不能为空, 且要求跨度一致。

   - BM_IVE_NORM_GRAD_OUT_HOR 时, output_h 不能为空。

   - BM_IVE_NORM_GRAD_OUT_VER 时, output_v 不能为空。

   - BM_IVE_NORM_GRAD_OUT_COMBINE 时, output_hv 不能为空。

3. 计算公式如下：

  .. math::

    I_{\text{out}}(x, y) = \left\{ \sum_{-2 < j < 2} \sum_{-2 < i < 2} I(x+i, y+j)
                           \cdot \text{coef}(i, j) \right\} \gg \text{norm}(x)


**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <pthread.h>
      #include <math.h>
      #include <sys/time.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>
      extern void bm_ive_read_bin(bm_image src, const char *input_name);
      extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
          bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);
      int main(){
      int ret = (int)bm_dev_request(&handle, dev_id);
      if (ret != 0) {
          printf("Create bm handle failed. ret = %d\n", ret);
          exit(-1);
      }
      int dev_id = 0;int height = 288, width = 352;
      int thrSize = 0; // 0 -> 3x3  1-> 5x5
      bmcv_ive_normgrad_outmode enMode = BM_IVE_NORM_GRAD_OUT_HOR_AND_VER;
      bm_image_format_ext fmt = FORMAT_GRAY;
      char *src_name = "./data/00_352x288_y.yuv";
      char *dst_hName = "ive_normGradH_res.yuv", *dst_vName = "ive_normGradV_res.yuv";
      char *dst_hvName = "ive_normGradHV_res.yuv";
      bm_handle_t handle = NULL;
      /* 5 by 5*/
      signed char arr5by5[25] = { -1, -2, 0,  2,  1, -4, -8, 0,  8,  4, -6, -12, 0,
                        12, 6,  -4, -8, 0, 8,  4,  -1, -2, 0, 2, 1 };
      /* 3 by 3*/
      signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                        2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };
      bm_image src;
      bm_image dst_H, dst_V, dst_conbine_HV;
      int src_stride[4];
      int dst_stride[4], dst_combine_stride[4];
      unsigned int i = 0, loop_time = 0;
      unsigned long long time_single, time_total = 0, time_avg = 0;
      unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
      struct timeval tv_start;
      struct timeval tv_end;
      struct timeval timediff;
      bmcv_ive_normgrad_ctrl normgrad_attr;
      normgrad_attr.u8_norm = 8;
      normgrad_attr.en_mode = enMode;
      (thrSize == 0) ? (memcpy(normgrad_attr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char))) :
                        (memcpy(normgrad_attr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char)));

      // calc ive image stride && create bm image struct
      bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
      bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
      ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
      if (ret != BM_SUCCESS) {
          printf("src bm_image_alloc_dev_mem_src. ret = %d\n", ret);
          exit(-1);
      }
      bm_ive_read_bin(src, src_name);

      if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_HOR){
          bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, dst_stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, &dst_H, dst_stride);
          ret = bm_image_alloc_dev_mem(dst_H, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("dst_H bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              exit(-1);
          }
      }

      if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_VER){
          bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, dst_stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, &dst_V, dst_stride);
          ret = bm_image_alloc_dev_mem(dst_V, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("dst_V bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              exit(-1);
          }
      }

      if(enMode == BM_IVE_NORM_GRAD_OUT_COMBINE){
          bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_U16, dst_combine_stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &dst_conbine_HV, dst_combine_stride);
          ret = bm_image_alloc_dev_mem(dst_conbine_HV, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("dst_conbine_HV bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              exit(-1);
          }
      }

      for (i = 0; i < loop_time; i++) {
          gettimeofday(&tv_start, NULL);
          ret = bmcv_ive_norm_grad(handle, &src, &dst_H, &dst_V, &dst_conbine_HV, normgrad_attr);
          gettimeofday(&tv_end, NULL);
          timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
          timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
          time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

          if(time_single>time_max){time_max = time_single;}
          if(time_single<time_min){time_min = time_single;}
          time_total = time_total + time_single;
          if(ret != BM_SUCCESS){
              printf("bmcv_ive_normgrad failed. ret = %d\n", ret);
              exit(-1);
          }
      }

      time_avg = time_total / loop_time;
      fps_actual = 1000000 / time_avg;
      bm_image_destroy(&src);
      if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_HOR)
          bm_image_destroy(&dst_H);
      if(enMode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER || enMode == BM_IVE_NORM_GRAD_OUT_VER)
          bm_image_destroy(&dst_V);
      if(enMode == BM_IVE_NORM_GRAD_OUT_COMBINE) bm_image_destroy(&dst_conbine_HV);

      char fmt_str[100], thr_str[50], modeStr[100];
      format_to_str(src.image_format, fmt_str);
      (thrSize == 0) ? memcpy(thr_str, "3x3", 50) : memcpy(thr_str, "5x5", 50);

      normGradMode_to_str(normgrad_attr.en_mode, modeStr);

      printf("bmcv_ive_normgrad: format %s, normgradOutMode is %s , thrSize is %s, size %d x %d \n",
              fmt_str, modeStr, thr_str, width, height);
      printf(" bmcv_ive_normgrad: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
              loop_time, time_max, time_avg, fps_actual);
      printf("bmcv ive normgrad test successful \n");

      return 0;
      }





































