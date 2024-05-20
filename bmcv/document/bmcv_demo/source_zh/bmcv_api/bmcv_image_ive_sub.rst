bmcv_ive_sub
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两张图像的相减操作。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_sub(
        bm_handle_t          handle,
        bm_image             input1,
        bm_image             input2,
        bm_image             output,
        bmcv_ive_sub_attr    attr);

| 【参数】

.. list-table:: bmcv_ive_sub 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input1
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \input2
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体, 不能为空。
    * - \attr
      - 输入
      - 相减操作对应模式的结构体，不能为空。


.. list-table::
    :widths: 22 40 64 42

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input1
      - GRAY

        GRAY 二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - input2
      - GRAY

        GRAY 二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 Input1

【注意】 output 数据类型格式也有可能是 DATA_TYPE_EXT_1N_BYTE_SIGNED。

| 【数据类型说明】

【说明】定义两张图像相减输出格式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_sub_mode_e{
        IVE_SUB_ABS = 0x0,
        IVE_SUB_SHIFT = 0x1,
        IVE_SUB_BUTT
    } bmcv_ive_sub_mode;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - IVE_SUB_ABS
      - 取差的绝对值。
    * - IVE_SUB_SHIFT
      - 将结果右移一位输出, 保留符号位。

【说明】定义两图像相减控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_sub_attr_s {
        bmcv_ive_sub_mode en_mode;
    } bmcv_ive_sub_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - MOD_SUB 下 两图像相减模式。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入输出图像的 width 都需要16对齐。

* MOD_SUB:

     - IVE_SUB_ABS:
        .. math::

           \begin{aligned}
            & I_{\text{out}}(x, y) = abs(I_{\text{src1}}(x, y) - I_{\text{src2}}(x, y))
           \end{aligned}

     输出格式是 DATA_TYPE_EXT_1N_BYTE。

     - IVE_SUB_SHIFT:
        .. math::

           \begin{aligned}
            & I_{\text{out}}(x, y) = (I_{\text{src1}}(x, y) - I_{\text{src2}}(x, y)) \gg 1
           \end{aligned}

     输出格式是 DATA_TYPE_EXT_1N_BYTE_SIGNED。


**示例代码**

  .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <pthread.h>
      #include <sys/time.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>
      extern void bm_ive_read_bin(bm_image src, const char *input_name);
      extern void bm_ive_write_bin(bm_image dst, const char *output_name);
      extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
      bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);

      int main(){
        int dev_id = 0;int height = 288, width = 352;
        bmcv_ive_sub_mode sub_mode = IVE_SUB_ABS; /* IVE_SUB_MODE_ABS */
        bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
        char *src1_name = "./ive_data/00_352x288_y.yuv", *src2_name = "./ive_data/01_352x288_y.yuv";
        char *dst_name = "ive_sample_Sub.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bmcv_ive_sub_attr sub_attr;
        bm_image src1, src2, dst;
        int src_stride[4];
        int dst_stride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;
        // set ive sub params
        memset(&sub_attr, 0, sizeof(bmcv_ive_sub_attr));
        sub_attr.en_mode = sub_mode;

        // calc ive image stride
        bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
        bm_ive_image_calc_stride(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, dst_stride);

        // create bm image struct
        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src1, src_stride);
        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src2, src_stride);
        bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

        // alloc bm image memory
        ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
            exit(-1);
        }

        // read image data from input files
        bm_ive_read_bin(src1, src1_name);
        bm_ive_read_bin(src2, src2_name);
        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);

            ret = bmcv_ive_sub(handle, src1, src2, dst, sub_attr);

            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("[bmcv_ive_sub] is failed \n");
                exit(-1);
            }
        }
        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        pixel_per_sec = width * height * fps_actual/1024/1024;

        bm_image_destroy(&src1);
        bm_image_destroy(&src2);
        bm_image_destroy(&dst);
        printf("bm_ive_sub: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
            loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

        return 0;
      }












