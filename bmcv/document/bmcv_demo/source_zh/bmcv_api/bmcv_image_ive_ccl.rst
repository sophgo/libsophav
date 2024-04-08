bmcv_ive_ccl
------------------------------

| 【描述】

| 该 API 使用ive硬件资源的 ccl 功能, 创建二值图像的连通区域标记任务，即图像中具有相同像素值且位置相邻的前景像素点组成的图像区域。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_ccl(
        bm_handle_t      handle,
        bm_image         src_dst_image,
        bm_device_mem_t  ccblob_output,
        bmcv_ive_ccl_attr  ccl_attr);

| 【参数】

.. list-table:: bmcv_ive_ccl 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \src_dst_image
      - 输入、输出
      - 输入 bm_image 对象结构体, 连通区域标记在输入图像上进行，即输入图像同时也是标记图像输出, 不能为空。
    * - \ccblob_output
      - 输出
      - 连通区域信息数据结构体，不能为空，内存至少需要配置 sizoef(bm_ive_ccblob) 大小, 最多输出254个有效连通区域。
    * - \ccl_attr
      - 输入
      - 连通区域标记控制参数结构体, 不能为空。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - srcDstImage
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080

| 【数据类型说明】

【说明】定义连通区域信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_region_s{
        int u32_area;
        unsigned short u16_left;
        unsigned short u16_right;
        unsigned short u16_top;
        unsigned short u16_bottom;
    } bmcv_ive_region;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - u32_area
      - 连通区域面积，以及连通区域像素数目。
    * - u16_left
      - 连通区域外接矩形的最左边坐标。
    * - u16_right
      - 连通区域外接矩形的最右边坐标。
    * - u16_top
      - 连通区域外接矩形的最上边坐标。
    * - u16_bottom
      - 连通区域外接矩形的最下边坐标。

【说明】定义连通区域标记的输出信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_ccblob_s{
        unsigned short    u16_cur_aera_thr;
        signed char       s8_label_status;
        unsigned char     u8_region_num;
        bmcv_ive_region   ast_region[BM_IVE_MAX_REGION_NUM];
    } bmcv_ive_ccblob;


.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - u16_cur_aera_thr
      - 有效连通区域的面积阈值，astRegion 中面积小于这个阈值的都被置为 0 。
    * - s8_label_status
      - 连通区域标记是否成功。1： 标记失败; 0：标记成功。
    * - u8_region_num
      - 有效连通区域个数。
    * - ast_region
      - 连通区域信息：有效的连通区域其面积大于 0, 对应标记为数组下标加 1。

【说明】定义连通区域模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_ccl_mode_e{
        BM_IVE_CCL_MODE_4C = 0x0,
        BM_IVE_CCL_MODE_8C = 0x1,
    } bmcv_ive_ccl_mode;

.. list-table::
    :widths: 50 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_CCL_MODE_4C
      - 4 连通。
    * - BM_IVE_CCL_MODE_8C
      - 8 连通。

【说明】定义连通区域标记控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_ccl_attr_s{
        bmcv_ive_ccl_mode en_mode;
        unsigned short  u16_init_area_thr;
        unsigned short  u16_step;
    } bmcv_ive_ccl_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - 连通区域模式。
    * - u16_init_area_thr
      - 初始面积阈值。

        取值范围：[0, 65535]，参考取值：4。
    * - u16_step
      - 面积阈值增长步长。

        取值范围：[1, 65535]，参考取值：2。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 连通区域的信息保存在 ccblob_output.ast_region 中。

3. ccblob_output.u8_region_num 表示有效的连通区域数目，最多 254 个有效连通区域；有效的连通区域的面积大于 ccblob_output.u16_cur_aera_thr, 标记号为其所在 ccblob_output.ast_region 数组元素的下标 + 1。有效的连通区域并不一定连续地存储在数组中, 而很可能是间断的分布在数组中。

4. 若 ccblob_output.s8_label_status 为0, 则标记成功(一个区域一个标记); 若为 -1, 则标记失败(一个区域多个标记或者多个区域共用一个标记)。对于后者， 若用户需要正确的标记号， 还需要再次根据 ccblob_output 中的外接矩形信息重新标记。 不管标记是否成功，连通区域的外接矩形信息一定是正确可用的。

5. 输出的连通区域会用 ccl_attr.u16_init_area_thr 进行筛选， 面积小于等于pstCclCtrl→u16_init_area_thr 均会被置为 0。

6. 当连通区域数目大于 254, 会用 ccl_attr.u16_init_area_thr 删除面积小的连通区域；若 ccl_attr.u16_init_area_thr 不满足删除条件，会以 pstCclCtrl→u16_step 为步长，增大删除连通区域的面积阈值。

7. 最终的面积阈值存储在 ccblob_output.u16_cur_aera_thr 中。