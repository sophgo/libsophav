bmcv_ive_frame_diff_motion
---------------------------------

| 【描述】

| 该 API 使用ive硬件资源,  创建背景相减任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_frame_diff_motion(
        bm_handle_t                     handle,
        bm_image                        input1,
        bm_image                        input2,
        bm_image                        output,
        bmcv_ive_frame_diff_motion_attr attr);

| 【参数】

.. list-table:: bmcv_ive_frame_diff_motion 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input1
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \input2
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体，不能为空。
    * - \attr
      - 输入
      - 控制参数结构体，不能为空。

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
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义该算子控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_frame_diff_motion_attr_s{
        bmcv_ive_sub_mode sub_mode;
        bmcv_ive_thresh_mode thr_mode;
        unsigned char u8_thr_low;
        unsigned char u8_thr_high;
        unsigned char u8_thr_min_val;
        unsigned char u8_thr_mid_val;
        unsigned char u8_thr_max_val;
        unsigned char au8_erode_mask[25];
        unsigned char au8_dilate_mask[25];
    } bmcv_ive_frame_diff_motion_attr;

.. list-table::
    :widths: 40 100

    * - **成员名称**
      - **描述**
    * - sub_mode
      - 定义两张图像相减输出格式。
    * - thr_mode
      - 定义阈值化计算方式，仅支持 MOD_U8 模式，即 U8 数据到 U8 数据的阈值化。
    * - u8_thr_low
      - 低阈值，取值范围：[0, 255]。
    * - u8_thr_high
      - 表示高阈值， 0 ≤ u8ThrLow ≤ u8ThrHigh ≤ 255。
    * - u8_thr_min_val
      - 表示最小值，取值范围：[0, 255]。
    * - u8_thr_mid_val
      - 表示中间值，取值范围：[0, 255]。
    * - u8_thr_max_val
      - 表示最大值，取值范围：[0, 255]。
    * - au8_erode_mask[25]
      - 表示腐蚀的 5x5 模板系数，取值范围：0 或 255。
    * - au8_dilate_mask[25]
      - 表示膨胀的 5x5 模板系数，取值范围：0 或 255。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。


**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

      int main() {
          int dev_id = 0;
          int height = 1080, width = 1920;
          bm_image_format_ext fmt = FORMAT_GRAY;
          char *src1_name = "path/to/src1";
          char *src2_name = "path/to/src2";
          char *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bm_image src1, src2, dst;
          int stride[4];

          // mask data
          unsigned char arr[25] = {0, 0, 255, 0, 0, 0, 0, 255, 0, 0, 255,
                  255, 255, 255, 255, 0, 0, 255, 0, 0, 0, 0, 255, 0, 0};

          // config setting(Sub->threshold->erode->dilate)
          bmcv_ive_frame_diff_motion_attr attr;
          attr.sub_mode = IVE_SUB_ABS;
          attr.thr_mode = IVE_THRESH_BINARY;
          attr.u8_thr_min_val = 0;
          attr.u8_thr_max_val = 255;
          attr.u8_thr_low = 30;

          memcpy(&attr.au8_erode_mask, &arr, 25 * sizeof(unsigned char));
          memcpy(&attr.au8_dilate_mask, &arr, 25 * sizeof(unsigned char));

          // calc ive image stride && create bm image struct
          int data_size = 1;
          stride[0] = align_up(width, 16) * data_size;

          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src1, stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src2, stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

          ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

          // read image data from input files
          int byte_size;
          unsigned char *input_data;
          int image_byte_size[4] = {0};
          char *filename[] = {src1_name, src2_name};
          bm_image src_images[] = {src1, src2};
          for (int i = 0; i < 2; i++) {
              bm_image_get_byte_size(src_images[i], image_byte_size);
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
              bm_image_copy_host_to_device(src_images[i], in_ptr);
          }

          ret = bmcv_ive_frame_diff_motion(handle, src1, src2, dst, attr);


          unsigned char *ive_add_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
          memset(ive_add_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(dst, (void **)&ive_add_res);
          if(ret != BM_SUCCESS){
              printf("dst bm_image_copy_device_to_host failed, ret is %d \n", ret);
              exit(-1);
          }
          FILE *fp = fopen(dst_name, "wb");
          fwrite((void *)ive_add_res, 1, width * height * sizeof(unsigned char), fp);
          fclose(fp);

          free(input_data);
          free(ive_add_res);
          bm_image_destroy(&src1);
          bm_image_destroy(&src2);
          bm_image_destroy(&dst);

          bm_dev_free(handle);

          return 0;
      }
