bmcv_ive_canny
------------------------------

| 【描述】

| 该 API 使用ive硬件资源的 ccl 功能, 计算 canny 边缘图。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_canny(
        bm_handle_t                  handle,
        bm_image                     input,
        bm_device_mem_t              output_edge,
        bmcv_ive_canny_hys_edge_ctrl   canny_hys_edge_attr);

| 【参数】

.. list-table:: bmcv_ive_canny 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output_edge
      - 输出
      - 输出 bm_device_mem_t 对象结构体，不能为空。
    * - \canny_hys_edge_attr
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

| 【数据类型说明】

【说明】定义 Canny 边缘前半部分计算任务的控制参数 。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_canny_hys_edge_ctrl_s{
        bm_device_mem_t  st_mem;
        unsigned short u16_low_thr;
        unsigned short u16_high_thr;
        signed char as8_mask[25];
    } bmcv_ive_canny_hys_edge_ctrl;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - stmem
      - 辅助内存。
    * - u16_low_thr
      - 低阈值，取值范围：[0, 255]。
    * - u16_high_thr
      - 高阈值，取值范围: [u16LowThr, 255]。
    * - as8_mask
      - 用于计算梯度的参数模板。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. canny_hys_edge_attr.st_mem 至少需要分配的内存大小:

   canny_hys_edge_attr.st_mem.size = input_stride * 3 * input.height。



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
        int dev_id = 0;
        int thrSize = 0; //0 -> 3x3; 1 -> 5x5
        int height = 288, width = 352;
        bm_image_format_ext fmt = FORMAT_GRAY;
        char *src_name = "./ive_data/00_352x288_y.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bm_image src;
        bm_device_mem_t canny_stmem;
        int stride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;

        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;
        int stmem_len = width * height * 4 * (sizeof(unsigned short) + sizeof(unsigned char));

        unsigned char *edge_res = malloc(width * height * sizeof(unsigned char));
        memset(edge_res, 0, width * height * sizeof(unsigned char));

        bmcv_ive_canny_hys_edge_ctrl cannyHysEdgeAttr;
        memset(&cannyHysEdgeAttr, 0, sizeof(bmcv_ive_canny_hys_edge_ctrl));
        cannyHysEdgeAttr.u16_low_thr = (thrSize == 0) ? 42 : 108;
        cannyHysEdgeAttr.u16_high_thr = 3 * cannyHysEdgeAttr.u16_low_thr;
        (thrSize == 0) ? memcpy(cannyHysEdgeAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char)) :
                        memcpy(cannyHysEdgeAttr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char));

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);

            free(edge_res);
            bm_image_destroy(&src);
            bm_free_device(handle, cannyHysEdgeAttr.st_mem);
              exit(-1);
        }

        ret = bm_malloc_device_byte(handle, &canny_stmem, stmem_len);
        if (ret != BM_SUCCESS) {
            printf("cannyHysEdgeAttr_stmem bm_malloc_device_byte failed. ret = %d\n", ret);

            free(edge_res);
            bm_image_destroy(&src);
            bm_free_device(handle, cannyHysEdgeAttr.st_mem);
            exit(-1);
        }

        bm_ive_read_bin(src, src_name);
        cannyHysEdgeAttr.st_mem = canny_stmem;

        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_canny(handle, src, bm_mem_from_system((void *)edge_res), cannyHysEdgeAttr);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_ive_canny failed. ret = %d\n", ret);

                free(edge_res);
                bm_image_destroy(&src);
                bm_free_device(handle, cannyHysEdgeAttr.st_mem);
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        free(edge_res);
        bm_image_destroy(&src);
        bm_free_device(handle, cannyHysEdgeAttr.st_mem);
        printf("idx:%d, bmcv_ive_canny: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                ctx.i, loop_time, time_max, time_avg, fps_actual);
        printf("bmcv ive canny test successful \n");

        return 0;
      }