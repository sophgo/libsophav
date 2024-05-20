bmcv_ive_erode
------------------------------

| 【描述】

| 该 API 使用ive硬件资源，创建二值图像 5x5 模板腐蚀任务，将模板覆盖的区域中的最小像素值赋给参考点。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_erode(
        bm_handle_t           handle,
        bm_image              input,
        bm_image              output,
        unsigned char         erode_mask[25]);

| 【参数】

.. list-table:: bmcv_ive_erode 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体，不能为空, 宽、高同 input。
    * - \erode_mask
      - 输入
      - 腐蚀模板系数，不能为空，取值范围: 0 或 255。

| 【数据类型说明】

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY 的二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - GRAY 的二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 模板系数只能为 0 或 255。

3. 模板样例

  .. raw:: latex

   \begin{align*}
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{60pt} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 255 & 0 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
      255 & 255 & 255 & 255 & 255 \\
      0 & 0 & 255 & 0 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
   \end{bmatrix}
   \end{align*}


  .. raw:: latex

   \begin{align*}
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{60pt} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 255 & 0 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      255 & 255 & 255 & 255 & 255 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
   \end{bmatrix}
   \end{align*}

  .. raw:: latex

   \begin{align*}
   \begin{bmatrix}
      0 & 255 & 255 & 255 & 0 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      0 & 255 & 255 & 255 & 0 \\
   \end{bmatrix}
   \hspace{60pt} % 控制间隔
   \begin{bmatrix}
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
   \end{bmatrix}
   \end{align*}



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
          int dev_id = 0;int height = 288, width = 352;int erode_size = 0; // 0 : arr3by3; 1: arr5by5
          bm_image_format_ext fmt = FORMAT_GRAY;
          char *src_name = "./data/bin_352x288_y.yuv", *dst_name = "ive_erode_result.yuv";

          unsigned char arr3by3[25] = { 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 255,
                          255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0 };

          unsigned char arr5by5[25] = { 0, 0, 255, 0, 0, 0, 0, 255, 0,
                            0, 255, 255, 255, 255, 255, 0, 0, 255,
                            0, 0, 0, 0, 255, 0, 0 };
          bm_handle_t handle = NULL;
            int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          bm_image src, dst;
          int stride[4];
          unsigned int i = 0, loop_time = 0;
          unsigned long long time_single, time_total = 0, time_avg = 0;
          unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
          struct timeval tv_start;
          struct timeval tv_end;
          struct timeval timediff;


            // calc ive image stride && create bm image struct
            bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);

            bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
            bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

            ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
            if (ret != BM_SUCCESS) {
                printf("src bm_image_alloc_dev_mem_src. ret = %d\n", ret);
                exit(-1);
            }

            ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
            if (ret != BM_SUCCESS) {
                printf("dst bm_image_alloc_dev_mem_src. ret = %d\n", ret);
                exit(-1);
            }
            bm_ive_read_bin(src, src_name);

            for (i = 0; i < loop_time; i++) {
                gettimeofday(&tv_start, NULL);
                if (erode_size == 0) {
                    ret = bmcv_ive_erode(handle, src, dst, arr3by3);
                } else {
                    ret = bmcv_ive_erode(handle, src, dst, arr5by5);
                }
                gettimeofday(&tv_end, NULL);
                timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
                timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
                time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

                if(time_single>time_max){time_max = time_single;}
                if(time_single<time_min){time_min = time_single;}
                time_total = time_total + time_single;
                if(ret != BM_SUCCESS){
                    printf("bmcv_ive_erode failed, ret is %d \n", ret);
                    exit(-1);
                }
            }

            time_avg = time_total / loop_time;
            fps_actual = 1000000 / time_avg;

            bm_image_destroy(&src);
            bm_image_destroy(&dst);

            printf("bmcv_ive_erode: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                    loop_time, time_max, time_avg, fps_actual);
            printf("bmcv ive erode test successful \n");

            return 0;
        }