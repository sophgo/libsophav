bmcv_ive_match_bgmodel
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 输入当前图像和模型，通过当前帧数据和背景之间差异来，获取前景数据。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_match_bgmodel(
        bm_handle_t                 handle,
        bm_image                    cur_img,
        bm_image                    bgmodel_img,
        bm_image                    fgflag_img,
        bm_image                    diff_fg_img,
        bm_device_mem_t             stat_data_mem,
        bmcv_ive_match_bgmodel_attr   attr);

| 【参数】

.. list-table:: bmcv_ive_match_bgmodel 参数表
    :widths: 25 15 40

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \cur_img
      - 输入
      - 输入 bm_image 对象结构体, 表示当前图像，不能为空。
    * - \bgmodel_img
      - 输入/输出
      - 背景模型 bm_image 结构体，不能为空，高同 cur_img， 宽 = cur_img.width * sizeof(bm_ive_bg_model_pix)。
    * - fgflag_img
      - 输入/输出
      - 前景状态图像 bm_image 结构体, 不能为空，宽、高同 cur_img。
    * - diff_fg_img
      - 输出
      - 帧间差分前景图像 bm_image 结构体，不能为空，宽、高同 cur_img。
    * - stat_data_mem
      - 输出
      - 前景状态数据 bm_device_mem_t 结构体，不能为空，内存至少需配置 sizeof(bm_ive_bg_stat_data)。
    * - attr
      - 输入
      - 控制参数结构体, 不能为空。

.. list-table::
    :widths: 30 25 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - cur_img
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - bgmodel_img
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - --
    * - fgflag_img
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - diff_fg_img
      - GRAY
      - DATA_TYPE_EXT_S16
      - 64x64~1920x1080
    * - stat_data_mem
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - --


| 【数据类型说明】

【说明】定义背景模型数据。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_bg_model_pix_s{
        bmcv_ive_work_bg_pix st_work_bg_pixel;
        bmcv_ive_candi_bg_pix st_candi_pixel;
        bmcv_ive_bg_life st_bg_life;
    } bmcv_ive_bg_model_pix;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - st_work_bg_pixel
      - 工作背景数据。
    * - st_candi_pixel
      - 候选背景数据。
    * - st_bg_life
      - 背景生命力。

【说明】定义背景状态数据。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_bg_stat_data_s{
        unsigned int u32_pix_num;
        unsigned int u32_sum_lum;
        unsigned char u8_reserved[8];
    } bmcv_ive_bg_stat_data;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - u32_pix_num
      - 前景像素数目。
    * - u32_sum_lum
      - 输入图像的所有像素亮度累加和。
    * - u8_reserved
      - 保留字段。

【说明】定义背景匹配控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_match_bgmodel_attr_s{
        unsigned int u32_cur_frm_num;
        unsigned int u32_pre_frm_num;
        unsigned short u16_time_thr;
        unsigned char u8_diff_thr_crl_coef;
        unsigned char u8_diff_max_thr;
        unsigned char u8_diff_min_thr;
        unsigned char u8_diff_thr_inc;
        unsigned char u8_fast_learn_rate;
        unsigned char u8_det_chg_region;
    } bmcv_ive_match_bgmodel_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - u32_cur_frm_num
      - 当前帧时间。
    * - u32_pre_frm_num
      - 前一帧时间，要求 u32_pre_frm_num < u32_cur_frm_num
    * - u16_time_thr
      - 潜在背景替换时间阈值。

        取值范围：[2, 100]， 参考取值：20。
    * - u8_diff_thr_crl_coef
      - 差分阈值与灰度值相关系数。

        取值范围：[0, 5]， 参考取值：0。
    * - u8_diff_max_thr
      - 背景差分阈值调整上限。

        取值范围：[3, 15]， 参考取值：6。
    * - u8_diff_min_thr
      - 背景差分阈值调整下限。

        取值范围：[3, diff_max_thr_u8]， 参考取值：4。
    * - u8_diff_thr_inc
      - 动态背景下差分阈值增量。

        取值范围：[0, 6]， 参考取值：0。
    * - u8_fast_learn_rate
      - 快速背景学习速率。

        取值范围：[0, 4]， 参考取值：2。
    * - u8_det_chg_region
      - 是否检测变化区域。

        取值范围：{0，1}， 0 表示不检测，1表示检测；参考取值为0。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。


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

      int test_loop_times  = 1;
      int test_threads_num = 1;
      int dev_id = 0;
      int bWrite = 0;
      int height = 288, width = 352;
      bm_image_format_ext fmt = FORMAT_GRAY;
      char *input_name = "./ive_data/campus.u8c1.1_100.raw";
      char *bgModel_name = "bgmodel_bgmdl_output_100.bin";
      bm_handle_t handle = NULL;

      bm_status_t init_match_bgmodel(bm_handle_t handle, bm_image* pstCurImg,
                          bm_image* pstBgModel, bm_image* pstFgFlag,
                          bm_image* pstDiffFg, bm_device_mem_t* pstStatData,
                          bmcv_ive_match_bgmodel_attr* matchBgmodel_attr,
                          int width, int height)
      {
          bm_status_t ret = BM_SUCCESS;
          int u8Stride[4] = {0}, s16Stride[4] = {0}, bgModelStride[4] = {0};

          bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, u8Stride);
          bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_S16, s16Stride);
          bm_ive_image_calc_stride(handle, height, width * sizeof(bmcv_ive_bg_model_pix),
                                              fmt, DATA_TYPE_EXT_1N_BYTE, bgModelStride);

          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, pstCurImg, u8Stride);
          if(ret != BM_SUCCESS){
              printf("pstCurImg bm_image_create failed , ret = %d \n", ret);
              return ret;
          }

          ret = bm_image_create(handle, height, width * sizeof(bmcv_ive_bg_model_pix),
                                      fmt, DATA_TYPE_EXT_1N_BYTE, pstBgModel, bgModelStride);
          if(ret != BM_SUCCESS){
              printf("pstBgModel bm_image_create failed , ret = %d \n", ret);
              return ret;
          }

          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, pstFgFlag, u8Stride);
          if(ret != BM_SUCCESS){
              printf("pstFgFlag bm_image_create failed , ret = %d \n", ret);
              return ret;
          }

          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_S16, pstDiffFg, s16Stride);
          if(ret != BM_SUCCESS){
              printf("pstDiffFg bm_image_create failed , ret = %d \n", ret);
              return ret;
          }

          ret = bm_image_alloc_dev_mem(*pstCurImg, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("pstCurImg bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              return ret;
          }

          ret = bm_image_alloc_dev_mem(*pstFgFlag, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("pstFgFlag bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              return ret;
          }

          ret = bm_image_alloc_dev_mem(*pstDiffFg, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("pstDiffFg bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              return ret;
          }

          ret = bm_image_alloc_dev_mem(*pstBgModel, BMCV_HEAP_ANY);
          if(ret != BM_SUCCESS){
              printf("pstBgModel bm_malloc_device_byte. ret = %d\n", ret);
              return ret;
          }

          ret = bm_malloc_device_byte(handle, pstStatData, sizeof(bmcv_ive_bg_stat_data));
          if(ret != BM_SUCCESS){
              printf("pstStatData bm_malloc_device_byte. ret = %d\n", ret);
              return ret;
          }

          // init match_bgmodel config
          matchBgmodel_attr->u32_cur_frm_num = 0;
          matchBgmodel_attr->u32_pre_frm_num = 0;
          matchBgmodel_attr->u16_time_thr = 20;
          matchBgmodel_attr->u8_diff_thr_crl_coef = 0;
          matchBgmodel_attr->u8_diff_max_thr = 10;
          matchBgmodel_attr->u8_diff_min_thr = 10;
          matchBgmodel_attr->u8_diff_thr_inc = 0;
          matchBgmodel_attr->u8_fast_learn_rate = 4;
          matchBgmodel_attr->u8_det_chg_region = 1;

          return ret;
      }

      bm_status_t destory_match_bgmodel(bm_handle_t handle, bm_image* pstCurImg,
                          bm_image* pstBgModel, bm_image* pstFgFlag,
                          bm_image* pstDiffFg, bm_device_mem_t* pstStatData)
      {
          bm_image_destroy(pstCurImg);
          bm_image_destroy(pstFgFlag);
          bm_image_destroy(pstDiffFg);
          bm_image_destroy(pstBgModel);
          bm_free_device(handle, *pstStatData);

          return BM_SUCCESS;
      }

      bm_status_t init_update_bgmodel(bm_handle_t handle, bm_image* pstBgImg, bm_image* pstChgSta,
                                    bmcv_ive_update_bgmodel_attr* update_bgmodel_attr,
                                    int width, int height)
      {
          int bg_img_stride[4] = {0}, chg_sta_stride[4] = {0};
          bm_status_t ret = BM_SUCCESS;

          bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, bg_img_stride);
          bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_U32, chg_sta_stride);

          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, pstBgImg, bg_img_stride);
          if(ret != BM_SUCCESS){
              printf("pstBgImg bm_image_create failed , ret = %d \n", ret);
              return ret;
          }

          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U32, pstChgSta, chg_sta_stride);
          if(ret != BM_SUCCESS){
              printf("pstChgSta bm_image_create failed , ret = %d \n", ret);
              return ret;
          }

          ret = bm_image_alloc_dev_mem(*pstBgImg, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("pstBgImg bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              return ret;
          }

          ret = bm_image_alloc_dev_mem(*pstChgSta, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("pstChgSta bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              return ret;
          }

          // init update_bgmodel config
          update_bgmodel_attr->u32_cur_frm_num = 0;
          update_bgmodel_attr->u32_pre_chk_time = 0;
          update_bgmodel_attr->u32_frm_chk_period = 30;
          update_bgmodel_attr->u32_init_min_time = 25;
          update_bgmodel_attr->u32_sty_bg_min_blend_time = 100;
          update_bgmodel_attr->u32_sty_bg_max_blend_time = 1500;
          update_bgmodel_attr->u32_dyn_bg_min_blend_time = 0;
          update_bgmodel_attr->u32_static_det_min_time = 80;
          update_bgmodel_attr->u16_fg_max_fade_time = 15;
          update_bgmodel_attr->u16_bg_max_fade_time = 60;
          update_bgmodel_attr->u8_sty_bg_acc_time_rate_thr = 80;
          update_bgmodel_attr->u8_chg_bg_acc_time_rate_thr = 60;
          update_bgmodel_attr->u8_dyn_bg_acc_time_thr = 0;
          update_bgmodel_attr->u8_dyn_bg_depth = 3;
          update_bgmodel_attr->u8_bg_eff_sta_rate_thr = 90;
          update_bgmodel_attr->u8_acce_bg_learn = 0;
          update_bgmodel_attr->u8_det_chg_region = 1;

          return ret;
      }

      bm_status_t destory_update_bgmodel(bm_image* pstBgImg, bm_image* pstChgSta)
      {
          bm_image_destroy(pstBgImg);
          bm_image_destroy(pstChgSta);

          return BM_SUCCESS;
      }

      static void * ive_bgmodel()
      {
          bm_status_t ret;
          int srcStride[4];
          unsigned int i = 0, loop_time = 0;
          unsigned long long match_time_single, match_time_total = 0, match_time_avg = 0;
          unsigned long long match_time_max = 0, match_time_min = 10000, match_fps_actual = 0;

          unsigned long long update_time_single, update_time_total = 0, update_time_avg = 0;
          unsigned long long update_time_max = 0, update_time_min = 10000, update_fps_actual = 0;


          struct timeval tv_start;
          struct timeval tv_end;
          struct timeval timediff;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          loop_time = 1;
          int update_cnt = 0;

          int frameNum;
          int frameNumMax = 100;
          int frmCnt = 0;

          int buf_size = frameNumMax * width * height;
          unsigned char *input_data = (unsigned char*)malloc(buf_size * sizeof(unsigned char));
          unsigned char *src_data = (unsigned char*)malloc(buf_size * sizeof(unsigned char));
          unsigned char *bgmodel_res = (unsigned char*) malloc(width * sizeof(bmcv_ive_bg_model_pix) * height);

          memset(input_data, 0, buf_size * sizeof(unsigned char));
          memset(src_data, 0, buf_size * sizeof(unsigned char));
          memset(bgmodel_res, 0, width * sizeof(bmcv_ive_bg_model_pix) * height);

          // create src image
          bm_image src, stFgFlag, stDiffFg, stBgModel;
          bm_device_mem_t stStatData;
          bmcv_ive_match_bgmodel_attr matchBgModelAttr;

          ret = init_match_bgmodel(handle, &src, &stBgModel, &stFgFlag,
                          &stDiffFg, &stStatData, &matchBgModelAttr, width, height);
          if(ret != BM_SUCCESS){
              printf("init_match_bgmodel failed \n");
              exit(-1);
          }

          ret = bm_image_get_stride(src, srcStride);
          if(ret != BM_SUCCESS){
              printf("bm_image_get_stride failed \n");
              exit(-1);
          }

          // create dst image
          bm_image stBgImg, stChgStaLife;
          bmcv_ive_update_bgmodel_attr updateBgModelAttr;
          ret = init_update_bgmodel(handle, &stBgImg, &stChgStaLife, &updateBgModelAttr, width, height);
          if(ret != BM_SUCCESS){
              printf("init_update_bgmodel failed \n");
              exit(-1);
          }

          unsigned char* fg_flag = malloc(width * height);
          unsigned char* bg_model =
                (unsigned char*)malloc(width * sizeof(bmcv_ive_bg_model_pix) * height);

          bmcv_ive_bg_stat_data *stat = malloc(sizeof(bmcv_ive_bg_stat_data));

          memset(fg_flag, 0, width * height);
          memset(bg_model, 0, width * sizeof(bmcv_ive_bg_model_pix) * height);

          for(i = 0; i < loop_time; i++){
              int updCnt = 5;
              int preUpdTime = 0;
              int preChkTime = 0;
              int frmUpdPeriod = 10;
              int frmChkPeriod = 30;

              ret = bm_image_copy_host_to_device(stFgFlag, (void**)&fg_flag);
              if(ret != BM_SUCCESS){
                  printf("stFgFlag bm_image_copy_host_to_device. ret = %d\n", ret);
                  exit(-1);
              }

            ret = bm_image_copy_host_to_device(stBgModel, (void**)&bg_model);
            if(ret != BM_SUCCESS){
                printf("pstBgModel bm_image_copy_host_to_device. ret = %d\n", ret);
                exit(-1);
            }

              // config setting
              matchBgModelAttr.u32_cur_frm_num = frmCnt;
              for(frmCnt = 0; frmCnt < frameNumMax; frmCnt++){
                  frameNum = frmCnt + 1;
                  if(width > 480){
                      for(int j = 0; j < 288; j++){
                          memcpy(src_data + (j * width),
                                input_data + (frmCnt * 352 * 288 + j * 352), 352);
                          memcpy(src_data + (j * width + 352),
                                input_data + (frmCnt * 352 * 288 + j * 352), 352);
                      }
                  } else {
                      for(int j = 0; j < 288; j++){
                          memcpy(src_data + j * srcStride[0],
                                input_data + frmCnt * width * 288 + j * width, width);
                          int s = srcStride[0] - width;
                          memset(src_data + j * srcStride[0] + width, 0, s);
                      }
                  }

                  ret = bm_image_copy_host_to_device(src, (void**)&src_data);
                  if(ret != BM_SUCCESS){
                      printf("bm_image copy src h2d failed. ret = %d \n", ret);
                      exit(-1);
                  }

                  matchBgModelAttr.u32_pre_frm_num = matchBgModelAttr.u32_cur_frm_num;
                  matchBgModelAttr.u32_cur_frm_num = frameNum;
                  gettimeofday(&tv_start, NULL);
                  ret = bmcv_ive_match_bgmodel(handle, src, stBgModel,
                                  stFgFlag, stDiffFg, stStatData, matchBgModelAttr);
                  gettimeofday(&tv_end, NULL);
                  if(ret != BM_SUCCESS){
                      printf("bmcv_ive_match_bgmodel failed. ret = %d \n", ret);
                      exit(-1);
                  }
                  timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
                  timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
                  match_time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

                  if(match_time_single > match_time_max){match_time_max = match_time_single;}
                  if(match_time_single < match_time_min){match_time_min = match_time_single;}
                  match_time_total = match_time_total + match_time_single;

                  ret = bm_memcpy_d2s(handle, (void*)stat, stStatData);
                  if(ret != BM_SUCCESS){
                      printf("stStatData bm_memcpy_d2s failed \n");
                      exit(-1);
                  }

                  printf("BM_IVE_MatchBgModel u32UpdCnt %d, u32FrameNum %d, frm %d, ", updCnt, frameNum, frmCnt);
                  printf("stat pix_num_u32 = %d, sum_lum_u32 = %d\n", stat->u32_pix_num, stat->u32_sum_lum);

                  if(updCnt == 0 || frameNum >= preUpdTime + frmUpdPeriod){
                      updCnt++;
                      preUpdTime = frameNum;
                      updateBgModelAttr.u32_cur_frm_num = frameNum;
                      updateBgModelAttr.u32_pre_chk_time = preChkTime;
                      updateBgModelAttr.u32_frm_chk_period = 0;
                      if(frameNum >= preChkTime + frmChkPeriod){
                          updateBgModelAttr.u32_frm_chk_period = frmChkPeriod;
                          preChkTime = frameNum;
                      }

                      gettimeofday(&tv_start, NULL);
                      ret = bmcv_ive_update_bgmodel(handle, &src, &stBgModel,
                                  &stFgFlag, &stBgImg, &stChgStaLife, stStatData, updateBgModelAttr);
                      gettimeofday(&tv_end, NULL);
                      if(ret != BM_SUCCESS){
                          printf("bmcv_ive_update_bgmodel failed, ret = %d \n", ret);
                          exit(-1);
                      }
                      timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
                      timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
                      update_time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

                      if(update_time_single > update_time_max){update_time_max = update_time_single;}
                      if(update_time_single < update_time_min){update_time_min = update_time_single;}
                      update_time_total = update_time_total + update_time_single;
                      update_cnt++;

                      ret = bm_memcpy_d2s(handle, (void*)stat, stStatData);
                      if(ret != BM_SUCCESS){
                          printf("stStatData bm_memcpy_d2s failed \n");
                          exit(-1);
                      }
                      printf("BM_IVE_UpdateBgModel frm %d ,", frmCnt);
                      printf("stat pix_num_u32 = %d, sum_lum_u32 = %d\n", stat->u32_pix_num, stat->u32_sum_lum);
                  }
              }


          }
          free(input_data);
          free(src_data);
          free(fg_flag);
          free(bg_model);
          free(stat);

          match_time_avg = match_time_total / (loop_time * frameNumMax);
          match_fps_actual = 1000000 / (match_time_avg * frameNumMax);

          update_time_avg = update_time_total / update_cnt;
          update_fps_actual = 1000000 / (update_time_avg * update_cnt);

          if(bWrite){
              FILE *fp = fopen(bgModel_name, "wb");
              fwrite((void *)bgmodel_res, 1, width * sizeof(bmcv_ive_bg_model_pix) * height, fp);
              fclose(fp);
          }

          free(bgmodel_res);
          destory_match_bgmodel(handle, &src, &stBgModel, &stFgFlag, &stDiffFg, &stStatData);
          destory_update_bgmodel(&stBgImg, &stChgStaLife);

          char fmt_str[100];
          format_to_str(src.image_format, fmt_str);



          printf(" bmcv_ive_match_bgmodel: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                  loop_time, match_time_max, match_time_avg, match_fps_actual);

          printf(" bmcv_ive_update_bgmodel: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                  loop_time, update_time_max, update_time_avg, update_fps_actual);
          printf("bmcv ive bgmodel test successful \n");

          return 0;
      }

      int main(){
        ive_bgmodel();
          return 0;
      }
