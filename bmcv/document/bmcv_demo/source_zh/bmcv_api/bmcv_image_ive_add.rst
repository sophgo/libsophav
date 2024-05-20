bmcv_ive_add
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两张图像的加权相加操作。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_add(
        bm_handle_t          handle,
        bm_image             input1,
        bm_image             input2,
        bm_image             output,
        bmcv_ive_add_attr    attr);

| 【参数】

.. list-table:: bmcv_ive_add 参数表
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
      - 相加操作的需要的加权系数对应的结构体，不能为空。


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


【说明】定义两图像的加权加控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_add_attr_s {
        unsigned short param_x;
        unsigned short param_y;
    } bmcv_ive_add_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - param_x
      - 加权加 "xA + yB" 中的权重x。
    * - param_y
      - 加权加 "xA + yB" 中的权重y。


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入输出图像的 width 都需要16对齐。

* MOD_ADD:
    .. math::

       \begin{aligned}
        & I_{\text{out}}(i, j) = x * I_{1}(i, j) + y * I_{2}(i, j)
      \end{aligned}

    其中，:math:`I_{1}(i, j)` 对应 input1, :math:`I_{2}(i, j)` 对应 input2, :math:`x, y` 分别对应 bmcv_ive_add_attr 中的 param_x 与 param_y。

    若定点化前, :math:`x` 和 :math:`y` 满足 :math:`x + y > 1`, 则当前计算结果超过 8bit 取 低 8bit 作为最终结果。


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
      extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
          bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);

      int main(){
        int dev_id = 0;
        int loop_time = 1;
        int height = 288, width = 352;
        unsigned short x = 19584, y = 45952; /* x + y = 65536 */
        bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
        char *src1_name = "ive_data/00_352x288_y.yuv", *src2_name = "ive_data/01_352x288_y.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bm_image src1, src2, dst;
        int src_stride[4];
        int dst_stride[4];
        unsigned int i = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;
        bmcv_ive_add_attr add_attr;
        memset(&add_attr, 0, sizeof(bmcv_ive_add_attr));

        add_attr.param_x = x;
        add_attr.param_y = y;

        bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
        bm_ive_image_calc_stride(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, dst_stride);

        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src1, src_stride);
        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src2, src_stride);
        bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);
        ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src1 bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src2 bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("dst bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            exit(-1);
        }

        // read image data from input files
        bm_ive_read_bin(src1, src1_name);
        bm_ive_read_bin(src2, src2_name);
        for(i = 0; i < loop_time; i++)
        {
            gettimeofday(&tv_start, NULL);

            ret = bmcv_ive_add(handle, src1, src2, dst, add_attr);

            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("[bmcv_ive_add] is failed \n");
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        pixel_per_sec = width * height * fps_actual/1024/1024;
        bm_image_destroy(&src1);
        bm_image_destroy(&src2);
        bm_image_destroy(&dst);
        printf("bm_ive_add: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
            loop_time, time_max, time_avg, fps_actual, pixel_per_sec);
        return 0;
      }