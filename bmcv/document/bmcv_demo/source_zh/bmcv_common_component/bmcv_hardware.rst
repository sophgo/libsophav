BMCV 接口总览
------------------

| 将 BMCV API 按实现的硬件分类，并简要说明功能。

.. list-table:: 多模块实现接口
    :widths: 3 27 20 15

    * - **num**
      - **API**
      - **功能**
      - **硬件**
    * - 1
      - bmcv_image_copy_to
      - 拷贝到目的图指定区域
      - TPU / VPSS
    * - 2
      - bmcv_image_put_text
      - 写字
      - CPU / VPSS
    * - 3
      - bmcv_image_convert_to
      - 线性变换
      - TPU / VPSS
    * - 4
      - bmcv_image_rotate
      - 旋转
      - LDC / VPSS
    * - 5
      - bmcv_image_rotate_trans
      - 旋转(指定TPU实现)
      - TPU / VPSS

.. list-table:: VPSS实现接口
    :widths: 3 36 26

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_image_vpp_convert
      - crop+色彩转换+缩放
    * -
      - bmcv_image_vpp_convert_padding
      - 填充背景色
    * - 2
      - bmcv_image_vpp_csc_matrix_convert
      - 自定义矩阵色彩转换
    * - 3
      - bmcv_image_vpp_basic
      - 多功能组合
    * - 4
      - bmcv_image_csc_convert_to
      - 多功能组合+线性变换
    * - 5
      - bmcv_image_draw_rectangle
      - 画空心框
    * - 6
      - bmcv_image_fill_rectangle
      - 画实心框
    * - 7
      - bmcv_image_flip
      - 图像横向/纵向翻转
    * - 8
      - bmcv_image_mosaic
      - 图像局部马赛克
    * - 9
      - bmcv_image_storage_convert
      - 色彩转换+缩放
    * -
      - bmcv_image_storage_convert_with_csctype
      - 可选标准色彩转换+缩放
    * - 10
      - bmcv_image_resize
      - 图像缩放
    * - 11
      - bmcv_image_watermark_superpose
      - 水印叠加
    * -
      - bmcv_image_watermark_repeat_superpose
      - 重复水印叠加
    * - 12
      - bmcv_image_overlay
      - OSD叠加
    * - 13
      - bmcv_image_vpp_stitch
      - 图像拼接(无重叠)
    * - 14
      - bmcv_image_circle
      - 画圆
    * - 15
      - bmcv_image_draw_point
      - 画正方形实心点
    * - 16
      - bmcv_image_csc_overlay
      - 多功能组合+OSD

.. list-table:: JPEG实现接口
    :widths: 3 36 26

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_image_jpeg_dec
      - 图像解码
    * - 2
      - bmcv_image_jpeg_enc
      - 图像编码

.. list-table:: TPU实现接口
    :widths: 3 42 20

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_image_absdiff
      - 两图相减取绝对值
    * - 2
      - bmcv_image_axpy
      - F=A*X+Y的缩放加法
    * - 3
      - bmcv_image_add_weighted
      - 两图加权融合
    * - 4
      - bmcv_image_bitwise_and
      - 按位与
    * -
      - bmcv_image_bitwise_or
      - 按位或
    * -
      - bmcv_image_bitwise_xor
      - 按位异或
    * - 5
      - bmcv_image_width_align
      - 按指定宽对齐
    * - 6
      - bmcv_sort
      - 浮点数据排序
    * - 7
      - bmcv_image_threshold
      - 像素阈值化
    * - 8
      - bmcv_feature_match
      - 特征比对(INT8)
    * -
      - bmcv_feature_match_normalized
      - 特征比对(FP32)
    * - 9
      - bmcv_hist_balance
      - 直方图均衡化
    * - 10
      - bmcv_image_quantify
      - FP32转INT8
    * - 11
      - bmcv_faiss_indexflatIP
      - 向量相似度搜索
    * -
      - bmcv_faiss_indexflatL2
      - L2距离的平方
    * -
      - bmcv_faiss_indexPQ_encode
      - 向量量化编码
    * -
      - bmcv_faiss_indexPQ_encode_ext
      - 可选数据类型
    * -
      - bmcv_faiss_indexPQ_ADC
      - 原始向量查询
    * -
      - bmcv_faiss_indexPQ_ADC_ext
      - 可选数据类型
    * -
      - bmcv_faiss_indexPQ_SDC
      - 编码向量查询
    * -
      - bmcv_faiss_indexPQ_SDC_ext
      - 可选数据类型
    * - 12
      - bmcv_image_transpose
      - 图像转置
    * - 13
      - bmcv_image_pyramid_down
      - 高斯金字塔下采样
    * - 14
      - bmcv_image_bayer2rgb
      - Bayer转RGBP
    * - 15
      - bmcv_batch_topk
      - 取前K个最大/小值
    * - 16
      - bmcv_matmul
      - 矩阵乘法
    * - 17
      - bmcv_calc_hist
      - 直方图计算
    * -
      - bmcv_calc_hist_with_weight
      - 带权重直方图
    * -
      - bmcv_hist_balance
      - 直方图均衡化
    * - 18
      - bmcv_as_strided
      - 重指定行列和步长
    * - 19
      - bmcv_min_max
      - 取最大/小值。
    * - 20
      - bmcv_cmulp
      - 复数乘法
    * - 21
      - bmcv_image_laplacian
      - 梯度计算
    * - 22
      - bmcv_gemm
      - 广义矩阵乘加
    * -
      - bmcv_gemm_ext
      - 可选数据类型
    * - 23
      - bmcv_image_warp
      - 仿射变换
    * -
      - bmcv_image_warp_affine
      - 仿射变换
    * -
      - bmcv_image_warp_affine_padding
      - 背景图填充
    * -
      - bmcv_image_warp_affine_similar_to_opencv
      - 矩阵对齐opencv
    * -
      - bmcv_image_warp_affine_similar_to_opencv_padding
      - 功能结合
    * - 24
      - bmcv_image_warp_perspective
      - 透射变换
    * -
      - bmcv_image_warp_perspective_with_coordinate
      - 坐标输入
    * -
      - bmcv_image_warp_perspective_similar_to_opencv
      - 矩阵对齐opencv
    * - 25
      - bmcv_distance
      - 多点到一点欧式距离
    * - 26
      - bmcv_fft_execute
      - 快速傅里叶变换
    * -
      - bmcv_fft_execute_real_input
      - 实数输入
    * - 27
      - bmcv_stft
      - 短时傅里叶变换
    * -
      - bmcv_istft
      - 逆短时傅里叶变换

.. list-table:: IVE实现接口
    :widths: 3 36 26

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_ive_filter
      - 5x5模板滤波
    * - 2
      - bmcv_ive_csc
      - 色彩空间转换
    * - 3
      - bmcv_ive_filter_and_csc
      - 功能组合
    * - 4
      - bmcv_ive_dilate
      - 5x5模板膨胀
    * - 5
      - bmcv_ive_erode
      - 5x5模板腐蚀
    * - 6
      - bmcv_ive_ccl
      - 连通区域标记
    * - 7
      - bmcv_ive_integ
      - 积分图计算
    * - 8
      - bmcv_ive_hist
      - 直方图计算
    * - 9
      - bmcv_ive_gradfg
      - 梯度前景图计算
    * - 10
      - bmcv_ive_lbp
      - 局部二值计算
    * - 11
      - bmcv_ive_normgrad
      - 归一化梯度计算
    * - 12
      - bmcv_ive_sad
      - 绝对差和
    * - 13
      - bmcv_ive_stcandicorner
      - 角点计算
    * - 14
      - bmcv_ive_mag_and_ang
      - 5x5模板幅值/幅角计算
    * - 15
      - bmcv_ive_map
      - 映射赋值
    * - 16
      - bmcv_ive_ncc
      - 归一化互相关系数计算
    * - 17
      - bmcv_ive_ord_stat_filter
      - 3x3模板顺序统计量滤波
    * - 18
      - bmcv_ive_sobel
      - 5x5模板梯度计算
    * - 19
      - bmcv_ive_gmm
      - 背景建模
    * - 20
      - bmcv_ive_gmm2
      - 背景建模支持更多参数
    * - 21
      - bmcv_ive_resize
      - 图像缩放
    * - 22
      - bmcv_ive_thresh
      - 阈值化
    * - 23
      - bmcv_ive_add
      - 两图加权相加
    * - 24
      - bmcv_ive_sub
      - 两图相减
    * - 25
      - bmcv_ive_and
      - 两图相与
    * - 26
      - bmcv_ive_or
      - 两图相或
    * - 27
      - bmcv_ive_xor
      - 两图异或
    * - 28
      - bmcv_ive_canny
      - 边缘图计算
    * - 29
      - bmcv_ive_match_bgmodel
      - 获取前景数据
    * - 30
      - bmcv_ive_update_bgmodel
      - 更新背景模型
    * - 31
      - bmcv_ive_frame_diff_motion
      - 背景相减
    * - 32
      - bmcv_ive_bernsen
      - 图像二值化
    * - 33
      - bmcv_ive_16bit_to_8bit
      - 16Bit转8Bit
    * - 34
      - bmcv_ive_dma
      - 内存拷贝

.. list-table:: LDC实现接口
    :widths: 3 36 26

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_ldc_rot
      - 旋转
    * - 2
      - bmcv_ldc_gdc
      - 畸变校正
    * - 3
      - bmcv_ldc_gdc_load_mesh
      - 畸变校正(加载已有MESH表)

.. list-table:: DWA实现接口
    :widths: 3 36 26

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_dwa_rot
      - 旋转
    * - 2
      - bmcv_dwa_gdc
      - 畸变校正
    * - 3
      - bmcv_dwa_affine
      - 仿射校正
    * - 4
      - bmcv_dwa_fisheye
      - 鱼眼校正
    * - 5
      - bmcv_dwa_dewarp
      - 畸变校正(输入GRIDINFO)

.. list-table:: DPU实现接口
    :widths: 3 36 26

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_dpu_sgbm_disp
      - 半全局块匹配计算
    * - 2
      - bmcv_dpu_fgs_disp
      - 快速全局平滑计算
    * - 3
      - bmcv_dpu_online_disp
      - 功能组合

.. list-table:: BLEND实现接口
    :widths: 3 36 26

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_blend
      - 图像拼接(重叠区域平滑过渡)

.. list-table:: SPACC实现接口
    :widths: 3 36 26

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_base64_enc
      - base64编码
    * - 2
      - bmcv_base64_dec
      - base64解码

.. list-table:: CPU实现接口
    :widths: 3 36 26

    * - **num**
      - **API**
      - **功能**
    * - 1
      - bmcv_image_draw_lines
      - 划线
    * - 2
      - bmcv_fft_1d_create_plan
      - 一维FFT任务创建
    * -
      - bmcv_fft_2d_create_plan
      - 二维FFT任务创建
    * -
      - bmcv_fft_destroy_plan
      - FFT任务销毁
    * - 3
      - bmcv_ldc_gdc_gen_mesh
      - 畸变校正MESH表计算
    * - 4
      - bmcv_gen_text_watermark
      - 生成文字水印
    * - 5
      - bm_image_write_to_bmp
      - 保存BMP图像到文件
    * - 6
      - bm_read_bin
      - 读取RAW图像到内存
    * - 7
      - bm_write_bin
      - 保存RAW图像到文件