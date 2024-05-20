bmcv_ive_stcandicorner
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 完成灰度图像 Shi-Tomasi-like 角点（两条边缘的交点）计算，角点在任意一个方向上做微小移动，都会引起该区域的梯度图的方向和幅值发生很大变化。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_stcandicorner(
    bm_handle_t                 handle,
    bm_image                    input,
    bm_image                    output,
    bmcv_ive_stcandicorner_attr   stcandicorner_attr);

| 【参数】

.. list-table:: bmcv_ive_stcandicorner 参数表
    :widths: 25 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象数据结构体, 候选角点响应值图像指针, 不能为空, 宽、高同 input。
    * - \stcandicorner_attr
      - 输入
      - Shi-Tomas-like 候选角点计算控制参数结构体, 不能为空。

.. list-table::
    :widths: 25 18 62 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64～1920x1080
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

【数据类型说明】

【说明】定义 Shi-Tomas-like 候选角点计算控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_stcandicorner_attr_s{
        bm_device_mem_t st_mem;
        unsigned char   u0q8_quality_level;
    } bmcv_ive_stcandicorner_attr;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - st_mem
      - 内存配置大小见 bmcv_image_ive_stcandicorner 的注意。
    * - u0q8_quality_level
      - ShiTomasi 角点质量控制参数，角点响应值小于 u0q8_quality_level * 最大角点响应值 的点将直接被确认为非角点。

        取值范围: [1, 255], 参考取值: 25。

【说明】定义 Shi-Tomas-like 角点计算时最大角点响应值结构体。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_st_max_eig_s{
        unsigned short u16_max_eig;
        unsigned char  u8_reserved[14];
    } bmcv_ive_st_max_eig;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - u16_max_eig
      - 最大角点响应值。
    * - u8_reserved
      - 保留位。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 与 OpenCV 中 ShiTomas 角点计算原理类似。

3. stcandicorner_attr.st_mem 至少需要开辟的内存大小:

     stcandicorner_attr.st_mem.size = 4 * input_stride * input.height + sizoef(bm_ive_st_max_eig)。

4. 该任务完成后, 得到的并不是真正的角点, 还需要进行下一步计算。

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
        int u0q8QualityLevel = 25;
        bm_image_format_ext fmt = FORMAT_GRAY;
        char *src_name = "./ive_data/penguin_352x288.gray.shitomasi.raw";
        char *dst_name = "./stCandiCorner_res.yuv";
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
        bmcv_ive_stcandicorner_attr stCandiCorner_attr;
        memset(&stCandiCorner_attr, 0, sizeof(bmcv_ive_stcandicorner_attr));

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }

        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }

        int attr_len = 4 * height * stride[0] + sizeof(bmcv_ive_st_max_eig);
        ret = bm_malloc_device_byte(handle, &stCandiCorner_attr.st_mem, attr_len * sizeof(unsigned char));
        if (ret != BM_SUCCESS) {
            printf("cannyHysEdgeAttr_stmem bm_malloc_device_byte failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            bm_free_device(handle, stCandiCorner_attr.st_mem);
            exit(-1);
        }
        stCandiCorner_attr.u0q8_quality_level = u0q8QualityLevel;
        bm_ive_read_bin(src, src_name);

        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_stcandicorner(handle, src, dst, stCandiCorner_attr);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_ive_stCandiCorner failed. ret = %d\n", ret);
                bm_image_destroy(&src);
                bm_image_destroy(&dst);
                bm_free_device(handle, stCandiCorner_attr.st_mem);
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        bm_free_device(handle, stCandiCorner_attr.st_mem);
        printf("bmcv_ive_stCandiCorner: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                loop_time, time_max, time_avg, fps_actual);
        printf("bmcv ive stCandiCorner test successful \n");
        return 0;
      }






































