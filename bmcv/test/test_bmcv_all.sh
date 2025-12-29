#!/bin/bash

failed_count=0
count=1
failed_scripts=""
bmcv_case=${1:-'all'}
loop=${2:-1}

run_command() {
  local command="$1"
  local description="$1"

  echo "Running: $description"
  eval "$command"

  if [ $? -ne 0 ]; then
    echo "Command failed: $description"
    failed_count=$((failed_count + 1))
    failed_scripts="$failed_scripts$description\n"
  fi

  echo ""
}

run_vpss() {
  run_command "test_vpss_convert 1920 1080 0 0 1920 1080 1920 1080 0 10 1 1 1 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin"
  run_command "test_vpss_convert 1920 1080 480 270 960 540 960 540 8 10 1 1 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_convert 1920 1080 0 0 1920 1080 666 888 8 9 1 1 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_draw_rectangle 1920 1080 8 1 480 270 960 540 2 0 255 0 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_fill_rectangle 1920 1080 8 1 480 270 960 540 0 255 0 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_watermark 1920 1080 128 128 8 1 900 480 128 128 151 255 152 1 /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_convert_to 1920 1080 1.1 0.9 3.14159 10 20 -30 8 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_padding 1920 1080 480 270 960 540 20 20 960 960 151 255 152 1000 1000 8 1 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"

  run_command "test_vpss_convert_thread"
  run_command "test_vpss_convert_to_thread"
  run_command "test_vpss_copy_to_thread"
  run_command "test_vpss_draw_rectangle_thread"
  run_command "test_vpss_fbd_thread"
  run_command "test_vpss_fill_rectangle_thread"
  run_command "test_vpss_mosaic_thread"
  run_command "test_vpss_padding_thread"
  run_command "test_vpss_stitch_thread"
  run_command "test_vpss_water_thread"
  run_command "test_vpss_point_thread"
  run_command "test_vpss_csc_overlay_thread"
  run_command "test_gen_text_watermark"
  run_command "test_gen_text_watermark sophgo 255 0 0 0.8 out/text1.bmp 4ea7c1adcd8486388c87abc078b35ef8"
  run_command "test_gen_text_watermark sophgo 255 0 0 0.8 out/text2.bmp 1612a8c95b32310d2478114e5ab439d1 /opt/sophon/libsophon-current/bin/res/1920x1080_nv12.bin 1920 1080 3 99 99"
  run_command "test_gen_text_watermark sophgo 255 0 0 2 out/text3.bmp eda2b2de92dc1b1dfeb6fc686ab35a3a /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 1920 1080 0 1810 100"
  run_command "test_vpss_random_thread 10 1000"
  run_command "test_vpss_random_thread 1920 1080 0 0 0 1917 1077 1917 1077 3 0 0"
  run_command "test_vpss_random_thread 1917 1077 0 0 0 1917 1077 1920 1080 3 0 0"
  run_command "test_vpss_random_thread 1917 1077 0 0 0 1917 1077 1917 1077 3 0 0"

  run_command "test_vpss_convert_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 1917 1077 1917 1077 5 out/crop_1917x1077.nv16 1 0"
  run_command "test_vpss_convert_thread 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 0 0 1920 1080 800 600 10 out/resize_800x600.rgb 1 0"
  run_command "test_vpss_convert_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 1920 1080 1920 1080 10 out/convert_1920x1080.rgb 1 0"
  run_command "test_vpss_convert_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 800 600 800 600 0 out/crop_800x600.yuv420 1 0"
  run_command "test_vpss_convert_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 800 600 600 800 0 out/resize_600x800.yuv420 0 0 0 1 1 d2b6a3d00c40c585ef9e2fd2ef725443"
  run_command "test_vpss_convert_to_thread 1920 1080 8 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin 0.5 0.5 0.5 200 200 200 8 out/convert_to_1920x1080.rgbp 0"
  run_command "test_vpss_copy_to_thread 800 600 0 /opt/sophon/libsophon-current/bin/res/800x600_yuv420.bin 0 0 1920 1080 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin  out/copy_to_1920x1080.yuv 0"
  run_command "test_vpss_draw_rectangle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 800 600 out/draw_rect_1920x1080.yuv420 0"
  run_command "test_vpss_fbd_thread 1920 1080 69632 /opt/sophon/libsophon-current/bin/res/fbc/1920x1080_table_y.bin /opt/sophon/libsophon-current/bin/res/fbc/1920x1080_data_y.bin /opt/sophon/libsophon-current/bin/res/fbc/1920x1080_table_c.bin /opt/sophon/libsophon-current/bin/res/fbc/1920x1080_data_c.bin 0 0 1920 1080 1920 1080 0 out/fbd_1920x1080.yuv420 0"
  run_command "test_vpss_fill_rectangle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 100 100 200 200 out/fill_rect_1920x1080_yuv420.bin 0"
  run_command "test_vpss_mosaic_thread 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 100 100 500 500 out/mosaic_1920x1080.rgb 0"
  run_command "test_vpss_padding_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 1920 1080 2048 2048 0 out/pad_2048x2048_yuv420.bin 1 0"
  run_command "test_vpss_padding_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 150 150 150 220 0 out/pad_150x220_yuv420.bin 1 0 1 1 14f7de62b33736eda6eb95a4b81c69e7"
  run_command "test_vpss_stitch_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 1920 1080 0 1080 1920 1080 1920 2160 out/stitch_1920x2160_yuv420.bin 0"
  run_command "test_vpss_water_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin 16384 128 0 100 100 out/water_1920x1080.yuv420 0"
  run_command "test_vpss_overlay_thread 1920 1080 10 300 300 17 20 20 /opt/sophon/libsophon-current/bin/res/car_rgb888.rgb /opt/sophon/libsophon-current/bin/res/300x300_argb8888_dog.rgb 663ce7ae1f1b3154a66a6366a6332559"
  run_command "test_vpss_overlay_thread 1920 1080 10 80 60 30 100 100 /opt/sophon/libsophon-current/bin/res/car_rgb888.rgb /opt/sophon/libsophon-current/bin/res/dog_s_80x60_pngto4444.bin 52e0003832180b4c74b71395c200c72e"
  run_command "test_vpss_overlay_thread 1920 1080 10 80 60 32  200 200 /opt/sophon/libsophon-current/bin/res/car_rgb888.rgb /opt/sophon/libsophon-current/bin/res/dog_s_80x60_pngto1555.bin a858701cb01131f1356f80c76e7813c0"
  run_command "test_vpss_flip_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 1 h_flip.bin 0 1 1 37890f4ab5d6458b57824de9e24ba94f"
  run_command "test_vpss_flip_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 2 v_flip.bin 0 1 1 a3a86425b349c6517a2506e4df2334f0"
  run_command "test_vpss_flip_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 3 hv_flip.bin 0 1 1 80062a883675498f41d3309cfc6ddb52"
  run_command "test_vpss_circle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 960 540 250 -2 empty.bin 0 1 1 94a364c93d13aad8db3d38146c5ba4fa"
  run_command "test_vpss_circle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 960 540 250 -1 shape.bin 0 1 1 d2c467732a8021d3f212e27dd0a98677"
  run_command "test_vpss_circle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 960 540 250 10 line.bin 0 1 1 583e2f078ceccfa73b4b084d7cf74aca"
}

run_tpu(){
  run_command "test_cv_absdiff"
  run_command "test_cv_absdiff 2 1 0 12 8 8"
  run_command "test_cv_absdiff 1 1 1 10 1080 1920 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin out/asbdiff_output.bin"
  run_command "test_cv_add_weight"
  run_command "test_cv_add_weight 2"
  run_command "test_cv_add_weight 2 1"
  run_command "test_cv_add_weight 2 1 0 8 8 12 0 0.6 0.4 10"
  run_command "test_cv_add_weight 1 1 1 1920 1080 10 0 0.5 0.5 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin out/add_weight_output.bin"
  run_command "test_cv_as_strided"
  run_command "test_cv_as_strided 2"
  run_command "test_cv_as_strided 1 3"
  run_command "test_cv_as_strided 2 1 5 5 3 3 2 1"
  run_command "test_cv_axpy"
  run_command "test_cv_axpy 2"
  run_command "test_cv_axpy 2 1"
  run_command "test_cv_batch_topk"
  run_command "test_cv_batch_topk 2"
  run_command "test_cv_batch_topk 1 3"
  run_command "test_cv_batch_topk 1 1 50 20 2 0 0"
  run_command "test_cv_bayer2rgb"
  run_command "test_cv_bayer2rgb 2"
  run_command "test_cv_bayer2rgb 1 1 0 128 128 0"
  run_command "test_cv_bayer2rgb 1 1 1 1024 1024 0 /opt/sophon/libsophon-current/bin/res/bayer.bin out/out_bayer2rgb.bin"
  run_command "test_cv_bitwise"
  run_command "test_cv_bitwise 3"
  run_command "test_cv_bitwise 1 2"
  run_command "test_cv_bitwise 2 2 0 1024 1024 12 7"
  run_command "test_cv_bitwise 1 1 1 1080 1920 10 7 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin out/bitwise_output.bin"
  run_command "test_cv_calc_hist"
  run_command "test_cv_calc_hist 2"
  run_command "test_cv_calc_hist 3 1"
  run_command "test_cv_calc_hist 1 1 512 512 1 0 1 0"
  run_command "test_cv_cmulp"
  run_command "test_cv_cmulp 1 1 256 20"
  run_command "test_cv_convert_to"
  run_command "test_cv_copy_to"
  run_command "test_cv_distance"
  run_command "test_cv_distance 1 1 1 512 3"
  run_command "test_cv_draw_lines"
  run_command "test_cv_draw_lines 1 1 1080 1920 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin out/output_draw_lines.bin"
  run_command "test_cv_feature_match"
  run_command "test_cv_feature_match 1 1 2 100 1000 5 0"
  run_command "test_cv_fft 1 1 100 100 0 0"
  run_command "test_cv_fft 1 1 100 100 0 1"
  run_command "test_cv_fft 1 1 100 100 1 0"
  run_command "test_cv_fft 1 1 100 100 1 1"
  run_command "test_cv_gaussian_blur"
  run_command "test_cv_gaussian_blur 2 1 1 512 512 8 3 0.5 0.5"
  run_command "test_cv_gaussian_blur 1 1 0 1920 1080 10 3 0.5 0.5 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin out/out_gaussian_blur.bin"
  run_command "test_cv_gemm"
  run_command "test_cv_gemm 2"
  run_command "test_cv_hist_balance"
  run_command "test_cv_hist_balance 2 1 800 800"
  run_command "test_cv_hm_distance 1 1 8"
  run_command "test_cv_jpeg"
  run_command "test_cv_laplace"
  run_command "test_cv_laplace 2 1 1 0 512 512 14"
  run_command "test_cv_laplace 2 1 3 1 1080 1920 14 /opt/sophon/libsophon-current/bin/res/1920x1080_gray.bin out/laplace_output.bin"
  run_command "test_cv_matmul"
  run_command "test_cv_matmul 2"
  run_command "test_cv_matmul 1 1 0 0 0 0 1 50 100"
  run_command "test_cv_matmul 1 1 1 1 1 2 5 50 100"
  run_command "test_cv_min_max"
  run_command "test_cv_min_max 2 1 512"
  run_command "test_cv_nms 2 1 100 0.7"
  run_command "test_cv_nms 1 1 100 0.7"
  run_command "test_cv_put_text"
  run_command "test_cv_put_text 1 1 1080 1920 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin out/output_put_text.bin"
  run_command "test_cv_pyramid"
  run_command "test_cv_pyramid 1 1 0 512 512"
  run_command "test_cv_pyramid 1 1 1 1080 1920 /opt/sophon/libsophon-current/bin/res/1920x1080_gray.bin out/pyramid_output.bin"
  run_command "test_cv_quantify"
  run_command "test_cv_quantify 2"
  run_command "test_cv_quantify 1 1 0 512 512"
  run_command "test_cv_sort"
  run_command "test_cv_sort 2"
  run_command "test_cv_sort 1 1 1000 1000"
  run_command "test_cv_stft 1"
  run_command "test_cv_stft 1 1 1 200 10 0 256 128 256 0 0 0 1"
  run_command "test_cv_istft 1"
  run_command "test_cv_istft 1 1 4096 1 1 1 1 1 4096 1024"
  run_command "test_cv_threshold"
  run_command "test_cv_threshold 2"
  run_command "test_cv_threshold 1 1 1 1920 1080 2 /opt/sophon/libsophon-current/bin/res/1920x1080_gray.bin out/threshold_output.bin"
  run_command "test_cv_transpose"
  run_command "test_cv_transpose 2 1 0 1 3 512 512"
  run_command "test_cv_transpose 1 1 1 4 1 1080 1920 /opt/sophon/libsophon-current/bin/res/1920x1080_gray.bin out/transpose_output.bin"
  run_command "test_cv_warp_affine"
  run_command "test_cv_warp_perspective"
  run_command "test_faiss_indexflatIP"
  run_command "test_faiss_indexflatL2"
  run_command "test_faiss_indexPQ"
}

run_dpu(){
  if [ ! -d "dpu_data" ]; then
    echo "Error: Directory 'dpu_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  run_command "test_dpu_sgbm_thread 512 284 3 dpu_data/sofa_left_img_512x284.bin dpu_data/sofa_right_img_512x284.bin  dpu_data/205pU8Disp_ref_512x284.bin"
  run_command "test_dpu_sgbm_thread 1920 1080 3 dpu_data/pendulum_left_img_1920x1080.bin dpu_data/pendulum_right_img_1920x1080.bin dpu_data/pU8Disp_ref1_1920x1080.bin"
  run_command "test_dpu_sgbm_thread 804 540 3 dpu_data/804_left_img.bin dpu_data/804_right_img.bin dpu_data/804_sgbm_u8_median_res.bin"
  run_command "test_dpu_sgbm_thread 1608 1080 3 dpu_data/1608_left_img.bin dpu_data/1608_right_img.bin dpu_data/1608_sgbm_u8_median_res.bin"
  run_command "test_dpu_online_thread 512 284 4 dpu_data/sofa_left_img_512x284.bin dpu_data/sofa_right_img_512x284.bin  dpu_data/fgs_512x284_res.bin 0 0 1 1"
  run_command "test_dpu_online_thread 1920 1080 4 dpu_data/pendulum_left_img_1920x1080.bin dpu_data/pendulum_right_img_1920x1080.bin  dpu_data/fgs_res_1920x1080.bin 0 0 1 1"
  run_command "test_dpu_online_thread 804 540 4 dpu_data/804_left_img.bin dpu_data/804_right_img.bin dpu_data/fgs_804_res.bin"
  run_command "test_dpu_online_thread 1608 1080 4 dpu_data/1608_left_img.bin dpu_data/1608_right_img.bin dpu_data/fgs_1608_res.bin"
  run_command "test_dpu_fgs_thread 512 284 7 dpu_data/sofa_left_img_512x284.bin dpu_data/205pU8Disp_ref_512x284.bin dpu_data/fgs_512x284_res.bin 0 19 3 864000 0 1 1"
  run_command "test_dpu_fgs_thread 1920 1080 7 dpu_data/pendulum_left_img_1920x1080.bin dpu_data/pU8Disp_ref1_1920x1080.bin dpu_data/fgs_res_1920x1080.bin 0 19 3 864000 0 1 1"
  run_command "test_dpu_fgs_thread 804 540 7 dpu_data/804_left_img.bin dpu_data/804_sgbm_u8_median_res.bin dpu_data/fgs_804_res.bin"
  run_command "test_dpu_fgs_thread 1608 1080 7 dpu_data/1608_left_img.bin dpu_data/1608_sgbm_u8_median_res.bin dpu_data/fgs_1608_res.bin"
}

run_ive(){
  if [ ! -d "ive_data" ]; then
    echo "Error: Directory 'ive_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  run_command "test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0"
  run_command "test_ive_and_thread 352 288 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_And.yuv 0 1 1 0"
  run_command "test_ive_dilate_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Dilate_3x3.yuv 0 1 1 0"
  run_command "test_ive_dilate_thread 640 480 1 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Dilate_5x5.yuv 0 1 1 0"
  run_command "test_ive_dilate_thread 352 288 0 ive_data/bin_352x288_y.yuv ive_data/result/sample_Dilate_3x3_dilate_only.bin 0 1 1 0"
  run_command "test_ive_dilate_thread 352 288 1 ive_data/bin_352x288_y.yuv ive_data/result/sample_Dilate_5x5_dilate_only.bin 0 1 1 0"
  run_command "test_ive_dma_thread 352 288 0 0 0 0 14 14 ive_data/00_352x288_y.yuv ive_data/result/sample_DMA_Direct.bin 0 1 1 0"
  run_command "test_ive_erode_thread 352 288 0 ive_data/bin_352x288_y.yuv ive_data/result/sample_Erode_3x3.bin.only_erode 0 1 1 0"
  run_command "test_ive_erode_thread 352 288 1 ive_data/bin_352x288_y.yuv ive_data/result/sample_Erode_5x5.bin.only_erode 0 1 1 0"
  run_command "test_ive_erode_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Erode_3x3.yuv 0 1 1 0"
  run_command "test_ive_erode_thread 640 480 1 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Erode_5x5.yuv 0 1 1 0"
  run_command "test_ive_hist_thread 352 288 ive_data/00_352x288_y.yuv ./hist_res.bin ive_data/result/sample_Hist.bin 0 1 1 0"
  run_command "test_ive_intg_thread 352 288 0 0 ive_data/00_352x288_y.yuv intge_combine.bin ive_data/result/sample_Integ_Combine.yuv 0 1 1"
  run_command "test_ive_intg_thread 352 288 2 1 ive_data/00_352x288_y.yuv integ_sqsum.bin ive_data/result/sample_Integ_Sqsum.yuv 0 1 1"
  run_command "test_ive_intg_thread 352 288 2 1 ive_data/00_352x288_y.yuv integ_sqsum.bin ive_data/result/sample_Integ_Sqsum.yuv 0 1 1"
  run_command "test_ive_lbp_thread 352 288 0 ive_data/00_352x288_y.yuv ive_data/result/sample_LBP_Normal.yuv 0 1 1 0"
  run_command "test_ive_lbp_thread 352 288 1 ive_data/00_352x288_y.yuv ive_data/result/sample_LBP_Abs.yuv 0 1 1 0"
  run_command "test_ive_mag_and_ang_thread 640 480 ive_data/sky_640x480.yuv 0 0 1 ive_data/result/sample_tile_MagAndAng_MagAndAng3x3_Mag.yuv ive_data/result/sample_tile_MagAndAng_MagAndAng3x3_Ang.yuv 0 1 1 0"
  run_command "test_ive_mag_and_ang_thread 640 480 ive_data/sky_640x480.yuv 0 0 0 ive_data/result/sample_tile_MagAndAng_MagAndAng3x3_Mag.yuv"
  run_command "test_ive_mag_and_ang_thread 640 480 ive_data/sky_640x480.yuv 0 1 1 ive_data/result/sample_tile_MagAndAng_MagAndAng5x5_Mag.yuv ive_data/result/sample_tile_MagAndAng_MagAndAng5x5_Ang.yuv"
  run_command "test_ive_mag_and_ang_thread 640 480 ive_data/sky_640x480.yuv 0 1 0 ive_data/result/sample_tile_MagAndAng_MagAndAng5x5_Mag.yuv"
  run_command "test_ive_mag_and_ang_thread 640 480 ive_data/sky_640x480.yuv 42 0 0 ive_data/result/sample_tile_MagAndAng_Thresh3x3_Mag.yuv"
  run_command "test_ive_mag_and_ang_thread 640 480 ive_data/sky_640x480.yuv 18468 1 0 ive_data/result/sample_tile_MagAndAng_Thresh5x5_Mag.yuv"
  run_command "test_ive_mag_and_ang_thread 352 288 ive_data/00_352x288_y.yuv 0 0 1 ive_data/result/sample_MagAndAng_MagAndAng3x3_Mag.yuv ive_data/result/sample_MagAndAng_MagAndAng3x3_Ang.yuv 0 1 1 0"
  run_command "test_ive_mag_and_ang_thread 352 288 ive_data/00_352x288_y.yuv 0 0 0 ive_data/result/sample_MagAndAng_MagAndAng3x3_Mag.yuv"
  run_command "test_ive_mag_and_ang_thread 352 288 ive_data/00_352x288_y.yuv 0 1 1 ive_data/result/sample_MagAndAng_MagAndAng5x5_Mag.yuv ive_data/result/sample_MagAndAng_MagAndAng5x5_Ang.yuv"
  run_command "test_ive_mag_and_ang_thread 352 288 ive_data/00_352x288_y.yuv 0 1 0 ive_data/result/sample_MagAndAng_MagAndAng5x5_Mag.yuv"
  run_command "test_ive_mag_and_ang_thread 352 288 ive_data/00_352x288_y.yuv 42 0 0 ive_data/result/sample_MagAndAng_Thresh3x3_Mag.yuv"
  run_command "test_ive_mag_and_ang_thread 352 288 ive_data/00_352x288_y.yuv 18468 1 0 ive_data/result/sample_MagAndAng_Thresh5x5_Mag.yuv"
  run_command "test_ive_map_thread 352 288 0 14 14 ive_data/00_352x288_y.yuv ive_data/result/sample_Map.yuv 0 1 1 0"
  run_command "test_ive_ncc_thread 352 288 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv dst_bin ive_data/result/sample_NCC_Mem.bin 0 1 1"
  run_command "test_ive_or_thread 352 288 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Or.yuv 0 1 1 0"
  run_command "test_ive_ordstatfilter_thread 352 288 0 ive_data/00_352x288_y.yuv ive_data/result/sample_OrdStaFilter_Median.yuv 0 1 1 0"
  run_command "test_ive_ordstatfilter_thread 352 288 1 ive_data/00_352x288_y.yuv ive_data/result/sample_OrdStaFilter_Max.yuv 0 1 1 0"
  run_command "test_ive_ordstatfilter_thread 352 288 2 ive_data/00_352x288_y.yuv ive_data/result/sample_OrdStaFilter_Min.yuv 0 1 1 0"
  run_command "test_ive_sub_thread 352 288 0 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Sub_Abs.yuv 0 1 1 0"
  run_command "test_ive_sub_thread 352 288 1 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Sub_Shift.yuv 0 1 1 0"
  run_command "test_ive_thresh_s16_thread 352 288 14 14 8 41 105 -63 -5 -98 ive_data/00_352x288_s8_to_s16_reverse.yuv ive_data/result/sample_Thresh_S16_To_S8_MinMidMax_352x288.yuv 0 1 1 0"
  run_command "test_ive_thresh_thread 352 288 14 14 3 236 249 166 219 60 ive_data/00_352x288_y.yuv ive_data/result/sample_Thresh_MinMidMax.yuv 0 1 1 0"
  run_command "test_ive_thresh_u16_thread 352 288 14 14 18 41 105 190 132 225 ive_data/00_352x288_u8_to_u16_reverse.yuv ive_data/result/sample_Thresh_U16_To_U8_MinMidMax_352x288.yuv 0 1 1 0"
  run_command "test_ive_xor_thread 352 288 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Xor.yuv 0 1 1 0"
  run_command "test_ive_sobel_thread 640 480 ive_data/sky_640x480.yuv 0 0 ive_data/result/sample_tile_Sobel_Hor3x3.yuv ive_data/result/sample_tile_Sobel_Ver3x3.yuv 0 1 1 0 out/dst_sobelh.bin out/dst_sobelv.bin"
  run_command "test_ive_sobel_thread 640 480 ive_data/sky_640x480.yuv 0 1 ive_data/result/sample_tile_Sobel_Hor3x3.yuv"
  run_command "test_ive_sobel_thread 640 480 ive_data/sky_640x480.yuv 0 2 null ive_data/result/sample_tile_Sobel_Ver3x3.yuv"
  run_command "test_ive_sobel_thread 640 480 ive_data/sky_640x480.yuv 1 0 ive_data/result/sample_tile_Sobel_Hor5x5.yuv ive_data/result/sample_tile_Sobel_Ver5x5.yuv 0 1 1 0 out/dst_sobelh.bin out/dst_sobelv.bin"
  run_command "test_ive_sobel_thread 640 480 ive_data/sky_640x480.yuv 1 1 ive_data/result/sample_tile_Sobel_Hor5x5.yuv"
  run_command "test_ive_sobel_thread 640 480 ive_data/sky_640x480.yuv 1 2 null ive_data/result/sample_tile_Sobel_Ver5x5.yuv"
  run_command "test_ive_sobel_thread 352 288 ive_data/00_352x288_y.yuv 0 0 ive_data/result/sample_Sobel_Hor3x3.yuv ive_data/result/sample_Sobel_Ver3x3.yuv 0 1 1 0 out/dst_sobelh.bin out/dst_sobelv.bin"
  run_command "test_ive_sobel_thread 352 288 ive_data/00_352x288_y.yuv 0 1 ive_data/result/sample_Sobel_Hor3x3.yuv"
  run_command "test_ive_sobel_thread 352 288 ive_data/00_352x288_y.yuv 0 2 null ive_data/result/sample_Sobel_Ver3x3.yuv"
  run_command "test_ive_sobel_thread 352 288 ive_data/00_352x288_y.yuv 1 0 ive_data/result/sample_Sobel_Hor5x5.yuv ive_data/result/sample_Sobel_Ver5x5.yuv 0 1 1 0 out/dst_sobelh.bin out/dst_sobelv.bin"
  run_command "test_ive_sobel_thread 352 288 ive_data/00_352x288_y.yuv 1 1 ive_data/result/sample_Sobel_Hor5x5.yuv"
  run_command "test_ive_sobel_thread 352 288 ive_data/00_352x288_y.yuv 1 2 null ive_data/result/sample_Sobel_Ver5x5.yuv"
  run_command "test_ive_normgrad_thread 352 288 ive_data/00_352x288_y.yuv 0 0 ive_data/result/sample_NormGrad_Hor3x3.yuv ive_data/result/sample_NormGrad_Ver3x3.yuv"
  run_command "test_ive_normgrad_thread 352 288 ive_data/00_352x288_y.yuv 0 1 ive_data/result//sample_NormGrad_Hor3x3.yuv"
  run_command "test_ive_normgrad_thread 352 288 ive_data/00_352x288_y.yuv 0 2 null ive_data/result/sample_NormGrad_Ver3x3.yuv"
  run_command "test_ive_normgrad_thread 352 288 ive_data/00_352x288_y.yuv 0 3 null null ive_data/result/sample_NormGrad_Combine3x3.yuv"
  run_command "test_ive_normgrad_thread 352 288 ive_data/00_352x288_y.yuv 1 0 ive_data/result/sample_NormGrad_Hor5x5.yuv ive_data/result/sample_NormGrad_Ver5x5.yuv null 0 1 1 0 "
  run_command "test_ive_normgrad_thread 352 288 ive_data/00_352x288_y.yuv 1 1 ive_data/result/sample_NormGrad_Hor5x5.yuv"
  run_command "test_ive_normgrad_thread 352 288 ive_data/00_352x288_y.yuv 1 2 null ive_data/result/sample_NormGrad_Ver5x5.yuv"
  run_command "test_ive_normgrad_thread 640 480 ive_data/sky_640x480.yuv 0 0 ive_data/result/sample_tile_NormGrad_Hor3x3.yuv ive_data/result/sample_tile_NormGrad_Ver3x3.yuv"
  run_command "test_ive_normgrad_thread 640 480 ive_data/sky_640x480.yuv 0 1 ive_data/result/sample_tile_NormGrad_Hor3x3.yuv"
  run_command "test_ive_normgrad_thread 640 480 ive_data/sky_640x480.yuv 0 2 null ive_data/result/sample_tile_NormGrad_Ver3x3.yuv"
  run_command "test_ive_normgrad_thread 640 480 ive_data/sky_640x480.yuv 0 3 null null ive_data/result/sample_tile_NormGrad_Combine3x3.yuv"
  run_command "test_ive_normgrad_thread 640 480 ive_data/sky_640x480.yuv 1 0 ive_data/result/sample_tile_NormGrad_Hor5x5.yuv ive_data/result/sample_tile_NormGrad_Ver5x5.yuv"
  run_command "test_ive_normgrad_thread 640 480 ive_data/sky_640x480.yuv 1 1 ive_data/result/sample_tile_NormGrad_Hor5x5.yuv"
  run_command "test_ive_normgrad_thread 640 480 ive_data/sky_640x480.yuv 1 2 null ive_data/result/sample_tile_NormGrad_Ver5x5.yuv"
  run_command "test_ive_canny_thread 640 480 ive_data/sky_640x480.yuv 1 ive_data/result/sample_tile_CannyEdge_5x5.yuv 0 1 1 0 out/tile_dst5x5_edge.bin"
  run_command "test_ive_canny_thread 640 480 ive_data/sky_640x480.yuv 0 ive_data/result/sample_tile_CannyEdge_3x3.yuv"
  run_command "test_ive_canny_thread 352 288 ive_data/00_352x288_y.yuv 1 ive_data/result/sample_CannyEdge_5x5.yuv 0 1 1"
  run_command "test_ive_canny_thread 352 288 ive_data/00_352x288_y.yuv 0 ive_data/result/sample_CannyEdge_3x3.yuv"
  run_command "test_ive_gmm_thread 704 288 14 ive_data/campus.u8c1.1_100.raw ive_data/result/sample_tile_GMM_U8C1_fg_31.yuv ive_data/result/sample_tile_GMM_U8C1_bg_31.yuv"
  run_command "test_ive_gmm_thread 352 288 14 ive_data/campus.u8c1.1_100.raw ive_data/result/sample_GMM_U8C1_fg_31.yuv ive_data/result/sample_GMM_U8C1_bg_31.yuv"
  run_command "test_ive_gmm2_thread 352 288 14 0 0 ive_data/campus.u8c1.1_100.raw null ive_data/result/sample_GMM2_U8C1_fg_31.yuv ive_data/result/sample_GMM2_U8C1_bg_31.yuv"
  run_command "test_ive_gmm2_thread 352 288 14 1 0 ive_data/campus.u8c1.1_100.raw ive_data/sample_GMM2_U8C1_PixelCtrl_Factor.raw ive_data/result/sample_GMM2_U8C1_PixelCtrl_fg_31.yuv ive_data/result/sample_GMM2_U8C1_PixelCtrl_bg_31.yuv ive_data/result/sample_GMM2_U8C1_PixelCtrl_match_31.yuv"
  run_command "test_ive_gmm2_thread 704 288 14 0 0 ive_data/campus.u8c1.1_100.raw null ive_data/result/sample_tile_GMM2_U8C1_fg_31.yuv ive_data/result/sample_tile_GMM2_U8C1_bg_31.yuv"
  run_command "test_ive_filter_thread 352 288 14 ive_data/00_352x288_y.yuv 0 4 ive_data/result/sample_Filter_Y3x3.yuv"
  run_command "test_ive_filter_thread 352 288 14 ive_data/00_352x288_y.yuv 1 7 ive_data/result/sample_Filter_Y5x5.yuv"
  run_command "test_ive_filter_thread 352 288 4 ive_data/00_352x288_SP420.yuv 0 4 ive_data/result/sample_Filter_420SP3x3.yuv 0 1 1 0 ./out/ive_fileter_nv21_3x3_res.bin"
  run_command "test_ive_filter_thread 352 288 4 ive_data/00_352x288_SP420.yuv 1 7 ive_data/result/sample_Filter_420SP5x5.yuv 0 1 1 0 ./out/ive_fileter_nv21_5x5_res.bin"
  run_command "test_ive_filter_thread 352 288 6 ive_data/00_352x288_SP422.yuv 0 4 ive_data/result/sample_Filter_422SP3x3.yuv 0 1 1"
  run_command "test_ive_filter_thread 352 288 6 ive_data/00_352x288_SP422.yuv 1 7 ive_data/result/sample_Filter_422SP5x5.yuv 0 1 1"
  run_command "test_ive_csc_thread 352 288 2 1 10 ive_data/00_352x288_444.yuv ive_data/result/sample_CSC_YUV2RGB.rgb"
  run_command "test_ive_csc_thread 352 288 4 1 10 ive_data/00_352x288_SP420.yuv ive_data/result/sample_CSC_YUV2RGB.rgb"
  run_command "test_ive_csc_thread 480 480 2 0 16 ive_data/lena_480x480_planar.yuv ive_data/result/sample_CSC_BT601_YUV2HSV_480x480.vsh"
  run_command "test_ive_csc_thread 480 480 2 3 16 ive_data/lena_480x480_planar.yuv ive_data/result/sample_CSC_BT709_YUV2HSV_480x480.vsh"
  run_command "test_ive_csc_thread 352 288 10 2 2 ive_data/dst0_2_10.rgb ive_data/result/dst2_2_10.yuv 0 1 1"
  run_command "test_ive_csc_thread 352 288 10 4 2 ive_data/dst1_2_10.rgb ive_data/result/dst4_2_10.yuv 0 1 1"
  run_command "test_ive_csc_thread 352 288 10 5 2 ive_data/dst3_2_10.rgb ive_data/result/dst5_2_10.yuv 0 1 1"
  run_command "test_ive_csc_thread 352 288 10 7 2 ive_data/dst6_2_10.rgb ive_data/result/dst7_2_10.yuv 0 1 1"
  run_command "test_ive_resize_thread 1 ive_data/result/sample_Resize_Bilinear_rgb.rgb ive_data/result/sample_Resize_Bilinear_gray.yuv ive_data/result/sample_Resize_Bilinear_240p.rgb"
  run_command "test_ive_resize_thread 3 ive_data/result/sample_Resize_Area_rgb.rgb ive_data/result/sample_Resize_Area_gray.yuv ive_data/result/sample_Resize_Area_240p.rgb"
  run_command "test_ive_stcandicorner_thread 352 288 25 ive_data/penguin_352x288.gray.shitomasi.raw ive_data/result/sample_Shitomasi_CandiCorner.yuv 0 1 1"
  run_command "test_ive_stcandicorner_thread 640 480 25 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Shitomasi_sky_640x480.yuv 0 1 1"
  run_command "test_ive_gradfg_thread 352 288 0 ive_data/00_352x288_y.yuv ive_data/result/sample_GradFg_USE_CUR_GRAD.out"
  run_command "test_ive_gradfg_thread 352 288 1 ive_data/00_352x288_y.yuv ive_data/result/sample_GradFg_FIND_MIN_GRAD.out"
  run_command "test_ive_gradfg_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_GradFg_USE_CUR_GRAD.out"
  run_command "test_ive_gradfg_thread 640 480 1 ive_data/sky_640x480.yuv ive_data/result/sample_tile_GradFg_FIND_MIN_GRAD.out"
  run_command "test_ive_sad_thread 352 288 1 0 0 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode1_out0.bin ive_data/result/sample_Sad_thr_mode1_out0.bin 0 1 1 0"
  run_command "test_ive_sad_thread 352 288 2 0 0 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode2_out0.bin ive_data/result/sample_Sad_thr_mode2_out0.bin 0 1 1 0"
  run_command "test_ive_sad_thread 352 288 0 0 0 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode0_out0.bin ive_data/result/sample_Sad_thr_mode0_out0.bin 0 1 1 0"
  run_command "test_ive_sad_thread 352 288 0 0 1 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode0_out1.bin ive_data/result/sample_Sad_thr_mode0_out1.bin 0 1 1 0"
  run_command "test_ive_sad_thread 352 288 1 0 1 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode1_out1.bin ive_data/result/sample_Sad_thr_mode1_out1.bin 0 1 1 0"
  run_command "test_ive_ccl_thread 720 576 1 4 2 ive_data/ccl_raw_1.raw ive_data/result/sample_CCL_1.bin 0 1 1 0"
  run_command "test_ive_ccl_thread 1280 720 0 4 2 ive_data/ccl_raw_0.raw ive_data/result/sample_CCL_0.bin 0 1 1 0"
  run_command "test_ive_bgmodel_thread 352 288 ive_data/campus.u8c1.1_100.raw ive_data/result/sample_BgModelSample2_BgMdl_100.bin 0 1 1"
  run_command "test_ive_bgmodel_thread 704 288 ive_data/campus.u8c1.1_100.raw ive_data/result/sample_tile_BgModelSample2_BgMdl_100.bin 0 1 1"
  run_command "test_ive_bernsen_thread 352 288 0 5 ive_data/00_352x288_y.yuv ive_data/result/sample_Bernsen_5x5.yuv 0 1 1 0"
  run_command "test_ive_bernsen_thread 352 288 1 3 ive_data/00_352x288_y.yuv ive_data/result/sample_Bernsen_3x3_Thresh.yuv"
  run_command "test_ive_bernsen_thread 352 288 2 5 ive_data/00_352x288_y.yuv ive_data/result/sample_Bernsen_5x5_Paper.yuv"
  run_command "test_ive_filterandcsc_thread 352 288 1 0 4 4 8 ive_data/00_352x288_SP420.yuv ive_data/result/sample_FilterAndCSC_420SPToVideoPlanar3x3.yuv"
  run_command "test_ive_filterandcsc_thread 352 288 1 1 7 4 8 ive_data/00_352x288_SP420.yuv ive_data/result/sample_FilterAndCSC_420SPToVideoPlanar5x5.yuv"
  run_command "test_ive_16bitto8bit_thread 352 288 0 41 18508 0 6 2 ive_data/00_704x576.s16 ive_data/result/sample_16BitTo8Bit_S16ToS8.yuv"
  run_command "test_ive_16bitto8bit_thread 352 288 1 190 26690 0 6 1 ive_data/00_704x576.s16 ive_data/result/sample_16BitTo8Bit_Abs.yuv"
  run_command "test_ive_16bitto8bit_thread 352 288 2 225 15949 -42 6 1 ive_data/00_704x576.s16 ive_data/result/sample_16BitTo8Bit_Shift.yuv"
  run_command "test_ive_16bitto8bit_thread 352 288 3 174 27136 0 5 1 ive_data/00_704x576.u16 ive_data/result/sample_16BitTo8Bit_U16ToU8.yuv"
  run_command "test_ive_framediffmotion_thread 480 480 ive_data/md1_480x480.yuv ive_data/md2_480x480.yuv ive_data/result/sample_FrameDiffMotion.yuv"
  run_command "test_ive_framediffmotion_thread 960 480 ive_data/input1_960x480.yuv ive_data/input2_960x480.yuv ive_data/result/sample_tile_FrameDiffMotion.yuv"
}

run_blend(){
  if [ ! -d "stitch" ]; then
    echo "Error: Directory 'stitch' does not exist"
    echo "please prepare test data"
    return 1
  fi
  run_command "test_2way_blending -a stitch/c01_img1__2304x288-lft.yuv -b stitch/c01_img2__4608x288-lft.yuv -c 2304 -d 288 -e 4608 -f 288 -g 0 -h out/2way-4608x288.yuv420p -i 4608 -j 288 -k 0 -l 0 -m 2303 -r stitch/c01_alpha12_444p_m2__0_288x2304.bin -s stitch/c01_beta12_444p_m2__0_288x2304.bin -z stitch/c01_result_420p_c2_4608x288_lft_ovlp.yuv"
  run_command "test_2way_blending -a stitch/c01_img1__4608x288.yuv -b stitch/c01_img2__2304x288.yuv -c 4608 -d 288 -e 2304 -f 288 -g 0 -h out/2way-4608x288.yuv420p -i 4608 -j 288 -k 0 -l 2304 -m 4607 -r stitch/c01_alpha12_444p_m2__0_288x2304.bin -s stitch/c01_beta12_444p_m2__0_288x2304.bin -z stitch/c01_result_420p_c2_4608x288_rht_ovlp.yuv"
  run_command "test_2way_blending -a stitch/c01_lft__1536x384_pure_color.yuv -b stitch/c01_rht__1088x384_pure_color.yuv -c 1536 -d 384 -e 1088 -f 384 -g 0 -h out/2way-2400x384.yuv420p -i 2400 -j 384 -k 0 -l 1312 -m 1535 -r stitch/c01_alpha_444p_m2__0_384x224.bin -s stitch/c01_beta_444p_m2__0_384x224.bin -z stitch/c01_result_c2_2400x384_pure_color.yuv"
  run_command "test_2way_blending -a stitch/c01_lft__1536x384_pure_color.yuv -b stitch/c01_rht__1088x384_pure_color.yuv -c 1536 -d 384 -e 1088 -f 384 -g 0 -h out/2way-2400x384.yuv420p -i 2400 -j 384 -k 0 -l 1312 -m 1535 -r stitch/c01_alpha_m2__384x224_short.bin -s stitch/c01_beta_m2__384x224_short.bin -z stitch/c01_result_c2_2400x384_pure_color1.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_img1__4608x288.yuv -b stitch/c01_img2__4608x288.yuv -e 4608 -f 288 -g 0 -h out/2way-6912x288.yuv420p -i 6912 -j 288 -k 0 -l 2304 -m 4607 -r stitch/c01_alpha12_444p_m2__0_288x2304.bin -s stitch/c01_beta12_444p_m2__0_288x2304.bin -z stitch/c01_img12__6912x288.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_lft__4608x288_full_ovlp.yuv -b stitch/c01_rht__4608x288_full_ovlp.yuv -e 4608 -f 288 -g 0 -h out/2way-4608x288.yuv420p -i 4608 -j 288 -k 0 -l 0 -m 4607 -r stitch/c01_alpha_444p_m2__0_288x4608_full_ovlp.bin -s stitch/c01_beta_444p_m2__0_288x4608_full_ovlp.bin -z stitch/c01_result_420p_c2_4608x288_full_ovlp.yuv"
  run_command "test_4way_blending -N 2 -a stitch/1920x1080.yuv -b stitch/1920x1080.yuv -e 1920 -f 1080 -g 0 -h out/2way-3840x1080.yuv420p -i 3840 -j 1080 -k 0 -l 1920 -m 1919 -z stitch/c01_result_420p_c2_3840x1080_none_ovlp.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_img1__1536x288.yuv -b stitch/c01_img2__1536x288.yuv -e 1536 -f 288 -g 0 -h out/2way-3840x1080.yuv420p -i 3072 -j 288 -k 0 -l 1536 -m 1535 -z stitch/c01_result_420p_c2_3072x288_none_ovlp.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_img1__4608x288.yuv -b stitch/c01_img2__4608x288.yuv -e 4608 -f 288 -g 14 -h out/2way-6912x288.yuv400p -i 6912 -j 288 -k 14 -l 2304 -m 4607 -r stitch/c01_alpha12_444p_m2__0_288x2304.bin -s stitch/c01_beta12_444p_m2__0_288x2304.bin -z stitch/c01_img12__6912x288_fmt400.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_img1_422p__4608x288.yuv -b stitch/c01_img2_422p__4608x288.yuv -e 4608 -f 288 -g 1 -h out/2way-6912x288.yuv422p -i 6912 -j 288 -k 1 -l 2304 -m 4607 -r stitch/c01_alpha12_444p_m2__0_288x2304.bin -s stitch/c01_beta12_444p_m2__0_288x2304.bin -z stitch/c01_img12__6912x288_fmt422p.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_img1_444p__4608x288.yuv -b stitch/c01_img2_444p__4608x288.yuv -e 4608 -f 288 -g 2 -h out/2way-6912x288.yuv444p -i 6912 -j 288 -k 2 -l 2304 -m 4607 -r stitch/c01_alpha12_444p_m2__0_288x2304.bin -s stitch/c01_beta12_444p_m2__0_288x2304.bin -z stitch/c01_img12__6912x288_fmt444p.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_img1__4608x288.yuv -b stitch/c01_img2__4608x288.yuv -e 4608 -f 288 -g 0 -h out/2way-6912x288.yuv420p -i 6912 -j 288 -k 0 -l 2304 -m 4607 -r stitch/c01_alpha12_m2__288x2304_short.bin -s stitch/c01_beta12_m2__288x2304_short.bin -z stitch/c01_img12__6912x288_1.yuv -W 1"
  run_command "test_4way_blending -N 4 -a stitch/c01_img1__4608x288.yuv -b stitch/c01_img2__4608x288.yuv -c stitch/c01_img3__4608x288.yuv -d stitch/c01_img4__4608x288.yuv -e 4608 -f 288 -g 0 -h out/4way-11520x288.yuv420p -i 11520 -j 288 -k 0 -l 2304 -m 4607 -n 4608 -o 6911 -p 6912 -q 9215 -r stitch/c01_alpha12_444p_m2__0_288x2304.bin -s stitch/c01_beta12_444p_m2__0_288x2304.bin -t stitch/c01_alpha23_444p_m2__0_288x2304.bin -u stitch/c01_beta23_444p_m2__0_288x2304.bin -v stitch/c01_alpha34_444p_m2__0_288x2304.bin -w stitch/c01_beta34_444p_m2__0_288x2304.bin -x 1 -y 0 -z stitch/c01_img1234__11520x288_real.yuv"
  run_command "test_4way_blending -N 4 -a stitch/c01_lft__1024x1024.yuv -b stitch/c01_rht__1024x1024.yuv -c stitch/c01_lft__1024x1024.yuv -d stitch/c01_rht__1024x1024.yuv -e 1024 -f 1024 -g 0 -h out/4way-3072x1024.yuv420p -i 3072 -j 1024 -k 0 -l 768 -m 1023 -n 1280 -o 1791 -p 2048 -q 2303 -r stitch/c01_alpha_444p_m2__0_256x1024.bin -s stitch/c01_beta_444p_m2__0_256x1024.bin -t stitch/c01_alpha_444p_m2__0_1024x512.bin -u stitch/c01_beta_444p_m2__0_1024x512.bin -v stitch/c01_alpha_444p_m2__0_256x1024.bin -w stitch/c01_beta_444p_m2__0_256x1024.bin -x 1 -y 0 -z stitch/c01_img1234__3072x1024_real.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_lft__64x64.yuv -b stitch/c01_rht__64x64.yuv -e 64 -f 64 -g 0 -h out/2way-96x64.yuv420p -i 96 -j 64 -k 0 -l 32 -m 63 -r stitch/c01_alpha_444p_m2__0_64x32.bin -s stitch/c01_beta_444p_m2__0_64x32.bin -z stitch/c01_result_c2_96x64.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_lft__128x128.yuv -b stitch/c01_rht__128x128.yuv -e 128 -f 128 -g 0 -h out/2way-192x128.yuv420p -i 192 -j 128 -k 0 -l 64 -m 127 -r stitch/c01_alpha_444p_m2__0_128x64.bin -s stitch/c01_beta_444p_m2__0_128x64.bin -z stitch/c01_result_c2_192x128.yuv"
  run_command "test_4way_blending -N 2 -a stitch/c01_lft__1024x1024.yuv -b stitch/c01_rht__1024x1024.yuv -e 1024 -f 1024 -g 0 -h out/2way-1536x1024.yuv420p -i 1536 -j 1024 -k 0 -l 512 -m 1023 -r stitch/c01_alpha_444p_m2__0_1024x512.bin -s stitch/c01_beta_444p_m2__0_1024x512.bin -z stitch/c01_result_c2_1536x1024.yuv"
  run_command "test_4way_blending -N 3 -a stitch/c01_lft__1024x1024.yuv -b stitch/c01_rht__1024x1024.yuv  -c stitch/c01_lft__1024x1024.yuv   -e 1024 -f 1024 -g 0 -h out/3way-2304x1024.yuv420p -i 2304 -j 1024 -k 0  -l 768 -m 1023 -n 1280 -o 1791  -r  stitch/c01_alpha_444p_m2__0_256x1024.bin  -s stitch/c01_beta_444p_m2__0_256x1024.bin -t stitch/test_alpha_512x1024.bin  -u stitch/test_beta_444p_512x1024.bin -z stitch/test-3way-2304x1024.yuv420p"

}

run_ldc(){
  if [ ! -d "ldc_data" ]; then
    echo "Error: Directory 'ldc_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  run_command "test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1"
  run_command "test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot90.yuv 1920 1088 1 4 4 1 1"
  run_command "test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot270.yuv 1920 1088 3 4 4 1 1"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_load_mesh_barrel_0.yuv 1920 1080 4 4 ldc_data/1920x1080_barrel_0.3_r0_ofst_0_0_d-200.mesh 1 1"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_load_mesh_barrel_1.yuv 1920 1080 4 4 ldc_data/test_mesh_1920x1080_1_0_0_90_0_0_-200.mesh 1 1"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_load_mesh_pincushion_0.yuv 1920 1080 4 4 ldc_data/1920x1080_pincushion_0.3_r0_ofst_0_0_d400.mesh 1 1"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_load_mesh_pincushion_1.yuv 1920 1080 4 4 ldc_data/1920x1080_pincushion_0.3_r100_ofst_0_0_d400.mesh 1 1"
  run_command "test_ldc_gdc_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_0.yuv 1920 1080 4 4 1 0 0 0 0 0 -200 1 1"
  run_command "test_ldc_gdc_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_90.yuv 1920 1080 4 4 1 0 0 90 0 0 -200 1 1"
  run_command "test_ldc_gdc_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_pincushion_0.yuv 1920 1080 4 4 1 0 0 0 0 0 400 1 1"
  run_command "test_ldc_gdc_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_pincushion_100.yuv 1920 1080 4 4 1 0 0 100 0 0 400 1 1"
  run_command "test_ldc_gdc_gen_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_0.yuv 1920 1080 4 4 1 0 0 0 0 0 -200 1 1"
  run_command "test_ldc_gdc_gen_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_100.yuv 1920 1080 4 4 1 0 0 100 0 0 -200 1 1"
  run_command "test_ldc_gdc_gen_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_pincushion_0.yuv 1920 1080 4 4 1 0 0 0 0 0 400 1 1"
  run_command "test_ldc_gdc_gen_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_pincushion_100.yuv 1920 1080 4 4 1 0 0 100 0 0 400 1 1"
  run_command "test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 03fd51eb71461e1fcc64e072588b5754"
  run_command "test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot90.yuv 1920 1088 1 4 4 1 1 03ba2f6618972aa7d7e7194191a274a5"
  run_command "test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot270.yuv 1920 1088 3 4 4 1 1 808de3490552a1eedf94bf11a6dccd76"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_load_mesh_barrel_0.yuv 1920 1080 4 4 ldc_data/1920x1080_barrel_0.3_r0_ofst_0_0_d-200.mesh 1 1 9858d320f4ae52cade10d20ea61473c4"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_load_mesh_barrel_1.yuv 1920 1080 4 4 ldc_data/test_mesh_1920x1080_1_0_0_90_0_0_-200.mesh 1 1 2da6c1a297a52d0916bec1a7226fed53"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_load_mesh_pincushion_0.yuv 1920 1080 4 4 ldc_data/1920x1080_pincushion_0.3_r0_ofst_0_0_d400.mesh 1 1 78acfb91bd08f159623a85fe03a01169"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_load_mesh_pincushion_1.yuv 1920 1080 4 4 ldc_data/1920x1080_pincushion_0.3_r100_ofst_0_0_d400.mesh 1 1 8766b0ba7e315e5a8b413f69e0a26127"
  run_command "test_ldc_gdc_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_0.yuv 1920 1080 4 4 1 0 0 0 0 0 -200 1 1 9858d320f4ae52cade10d20ea61473c4"
  run_command "test_ldc_gdc_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_90.yuv 1920 1080 4 4 1 0 0 90 0 0 -200 1 1 2da6c1a297a52d0916bec1a7226fed53"
  run_command "test_ldc_gdc_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_pincushion_0.yuv 1920 1080 4 4 1 0 0 0 0 0 400 1 1 78acfb91bd08f159623a85fe03a01169"
  run_command "test_ldc_gdc_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_pincushion_100.yuv 1920 1080 4 4 1 0 0 100 0 0 400 1 1 8766b0ba7e315e5a8b413f69e0a26127"
  run_command "test_ldc_gdc_gen_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_0.yuv 1920 1080 4 4 1 0 0 0 0 0 -200 1 1 eabb55a370ece25767ffc09915b802b0"
  run_command "test_ldc_gdc_gen_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_100.yuv 1920 1080 4 4 1 0 0 100 0 0 -200 1 1 26488964ba2b88354f4d665ca6056836"
  run_command "test_ldc_gdc_gen_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_pincushion_0.yuv 1920 1080 4 4 1 0 0 0 0 0 400 1 1 fcb419d98eb1e8422bbc81cb9b5a0d6f"
  run_command "test_ldc_gdc_gen_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_pincushion_100.yuv 1920 1080 4 4 1 0 0 100 0 0 400 1 1 af4c825e1cca6b622d4d49476f7184cb"
  # multi-thread
  run_command "test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 8 1000 03fd51eb71461e1fcc64e072588b5754"
  run_command "test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot90.yuv 1920 1088 1 4 4 8 1000 03ba2f6618972aa7d7e7194191a274a5"
  run_command "test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot270.yuv 1920 1088 3 4 4 8 1000 808de3490552a1eedf94bf11a6dccd76"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_load_mesh_barrel_0.yuv 1920 1080 4 4 ldc_data/1920x1080_barrel_0.3_r0_ofst_0_0_d-200.mesh 8 1000 9858d320f4ae52cade10d20ea61473c4"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_load_mesh_barrel_1.yuv 1920 1080 4 4 ldc_data/test_mesh_1920x1080_1_0_0_90_0_0_-200.mesh 8 1000 2da6c1a297a52d0916bec1a7226fed53"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_load_mesh_pincushion_0.yuv 1920 1080 4 4 ldc_data/1920x1080_pincushion_0.3_r0_ofst_0_0_d400.mesh 8 1000 78acfb91bd08f159623a85fe03a01169"
  run_command "test_ldc_gdc_load_mesh_thread ldc_data/1920x1080_pincushion_0.3.yuv ldc_data/out_load_mesh_pincushion_1.yuv 1920 1080 4 4 ldc_data/1920x1080_pincushion_0.3_r100_ofst_0_0_d400.mesh 8 1000 8766b0ba7e315e5a8b413f69e0a26127"
  # grid_info
  run_command "test_ldc_gdc_grid_info_thread ldc_data/1280x768.yuv ldc_data/out_grid_info.yuv 1280 768 4 4 ldc_data/grid_info_79_44_3476_80_45_1280x720.dat 336080 1 1 9fe88b54ed51a09a9d245b805b346435"
  run_command "test_ldc_gdc_grid_info_thread ldc_data/grid_info_in_nv21/left_00.yuv ldc_data/grid_info_out_nv21/out_left_00.yuv 1280 720 4 4 ldc_data/bianli_grid_info_79_44_3476_80_45_1280x720.dat 336080 1 1 c547898b0af47720760c4ef40145a772"
  # run_command "test_ldc_gdc_grid_info_thread 1 1"
  # parameters loop test
  # run_command "test_ldc_rot_thread 1 1 64 64"
  # run_command "test_ldc_gdc_load_mesh_thread 1 1 64 64"
  # run_command "test_ldc_gdc_thread 1 1 64 64"
}

run_dwa(){
  if [ ! -d "dwa_data" ]; then
    echo "Error: Directory 'dwa_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  run_command "test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 1 1 e001fc14213febf4751fbd8d739d8f28"
  run_command "test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot90.bin 1 1 1 49361e51b503d001848b769f7dff6e87"
  run_command "test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot270.bin 3 1 1 b47f8bc0f5dbf60ff8cbe8fab5ca9cf6"
  run_command "test_dwa_gdc_thread 1920 1080 0 dwa_data/1920x1080_barrel_0.3.yuv 1920 1080 dwa_data/out_barrel_0.yuv 1 0 0 0 0 0 -200 1 1 0b8dfc8c16d1fa8b3024107a49471253"
  run_command "test_dwa_gdc_thread 1920 1080 0 dwa_data/1920x1080_barrel_0.3.yuv 1920 1080 dwa_data/out_barrel_100.yuv 1 0 0 100 0 0 -200 1 1 6fb920bfec14140d4bcc48571b051ff9"
  run_command "test_dwa_gdc_thread 1920 1080 0 dwa_data/1920x1080_pincushion_0.3.yuv 1920 1080 dwa_data/out_pincushion_0.yuv 1 0 0 0 0 0 400 1 1 6c05d5765415b17a3a458e2c08758ebf"
  run_command "test_dwa_gdc_thread 1920 1080 0 dwa_data/1920x1080_pincushion_0.3.yuv 1920 1080 dwa_data/out_pincushion_100.yuv 1 0 0 100 0 0 400 1 1 88ed7cebe4d5598a0659806bb10d3298"
  run_command "test_dwa_affine_thread 1920 1080 128 1152 0 dwa_data/girls_1920x1080.yuv dwa_data/out_affine.yuv 9 128 128 1 1 4bda32574cf6e2c01251a8b806dc89d1"
  run_command "test_dwa_fisheye_thread 1024 1024 1280 720 0 dwa_data/fisheye_floor_1024x1024.yuv dwa_data/out_fisheye_PANORAMA_360.yuv 1 1 0 128 128 512 512 0 0 0 1 0 1 1 1 3aecb97ec9360bbeb828d408f7b1621f"
  run_command "test_dwa_fisheye_thread 1024 1024 1280 720 0 dwa_data/fisheye_floor_1024x1024.yuv dwa_data/out_fisheye_PANORAMA_180.yuv 1 1 0 128 128 512 512 0 0 2 2 1 1 1 1 72c42538b8dca6404ef1fb54fbc99e73"
  run_command "test_dwa_fisheye_thread 1024 1024 1280 720 0 dwa_data/fisheye_floor_1024x1024.yuv dwa_data/out_fisheye_02_1O4R.yuv 1 1 0 128 128 512 512 0 0 0 4 0 1 1 1 d731d25f6b5377b1074e6d270fbd51af"
  run_command "test_dwa_fisheye_thread 1024 1024 1280 720 0 dwa_data/fisheye_floor_1024x1024.yuv dwa_data/out_fisheye_03_4R.yuv 1 1 0 128 128 512 512 0 0 0 5 0 1 1 1 65a0113738a9ef1edd1a70103e5aedf7"
  run_command "test_dwa_fisheye_thread 1024 1024 1280 720 0 dwa_data/fisheye_floor_1024x1024.yuv dwa_data/out_fisheye_04_1P2R.yuv 1 1 0 128 128 512 512 0 0 2 6 1 1 1 1 6476ecf48455a3aff7c2790ed062c8a3"
  run_command "test_dwa_fisheye_thread 1024 1024 1280 720 0 dwa_data/fisheye_floor_1024x1024.yuv dwa_data/out_fisheye_05_1P2R.yuv 1 1 0 128 128 512 512 0 0 2 7 1 1 1 1 aa5cc391a234ad7b9a58c6bf9859af3b"
  run_command "test_dwa_fisheye_thread 1024 1024 1280 720 0 dwa_data/fisheye_floor_1024x1024.yuv dwa_data/out_fisheye_06_1P.yuv 1 1 0 128 128 512 512 0 0 2 8 1 1 1 1 0164d0e050b7e8bdb3d7e88ee5d5e32e"
  run_command "test_dwa_fisheye_thread 1024 1024 1280 720 0 dwa_data/fisheye_floor_1024x1024.yuv dwa_data/out_fisheye_07_2P.yuv 1 1 0 128 128 512 512 0 0 0 9 0 1 1 1 0e93ac36d589e50e9fa84522605ee8bf"
  # grid_info
  run_command "test_dwa_gdc_grid_info_thread 1280 720 14 dwa_data/imgL_1280X720.yonly.yuv 1280 720 dwa_data/out_gdc_grid_L.yuv dwa_data/grid_info_79_43_3397_80_45_1280x720.dat 328480 1 1 2c7b7d382222b0e91c1cf778db875d01"
  run_command "test_dwa_gdc_grid_info_thread 1280 720 14 dwa_data/imgR_1280X720.yonly.yuv 1280 720 dwa_data/out_gdc_grid_R.yuv dwa_data/grid_info_79_44_3476_80_45_1280x720.dat 336080 1 1 c35ea9e07bdae46fbeae554133ff70b1"
  run_command "test_dwa_dewarp_grid_info_thread 1280 720 14 dwa_data/imgL_1280X720.yonly.yuv 1280 720 dwa_data/out_dewarp_grid_L.yuv dwa_data/grid_info_79_43_3397_80_45_1280x720.dat 328480 1 1 2c7b7d382222b0e91c1cf778db875d01"
  run_command "test_dwa_dewarp_grid_info_thread 1280 720 14 dwa_data/imgR_1280X720.yonly.yuv 1280 720 dwa_data/out_dewarp_grid_R.yuv dwa_data/grid_info_79_44_3476_80_45_1280x720.dat 336080 1 1 c35ea9e07bdae46fbeae554133ff70b1"
  run_command "test_dwa_fisheye_grid_info_thread 2240 2240 0 dwa_data/dc_src_2240x2240_L.yuv 2240 2240 dwa_data/out_fisheye_grid_L.yuv 1 dwa_data/L_grid_info_68_68_4624_70_70_dst_2240x2240_src_2240x2240.dat 446496 1 1 be161e6ff1ec06494f949862aaa62bc9"
  run_command "test_dwa_fisheye_grid_info_thread 2240 2240 0 dwa_data/dc_src_2240x2240_R.yuv 2240 2240 dwa_data/out_fisheye_grid_R.yuv 1 dwa_data/R_grid_info_68_68_4624_70_70_dst_2240x2240_src_2240x2240.dat 446496 1 1 9afd789c7940e50cfdd57718a3f0a001"
  # multi-thread
  run_command "test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 8 1000 e001fc14213febf4751fbd8d739d8f28"
  run_command "test_dwa_gdc_thread 1920 1080 0 dwa_data/1920x1080_barrel_0.3.yuv 1920 1080 dwa_data/out_barrel_100.yuv 1 0 0 100 0 0 -200 8 1000 6fb920bfec14140d4bcc48571b051ff9"
  run_command "test_dwa_affine_thread 1920 1080 128 1152 0 dwa_data/girls_1920x1080.yuv dwa_data/out_affine.yuv 9 128 128 8 1000 4bda32574cf6e2c01251a8b806dc89d1"
  run_command "test_dwa_fisheye_thread 1024 1024 1280 720 0 dwa_data/fisheye_floor_1024x1024.yuv dwa_data/out_fisheye_PANORAMA_360.yuv 1 1 0 128 128 512 512 0 0 0 1 0 1 8 1000 3aecb97ec9360bbeb828d408f7b1621f"
  run_command "test_dwa_gdc_grid_info_thread 1280 720 14 dwa_data/imgL_1280X720.yonly.yuv 1280 720 dwa_data/out_gdc_grid_L.yuv dwa_data/grid_info_79_43_3397_80_45_1280x720.dat 328480 8 1000 2c7b7d382222b0e91c1cf778db875d01"
  run_command "test_dwa_fisheye_grid_info_thread 2240 2240 0 dwa_data/dc_src_2240x2240_L.yuv 2240 2240 dwa_data/out_fisheye_grid_L.yuv 1 dwa_data/L_grid_info_68_68_4624_70_70_dst_2240x2240_src_2240x2240.dat 446496 8 1000 be161e6ff1ec06494f949862aaa62bc9"
  # parameters loop test
  # run_command "test_dwa_rot_thread 1 1 32 32"
  # run_command "test_dwa_gdc_thread 1 1 32 32"
  # run_command "test_dwa_fisheye_thread 1 1 32 32"
}

run_tde(){
  if [ ! -d "tde_data" ]; then
    echo "Error: Directory 'tde_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  run_command "rm -rf ./out/result_bmp; mkdir ./out/result_bmp"
  run_command "cp /opt/sophon/libsophon-current/bin/test_tde ./out/"
  run_command "pushd ./out/"
  run_command "test_tde"
  run_command "popd"
  run_command "diff out/result_bmp tde_data"
}

run_all(){
  run_vpss
  run_tpu
  run_dpu
  run_ldc
  run_dwa
  run_blend
  run_ive
  run_tde
  run_kill_vpss
  run_kill_tpu
  run_kill_dpu
  run_kill_ldc
  run_kill_dwa
  run_kill_ive
  run_kill_blend
}

run_kill(){
  run_kill_vpss
  run_kill_tpu
  run_kill_dpu
  run_kill_ldc
  run_kill_dwa
  run_kill_ive
  run_kill_blend
}

run_kill_vpss(){
  COMMAND1="test_vpss_convert_thread 1 2147483647000 &
            test_vpss_convert_thread 1 2147483647000 &
            test_vpss_convert_thread 1 2147483647000 &
            test_vpss_convert_thread 1 2147483647000 &
            test_vpss_convert_thread 1 2147483647000 &
            test_vpss_convert_thread 1 2147483647000 &
            test_vpss_convert_thread 1 2147483647000 &
            test_vpss_convert_thread 1 2147483647000 &
            "
  COMMAND2="test_vpss_convert_to_thread 1 300 & \
            test_vpss_convert_to_thread 1 300 & \
            test_vpss_convert_to_thread 1 300 & \
            test_vpss_convert_to_thread 1 300 &
            "


  # Check and start command 1, test_vpss_convert_thread
  start_command1() {
    if ! pgrep -f "test_vpss_convert_thread" >/dev/null; then
      eval "$COMMAND1" &
      eval "$COMMAND1" &
    fi
  }

  # Start command 2, test_vpss_convert_to_thread
  start_command2() {
    eval "$COMMAND2" &
    eval "$COMMAND2" &
  }

  kill_processes() {
      pkill -9 -f "test_vpss_convert_thread"
  }

  # Get the current time plus 5 minutes (5 * 60 seconds)
  END=$(( $(date +%s) + 5*60 ))

  while [ $(date +%s) -lt $END ]; do
    start_command1

    start_command2

    if [ $(( $(date +%s) % 20 )) -lt 2 ]; then
      kill_processes
    fi

    sleep 10
  done
  kill_processes
}


run_kill_tpu(){
  COMMAND1="test_cv_absdiff 1 1 0 10 1080 1920 &
            test_cv_absdiff 1 1 0 10 1080 1920 &
            test_cv_absdiff 1 1 0 10 1080 1920 &
            test_cv_absdiff 1 1 0 10 1080 1920 &
            test_cv_absdiff 1 1 0 10 1080 1920 &
            test_cv_absdiff 1 1 0 10 1080 1920 &
            test_cv_absdiff 1 1 0 10 1080 1920 &
            test_cv_absdiff 1 1 0 10 1080 1920 &
            "
  COMMAND2="test_cv_add_weight 1 1 0 1920 1080 10 & \
            test_cv_add_weight 1 1 0 1920 1080 10 & \
            test_cv_add_weight 1 1 0 1920 1080 10 & \
            test_cv_add_weight 1 1 0 1920 1080 10 &
            "

  start_command1() {
    if ! pgrep -f "test_cv_absdiff" >/dev/null; then
      eval "$COMMAND1" &
      eval "$COMMAND1" &
    fi
  }

  start_command2() {
    eval "$COMMAND2" &
    eval "$COMMAND2" &
  }

  kill_processes() {
      pkill -9 -f "test_cv_absdiff"
  }

  END=$(( $(date +%s) + 5*60 ))

  while [ $(date +%s) -lt $END ]; do
    start_command1
    start_command2

    if [ $(( $(date +%s) % 20 )) -lt 2 ]; then
      kill_processes
    fi

    sleep 10
  done
  kill_processes
}

run_kill_dpu(){
  if [ ! -d "dpu_data" ]; then
    echo "Error: Directory 'dpu_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  COMMAND1="test_dpu_sgbm_thread 512 284 3 dpu_data/sofa_left_img_512x284.bin dpu_data/sofa_right_img_512x284.bin dpu_data/205pU8Disp_ref_512x284.bin 1 0 1 1500 &
            test_dpu_sgbm_thread 512 284 3 dpu_data/sofa_left_img_512x284.bin dpu_data/sofa_right_img_512x284.bin dpu_data/205pU8Disp_ref_512x284.bin 1 0 1 1500 &
            "
  COMMAND2="test_dpu_online_thread 512 284 4 dpu_data/sofa_left_img_512x284.bin dpu_data/sofa_right_img_512x284.bin dpu_data/fgs_512x284_res.bin 0 0 1 600 & \
            test_dpu_online_thread 512 284 4 dpu_data/sofa_left_img_512x284.bin dpu_data/sofa_right_img_512x284.bin dpu_data/fgs_512x284_res.bin 0 0 1 600 &
            "

  start_command1() {
    if ! pgrep -f "test_dpu_sgbm_thread" >/dev/null; then
      eval "$COMMAND1" &
      eval "$COMMAND1" &
    fi
  }

  start_command2() {
    eval "$COMMAND2" &
    eval "$COMMAND2" &
  }

  kill_processes() {
      pkill -9 -f "test_dpu_sgbm_thread"
  }

  END=$(( $(date +%s) + 5*60 ))

  while [ $(date +%s) -lt $END ]; do
    start_command1
    start_command2

    if [ $(( $(date +%s) % 20 )) -lt 2 ]; then
      kill_processes
    fi

    sleep 10
  done
  kill_processes
}

run_kill_ldc(){
  if [ ! -d "ldc_data" ]; then
    echo "Error: Directory 'ldc_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  COMMAND1="test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 &
            test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 &
            test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 &
            test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 &
            test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 &
            test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 &
            test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 &
            test_ldc_rot_thread ldc_data/1920x1088_nv21.bin ldc_data/out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 &
            "
  COMMAND2="test_ldc_gdc_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_0.yuv 1920 1080 4 4 1 0 0 0 0 0 -200 1 1 & \
            test_ldc_gdc_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_0.yuv 1920 1080 4 4 1 0 0 0 0 0 -200 1 1 & \
            test_ldc_gdc_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_0.yuv 1920 1080 4 4 1 0 0 0 0 0 -200 1 1 & \
            test_ldc_gdc_thread ldc_data/1920x1080_barrel_0.3.yuv ldc_data/out_barrel_0.yuv 1920 1080 4 4 1 0 0 0 0 0 -200 1 1 &
            "

  start_command1() {
    if ! pgrep -f "test_ldc_rot_thread" >/dev/null; then
      eval "$COMMAND1" &
      eval "$COMMAND1" &
    fi
  }

  start_command2() {
    eval "$COMMAND2" &
    eval "$COMMAND2" &
  }

  kill_processes() {
      pkill -9 -f "test_ldc_rot_thread"
  }

  END=$(( $(date +%s) + 5*60 ))

  while [ $(date +%s) -lt $END ]; do
    start_command1
    start_command2

    if [ $(( $(date +%s) % 20 )) -lt 2 ]; then
      kill_processes
    fi

    sleep 10
  done
  kill_processes
}

run_kill_dwa(){
  if [ ! -d "dwa_data" ]; then
    echo "Error: Directory 'dwa_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  COMMAND1="test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 1 1 e001fc14213febf4751fbd8d739d8f28 &
            test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 1 1 e001fc14213febf4751fbd8d739d8f28 &
            test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 1 1 e001fc14213febf4751fbd8d739d8f28 &
            test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 1 1 e001fc14213febf4751fbd8d739d8f28 &
            test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 1 1 e001fc14213febf4751fbd8d739d8f28 &
            test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 1 1 e001fc14213febf4751fbd8d739d8f28 &
            test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 1 1 e001fc14213febf4751fbd8d739d8f28 &
            test_dwa_rot_thread 128 128 14 dwa_data/128x128_sophgo.bin 128 128 dwa_data/out_128x128_rot0.bin 0 1 1 e001fc14213febf4751fbd8d739d8f28 &
            "
  COMMAND2="test_dwa_gdc_thread 1920 1080 0 dwa_data/1920x1080_barrel_0.3.yuv 1920 1080 dwa_data/out_barrel_0.yuv 1 0 0 0 0 0 -200 1 1 0b8dfc8c16d1fa8b3024107a49471253 & \
            test_dwa_gdc_thread 1920 1080 0 dwa_data/1920x1080_barrel_0.3.yuv 1920 1080 dwa_data/out_barrel_0.yuv 1 0 0 0 0 0 -200 1 1 0b8dfc8c16d1fa8b3024107a49471253 & \
            test_dwa_gdc_thread 1920 1080 0 dwa_data/1920x1080_barrel_0.3.yuv 1920 1080 dwa_data/out_barrel_0.yuv 1 0 0 0 0 0 -200 1 1 0b8dfc8c16d1fa8b3024107a49471253 & \
            test_dwa_gdc_thread 1920 1080 0 dwa_data/1920x1080_barrel_0.3.yuv 1920 1080 dwa_data/out_barrel_0.yuv 1 0 0 0 0 0 -200 1 1 0b8dfc8c16d1fa8b3024107a49471253 &
            "

  start_command1() {
    if ! pgrep -f "test_dwa_rot_thread" >/dev/null; then
      eval "$COMMAND1" &
      eval "$COMMAND1" &
    fi
  }

  start_command2() {
    eval "$COMMAND2" &
    eval "$COMMAND2" &
  }

  kill_processes() {
      pkill -9 -f "test_dwa_rot_thread"
  }

  END=$(( $(date +%s) + 5*60 ))

  while [ $(date +%s) -lt $END ]; do
    start_command1
    start_command2

    if [ $(( $(date +%s) % 20 )) -lt 2 ]; then
      kill_processes
    fi

    sleep 10
  done
  kill_processes
}

run_kill_ive(){
  if [ ! -d "ive_data" ]; then
    echo "Error: Directory 'ive_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  COMMAND1="test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0 &
            test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0 &
            test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0 &
            test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0 &
            test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0 &
            test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0 &
            test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0 &
            test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0 &
            "
  COMMAND2="test_ive_dilate_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Dilate_3x3.yuv 0 1 1 0 & \
            test_ive_dilate_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Dilate_3x3.yuv 0 1 1 0 & \
            test_ive_dilate_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Dilate_3x3.yuv 0 1 1 0 & \
            test_ive_dilate_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Dilate_3x3.yuv 0 1 1 0 &
            "

  start_command1() {
    if ! pgrep -f "test_ive_add_thread" >/dev/null; then
      eval "$COMMAND1" &
      eval "$COMMAND1" &
    fi
  }

  start_command2() {
    eval "$COMMAND2" &
    eval "$COMMAND2" &
  }

  kill_processes() {
      pkill -9 -f "test_ive_add_thread"
  }

  END=$(( $(date +%s) + 5*60 ))

  while [ $(date +%s) -lt $END ]; do
    start_command1
    start_command2

    if [ $(( $(date +%s) % 20 )) -lt 2 ]; then
      kill_processes
    fi

    sleep 10
  done
  kill_processes
}

run_kill_blend(){
  if [ ! -d "stitch" ]; then
    echo "Error: Directory 'stitch' does not exist"
    echo "please prepare test data"
    return 1
  fi
  COMMAND1="test_2way_blending -a stitch/c01_lft__1536x384_pure_color.yuv -b stitch/c01_rht__1088x384_pure_color.yuv -c 1536 -d 384 -e 1088 -f 384 -g 0 -h out/2way-2400x384.yuv420p -i 2400 -j 384 -k 0 -l 1312 -m 1535 -r stitch/c01_alpha_m2__384x224_short.bin -s stitch/c01_beta_m2__384x224_short.bin -z stitch/c01_result_c2_2400x384_pure_color1.yuv &
            test_2way_blending -a stitch/c01_lft__1536x384_pure_color.yuv -b stitch/c01_rht__1088x384_pure_color.yuv -c 1536 -d 384 -e 1088 -f 384 -g 0 -h out/2way-2400x384.yuv420p -i 2400 -j 384 -k 0 -l 1312 -m 1535 -r stitch/c01_alpha_m2__384x224_short.bin -s stitch/c01_beta_m2__384x224_short.bin -z stitch/c01_result_c2_2400x384_pure_color1.yuv &
            test_2way_blending -a stitch/c01_lft__1536x384_pure_color.yuv -b stitch/c01_rht__1088x384_pure_color.yuv -c 1536 -d 384 -e 1088 -f 384 -g 0 -h out/2way-2400x384.yuv420p -i 2400 -j 384 -k 0 -l 1312 -m 1535 -r stitch/c01_alpha_m2__384x224_short.bin -s stitch/c01_beta_m2__384x224_short.bin -z stitch/c01_result_c2_2400x384_pure_color1.yuv &
            test_2way_blending -a stitch/c01_lft__1536x384_pure_color.yuv -b stitch/c01_rht__1088x384_pure_color.yuv -c 1536 -d 384 -e 1088 -f 384 -g 0 -h out/2way-2400x384.yuv420p -i 2400 -j 384 -k 0 -l 1312 -m 1535 -r stitch/c01_alpha_m2__384x224_short.bin -s stitch/c01_beta_m2__384x224_short.bin -z stitch/c01_result_c2_2400x384_pure_color1.yuv &
            "
  COMMAND2="test_4way_blending -N 2 -a stitch/c01_lft__128x128.yuv -b stitch/c01_rht__128x128.yuv -e 128 -f 128 -g 0 -h out/2way-192x128.yuv420p -i 192 -j 128 -k 0 -l 64 -m 127 -r stitch/c01_alpha_444p_m2__0_128x64.bin -s stitch/c01_beta_444p_m2__0_128x64.bin -z stitch/c01_result_c2_192x128.yuv & \
            test_4way_blending -N 2 -a stitch/c01_lft__128x128.yuv -b stitch/c01_rht__128x128.yuv -e 128 -f 128 -g 0 -h out/2way-192x128.yuv420p -i 192 -j 128 -k 0 -l 64 -m 127 -r stitch/c01_alpha_444p_m2__0_128x64.bin -s stitch/c01_beta_444p_m2__0_128x64.bin -z stitch/c01_result_c2_192x128.yuv &
            "
  start_command1() {
    if ! pgrep -f "test_2way_blending" >/dev/null; then
      eval "$COMMAND1" &
      eval "$COMMAND1" &
    fi
  }

  start_command2() {
    eval "$COMMAND2" &
    eval "$COMMAND2" &
  }

  kill_processes() {
    pkill -9 -f "test_2way_blending"
  }

  END=$(( $(date +%s) + 5*60 ))

  while [ $(date +%s) -lt $END ]; do
    start_command1
    start_command2

    if [ $(( $(date +%s) % 20 )) -lt 2 ]; then
      kill_processes
    fi

    sleep 10
  done
  kill_processes
}

if [ $bmcv_case = "all" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_all
      ((count++))
  done
fi

if [ $bmcv_case = "kill" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_kill
      ((count++))
  done
fi

if [ $bmcv_case = "vpss" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_vpss
      ((count++))
  done
fi

if [ $bmcv_case = "tpu" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_tpu
      ((count++))
  done
fi

if [ $bmcv_case = "dpu" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_dpu
      ((count++))
  done
fi

if [ $bmcv_case = "ldc" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_ldc
      ((count++))
  done
fi

if [ $bmcv_case = "dwa" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_dwa
      ((count++))
  done
fi

if [ $bmcv_case = "blend" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_blend
      ((count++))
  done
fi

if [ $bmcv_case = "ive" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_ive
      ((count++))
  done
fi

if [ $bmcv_case = "tde" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_tde
      ((count++))
  done
fi

if [ $bmcv_case = "kill_vpss" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_kill_vpss
      ((count++))
  done
fi

if [ $bmcv_case = "kill_tpu" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_kill_tpu
      ((count++))
  done
fi

if [ $bmcv_case = "kill_dpu" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_kill_dpu
      ((count++))
  done
fi

if [ $bmcv_case = "kill_ldc" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_kill_ldc
      ((count++))
  done
fi

if [ $bmcv_case = "kill_dwa" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_kill_dwa
      ((count++))
  done
fi

if [ $bmcv_case = "kill_ive" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_kill_ive
      ((count++))
  done
fi

if [ $bmcv_case = "kill_blend" ]; then
  eval "mkdir -p out"
  if [ ! -d "out" ]; then
    echo "Error: Directory 'out' does not exist after creation attempt"
    echo "please check the current directory permissions"
    return 1
  fi
  while [ $count -le $loop ]
  do
      run_kill_blend
      ((count++))
  done
fi

if [ $failed_count -gt 0 ]; then
  echo "Total failed commands: $failed_count"
  echo -e "Failed scripts:\n$failed_scripts"
else
  echo "All tests pass!"
fi