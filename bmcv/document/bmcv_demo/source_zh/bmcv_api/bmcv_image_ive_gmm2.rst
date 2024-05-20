bmcv_ive_gmm2
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 GMM 背景建模任务，支持 1-5 个高斯模型，支持灰度图和 RGB_PACKAGE 图输入， 支持全局及像素级别的灵敏度系数以及前景模型时长更新系数。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_gmm2(
        bm_handle_t        handle,
        bm_image *         input,
        bm_image *         input_factor,
        bm_image *         output_fg,
        bm_image *         output_bg,
        bm_image *         output_match_model_info,
        bm_device_mem_t    output_model,
        bmcv_ive_gmm2_ctrl   gmm2_attr);

| 【参数】

.. list-table:: bmcv_ive_gmm2 参数表
    :widths: 35 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*input
      - 输入
      - 输入 bm_image 对象指针, 不能为空。
    * - \*input_factor
      - 输入
      - 输入 bm_image 对象指针, 表示模型更新参数, 当且仅仅当 gmm2_attr.en_sns_factor_mode、gmm2_attr.en_life_update_factor_mode 均使用全局模式时可以为空。
    * - \*output_fg
      - 输出
      - 输出 bm_image 对象指针, 表示前景图像, 不能为空, 宽、高同 input。
    * - \*output_bg
      - 输出
      - 输出 bm_image 对象指针, 表示背景图像, 不能为空, 宽、高同 input。
    * - \*output_match_model_info
      - 输出
      - 输出 bm_image 对象指针, 对应模型匹配系数, 不能为空。
    * - \output_model
      - 输入、输出
      - bm_device_mem_t 对象结构体, 对应 GMM 模型参数, 不能为空。
    * - gmm2_attr
      - 输入
      - 控制信息结构体, 不能为空。

.. list-table::
    :widths: 38 30 63 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY

        RGB PACKED
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - input_factor
      - GRAY
      - DATA_TYPE_EXT_U16
      - 同 input
    * - output_fg
      - GRAY 二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input
    * - output_bg
      - 同 input
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input
    * - match_model_info
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义灵敏度系数模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum gmm2_sns_factor_mode_e{
        SNS_FACTOR_MODE_GLB = 0x0,
        SNS_FACTOR_MODE_PIX = 0x1,
    } gmm2_sns_factor_mode;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - SNS_FACTOR_MODE_GLB
      - 全局灵敏度系数模式，每个像素在模型匹配过程中, 方差灵敏度使用 gmm2_attr.u8_glb_sns_factor。
    * - SNS_FACTOR_MODE_PIX
      - 像素级灵敏度系数模式，每个像素在模型匹配过程中，方差灵敏度使用 input_factor 的灵敏度系数。

【说明】定义模型时长参数更新模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum gmm2_life_update_factor_mode_e{
        LIFE_UPDATE_FACTOR_MODE_GLB = 0x0,
        LIFE_UPDATE_FACTOR_MODE_PIX = 0x1,
    } gmm2_life_update_factor_mode;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - LIFE_UPDATE_FACTOR_MODE_GLB
      - 模型时长参数全局更新模式，每个像素模型时长参数在更新时使用 gmm2_attr.u16_glb_life_update_factor。
    * - LIFE_UPDATE_FACTOR_MODE_PIX
      - 模型时长参数像素级更新模式，每个像素模型时长在更新时使用 input_factor 的模型更新参数。

【说明】定义 GMM2 背景建模的控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_gmm2_ctrl_s{
        gmm2_sns_factor_mode en_sns_factor_mode;
        gmm2_life_update_factor_mode en_life_update_factor_mode;
        unsigned short u16_glb_life_update_factor;
        unsigned short u16_life_thr;
        unsigned short u16_freq_init_val;
        unsigned short u16_freq_redu_factor;
        unsigned short u16_freq_add_factor;
        unsigned short u16_freq_thr;
        unsigned short u16_var_rate;
        unsigned short u9q7_max_var;
        unsigned short u9q7_min_var;
        unsigned char u8_glb_sns_factor;
        unsigned char u8_model_num;
    } bmcv_ive_gmm2_ctrl;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - en_sns_factor_mode
      - 灵敏度模式, 默认全局模式。

        全局模式使用 u8_glb_sns_factor 作为灵敏度系数;

        像素模式使用 input_factor 的低 8 bit 值作为灵敏度系数。
    * - en_life_update_factor_mode
      - 模型时长更新模式, 默认全局模式。

        全局模式使用 u16_glb_life_update_factor 作为前进模型更新参数;

        像素模式使用 input_factor 的高 8bit 值作为前进模型时长更新参数。
    * - u16_glb_life_update_factor
      - 全局模型更新参数。

        取值范围： [0, 65535]，默认： 4。
    * - u16_life_thr
      - 背景模型生成时间，表示一个模型从前景模型转成背景模型需要的时间。

        取值范围： [0, 65535]，默认： 5000。
    * - u16_freq_init_val
      - 初始频率。

        取值范围： [0, 65535]，默认： 20000。
    * - u16_freq_redu_factor
      - 频率衰减系数.

        取值范围： [0, 65535]，默认： 0xFF00。
    * - u16_freq_add_factor
      - 模型匹配频率增加系数。

        取值范围：[0, 65535]，默认： 0xEF。
    * - u16_freq_thr
      - 模型失效频率阈值。

        取值范围： [0, 65535]，默认： 12000。
    * - u16_var_rate
      - 方差更新率。

        取值范围： [0, 65535]，默认： 1。
    * - u9q7_max_var
      - 方差最大值。

        取值范围： [0, 65535]，默认： (16x16) :math:`\ll` 7。
    * - u9q7_min_var
      - 方差最小值。

        取值范围：[0, u9q7MaxVar]，默认： (8x8) :math:`\ll` 7。
    * - u8_glb_sns_factor
      - 方差最小值。

        取值范围： [0, 255]，默认： 8。
    * - u8_model_num
      - 模型数量。

        取值范围 [1, 5]，默认： 3。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. GMM2 在参考了 OPENCV 的 MOG 和 MOG2 的基础上，增加了像素级别的参数控制。

3. 源图像 pstSrc 类型只能为 U8C1 或 U8C3_PACKAGE，分别用于灰度图和 RGB 图的 GMM 背景建模。

4. 模型更新参数 pstFactor 为 U16C1 图像：每个元素用 16 bit 表示，低 8 bit 为灵敏度系数，用于控制模型匹配时方差倍数；高 8 bit 为前景模型时长更新参数，用于控制背景模型形成时间。

5. 模型匹配系数指针 pstMatchModelInfo 为 U8C1 图像：每个元素用 8bit 表示， 低 1 bit 为高斯模型匹配标志， 0 表示匹配失败， 1 表示匹配成功； 高 7 bit 为频率最大模型序号。

6. GMM2 的频率参数（pstGmm2Ctrl 中的 u16_freq_init_val、 u16_freq_redu_factor、u16_freq_add_factor、 u16_freq_thr） 用于控制模型排序和模型有效时间。

   - u16_freq_init_val 越大，模型有效时间越大；

   - u16_freq_redu_factor 越大，模型有效时间越长，模型频率通过乘以频率衰减系数

   - u16_freq_redu_factor/65536，达到频率衰减的目的；

   - u16_freq_add_factor 越大，模型有效时间越长；

   - u16_freq_thr 越大，模型有效时间越短。

7. GMM2 的模型时长参数(pstGmm2Ctrl 中的 u16_life_thr)用于控制前景模型成为背景的时间。

   - u16_life_thr 越大，前景持续时间越长；

   - 单高斯模型下，模型时长参数不生效。

8. 灰度图像 GMM2 采用 n 个（1≤n≤5） 高斯模型。


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
        extern void bm_ive_write_bin(bm_image dst, const char *output_name);
        extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
            bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);
        int main(){
          int dev_id = 0;
          bool pixel_ctrl = false;
          gmm2_life_update_factor_mode
                  life_update_enMode = LIFE_UPDATE_FACTOR_MODE_GLB;
          int height = 288, width = 704;
          bm_image_format_ext src_fmt = FORMAT_GRAY;
          char *input_name = "./data/campus.u8c1.1_100.raw";
          char *inputFactor_name = "./data/sample_GMM2_U8C1_PixelCtrl_Factor.raw";
          char *dstFg_name = "gmm2_fg_res.yuv", *dstBg_name = "gmm2_bg_res.yuv";
          char *dstPcMatch_name = "gmm2_pcMatch_res.yuv";

          char *goldenFg_name = "./data/result/sample_tile_GMM2_U8C1_fg_31.yuv";
          char *goldenBg_name = "./data/result/sample_tile_GMM2_U8C1_bg_31.yuv";
          char *goldenPcMatch_name = "./data/result/sample_GMM2_U8C1_PixelCtrl_match_31.yuv";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          bm_image src, src_factor;
          bm_image dst_fg, dst_bg, dst_model_match_model_info;
          bm_device_mem_t dst_model;
          int stride[4], factorStride[4];
          unsigned int i = 0, loop_time = 0;
          unsigned long long time_single, time_total = 0, time_avg = 0;
          unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
          struct timeval tv_start;
          struct timeval tv_end;
          struct timeval timediff;

          unsigned int u32FrameNumMax = 32;
          unsigned int u32FrmCnt = 0;
          unsigned int u32FrmNum = 0;

          bmcv_ive_gmm2_ctrl gmm2Attr;
          gmm2Attr.u16_var_rate = 1;
          gmm2Attr.u8_model_num = 3;
          gmm2Attr.u9q7_max_var = (16 * 16) << 7;
          gmm2Attr.u9q7_min_var = (8 * 8) << 7;
          gmm2Attr.u8_glb_sns_factor = 8;
          gmm2Attr.en_sns_factor_mode =
                  (pixel_ctrl) ? SNS_FACTOR_MODE_PIX : SNS_FACTOR_MODE_GLB;
          gmm2Attr.u16_freq_thr = 12000;
          gmm2Attr.u16_freq_init_val = 20000;
          gmm2Attr.u16_freq_add_factor = 0xEF;
          gmm2Attr.u16_freq_redu_factor = 0xFF00;
          gmm2Attr.u16_life_thr = 5000;
          gmm2Attr.en_life_update_factor_mode = life_update_enMode;

          unsigned char* inputData = malloc(width * height * u32FrameNumMax * sizeof(unsigned char));
          FILE *input_fp = fopen(input_name, "rb");
          fread((void *)inputData, sizeof(unsigned char), width * height * u32FrameNumMax, input_fp);
          fclose(input_fp);

          unsigned char* srcData = malloc(width * height * sizeof(unsigned char));
          unsigned short* srcFactorData = malloc(width * height * sizeof(unsigned short));
          memset(srcData, 0, width * height * sizeof(unsigned char));
          memset(srcFactorData, 0, width * height * sizeof(unsigned short));

          if(pixel_ctrl){
              FILE *input_factor_fp = fopen(inputFactor_name, "rb");
              fread((void *)srcFactorData, sizeof(unsigned short), width * height, input_factor_fp);
              fclose(input_factor_fp);
          }

          int model_len = width * height * gmm2Attr.u8_model_num * 8;
          unsigned char* model_data = malloc(model_len * sizeof(unsigned char));
          memset(model_data, 0, model_len * sizeof(unsigned char));

          unsigned char* ive_fg_res = malloc(width * height * sizeof(unsigned char));
          unsigned char* ive_bg_res = malloc(width * height * sizeof(unsigned char));
          unsigned char* ive_pc_match_res = malloc(width * height * sizeof(unsigned char));

          memset(ive_fg_res, 0, width * height * sizeof(unsigned char));
          memset(ive_bg_res, 0, width * height * sizeof(unsigned char));
          memset(ive_pc_match_res, 0, width * height * sizeof(unsigned char));

          // calc ive image stride && create bm image struct
          bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, stride);

          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              goto fail;
          }

          bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_U16, factorStride);
          bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_U16, &src_factor, factorStride);
          ret = bm_image_alloc_dev_mem(src_factor, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              goto fail;
          }
          ret = bm_image_copy_host_to_device(src_factor, (void **)&srcFactorData);
          if (ret != BM_SUCCESS) {
              printf("bm_image_copy_host_to_device. ret = %d\n", ret);
              goto fail;
          }

          bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_fg, stride);
          ret = bm_image_alloc_dev_mem(dst_fg, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              goto fail;
          }

          bm_image_create(handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_bg, stride);
          ret = bm_image_alloc_dev_mem(dst_bg, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              goto fail;
          }

          bm_image_create(handle, height, width, FORMAT_GRAY,
                              DATA_TYPE_EXT_1N_BYTE, &dst_model_match_model_info, stride);
          ret = bm_image_alloc_dev_mem(dst_model_match_model_info, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              goto fail;
          }

          ret = bm_malloc_device_byte(handle, &dst_model, model_len);
          if (ret != BM_SUCCESS) {
              printf("bm_malloc_device_byte failed. ret = %d\n", ret);
              goto fail;
          }

          for (i = 0; i < loop_time; i++) {
              ret = bm_memcpy_s2d(handle, dst_model, model_data);
              if (ret != BM_SUCCESS) {
                  printf("bm_memcpy_s2d failed. ret = %d\n", ret);
                  goto fail;
              }

              for(u32FrmCnt = 0; u32FrmCnt < u32FrameNumMax; u32FrmCnt++){
                  if(width > 480){
                      for(int i = 0; i < 288; i++){
                          memcpy(srcData + (i * width),
                                inputData + (u32FrmCnt * 352 * 288 + i * 352), 352);
                          memcpy(srcData + (i * width + 352),
                                inputData + (u32FrmCnt * 352 * 288 + i * 352), 352);

                      }
                  } else {
                      for(int i = 0; i < 288; i++){
                          memcpy(srcData + i * stride[0],
                                inputData + u32FrmCnt * width * 288 + i * width, width);
                          int s = stride[0] - width;
                          memset(srcData + i * stride[0] + width, 0, s);
                      }
                  }

                  ret = bm_image_copy_host_to_device(src, (void**)&srcData);
                  if(ret != BM_SUCCESS){
                      printf("bm_image copy src h2d failed. ret = %d \n", ret);
                      goto fail;
                  }

                  u32FrmNum = u32FrmCnt + 1;
                  if(gmm2Attr.u8_model_num == 1)
                      gmm2Attr.u16_freq_redu_factor = (u32FrmNum >= 500) ? 0xFFA0 : 0xFC00;
                  else
                      gmm2Attr.u16_glb_life_update_factor =
                                            (u32FrmNum >= 500) ? 4 : 0xFFFF / u32FrmNum;

                  if(pixel_ctrl && u32FrmNum > 16)
                      gmm2Attr.en_life_update_factor_mode = LIFE_UPDATE_FACTOR_MODE_PIX;

                  gettimeofday(&tv_start, NULL);
                  ret = bmcv_ive_gmm2(handle, &src, &src_factor, &dst_fg,
                              &dst_bg, &dst_model_match_model_info, dst_model, gmm2Attr);
                  gettimeofday(&tv_end, NULL);
                  timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
                  timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
                  time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

                  if(time_single>time_max){time_max = time_single;}
                  if(time_single<time_min){time_min = time_single;}
                  time_total = time_total + time_single;

                  if(ret != BM_SUCCESS){
                      printf("bmcv_ive_gmm2 execution failed \n");
                      goto fail;
                  }
              }
          }

          time_avg = time_total / (loop_time * u32FrameNumMax);
          fps_actual = 1000000 / (time_avg * u32FrameNumMax);
          printf("idx:%d, bmcv_ive_gmm2: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                  ctx.i, loop_time, time_max, time_avg, fps_actual);
          return 0;
        }
