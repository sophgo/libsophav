bmcv_ive_mag_and_ang
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 5x5 模板，计算灰度图的每个像素区域位置灰度值变化梯度的幅值和幅角任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_mag_and_ang(
        bm_handle_t            handle,
        bm_image  *             input,
        bm_image  *            mag_output,
        bm_image  *            ang_output,
        bmcv_ive_mag_and_ang_ctrl  attr);

| 【参数】

.. list-table:: bmcv_ive_magandang 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*input
      - 输入
      - 输入 bm_image 对象指针, 不能为空。
    * - \*mag_output
      - 输出
      - 输出 bm_image 对象数据指针, 输出幅值图像, 不能为空, 宽、高同 input。
    * - \*ang_output
      - 输出
      - 输出 bm_image 对象数据指针, 输出幅角图像。根据 attr.en_out_ctrl, 需要输出则不能为空, 宽、高同 input。
    * - \attr
      - 输入
      - 控制信息结构体, 存放梯度幅值和幅角计算的控制信息，不能为空。


.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - mag_output
      - GRAY
      - DATA_TYPE_EXT_U16
      - 同 input
    * - ang_output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input


| 【数据类型说明】

【说明】定义梯度幅值与角度计算的输出格式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_mag_and_ang_outctrl_e{
        BM_IVE_MAG_AND_ANG_OUT_MAG = 0x0,
        BM_IVE_MAG_AND_ANG_OUT_ALL = 0X1,
    } bmcv_ive_mag_and_ang_outctrl;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_MAG_AND_ANG_OUT_MAG
      - 仅输出幅值。
    * - BM_IVE_MAG_AND_ANG_OUT_ALL
      - 同时输出幅值和角度值。

【说明】定义梯度幅值和幅角计算的控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_mag_and_ang_ctrl_s{
        bmcv_ive_mag_and_ang_outctrl     en_out_ctrl;
        unsigned short               u16_thr;
        signed char                  as8_mask[25];
    } bmcv_ive_mag_and_ang_ctrl;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - en_out_ctrl
      - 输出格式。
    * - u16_thr
      - 用于对幅值进行阈值化的阈值。
    * - as8_mask
      - 5x5 模板系数。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 可配置两种输出模式, 详细请参考 bm_ive_mag_and_ang_outctrl。

3. 配置 BM_IVE_MAG_AND_ANG_OUT_ALL 时， 要求 mag_output 与 ang_output 跨度一致。

4. 用户可以通过 attr.u16_thr 对幅值图进行阈值化操作(可用来实现EOH), 计算公式如下:

  .. math::

    \text{Mag}(x, y) =
    \begin{cases}
        \text{0} & \ (Mag(x, y) < u16_thr) \\
        \text{Mag(x, y)} & \ (Mag(x, y) \geq u16_thr) \\
    \end{cases}

  其中, :math:`Mag(x, y)` 对应 mag_output。


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
          // int templateSize = 0; // 0: 3x3 1:5x5
          unsigned short thrValue = 0;
          bm_image_format_ext fmt = FORMAT_GRAY;
          bmcv_ive_mag_and_ang_outctrl enMode = BM_IVE_MAG_AND_ANG_OUT_ALL;
          char *src_name = "path/to/src";
          char *dst_magName = "path/to/mag_dst", *dst_angName = "path/to/ang_dst";
          /* 3 by 3*/
          signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                          2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          bm_image src;
          bm_image dst_mag, dst_ang;
          int stride[4];
          int magStride[4];

          bmcv_ive_mag_and_ang_ctrl magAndAng_attr;
          memset(&magAndAng_attr, 0, sizeof(bmcv_ive_mag_and_ang_ctrl));
          magAndAng_attr.en_out_ctrl = enMode;
          magAndAng_attr.u16_thr = thrValue;
          memcpy(magAndAng_attr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

          // calc ive image stride && create bm image struct
          int data_size = 1;
          stride[0] = align_up(width, 16) * data_size;
          data_size = 2;
          magStride[0] = align_up(width, 16) * data_size;

          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &dst_mag, magStride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst_mag, BMCV_HEAP1_ID);

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


          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst_ang, stride);
          ret = bm_image_alloc_dev_mem(dst_ang, BMCV_HEAP1_ID);

          ret = bmcv_ive_mag_and_ang(handle, &src, &dst_mag, &dst_ang, magAndAng_attr);

          unsigned short* magOut_res = malloc(width * height * sizeof(unsigned short));
          unsigned char*  angOut_res = malloc(width * height * sizeof(unsigned char));
          memset(magOut_res, 0, width * height * sizeof(unsigned short));
          memset(angOut_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(dst_mag, (void **)&magOut_res);
          ret = bm_image_copy_device_to_host(dst_ang, (void **)&angOut_res);


          FILE *mag_fp = fopen(dst_magName, "wb");
          fwrite((void *)magOut_res, 1, width * height, mag_fp);
          fclose(mag_fp);

          FILE *ang_fp = fopen(dst_angName, "wb");
          fwrite((void *)angOut_res, 1, width * height, ang_fp);
          fclose(ang_fp);

          free(input_data);
          free(magOut_res);
          free(angOut_res);

          bm_image_destroy(&src);
          bm_image_destroy(&dst_mag);
          bm_image_destroy(&dst_ang);

          bm_dev_free(handle);
          return 0;
      }