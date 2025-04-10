.. raw:: latex

   \newpage

BMCV介绍
=============

| BMCV 提供了一套基于 Sophon 硬件优化的机器视觉库，通过利用VPSS、TPU、IVE、DPU、DWA等模块，可以完成色彩空间转换、尺度变换、仿射变换、透射变换、线性变换、画框、JPEG编解码、BASE64编解码、NMS、排序、特征匹配等操作。

.. list-table:: 修订记录
    :widths: 10 8 45

    * - **Revision**
      - **Date**
      - **Comments**
    * - 1.1.0
      - 2023/11
      - 初版, 加入vpss模块相关接口
    * - 1.2.0
      - 2023/12
      - 加入cv算子|ive|dpu|ldc|dwa|blend模块相关接口
    * - 1.3.0
      - 2024/01
      - 加入ldc_gen_mesh|dwa_affine|dwa_fisheye等接口
    * - 1.4.0
      - 2024/02
      - 加入cv算子hist_balance|quantify等接口
    * - 1.5.0
      - 2024/04
      - 加入cv算子put_text|draw_lines等接口
    * - 1.6.0
      - 2024/05
      - 加入flip的接口
    * - 1.7.0
      - 2024/07
      - 加入overlay的接口
    * - 1.8.0
      - 2024/11
      - 加入cv算子faiss|fft|stft|istft|rotate等接口
    * - 1.9.0
      - 2025/01
      - 各接口文档增加示例代码, put_text支持中文, vpss支持pcie模式, 优化overlay接口性能
    * - 2.0.0
      - 2025/04
      - 加入circle|point|csc_overlay等接口, 优化draw_rect|fill_rect接口性能, vpss接口多crop任务支持多核调用