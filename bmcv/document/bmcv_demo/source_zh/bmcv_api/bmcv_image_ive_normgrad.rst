bmcv_ive_normgrad
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建归一化梯度计算任务，梯度分量均归一化到 S8。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_norm_grad(
        bm_handle_t             handle,
        bm_image  *             input,
        bm_image  *             output_h,
        bm_image  *             output_v,
        bm_image  *             output_hv,
        bmcv_ive_normgrad_ctrl    normgrad_attr);

| 【参数】

.. list-table:: bmcv_ive_norm_grad 参数表
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
      #include <math.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

      int main(){
          int dev_id = 0;
          int height = 1080, width = 1920;
          bmcv_ive_normgrad_outmode enMode = BM_IVE_NORM_GRAD_OUT_HOR_AND_VER;
          bm_image_format_ext fmt = FORMAT_GRAY;
          char *src_name = "path/to/src";
          char *dst_hName = "path/to/dst_h", *dst_vName = "path/to/dst_v";

          bm_handle_t handle = NULL;
          /* 3 by 3*/
          signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                          2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bm_image src;
          bm_image dst_H, dst_V, dst_conbine_HV;
          int src_stride[4];
          int dst_stride[4];

          bmcv_ive_normgrad_ctrl normgrad_attr;
          normgrad_attr.u8_norm = 8;
          normgrad_attr.en_mode = enMode;
          (memcpy(normgrad_attr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char)));

          // calc ive image stride && create bm image struct
          int data_size = 1;
          src_stride[0] = align_up(width, 16) * data_size;
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);

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

          dst_stride[0] = align_up(width, 16) * data_size;
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, &dst_H, dst_stride);
          ret = bm_image_alloc_dev_mem(dst_H, BMCV_HEAP1_ID);


          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE_SIGNED, &dst_V, dst_stride);
          ret = bm_image_alloc_dev_mem(dst_V, BMCV_HEAP1_ID);

          ret = bmcv_ive_norm_grad(handle, &src, &dst_H, &dst_V, &dst_conbine_HV, normgrad_attr);

          signed char* ive_h_res = malloc(width * height * sizeof(signed char));
          signed char* ive_v_res = malloc(width * height * sizeof(signed char));
          unsigned short* ive_combine_res = malloc(width * height * sizeof(unsigned short));

          memset(ive_h_res, 0, width * height * sizeof(signed char));
          memset(ive_v_res, 0, width * height * sizeof(signed char));
          memset(ive_combine_res, 1, width * height * sizeof(unsigned short));

          ret = bm_image_copy_device_to_host(dst_H, (void**)&ive_h_res);
          ret = bm_image_copy_device_to_host(dst_V, (void**)&ive_v_res);

          FILE *iveH_fp = fopen(dst_hName, "wb");
          fwrite((void *)ive_h_res, sizeof(signed char), width * height, iveH_fp);
          fclose(iveH_fp);

          FILE *iveV_fp = fopen(dst_vName, "wb");
          fwrite((void *)ive_v_res, sizeof(signed char), width * height, iveV_fp);
          fclose(iveV_fp);

          free(input_data);
          free(ive_h_res);
          free(ive_v_res);
          free(ive_combine_res);

          bm_image_destroy(&dst_H);
          bm_image_destroy(&dst_V);

          bm_dev_free(handle);
          return ret;
      }