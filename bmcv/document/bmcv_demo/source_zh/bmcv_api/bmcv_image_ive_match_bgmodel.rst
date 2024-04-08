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