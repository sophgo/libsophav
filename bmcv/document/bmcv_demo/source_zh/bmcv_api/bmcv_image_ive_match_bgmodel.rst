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
      #include <math.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

      int main(){
          int dev_id = 0;
          int height = 1080, width = 1920;
          bm_image_format_ext fmt = FORMAT_GRAY;
          char *input_name = "path/to/input";
          char *ref_name = "path/to/dst";
          bm_handle_t handle = NULL;

          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          int srcStride[4];

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

          FILE *input_fp = fopen(input_name, "rb");
          if(input_fp == NULL){
              printf("%s : No such file \n", ref_name);
              exit(-1);
          }
          fread((void *)input_data, 1, buf_size * sizeof(unsigned char), input_fp);
          fclose(input_fp);

          // create src image
          bm_image src, stFgFlag, stDiffFg, stBgModel;
          bm_device_mem_t stStatData;
          bmcv_ive_match_bgmodel_attr matchBgmodel_attr;

          int u8Stride[4] = {0}, s16Stride[4] = {0}, bgModelStride[4] = {0};

          int data_size = 1;
          u8Stride[0] = align_up(width, 16) * data_size;
          bgModelStride[0] = align_up(width * sizeof(bmcv_ive_bg_model_pix), 16) * data_size;
          data_size = 2;
          s16Stride[0] = align_up(width, 16) * data_size;

          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, u8Stride);
          ret = bm_image_create(handle, height, width * sizeof(bmcv_ive_bg_model_pix), fmt, DATA_TYPE_EXT_1N_BYTE, &stBgModel, bgModelStride);
          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stFgFlag, u8Stride);
          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_S16, &stDiffFg, s16Stride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(stFgFlag, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(stDiffFg, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(stBgModel, BMCV_HEAP1_ID);
          ret = bm_malloc_device_byte(handle, &stStatData, sizeof(bmcv_ive_bg_stat_data));

          // init match_bgmodel config
          matchBgmodel_attr.u32_cur_frm_num = 0;
          matchBgmodel_attr.u32_pre_frm_num = 0;
          matchBgmodel_attr.u16_time_thr = 20;
          matchBgmodel_attr.u8_diff_thr_crl_coef = 0;
          matchBgmodel_attr.u8_diff_max_thr = 10;
          matchBgmodel_attr.u8_diff_min_thr = 10;
          matchBgmodel_attr.u8_diff_thr_inc = 0;
          matchBgmodel_attr.u8_fast_learn_rate = 4;
          matchBgmodel_attr.u8_det_chg_region = 1;

          data_size = 1;
          srcStride[0] = align_up(width, 16) * data_size;

          // create dst image
          bm_image stBgImg, stChgStaLife;
          bmcv_ive_update_bgmodel_attr update_bgmodel_attr;

          int bg_img_stride[4] = {0}, chg_sta_stride[4] = {0};

          data_size = 1;
          bg_img_stride[0] = align_up(width, 16) * data_size;

          data_size = 4;
          chg_sta_stride[0] = align_up(width, 16) * data_size;

          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &stBgImg, bg_img_stride);
          ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U32, &stChgStaLife, chg_sta_stride);
          ret = bm_image_alloc_dev_mem(stBgImg, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(stChgStaLife, BMCV_HEAP1_ID);

          // init update_bgmodel config
          update_bgmodel_attr.u32_cur_frm_num = 0;
          update_bgmodel_attr.u32_pre_chk_time = 0;
          update_bgmodel_attr.u32_frm_chk_period = 30;
          update_bgmodel_attr.u32_init_min_time = 25;
          update_bgmodel_attr.u32_sty_bg_min_blend_time = 100;
          update_bgmodel_attr.u32_sty_bg_max_blend_time = 1500;
          update_bgmodel_attr.u16_fg_max_fade_time = 15;
          update_bgmodel_attr.u16_bg_max_fade_time = 60;
          update_bgmodel_attr.u8_sty_bg_acc_time_rate_thr = 80;
          update_bgmodel_attr.u8_chg_bg_acc_time_rate_thr = 60;
          update_bgmodel_attr.u8_dyn_bg_acc_time_thr = 0;
          update_bgmodel_attr.u8_dyn_bg_depth = 3;
          update_bgmodel_attr.u8_bg_eff_sta_rate_thr = 90;
          update_bgmodel_attr.u8_acce_bg_learn = 0;
          update_bgmodel_attr.u8_det_chg_region = 1;

          unsigned char* fg_flag = malloc(width * height);
          unsigned char* bg_model = (unsigned char*)malloc(width * sizeof(bmcv_ive_bg_model_pix) * height);

          bmcv_ive_bg_stat_data *stat = malloc(sizeof(bmcv_ive_bg_stat_data));

          memset(fg_flag, 0, width * height);
          memset(bg_model, 0, width * sizeof(bmcv_ive_bg_model_pix) * height);

          for(int i = 0; i < 1; i++){
              int updCnt = 5;
              int preUpdTime = 0;
              int preChkTime = 0;
              int frmUpdPeriod = 10;
              int frmChkPeriod = 30;

              ret = bm_image_copy_host_to_device(stFgFlag, (void**)&fg_flag);
              ret = bm_image_copy_host_to_device(stBgModel, (void**)&bg_model);

              // config setting
              matchBgmodel_attr.u32_cur_frm_num = frmCnt;
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

                  matchBgmodel_attr.u32_pre_frm_num = matchBgmodel_attr.u32_cur_frm_num;
                  matchBgmodel_attr.u32_cur_frm_num = frameNum;
                  ret = bmcv_ive_match_bgmodel(handle, src, stBgModel,
                                  stFgFlag, stDiffFg, stStatData, matchBgmodel_attr);

                  ret = bm_memcpy_d2s(handle, (void*)stat, stStatData);

                  if(updCnt == 0 || frameNum >= preUpdTime + frmUpdPeriod){
                      updCnt++;
                      preUpdTime = frameNum;
                      update_bgmodel_attr.u32_cur_frm_num = frameNum;
                      update_bgmodel_attr.u32_pre_chk_time = preChkTime;
                      update_bgmodel_attr.u32_frm_chk_period = 0;
                      if(frameNum >= preChkTime + frmChkPeriod){
                          update_bgmodel_attr.u32_frm_chk_period = frmChkPeriod;
                          preChkTime = frameNum;
                      }

                      ret = bmcv_ive_update_bgmodel(handle, &src, &stBgModel, &stFgFlag, &stBgImg, &stChgStaLife, stStatData, update_bgmodel_attr);
                      ret = bm_memcpy_d2s(handle, (void*)stat, stStatData);
                  }
              }

              ret = bm_image_copy_device_to_host(stBgModel, (void**)&bgmodel_res);
          }
          free(input_data);
          free(src_data);
          free(fg_flag);
          free(bg_model);
          free(stat);

          FILE *fp = fopen(ref_name, "wb");
          fwrite((void *)bgmodel_res, 1, width * sizeof(bmcv_ive_bg_model_pix) * height, fp);
          fclose(fp);

          free(bgmodel_res);

          bm_image_destroy(&src);
          bm_image_destroy(&stFgFlag);
          bm_image_destroy(&stDiffFg);
          bm_image_destroy(&stBgModel);
          bm_free_device(handle, stStatData);

          bm_image_destroy(&stBgImg);
          bm_image_destroy(&stChgStaLife);

          bm_dev_free(handle);
          return ret;
      }
