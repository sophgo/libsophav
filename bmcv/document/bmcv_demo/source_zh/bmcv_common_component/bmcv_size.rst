BMCV 尺寸限制
------------------

.. list-table:: bmcv参数范围表
    :widths: 10 10 15 20 20
    :align: center

    * - **模块**
      - **最小尺寸**
      - **最大尺寸**
      - **输入 stride 对齐要求**
      - **输出 stride 对齐要求**
    * - TPU
      - 8*8
      - 4096*4096
      - 1
      - 1
    * - VPSS
      - 16*16
      - 8192*8192
      - 1
      - 1
    * - LDC
      - 64*64
      - 4608*4608
      - 64
      - 64
    * - DWA
      - 32*32
      - 4096*4096
      - 32
      - 32
    * - IVE
      - 64*64
      - 1920*1080
      - 16
      - 16
    * - DPU
      - 64*64
      - 1920*1080
      - 16
      - 16(sgbm), 32(fgs)
    * - BLEND
      - 64*64
      - 4608*2160
      - 16
      - 16
