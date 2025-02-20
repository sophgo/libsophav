bmcv_ive_sad
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 计算两幅图像按照 4x4/8x8/16x16 分块的 16bit/8bit SAD 图像, 以及对 SAD 进行阈值化输出。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_sad(
          bm_handle_t                handle,
          bm_image *                 input,
          bm_image *                 output_sad,
          bm_image *                 output_thr,
          bmcv_ive_sad_attr *        sad_attr,
          bmcv_ive_sad_thresh_attr*  thresh_attr);

| 【参数】

.. list-table:: bmcv_ive_sad 参数表
    :widths: 15 15 60

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*input
      - 输入
      - 输入 bm_image 对象数组，存放两个输入图像, 不能为空。
    * - \*output_sad
      - 输出
      - 输出 bm_image 对象数据指针, 输出 SAD 图像指针, 根据 sad_attr->en_out_ctrl, 若需要输出则不能为空。根据 sad_attr->en_mode, 对应 4x4、8x8、16x16 分块模式, 高、宽分别为 input1 的 1/4、1/8、1/16。
    * - \*output_thr
      - 输出
      - 输出 bm_image 对象数据指针, 输出 SAD 阈值化图像指针, 根据 sad_attr->en_out_ctrl, 若需要输出则不能为空。根据 sad_attr->en_mode, 对应 4x4、8x8、16x16 分块模式, 高、宽分别为 input1 的 1/4、1/8、1/16。
    * - \*sad_attr
      - 输入
      - SAD 控制参数指针结构体, 不能为空。
    * - \*thresh_attr
      - 输入
      - SAD thresh模式需要的阈值参数结构体指针，非thresh模式情况下，传入值但是不起作用。

.. list-table::
    :widths: 22 20 54 42

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input1
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - input2
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1
    * - output_sad
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE

        DATA_TYPE_EXT_U16
      - 根据 sad_attr.en_mode,

        对应 4x4、8x8、16x16分块模式, 高 、宽分别是 input1 的 1/4、1/8、1/16。
    * - output_thr
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 根据 sad_attr.en_mode,

        对应 4x4、8x8、16x16分块模式, 高 、宽分别是 input1 的 1/4、1/8、1/16。

| 【数据类型说明】

【说明】定义 SAD 计算模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_sad_mode_e{
        BM_IVE_SAD_MODE_MB_4X4 = 0x0,
        BM_IVE_SAD_MODE_MB_8X8 = 0x1,
        BM_IVE_SAD_MODE_MB_16X16 = 0x2,
    } bmcv_ive_sad_mode;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_SAD_MODE_MB_4X4
      - 按 4x4 像素块计算 SAD。
    * - BM_IVE_SAD_MODE_MB_8X8
      - 按 8x8 像素块计算 SAD。
    * - BM_IVE_SAD_MODE_MB_16X16
      - 按 16x16 像素块计算 SAD。

【说明】定义 SAD 输出控制模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_sad_out_ctrl_s{
        BM_IVE_SAD_OUT_BOTH = 0x0,
        BM_IVE_SAD_OUT_SAD  = 0x1,
        BM_IVE_SAD_OUT_THRESH  = 0x3,
    } bmcv_ive_sad_out_ctrl;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_SAD_OUT_BOTH
      - SAD 图和阈值化图输出模式。
    * - BM_IVE_SAD_OUT_SAD
      - SAD 图输出模式。
    * - BM_IVE_SAD_OUT_THRESH
      - 阈值化输出模式。

【说明】定义 SAD 控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_sad_attr_s{
        bm_ive_sad_mode en_mode;
        bm_ive_sad_out_ctrl en_out_ctrl;
    } bmcv_ive_sad_attr;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - SAD计算模式。
    * - en_out_ctrl
      - SAD 输出控制模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_sad_thresh_attr_s{
        unsigned short u16_thr;
        unsigned char  u8_min_val;
        unsigned char  u8_max_val;
    }bmcv_ive_sad_thresh_attr;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - u16_thr
      - 对计算的 SAD 图进行阈值化的阈值。取值范围依赖 enMode。
    * - u8_min_val
      - 阈值化不超过 u16Thr 时的取值。
    * - u8_max_val
      - 阈值化超过 u16Thr 时的取值。

【注意】

* BM_IVE_SAD_OUT_8BIT_BOTH：u16_thr 取值范围：[0, 255];

* BM_IVE_SAD_OUT_16BIT_BOTH 或者 BM_IVE_SAD_OUT_THRESH：u16_thr 取值范围：[0, 65535]。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 计算公式如下：

  .. math::

   \begin{aligned}
      & SAD_{\text{out}}(x, y) = \sum_{\substack{n \cdot x \leq i < n \cdot (x+1) \\ n \cdot y \leq j < n \cdot (j+1)}} \lvert I_{1}(x, y) - I_{2}(x, y) \rvert \\
      & THR_{\text{out}}(x, y) =
        \begin{cases}
          minVal & \ (SAD_{\text{out}}(x, y) \leq Thr) \\
          maxVal & \ (SAD_{\text{out}}(x, y) > Thr) \\
        \end{cases}
   \end{aligned}

**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <math.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

      const int sad_write_img(char* file_name, unsigned char *data,
                              int sad_width, int sad_height, int dsize,
                              int stride){
          unsigned char* tmp = malloc(sad_width * dsize);
          FILE *fp = fopen(file_name, "wb");
          for(int h = 0; h < sad_height; h++, data += stride){
              memcpy(tmp, data, sad_width * dsize);
              fwrite((void *)tmp, 1, sad_width * dsize, fp);
          }
          fclose(fp);
          free(tmp);
          return 0;
      }


      int main(){
          int dev_id = 0;
          int height = 1080, width = 1920;
          bm_image_format_ext fmt = FORMAT_GRAY;
          bmcv_ive_sad_mode sadMode = BM_IVE_SAD_MODE_MB_8X8;
          bmcv_ive_sad_out_ctrl sadOutCtrl = BM_IVE_SAD_OUT_BOTH;
          int u16_thr = 0x800;
          int u8_min_val = 2;
          int u8_max_val = 30;
          char *src1_name = "path/to/src1", *src2_name = "path/to/src2";
          char *dstSad_name = "path/to/dstSad", *dstThr_name = "path/to/dstThr";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          int dst_width = 0, dst_height = 0;
          bm_image src[2];
          bm_image dstSad, dstSadThr;
          int stride[4];
          int dstSad_stride[4];

          bmcv_ive_sad_attr sadAttr;
          bmcv_ive_sad_thresh_attr sadthreshAttr;
          sadAttr.en_mode = sadMode;
          sadAttr.en_out_ctrl = sadOutCtrl;
          sadthreshAttr.u16_thr = u16_thr;
          sadthreshAttr.u8_min_val = u8_min_val;
          sadthreshAttr.u8_max_val = u8_max_val;

          bm_image_data_format_ext dst_sad_dtype = DATA_TYPE_EXT_1N_BYTE;
          bool flag = false;

          // calc ive image stride && create bm image struct
          int data_size = 1;
          stride[0] = align_up(width, 16) * data_size;
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src[0], stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src[1], stride);

          dst_sad_dtype = DATA_TYPE_EXT_U16;
          data_size = 2;
          dstSad_stride[0] = align_up(width, 16) * data_size;
          bm_image_create(handle, height, width, fmt, dst_sad_dtype, &dstSad, dstSad_stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dstSadThr, stride);

          data_size = sizeof(unsigned short);
          dst_width = width  / 8;
          dst_height = height / 8;

          ret = bm_image_alloc_dev_mem(src[0], BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(src[1], BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dstSad, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dstSadThr, BMCV_HEAP1_ID);
          if (dstSad.data_type == DATA_TYPE_EXT_1N_BYTE){
              flag = true;
          }

          int byte_size;
          unsigned char *input_data;
          int image_byte_size[4] = {0};
          char *filename[] = {src1_name, src2_name};
          for (int i = 0; i < 2; i++) {
              bm_image_get_byte_size(src[i], image_byte_size);
              byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
              input_data = (unsigned char *)malloc(byte_size);
              FILE *fp_src = fopen(filename[i], "rb");
              if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
              printf("file size is less than required bytes%d\n", byte_size);
              };
              fclose(fp_src);
              void* in_ptr[4] = {(void *)input_data,
                                  (void *)((unsigned char*)input_data + image_byte_size[0]),
                                  (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1]),
                                  (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
              bm_image_copy_host_to_device(src[i], in_ptr);
          }

          ret = bmcv_ive_sad(handle, src, &dstSad, &dstSadThr, &sadAttr, &sadthreshAttr);

          unsigned char* sad_res = (unsigned char*)malloc(width * height * data_size);
          unsigned char* thr_res = (unsigned char*)malloc(width * height);
          memset(sad_res, 0, width * height * data_size);
          memset(thr_res, 0, width * height);

          ret = bm_image_copy_device_to_host(dstSad, (void **)&sad_res);
          ret = bm_image_copy_device_to_host(dstSadThr, (void **)&thr_res);

          int sad_stride = (flag) ? dstSad_stride[0] / 2 : dstSad_stride[0];
          sad_write_img(dstSad_name, sad_res, dst_width, dst_height, data_size, sad_stride);
          sad_write_img(dstThr_name, thr_res, dst_width, dst_height, sizeof(unsigned char), stride[0]);
          free(sad_res);
          free(thr_res);

          bm_image_destroy(&src[0]);
          bm_image_destroy(&src[1]);
          bm_image_destroy(&dstSad);
          bm_image_destroy(&dstSadThr);

          bm_dev_free(handle);
          return 0;
      }