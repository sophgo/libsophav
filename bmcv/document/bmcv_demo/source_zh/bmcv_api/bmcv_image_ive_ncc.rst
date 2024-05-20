bmcv_ive_ncc
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两相同分辨率灰度图像的归一化互相关系数计算任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_ncc(
    bm_handle_t       handle,
    bm_image          input1,
    bm_image          input2,
    bm_device_mem_t   output);

| 【参数】

.. list-table:: bmcv_ive_ncc 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input1
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - input2
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - output
      - 输出
      - 输出 数据 对象结构体, 不能为空, 内存至少需要配置: sizeof(bmcv_ive_ncc_dst_mem_t)。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input1
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 32x32~1920x1080
    * - input2
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1
    * - output
      - --
      - --
      - --

| 【数据类型说明】

【说明】定义LBP计算的比较模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct _bmcv_ive_ncc_dst_mem_s{
        unsigned long long u64_numerator;
        unsigned long long u64_quad_sum1;
        unsigned long long u64_quad_sum2;
        unsigned char u8_reserved[8];
    }bmcv_ive_ncc_dst_mem_t;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - u64_numerator
      - :math:`\sum_{i = 1}^{w} \sum_{j = 1}^{h} (I_{\text{src1}}(i, j) * I_{\text{src2}}(i, j))`
    * - u64_quad_sum1
      - :math:`\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src1}}^{2}(i, j)`
    * - u64_quad_sum2
      - :math:`\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src2}}^{2}(i, j)`
    * - u8_reserved
      - 保留字段。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入图像的 width 都需要16对齐。

* 计算公式如下：
   .. math::

       & NCC(I_{\text{src1}}, I_{\text{src2}}) =
         \frac{\sum_{i = 1}^{w} \sum_{j = 1}^{h} (I_{\text{src1}}(i, j) * I_{\text{src2}}(i, j))}
         {\sqrt{\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src1}}^{2}(i, j)}
          \sqrt{\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src2}}^{2}(i, j)}}


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
        int dev_id = 0;int height = 288, width = 352;
        bm_image_format_ext src_fmt = FORMAT_GRAY;
        char* src1_name = "./data/00_352x288_y.yuv", *src2_name = "./data/01_352x288_y.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bm_image src1, src2;
        bm_device_mem_t dst;
        int src_stride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;

        loop_time = ctx.loop;

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);

        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src1, src_stride);
        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src2, src_stride);
        ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src1 bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src2 bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
        bm_ive_read_bin(src1, src1_name);
        bm_ive_read_bin(src2, src2_name);

        int data_len = sizeof(bmcv_ive_ncc_dst_mem_t);

        ret = bm_malloc_device_byte(handle, &dst, data_len);
        if (ret != BM_SUCCESS) {
            printf("dst bm_malloc_device_byte failed. ret = %d\n", ret);
            exit(-1);
        }

        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_ncc(handle, src1, src2, dst);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;
            if(ret != BM_SUCCESS){
                printf("bmcv_image_ive_ncc failed, ret is %d \n", ret);
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        printf("bmcv_ive_ncc: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                loop_time, time_max, time_avg, fps_actual);
        printf("bmcv ive NCC test successful \n");

        bm_image_destroy(&src1);
        bm_image_destroy(&src2);
        bm_free_device(handle, dst);
        return 0;
      }

















