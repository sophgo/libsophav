BMCV 硬件介绍
------------------

| 简要说明BMCV API由哪一部分硬件实现。

.. list-table:: 多模块实现接口
    :widths: 15 35 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_image_copy_to
      - TPU / VPSS
    * - 2
      - bmcv_image_put_text
      - CPU / VPSS
    * - 3
      - bmcv_image_convert_to
      - TPU / VPSS
    * - 4
      - bmcv_image_rotate_trans
      - TPU / VPSS
    * - 5
      - bmcv_image_rotate
      - LDC / VPSS

.. list-table:: VPSS实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_image_vpp_convert
      - VPSS
    * -
      - bmcv_image_vpp_convert_padding
      - VPSS
    * - 2
      - bmcv_image_vpp_csc_matrix_convert
      - VPSS
    * - 3
      - bmcv_image_csc_convert_to
      - VPSS
    * - 4
      - bmcv_image_draw_rectangle
      - VPSS
    * - 5
      - bmcv_image_fill_rectangle
      - VPSS
    * - 6
      - bmcv_image_flip
      - VPSS
    * - 7
      - bmcv_image_mosaic
      - VPSS
    * - 8
      - bmcv_image_storage_convert
      - VPSS
    * -
      - bmcv_image_storage_convert_with_csctype
      - VPSS
    * - 9
      - bmcv_image_vpp_basic
      - VPSS
    * - 10
      - bmcv_image_resize
      - VPSS
    * - 11
      - bmcv_image_watermark_superpose
      - VPSS
    * -
      - bmcv_image_watermark_repeat_superpose
      - VPSS
    * - 12
      - bmcv_image_overlay
      - VPSS
    * - 13
      - bmcv_image_vpp_stitch
      - VPSS
    * - 14
      - bmcv_image_circle
      - VPSS
    * - 15
      - bmcv_image_draw_point
      - VPSS
    * - 16
      - bmcv_image_csc_overlay
      - VPSS

.. list-table:: JPEG实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_image_jpeg_dec
      - JPEG
    * - 2
      - bmcv_image_jpeg_enc
      - JPEG

.. list-table:: TPU实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_image_absdiff
      - TPU
    * - 2
      - bmcv_image_axpy
      - TPU
    * - 3
      - bmcv_image_add_weighted
      - TPU
    * - 4
      - bmcv_image_bitwise_and
      - TPU
    * -
      - bmcv_image_bitwise_or
      - TPU
    * -
      - bmcv_image_bitwise_xor
      - TPU
    * - 5
      - bmcv_image_sobel
      - TPU
    * - 6
      - bmcv_sort
      - TPU
    * - 7
      - bmcv_image_threshold
      - TPU
    * - 8
      - bmcv_feature_match
      - TPU
    * -
      - bmcv_feature_match_normalized
      - TPU
    * - 9
      - bmcv_hist_balance
      - TPU
    * - 10
      - bmcv_image_quantify
      - TPU
    * - 11
      - bmcv_faiss_indexflatIP
      - TPU
    * -
      - bmcv_faiss_indexflatL2
      - TPU
    * -
      - bmcv_faiss_indexPQ_encode_ext
      - TPU
    * -
      - bmcv_faiss_indexPQ_encode
      - TPU
    * -
      - bmcv_faiss_indexPQ_ADC_ext
      - TPU
    * -
      - bmcv_faiss_indexPQ_ADC
      - TPU
    * -
      - bmcv_faiss_indexPQ_SDC_ext
      - TPU
    * -
      - bmcv_faiss_indexPQ_SDC
      - TPU
    * - 12
      - bmcv_image_transpose
      - TPU
    * - 13
      - bmcv_image_pyramid_down
      - TPU
    * - 14
      - bmcv_image_bayer2rgb
      - TPU
    * - 15
      - bmcv_batch_topk
      - TPU
    * - 16
      - bmcv_matmul
      - TPU
    * - 17
      - bmcv_calc_hist
      - TPU
    * -
      - bmcv_calc_hist_with_weight
      - TPU
    * - 18
      - bmcv_as_strided
      - TPU
    * - 19
      - bmcv_min_max
      - TPU
    * - 20
      - bmcv_cmulp
      - TPU
    * - 21
      - bmcv_image_laplacian
      - TPU
    * - 22
      - bmcv_gemm\
        bmcv_gemm_ext
      - TPU
    * - 23
      - bmcv_image_warp
      - TPU
    * -
      - bmcv_image_warp_affine
      - TPU
    * -
      - bmcv_image_warp_affine_padding
      - TPU
    * -
      - bmcv_image_warp_affine_similar_to_opencv
      - TPU
    * -
      - bmcv_image_warp_affine_similar_to_opencv_padding
      - TPU
    * - 24
      - bmcv_image_warp_perspective
      - TPU
    * -
      - bmcv_image_warp_perspective_with_coordinate
      - TPU
    * -
      - bmcv_image_warp_perspective_similar_to_opencv
      - TPU
    * - 25
      - bmcv_nms
      - TPU
    * -
      - bmcv_nms_ext
      - TPU
    * - 26
      - bmcv_fft_execute
      - TPU
    * -
      - bmcv_fft_execute_real_input
      - TPU
    * - 27
      - bmcv_stft
      - TPU
    * -
      - bmcv_istft
      - TPU
    * - 28
      - bmcv_distance
      - TPU
    * - 29
      - bmcv_image_width_align
      - TPU

.. list-table:: IVE实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_image_ive_filter
      - IVE
    * - 2
      - bmcv_image_ive_csc
      - IVE
    * - 3
      - bmcv_image_ive_filterandcsc
      - IVE
    * - 4
      - bmcv_image_ive_dilate
      - IVE
    * - 5
      - bmcv_image_ive_erode
      - IVE
    * - 6
      - bmcv_image_ive_ccl
      - IVE
    * - 7
      - bmcv_image_ive_integ
      - IVE
    * - 8
      - bmcv_image_ive_hist
      - IVE
    * - 9
      - bmcv_image_ive_gradfg
      - IVE
    * - 10
      - bmcv_image_ive_lbp
      - IVE
    * - 11
      - bmcv_image_ive_normgrad
      - IVE
    * - 12
      - bmcv_image_ive_sad
      - IVE
    * - 13
      - bmcv_image_ive_stCandiCorner
      - IVE
    * - 14
      - bmcv_image_ive_magAndAng
      - IVE
    * - 15
      - bmcv_image_ive_map
      - IVE
    * - 16
      - bmcv_image_ive_ncc
      - IVE
    * - 17
      - bmcv_image_ive_ordStatFilter
      - IVE
    * - 18
      - bmcv_image_ive_sobel
      - IVE
    * - 19
      - bmcv_image_ive_gmm
      - IVE
    * - 20
      - bmcv_image_ive_gmm2
      - IVE
    * - 21
      - bmcv_image_ive_resize
      - IVE
    * - 22
      - bmcv_image_ive_thresh
      - IVE
    * - 23
      - bmcv_image_ive_add
      - IVE
    * - 24
      - bmcv_image_ive_sub
      - IVE
    * - 25
      - bmcv_image_ive_and
      - IVE
    * - 26
      - bmcv_image_ive_or
      - IVE
    * - 27
      - bmcv_image_ive_xor
      - IVE
    * - 28
      - bmcv_image_ive_canny
      - IVE
    * - 29
      - bmcv_image_ive_match_bgmodel
      - IVE
    * - 30
      - bmcv_image_ive_update_bgmodel
      - IVE
    * - 31
      - bmcv_image_ive_frame_diff_motion
      - IVE
    * - 32
      - bmcv_image_ive_bernsen
      - IVE
    * - 33
      - bmcv_image_ive_16bitto8bit
      - IVE
    * - 34
      - bmcv_image_ive_dma
      - IVE

.. list-table:: LDC实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_ldc_rot
      - LDC
    * - 2
      - bmcv_ldc_gdc
      - LDC
    * - 3
      - bmcv_ldc_gdc_load_mesh
      - LDC

.. list-table:: DWA实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_dwa_rot
      - DWA
    * - 2
      - bmcv_dwa_gdc
      - DWA
    * - 3
      - bmcv_dwa_affine
      - DWA
    * - 4
      - bmcv_dwa_fisheye
      - DWA
    * - 5
      - bmcv_dwa_dewarp
      - DWA

.. list-table:: DPU实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_dpu_sgbm_disp
      - DPU
    * - 2
      - bmcv_dpu_fgs_disp
      - DPU
    * - 3
      - bmcv_dpu_online_disp
      - DPU

.. list-table:: BLEND实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_blend
      - BLEND

.. list-table:: SPACC实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_base64_enc
      - SPACC
    * - 2
      - bmcv_base64_dec
      - SPACC

.. list-table:: CPU实现接口
    :widths: 5 45 15

    * - **num**
      - **API**
      - **硬件**
    * - 1
      - bmcv_image_draw_lines
      - CPU
    * - 2
      - bmcv_fft_1d_create_plan
      - CPU
    * -
      - bmcv_fft_2d_create_plan
      - CPU
    * -
      - bmcv_fft_destroy_plan
      - CPU
    * - 3
      - bmcv_ldc_gdc_gen_mesh
      - CPU
    * - 4
      - bmcv_gen_text_watermark
      - CPU
    * - 5
      - bm_image_write_to_bmp
      - CPU
    * - 6
      - bm_read_bin
      - CPU
    * - 7
      - bm_write_bin
      - CPU