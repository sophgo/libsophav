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
      #include <pthread.h>
      #include <math.h>
      #include <sys/time.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      #define SLEEP_ON 0
      #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

      extern void bm_ive_read_bin(bm_image src, const char *input_name);
      extern void bm_ive_write_bin(bm_image dst, const char *output_name);
      extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
          bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);

      typedef struct ive_sad_ctx_{
          int loop;
          int i;
      } ive_sad_ctx;

      int test_loop_times  = 1;
      int test_threads_num = 1;
      int dev_id = 0;
      int bWrite = 0;
      int height = 288, width = 352;
      bm_image_format_ext fmt = FORMAT_GRAY;
      bmcv_ive_sad_mode sadMode = BM_IVE_SAD_MODE_MB_8X8;
      bmcv_ive_sad_out_ctrl sadOutCtrl = BM_IVE_SAD_OUT_BOTH;
      int datatype = 0;
      int u16_thr = 0x800;
      int u8_min_val = 2;
      int u8_max_val = 30;
      char *src1_name = "./data/00_352x288_y.yuv", *src2_name = "./data/bin_352x288_y.yuv";
      char *dstSad_name = "sad_res.yuv", *dstThr_name = "sad_thr_res.yuv";

      bm_handle_t handle = NULL;

      static void * ive_sad(void* arg){
          bm_status_t ret;
          ive_sad_ctx ctx = *(ive_sad_ctx*)arg;
          int dst_width = 0, dst_height = 0;
          bm_image src[2];
          bm_image dstSad, dstSadThr;
          int stride[4];
          int dstSad_stride[4];
          unsigned int i = 0, loop_time = 0;
          unsigned long long time_single, time_total = 0, time_avg = 0;
          unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
      #if SLEEP_ON
          int fps = 60;
          int sleep_time = 1000000 / fps;
      #endif

          struct timeval tv_start;
          struct timeval tv_end;
          struct timeval timediff;

          loop_time = ctx.loop;

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
          bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);

          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src[0], stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src[1], stride);

          if(sadOutCtrl == BM_IVE_SAD_OUT_BOTH) {
              if (datatype == 0){
                  dst_sad_dtype = DATA_TYPE_EXT_U16;
                  bm_ive_image_calc_stride(handle, height, width, fmt, dst_sad_dtype, dstSad_stride);
                  bm_image_create(handle, height, width, fmt, dst_sad_dtype, &dstSad, dstSad_stride);
                  bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dstSadThr, stride);
              } else {
                  bm_ive_image_calc_stride(handle, height, width, fmt, dst_sad_dtype, dstSad_stride);
                  bm_image_create(handle, height, width, fmt, dst_sad_dtype, &dstSad, dstSad_stride);
                  bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dstSadThr, stride);
              }
          }

          int data_size = (dst_sad_dtype == DATA_TYPE_EXT_U16) ? sizeof(unsigned short) : sizeof(unsigned char);
          switch(sadMode){
              case BM_IVE_SAD_MODE_MB_4X4:
                  dst_width = width / 4;
                  dst_height = height / 4;
                  break;
              case BM_IVE_SAD_MODE_MB_8X8:
                  dst_width = width  / 8;
                  dst_height = height / 8;
                  break;
              case BM_IVE_SAD_MODE_MB_16X16:
                  dst_width = width  / 16;
                  dst_height = height / 16;
                  break;
              default:
                  printf("not support mode type %d, return\n", sadOutCtrl);
                  break;
          }

          ret = bm_image_alloc_dev_mem(src[0], BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("src1 bm_image_alloc_dev_mem failed. ret = %d\n", ret);
              exit(-1);
          }

          ret = bm_image_alloc_dev_mem(src[1], BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("src2 bm_image_alloc_dev_mem failed. ret = %d\n", ret);
              exit(-1);
          }

          ret = bm_image_alloc_dev_mem(dstSad, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("dstSad bm_image_alloc_dev_mem failed. ret = %d\n", ret);
              exit(-1);
          }

          ret = bm_image_alloc_dev_mem(dstSadThr, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("dstSadThr bm_image_alloc_dev_mem failed. ret = %d\n", ret);
              exit(-1);
          }
          if (dstSad.data_type == DATA_TYPE_EXT_1N_BYTE){
              flag = true;
          }

          bm_ive_read_bin(src[0], src1_name);
          bm_ive_read_bin(src[1], src2_name);

          for (i = 0; i < loop_time; i++) {
              gettimeofday(&tv_start, NULL);
              ret = bmcv_ive_sad(handle, src, &dstSad, &dstSadThr, &sadAttr, &sadthreshAttr);
              gettimeofday(&tv_end, NULL);
              timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
              timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
              time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

              if(time_single>time_max){time_max = time_single;}
              if(time_single<time_min){time_min = time_single;}
              time_total = time_total + time_single;

              if(ret != BM_SUCCESS){
                  printf("bmcv_ive_sad failed. ret = %d\n", ret);
                  exit(-1);
              }
          }

          time_avg = time_total / loop_time;
          fps_actual = 1000000 / time_avg;

          bm_image_destroy(&src[0]);
          bm_image_destroy(&src[1]);
          bm_image_destroy(&dstSad);
          bm_image_destroy(&dstSadThr);

          printf("idx:%d, bmcv_ive_sad: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                  ctx.i, loop_time, time_max, time_avg, fps_actual);
          printf("----- [bmcv ive sad] test successful -----\n");
          return 0;
      }

      int main(int argc, char **argv){
          if (argc >= 13) {
              width = atoi(argv[1]);
              height = atoi(argv[2]);
              sadMode = atoi(argv[3]);
              sadOutCtrl = atoi(argv[4]);
              datatype = atoi(argv[5]);
              u16_thr = atoi(argv[6]);
              u8_min_val = atoi(argv[7]);
              u8_max_val = atoi(argv[8]);
              src1_name = argv[9];
              src2_name = argv[10];
              if(argc > 11) dev_id = atoi(argv[11]);
              if(argc > 12) test_threads_num = atoi(argv[12]);
              if(argc > 13) test_loop_times  = atoi(argv[13]);
              if(argc > 14) bWrite = atoi(argv[14]);
              if(argc > 15) dstSad_name = argv[15];
              if(argc > 16) dstThr_name = argv[16];
          }
          if (argc == 2)
              test_threads_num  = atoi(argv[1]);
          else if (argc == 3){
              test_threads_num = atoi(argv[1]);
              test_loop_times  = atoi(argv[2]);
          } else if (argc > 3 && argc < 11) {
              printf("command input error, please follow this order:\n \
              %s width height sad_mode sad_outCtrl_mode sadThr sadMinValue sadMaxValue src1_name src2_name refSad_name refThr_name dev_id thread_num loop_num bWrite dstSad_name dstSadThr_name\n \
              %s thread_num loop_num\n", argv[0], argv[0]);
              exit(-1);
          }
          if (test_loop_times > 15000 || test_loop_times < 1) {
              printf("[TEST ive sad] loop times should be 1~15000\n");
              exit(-1);
          }
          if (test_threads_num > 20 || test_threads_num < 1) {
              printf("[TEST ive sad] thread nums should be 1~20\n");
              exit(-1);
          }

          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          ive_sad_ctx ctx[test_threads_num];
          #ifdef __linux__
              pthread_t * pid = (pthread_t *)malloc(sizeof(pthread_t)*test_threads_num);
              for (int i = 0; i < test_threads_num; i++) {
                  ctx[i].i = i;
                  ctx[i].loop = test_loop_times;
                  if (pthread_create(
                          &pid[i], NULL, ive_sad, (void *)(ctx + i))) {
                      free(pid);
                      perror("create thread failed\n");
                      exit(-1);
                  }
              }
              for (int i = 0; i < test_threads_num; i++) {
                  ret = pthread_join(pid[i], NULL);
                  if (ret != 0) {
                      free(pid);
                      perror("Thread join failed");
                      exit(-1);
                  }
              }
              bm_dev_free(handle);
              printf("--------ALL THREADS TEST OVER---------\n");
              free(pid);
          #endif
          return 0;
      }