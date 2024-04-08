bmcv_ive_update_bgmodel
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 输入当前图像和模型，更新背景模型。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_update_bgmodel(
        bm_handle_t                handle,
        bm_image  *                cur_img,
        bm_image  *                bgmodel_img,
        bm_image  *                fgflag_img,
        bm_image  *                bg_img,
        bm_image  *                chgsta_img,
        bm_device_mem_t            stat_data_mem,
        bmcv_ive_update_bgmodel_attr attr);

| 【参数】

.. list-table:: bmcv_ive_update_bgmodel 参数表
    :widths: 25 15 40

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*cur_img
      - 输入
      - 输入 bm_image 对象指针, 当前图像指针，不能为空。
    * - \*bgmodel_img
      - 输入/输出
      - 背景模型数据 bm_image 对象指针，不能为空。
    * - \*fgflag_img
      - 输入/输出
      - 前景状态图像 bm_image 对象指针, 不能为空。
    * - \*bg_img
      - 输出
      - 背景图像 bm_image 对象指针，不能为空，宽、高同 fgflag_img。
    * - \*chgsta_img
      - 输出
      - 前景更新状态图像 bm_image 对象指针，当 updateBgmodel_attr.u8DetChgRegion 为 0 时，可以为空，宽、高同 fgflag_img。
    * - \stat_data_mem
      - 输出
      - 前景状态数据结构体，不能为空，内存至少需配置 sizeof(bm_ive_bg_stat_data)。
    * - \attr
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
    * - bg_img
      - GRAY
      - DATA_TYPE_EXT_S16
      - 64x64~1920x1080
    * - chgsta_img
      - GRAY
      - DATA_TYPE_EXT_U32
      - 64x64~1920x1080


| 【数据类型说明】

【说明】定义背景更新控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_update_bgmodel_attr_s{
        unsigned int u32_cur_frm_num;
        unsigned int u32_pre_chk_time;
        unsigned int u32_frm_chk_period;
        unsigned int u32_init_min_time;
        unsigned int u32_sty_bg_min_blend_time;
        unsigned int u32_sty_bg_max_blend_time;
        unsigned int u32_dyn_bg_min_blend_time;
        unsigned int u32_static_det_min_time;
        unsigned short u16_fg_max_fade_time;
        unsigned short u16_bg_max_fade_time;
        unsigned char u8_sty_bg_acc_time_rate_thr;
        unsigned char u8_chg_bg_acc_time_rate_thr;
        unsigned char u8_dyn_bg_acc_time_thr;
        unsigned char u8_dyn_bg_depth;
        unsigned char u8_bg_eff_sta_rate_thr;
        unsigned char u8_acce_bg_learn;
        unsigned char u8_det_chg_region;
    } bmcv_ive_update_bgmodel_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - u32_cur_frm_num
      - 当前帧时间。
    * - u32_pre_chk_time
      - 上次进行背景状态检查的时间点。
    * - u32_frm_chk_period
      - 背景状态检查时间周期。

        取值范围：[0, 2000]。参考取值：50。
    * - u32_init_min_time
      - 背景初始化最短时间。

        取值范围：[20, 6000]。参考取值：100。
    * - u32_sty_bg_min_blend_time
      - 稳定背景融入最短时间

        取值范围：[u32_init_min_time, 6000]。参考取值：20。
    * - u32_sty_bg_max_blend_time
      - 稳定背景融入最长时间

        取值范围：[u32_sty_bg_min_blend_time, 40000]。参考取值：1500。
    * - u32_dyn_bg_min_blend_time
      - 动态背景融入最短时间。

        取值范围：[0, 6000]。参考取值：1500。
    * - u32_static_det_min_time
      - 静物检测最短时间。

        取值范围：[20, 6000]。参考取值：80。
    * - u16_fg_max_fade_time
      - 前景持续消失最长时间。

        取值范围：[1, 255]。参考取值：15。
    * - u16_bg_max_fade_time
      - 背景持续消失时间。

        取值范围：[1, 255]。参考取值：60。
    * - u8_sty_bg_acc_time_rate_thr
      - 稳态背景访问时间比率阈值。

        取值范围：[10, 100]。参考取值：80。
    * - u8_chg_bg_acc_time_rate_thr
      - 变化背景访问时间比率阈值。

        取值范围：[10, 100]。参考取值：60。
    * - u8_dyn_bg_acc_time_thr
      - 动态背景访问时间比率阈值。

        取值范围：[0, 50]。参考取值：0。
    * - u8_dyn_bg_depth
      - 动态背景深度。

        取值范围：[0, 3]。参考取值：3。
    * - u8_bg_eff_sta_rate_thr
      - 背景初始化阶段背景状态时间比率阈值。

        取值范围：[90, 100]。参考取值：90。
    * - u8_acce_bg_learn
      - 是否加速背景学习。

        取值范围：{0, 1}，0 表示不加速，1 表示加速。参考取值：0。
    * - u8_det_chg_region
      - 是否检测变化区域。

        取值范围：{0, 1}，0 表示不检测，1 表示检测。参考取值：0。

| 【注意事项】

要求 :math:`\leq u32InitMinTime \leq u32StyBgMinBlendTime \leq u32StyBgMaxBlendTime`

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入输出图像的 width 都需要16对齐。

* 变化状态指像素值发生变化而成为前景，并且变化后的像素值较长时间都保持稳定的状态，这一般是由静止遗留物或者静止移走物在图像中产生。