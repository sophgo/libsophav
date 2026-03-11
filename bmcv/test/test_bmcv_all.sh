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

run_tde(){
  run_command "test_tde_thread"
  run_command "test_tde_thread 1 100"
  run_command "test_tde_thread 2 100"
  run_command "test_tde_thread fill 1920 1080 17 0 0 1920 1080 out/tde_fill_argb8888.bin 0xffff0000 1 1 2d5c8e54fa6cf76e94e98a520fecccf6"
  run_command "test_tde_thread fill 1920 1080 33 0 0 1920 1080 out/tde_fill_abgr1555.bin 0xffff0000 1 1 e0a268915b826029fb6c1171dc9a9cc3"
  run_command "test_tde_thread fill 1920 1080 31 0 0 1920 1080 out/tde_fill_abgr4444.bin 0xffff0000 1 1 122a9498c74d7791511e9bdfdcf53192"
  run_command "test_tde_thread convert 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 1920 1080 1024 768 17 out/tde_convert_1024x768_argb8888.bin 0 0 1024 768 0 1 1 1c04fef7d6bd939608d9477601f8c8a0"
  run_command "test_tde_thread convert 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 0 0 1920 1080 1024 768 33 out/tde_convert_1024x768_abgr1555.bin 0 0 1024 768 0 1 1 77514ab14a54e92fcf1d3a718096f056"
  run_command "test_tde_thread convert 300 300 17 /opt/sophon/libsophon-current/bin/res/300x300_argb8888_dog.rgb 0 0 300 300 1280 720 31 out/tde_convert_1280x720_abgr4444.bin 0 0 1280 720 0 1 1 2769f7ab5f4f341a0fcddeaf742ab7ac"
  run_command "test_tde_thread convert 300 300 17 /opt/sophon/libsophon-current/bin/res/300x300_argb8888_dog.rgb 0 0 300 300 300 300 33 out/tde_rot_300x300_abgr1555.bin 0 0 300 300 90 1 1 271d938128c0c3bb1af6179a7075cb92"
  run_command "test_tde_thread convert 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 0 0 1920 1080 540 960 33 out/tde_rot_540x960_abgr1555.bin 0 0 540 960 270 1 1 2adc82f8c85e782a7f536d2a0c6a3039"
  run_command "test_tde_thread overlay 300 300 17 /opt/sophon/libsophon-current/bin/res/300x300_argb8888_dog.rgb 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 0 0 300 300 out/tde_overlay_1920x1080_rgb.bin 128 1 1 3e586826d36675834a7c4d65b0542fc9"
  run_command "test_tde_thread overlay 300 300 17 /opt/sophon/libsophon-current/bin/res/300x300_argb8888_dog.rgb 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 100 100 500 500 out/tde_overlay_1920x1080_rgb_01.bin 255 1 1 8dab16147817184f15b1fa6551972fe0"
  run_command "test_tde_thread line 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 100 100 500 500 out/tde_line_1920x1080_rgb.bin 0xffff0000 3 1 1 6c770378b088c8e65fc1366050a71be7"
  run_command "test_tde_thread line 300 300 17 /opt/sophon/libsophon-current/bin/res/300x300_argb8888_dog.rgb 200 200 100 150 out/tde_line_300x300_argb.bin 0x7fff0000 5 1 1 cdcb8f5b520479c5c963b513d454d945"
  run_command "test_tde_thread draw 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 300 300 5 200 out/tde_draw_1920x1080_rgb.bin 0xffff0000 1 1 9ff770a076338554af19086bc4614a43"
  run_command "test_tde_thread draw 300 300 17 /opt/sophon/libsophon-current/bin/res/300x300_argb8888_dog.rgb 150 150 6 100 out/tde_draw_300x300_argb.bin 0x7fff0000 1 1 5f9aa658a3a0b6a29209c77c01fdb6cc"
  run_command "test_tde_thread warp 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 500 500 10 out/tde_warp0_500x500_rgb.bin 376 282 482 292 378 366 1 1 3c570b3c4a2d0899c8338f8d3f0e225d"
  run_command "test_tde_thread warp 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 500 500 10 out/tde_warp1_500x500_rgb.bin 732 80 845 150 750 325 1 1 f29363b92306dcbefcea3aebd3269c18"
  run_command "test_tde_thread warp 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 500 500 10 out/tde_warp2_500x500_rgb.bin 1430 210 1530 180 1430 480 1 1 7dddbd03836eba4af6f08e605353a4ef"
}

run_vpss() {
  run_command "test_vpss_convert 1920 1080 0 0 1920 1080 1920 1080 0 10 1 1 1 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin"
  run_command "test_vpss_convert 1920 1080 480 270 960 540 960 540 8 10 1 1 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_convert 1920 1080 0 0 1920 1080 666 888 8 9 1 1 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_convert 16 16 0 0 9 9 16 16 2 0 1 1 0"
  run_command "test_vpss_convert 512 512 503 9 9 503 16 16 10 24 1 0 0"
  run_command "test_vpss_convert 1024 1024 512 512 512 512 16 8192 16 22 1 0 0"
  run_command "test_vpss_convert 512 512 256 400 256 112 8192 8192 26 0 1 1 0"
  run_command "test_vpss_draw_rectangle 1920 1080 8 1 480 270 960 540 2 0 255 0 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_draw_rectangle 16 16 8 1 0 0 2 2 1 0 0 0 0"
  run_command "test_vpss_draw_rectangle 512 512 9 1 0 0 512 512 1 0 255 0 0"
  run_command "test_vpss_draw_rectangle 512 512 10 1 0 0 512 512 256 255 0 255 0"
  run_command "test_vpss_draw_rectangle 1024 1024 11 1 512 0 256 2 1 0 125 0 0"
  run_command "test_vpss_draw_rectangle 4096 4096 8 1 4094 4094 2 2 1 0 0 255 0"
  run_command "test_vpss_fill_rectangle 1920 1080 8 1 480 270 960 540 0 255 0 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_fill_rectangle 16 16 8 1 0 0 2 2 1 0 0 0"
  run_command "test_vpss_fill_rectangle 1024 1024 9 1 0 0 1024 512 255 255 255 0"
  run_command "test_vpss_fill_rectangle 2048 2048 10 1 2046 0 2 2048 0 255 255 0"
  run_command "test_vpss_fill_rectangle 4096 4096 11 1 4 4 4000 4000 255 125 255 0"
  run_command "test_vpss_watermark 1920 1080 128 128 8 1 900 480 128 128 151 255 152 1 /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_watermark 16 16 16 16 8 1 0 0 32 32 0 0 0 1 vpss_data_add/16x16_rgbp.bin vpss_data_add/water_16x16_u8.bin"
  run_command "test_vpss_watermark 512 256 64 64 8 1 128 0 64 64 255 255 255 1 vpss_data_add/512x256_rgbp.bin vpss_data_add/water_64x64_binary.raw"
  run_command "test_vpss_watermark 1920 1080 1920 540 10 1 0 540 1920 540 0 128 255 1 vpss_data_add/512x256_rgb.bin vpss_data_add/water_1920x540_binary.raw"
  run_command "test_vpss_watermark 4096 4096 1280 720 10 1 2048 2048 1280 720 0 128 255 1 vpss_data_add/4096x4096_rgb.bin vpss_data_add/water_1280x720_binary.raw"
  run_command "test_vpss_watermark 8192 8192 2048 4096 10 1 0 0 2048 4096 255 0 128 1 vpss_data_add/8192x8192_rgb.bin vpss_data_add/water_2048x4096_u8.bin"
  run_command "test_vpss_convert_to 1920 1080 1.1 0.9 3.14159 10 20 -30 8 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_convert_to 16 16 1 1 1 0 0 0 8 0"
  run_command "test_vpss_convert_to 512 512 10 10 10 -10 -10 -10 9 0"
  run_command "test_vpss_convert_to 1024 1024 -100 -100 -100 100 100 100 8 0"
  run_command "test_vpss_convert_to 4096 4096 -100.1 100.1 -1000 100 -100 100.1 9 0"
  run_command "test_vpss_padding 1920 1080 480 270 960 540 20 20 960 960 151 255 152 1000 1000 8 1 1 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin"
  run_command "test_vpss_padding 16 16 0 0 4 4 0 0 4 4 0 255 0 16 16 8 0 0"
  run_command "test_vpss_padding 512 16 0 12 512 4 12 0 4 508 255 0 255 16 512 9 1 0"
  run_command "test_vpss_padding 4096 16 4092 0 4 8 4092 256 4 256 255 255 255 4096 512 10 2 0"
  run_command "test_vpss_padding 1024 1024 512 0 512 4 0 0 2048 2048 125 0 125 2048 4096 11 0 0"
  run_command "test_vpss_padding 8192 8192 0 0 8192 4 0 0 34 4 0 125 125 200 200 8 0 0"
  run_command "test_vpss_convert_thread"
  run_command "test_vpss_convert_to_thread"
  run_command "test_vpss_copy_to_thread"
  run_command "test_vpss_draw_rectangle_thread"
  run_command "test_vpss_draw_rectangle_thread 64 64 0 vpss_data_add/64x64_yuv420.bin 60 60 128 128 null 0 2 2 2b2b07a33ada2cb8bcb8dd9e11002e4b"
  run_command "test_vpss_draw_rectangle_thread 512 256 2 vpss_data_add/512x256_yuv444.bin 256 128 128 512 null 0 2 1 7c87b47c4a08eac1c43f57acca642069"
  run_command "test_vpss_draw_rectangle_thread 960 540 3 vpss_data_add/960x540_nv12.bin 100 100 860 340 null 0 2 1 6a52666454a09ee37239804b320cd8dd"
  run_command "test_vpss_draw_rectangle_thread 1920 1080 4 vpss_data_add/1920x1080_nv21.bin 192 108 2048 2048 null 0 1 2 7da1365102f2e49d8d6910adb7c67b01"
  run_command "test_vpss_draw_rectangle_thread 4096 4096 10 vpss_data_add/4096x4096_rgb.bin 2048 2048 128 512 null 0 1 1 ed431f366e26b73cd1df3f126b7175ba"
  run_command "test_vpss_draw_rectangle_thread 8192 8192 14 vpss_data_add/8192x8192_gray.bin 1920 1080 4096 4096 null 0 1 1 032bfa20b1d63b1ff175dcc8eecc1b54"
  run_command "test_vpss_fbd_thread"
  run_command "test_vpss_fill_rectangle_thread"
  run_command "test_vpss_fill_rectangle_thread 16 16 0 vpss_data_add/16x16_yuv420.bin 14 14 2 2 null 0 2 2 bbe8f73703cf0a128797ac4cbf1b75a8"
  run_command "test_vpss_fill_rectangle_thread 512 256 2 vpss_data_add/512x256_yuv444.bin 256 128 256 128 null 0 1 2 6b1d50fe3031b258c6efa4574fede1dd"
  run_command "test_vpss_fill_rectangle_thread 960 540 4 vpss_data_add/960x540_nv21.bin 0 270 480 54 null 0 1 2 be30e62d3a3b5e0a187813a62d588a53"
  run_command "test_vpss_fill_rectangle_thread 1920 1080 8 vpss_data_add/1920x1080_rgbp.bin 512 653 512 128 null 0 2 1 67321b80e023f5f8eca6f12522a745ac"
  run_command "test_vpss_fill_rectangle_thread 4096 4096 14 vpss_data_add/4096x4096_gray.bin 4000 400 40 400 null 0 1 1 2d0649d1b0ef3b696edf4900ca9460c2"
  run_command "test_vpss_fill_rectangle_thread 8192 8192 10 vpss_data_add/8192x8192_rgb.bin 0 0 8192 8192 null 0 1 1 496301de8b1e121eeb211fcaa06349c9"
  run_command "test_vpss_mosaic_thread"
  run_command "test_vpss_padding_thread"
  run_command "test_vpss_stitch_thread"
  run_command "test_vpss_water_thread"
  run_command "test_vpss_point_thread"
  run_command "test_vpss_csc_overlay_thread"
  run_command "test_vpss_csc_overlay_thread 16 16 0 vpss_data_add/16x16_yuv420.bin 100 16 4 null 0 0 0 2 2 7333e4543e0789361e28108a8c62b0d4"
  run_command "test_vpss_csc_overlay_thread 960 540 0 vpss_data_add/960x540_yuv420.bin 1920 1080 4 null 1 1820 1080 2 1 e2e24b3f8b9fa47f40303b8834211ff4"
  run_command "test_vpss_csc_overlay_thread 1920 1080 0 vpss_data_add/1920x1080_yuv420.bin 512 256 4 null 2 256 128 1 2 2e6eab5d4ce68a5b03a75beae1952622"
  run_command "test_vpss_csc_overlay_thread 8192 8192 0 vpss_data_add/8192x8192_yuv420.bin 8192 8192 4 null 3 1920 4096 1 1 87b69220eecfadffe0825a6d878a0c47"
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
  run_command "test_vpss_convert_thread 16 16 0 vpss_data_add/16x16_yuv420.bin 0 0 16 16 16 16 0 null 0 0 1 2 2 d6eb8830bac14fb2a536431b3abf532b"
  run_command "test_vpss_convert_thread 64 64 10 vpss_data_add/64x64_rgb.bin 8 8 40 40 16 16 10 null 1 0 1 2 1 a9fb1210e0cb7a7a566bf8c4d0a1bbee"
  run_command "test_vpss_convert_thread 960 540 10 vpss_data_add/960x540_rgb.bin 20 20 532 276 256 256 0 null 2 0 1 1 2 d31bd44827d8ca77cf764a68605374bf"
  run_command "test_vpss_convert_thread 1920 1080 0 vpss_data_add/1920x1080_yuv420.bin 960 540 64 128 1920 1080 10 null 1 0 1 1 1 47141e1d3469fbd2ec08e25a3b1a3838"
  run_command "test_vpss_convert_to_thread 1920 1080 8 /opt/sophon/libsophon-current/bin/res/1920x1080_rgbp.bin 0.5 0.5 0.5 200 200 200 8 out/convert_to_1920x1080.rgbp 0"
  run_command "test_vpss_convert_to_thread 16 16 8 vpss_data_add/16x16_rgbp.bin 0.1 0.1 0.1 0 0 0 8 null 0 2 2 0b7e25134e6dfef1bfdc715867211e5b"
  run_command "test_vpss_convert_to_thread 512 256 0 vpss_data_add/512x256_yuv420.bin -0.5 -0.5 -0.5 255 255 255 0 null 0 2 1 5ffa125b62d64c8ad9dd7a6daf38279f"
  run_command "test_vpss_convert_to_thread 1920 1080 10 vpss_data_add/1920x1080_rgb.bin 0.3 -0.3 0.3 -10 200 -10 10 null 0 1 2 335ccf5d101bbceb4d977ee5e10537b5"
  run_command "test_vpss_convert_to_thread 4096 4096 14 vpss_data_add/4096x4096_gray.bin 0.8 0.2 0.6 -50 100 -150 14 null 0 1 1 b94d9f8921e64824301c474feec74646"
  run_command "test_vpss_copy_to_thread 800 600 0 /opt/sophon/libsophon-current/bin/res/800x600_yuv420.bin 0 0 1920 1080 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin  out/copy_to_1920x1080.yuv 0"
  run_command "test_vpss_copy_to_thread 16 16 8 vpss_data_add/16x16_rgbp.bin 0 0 16 16 vpss_data_add/16x16_rgbp.bin null 0 2 2 fb1126ebf722d0c546595d325f0600fb"
  run_command "test_vpss_copy_to_thread 512 256 10 vpss_data_add/512x256_rgb.bin 50 50 960 540 vpss_data_add/960x540_rgb.bin null 0 2 1 67c84ead75d0affda77980d3cc78dddf"
  run_command "test_vpss_copy_to_thread 960 540 10 vpss_data_add/960x540_rgb.bin 500 500 1920 1080 vpss_data_add/1920x1080_rgb.bin null 0 1 2 621c31493ed3c64f8e0c3281ef3a30a9"
  run_command "test_vpss_copy_to_thread 4096 4096 14 vpss_data_add/4096x4096_gray.bin 4096 4096 8192 8192 vpss_data_add/8192x8192_gray.bin null 0 1 1 98ba545d623d9348e5cd1f5fabb8944e"
  run_command "test_vpss_draw_rectangle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 800 600 out/draw_rect_1920x1080.yuv420 0"
  run_command "test_vpss_fbd_thread 1920 1080 69632 /opt/sophon/libsophon-current/bin/res/fbc/1920x1080_table_y.bin /opt/sophon/libsophon-current/bin/res/fbc/1920x1080_data_y.bin /opt/sophon/libsophon-current/bin/res/fbc/1920x1080_table_c.bin /opt/sophon/libsophon-current/bin/res/fbc/1920x1080_data_c.bin 0 0 1920 1080 1920 1080 0 out/fbd_1920x1080.yuv420 0"
  run_command "test_vpss_fill_rectangle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 100 100 200 200 out/fill_rect_1920x1080_yuv420.bin 0"
  run_command "test_vpss_mosaic_thread 1920 1080 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin 100 100 500 500 out/mosaic_1920x1080.rgb 0"
  run_command "test_vpss_mosaic_thread 16 16 14 vpss_data_add/16x16_gray.bin 0 0 8 8 null 0 2 2 ec71e28cc59e6a71b21c1586c5340e76"
  run_command "test_vpss_mosaic_thread 64 64 10 vpss_data_add/64x64_rgb.bin 56 56 8 8 null 0 1 2 5fa44628ee2b30d30c60700116a15be0"
  run_command "test_vpss_mosaic_thread 512 256 9 vpss_data_add/512x256_bgrp.bin 51 25 81 81 null 0 2 1 a3efc714d994b7361e8e12a71317e942"
  run_command "test_vpss_mosaic_thread 4096 4096 3 vpss_data_add/4096x4096_nv12.bin 2048 2048 2048 1024 null 0 1 2 d7d325ad7246c3a5e4ab5b4e8db9bb7a"
  run_command "test_vpss_mosaic_thread 8192 8192 2 vpss_data_add/8192x8192_yuv444.bin 6512 3512 512 512 null 0 1 1 04e15fbb4f02c44ef6ba77ebcccc8ed7"
  run_command "test_vpss_padding_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 1920 1080 2048 2048 0 out/pad_2048x2048_yuv420.bin 1 0"
  run_command "test_vpss_padding_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 150 150 150 220 0 out/pad_150x220_yuv420.bin 1 0 1 1 14f7de62b33736eda6eb95a4b81c69e7"
  run_command "test_vpss_padding_thread 16 16 0 vpss_data_add/16x16_yuv420.bin 0 0 16 16 16 16 0 null 0 0 2 2 d6eb8830bac14fb2a536431b3abf532b"
  run_command "test_vpss_padding_thread 960 540 0 vpss_data_add/960x540_yuv420.bin 540 0 420 540 1920 1080 0 null 1 0 2 1 8543db26d14771c5c88ff8c1ef4d51e7"
  run_command "test_vpss_padding_thread 4096 4096 0 vpss_data_add/4096x4096_yuv420.bin 0 2048 500 100 4096 8192 0 null 2 0 1 2 85d834194886b0d03bf9d73bcc594532"
  run_command "test_vpss_padding_thread 8192 8192 0 vpss_data_add/8192x8192_yuv420.bin 4096 4096 1024 1024 8192 8192 0 null 1 0 1 1 6ec570c3bac10ccdd2cd5c2a51ebda22"
  run_command "test_vpss_point_thread 512 256 0 vpss_data_add/512x256_yuv420.bin 60 40 1 null 0 2 2 8e9d25f07402779734e0b5d46207c62a"
  run_command "test_vpss_point_thread 960 540 0 vpss_data_add/960x540_yuv420.bin 512 256 10 null 0 1 2 bd18a3655a57b5404f22c0e998b03de4"
  run_command "test_vpss_point_thread 4096 4096 0 vpss_data_add/4096x4096_yuv420.bin 2048 2048 2048 null 0 2 1 6f900534d6f0057f35a181d36b80f1a9"
  run_command "test_vpss_point_thread 8192 8192 0 vpss_data_add/8192x8192_yuv420.bin 1024 4096 512 null 0 1 1 daf5c311d1964851cf86b825dadd83cc"
  run_command "test_vpss_stitch_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 0 0 1920 1080 0 1080 1920 1080 1920 2160 out/stitch_1920x2160_yuv420.bin 0"
  run_command "test_vpss_stitch_thread 16 16 0 vpss_data_add/16x16_yuv420.bin 0 0 16 16 0 16 16 16 16 32 out/stitch_16x32_yuv420.bin 0 2 2"
  run_command "test_vpss_stitch_thread 64 64 0 vpss_data_add/64x64_yuv420.bin 8 0 56 32 0 32 32 64 64 256 out/stitch_64x256_yuv420.bin 0 2 1"
  run_command "test_vpss_stitch_thread 512 256 0 vpss_data_add/512x256_yuv420.bin 64 0 192 128 64 128 64 64 256 512 out/stitch_256x512_yuv420.bin 0 1 2"
  run_command "test_vpss_stitch_thread 8192 8192 0 vpss_data_add/8192x8192_yuv420.bin 4096 512 2048 1024 4096 128 1024 256 6144 4096 out/stitch_6144x4096_yuv420.bin 0 1 1"
  run_command "test_vpss_water_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin /opt/sophon/libsophon-current/bin/res/128x128_sophgo.bin 16384 128 0 100 100 out/water_1920x1080.yuv420 0"
  run_command "test_vpss_water_thread 16 16 0 vpss_data_add/16x16_yuv420.bin vpss_data_add/water_16x16_u8.bin 256 16 0 0 0 result/16x16_yuv420_water_res.bin 0 2 2 faa07a8b084dae79fdc5fabbbc1df6a0"
  run_command "test_vpss_water_thread 4096 4096 0 vpss_data_add/4096x4096_yuv420.bin vpss_data_add/water_1920x540_binary.raw 1036800 1920 0 2048 0 result/4096x4096_yuv420_water_res.bin 0 1 2 7e62bd976896a0abe841bf783247562e"
  run_command "test_vpss_water_thread 8192 8192 0 vpss_data_add/8192x8192_yuv420.bin vpss_data_add/water_2048x4096_u8.bin 8388608 2048 0 0 0 result/8192x8192_yuv420_water_res.bin 0 1 1 7607bd4936dca5f1086a1a8632715bec"
  run_command "test_vpss_overlay_thread 1920 1080 10 300 300 17 20 20 /opt/sophon/libsophon-current/bin/res/car_rgb888.rgb /opt/sophon/libsophon-current/bin/res/300x300_argb8888_dog.rgb 663ce7ae1f1b3154a66a6366a6332559"
  run_command "test_vpss_overlay_thread 1920 1080 10 80 60 30 100 100 /opt/sophon/libsophon-current/bin/res/car_rgb888.rgb /opt/sophon/libsophon-current/bin/res/dog_s_80x60_pngto4444.bin 52e0003832180b4c74b71395c200c72e"
  run_command "test_vpss_overlay_thread 1920 1080 10 80 60 32  200 200 /opt/sophon/libsophon-current/bin/res/car_rgb888.rgb /opt/sophon/libsophon-current/bin/res/dog_s_80x60_pngto1555.bin a858701cb01131f1356f80c76e7813c0"
  run_command "test_vpss_overlay_thread 16 16 10 16 16 17 0 0 vpss_data_add/16x16_rgb.bin vpss_data_add/16x16_argb.bin 0691aebf511120cd343b074494d06007 0 0 2 2"
  run_command "test_vpss_overlay_thread 960 540 10 512 256 30 200 20 vpss_data_add/960x540_rgb.bin vpss_data_add/512x256_argb4444.bin 5cf931177c2b79140ac79cae0e707743 0 0 2 1"
  run_command "test_vpss_overlay_thread 1920 1080 10 960 540 32 960 0 vpss_data_add/1920x1080_rgb.bin vpss_data_add/960x540_argb1555.bin 22793f042487e7a9440bba4a7ab62345 0 0 1 2"
  run_command "test_vpss_overlay_thread 8192 8192 10 4096 4096 17 0 4096 vpss_data_add/8192x8192_rgb.bin vpss_data_add/4096x4096_argb.bin 81470049e67aca2f159f0455effce154 0 0 1 1"
  run_command "test_vpss_flip_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 1 h_flip.bin 0 1 1 37890f4ab5d6458b57824de9e24ba94f"
  run_command "test_vpss_flip_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 2 v_flip.bin 0 1 1 a3a86425b349c6517a2506e4df2334f0"
  run_command "test_vpss_flip_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 3 hv_flip.bin 0 1 1 80062a883675498f41d3309cfc6ddb52"
  run_command "test_vpss_flip_thread 16 16 0 vpss_data_add/16x16_yuv420.bin 1 null 0 2 2 e9549948cc20f7a871aecc6b1978535c"
  run_command "test_vpss_flip_thread 960 540 0 vpss_data_add/960x540_yuv420.bin 2 null 0 1 2 cf530e4ac304f5707852ad6cce444573"
  run_command "test_vpss_flip_thread 4096 4096 0 vpss_data_add/4096x4096_yuv420.bin 3 null 0 2 1 314f16c4429997ed72b8d0303154ffd9"
  run_command "test_vpss_flip_thread 8192 8192 0 vpss_data_add/8192x8192_yuv420.bin 2 null 0 1 1 5e0b0366eef62edcbf6ddb224b4df45c"
  run_command "test_vpss_circle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 960 540 250 -2 empty.bin 0 1 1 94a364c93d13aad8db3d38146c5ba4fa"
  run_command "test_vpss_circle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 960 540 250 -1 shape.bin 0 1 1 d2c467732a8021d3f212e27dd0a98677"
  run_command "test_vpss_circle_thread 1920 1080 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin 960 540 250 10 line.bin 0 1 1 583e2f078ceccfa73b4b084d7cf74aca"
  run_command "test_vpss_circle_thread 16 16 0 vpss_data_add/16x16_yuv420.bin 16 16 17 -1 null 0 2 2 19e5e9bc4a957a9f53f1f687ed6f0ea1"
  run_command "test_vpss_circle_thread 512 256 0 vpss_data_add/512x256_yuv420.bin 200 50 50 -2 null 0 2 1 0db54759b597133bd28b7e94f6cb07fb"
  run_command "test_vpss_circle_thread 960 540 0 vpss_data_add/960x540_yuv420.bin 300 250 100 0 null 0 1 2 3086e0aa404772465ed4d6648485d2da"
  run_command "test_vpss_circle_thread 1920 1080 0 vpss_data_add/1920x1080_yuv420.bin 1000 500 500 15 null 0 1 1 741d45cc7bb1238de33d1a7f1cb44f22"
}

run_tpu(){
  for i in {1..5}; do
    run_command "test_bmcv_rotate"
  done
  run_command "test_bmcv_rotate 2 1 0 16 16"
  run_command "test_bmcv_rotate 1 1 0 128 128 2"
  run_command "test_bmcv_rotate 2 1 0 512 1024 9 90"
  run_command "test_bmcv_rotate 1 1 0 4096 4096 12 180"
  run_command "test_bmcv_rotate 1 1 0 8192 8192 14 270"
  for i in {1..5}; do
    run_command "test_cv_absdiff"
  done
  run_command "test_cv_absdiff 2 1 0 12 8 8"
  run_command "test_cv_absdiff 1 1 1 10 1080 1920 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin out/asbdiff_output.bin"
  run_command "test_cv_absdiff 1 2 0 0"
  run_command "test_cv_absdiff 2 2 0 4 512"
  run_command "test_cv_absdiff 1 1 0 8 2048"
  run_command "test_cv_absdiff 1 1 0 14 4096 4096"
  for i in {1..5}; do
    run_command "test_cv_add_weight"
  done
  run_command "test_cv_add_weight 2"
  run_command "test_cv_add_weight 2 1"
  run_command "test_cv_add_weight 2 1 0 8 8 12 0 0.6 0.4 10"
  run_command "test_cv_add_weight 1 1 1 1920 1080 10 0 0.5 0.5 10 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin out/add_weight_output.bin"
  run_command "test_cv_add_weight 1 1 0 128"
  run_command "test_cv_add_weight 2 2 0 512 128"
  run_command "test_cv_add_weight 2 1 0 1024 128 0"
  run_command "test_cv_add_weight 1 2 0 1024 2048 3 0"
  run_command "test_cv_add_weight 1 1 0 2048 2048 7 1 0.01"
  run_command "test_cv_add_weight 1 1 0 512 2048 10 0 0.99 0.01"
  run_command "test_cv_add_weight 1 1 0 64 4096 14 1 0.3 0.7 0"
  run_command "test_cv_add_weight 1 1 0 4096 4096 14 0 0.2 0.8 255"
  for i in {1..5}; do
    run_command "test_cv_as_strided"
  done
  run_command "test_cv_as_strided 2"
  run_command "test_cv_as_strided 1 3"
  run_command "test_cv_as_strided 2 1 5 5 3 3 2 1"
  run_command "test_cv_as_strided 2 2 8"
  run_command "test_cv_as_strided 2 1 8 1024"
  run_command "test_cv_as_strided 1 2 2048 8 8"
  run_command "test_cv_as_strided 1 1 8 8 3128 8 1 1"
  run_command "test_cv_as_strided 1 1 8 8 8 4096 4096 8"
  run_command "test_cv_as_strided 1 1 4096 4096 4096 4096 4096 4096"
  for i in {1..5}; do
    run_command "test_attribute_filter_topk"
  done
  run_command "test_attribute_filter_topk 1 1 0"
  run_command "test_attribute_filter_topk 1 2 0 5"
  run_command "test_attribute_filter_topk 1 1 1 8 5"
  run_command "test_attribute_filter_topk 2 2 0 5 100 1024"
  run_command "test_attribute_filter_topk 1 2 0 8 1000 2048 0"
  run_command "test_attribute_filter_topk 2 1 0 5 50000 51200 99 0"
  run_command "test_attribute_filter_topk 1 1 0 8 20000 20480 999 315360000 -128 0"
  run_command "test_attribute_filter_topk 1 1 0 8 10000 4096000 999999999 0 0 1"
  run_command "test_cv_axpy"
  run_command "test_cv_axpy 2"
  run_command "test_cv_axpy 2 1"
  run_command "test_cv_axpy 2 2"
  for i in {1..5}; do
    run_command "test_cv_batch_topk"
  done
  run_command "test_cv_batch_topk 2"
  run_command "test_cv_batch_topk 1 3"
  run_command "test_cv_batch_topk 1 1 50 20 2 0 0"
  run_command "test_cv_batch_topk 2 1"
  run_command "test_cv_batch_topk 2 2 10 10 1 0 0"
  run_command "test_cv_batch_topk 1 1 999999 100 32 1 0"
  run_command "test_cv_batch_topk 1 1 99999 50 1 0 1"
  run_command "test_cv_bayer2rgb"
  run_command "test_cv_bayer2rgb 2"
  run_command "test_cv_bayer2rgb 1 1 0 128 128 0"
  run_command "test_cv_bayer2rgb 1 1 1 1024 1024 0 /opt/sophon/libsophon-current/bin/res/bayer.bin out/out_bayer2rgb.bin"
  for i in {1..5}; do
    run_command "test_cv_bayer2rgb"
  done
  run_command "test_cv_bayer2rgb 1"
  run_command "test_cv_bayer2rgb 1 1"
  run_command "test_cv_bayer2rgb 2 1 0"
  run_command "test_cv_bayer2rgb 1 2 0 8"
  run_command "test_cv_bayer2rgb 2 2 0 8 8"
  run_command "test_cv_bayer2rgb 1 1 0 4094 4094 0"
  run_command "test_cv_bayer2rgb 1 1 0 2048 2048 1"
  for i in {1..5}; do
    run_command "test_cv_bitwise"
  done
  run_command "test_cv_bitwise 3"
  run_command "test_cv_bitwise 1 2"
  run_command "test_cv_bitwise 2 2 0 1024 1024 12 7"
  run_command "test_cv_bitwise 1 1 1 1080 1920 10 7 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin out/bitwise_output.bin"
  run_command "test_cv_bitwise 1 1 0 8"
  run_command "test_cv_bitwise 2 1 0 8 8"
  run_command "test_cv_bitwise 1 1 0 512 1024 0"
  run_command "test_cv_bitwise 1 1 0 512 1024 3 7"
  run_command "test_cv_bitwise 1 1 0 2048 2048 6 8"
  run_command "test_cv_bitwise 1 1 0 4096 4096 14 9"
  for i in {1..5}; do
    run_command "test_cv_calc_hist"
  done
  run_command "test_cv_calc_hist 2"
  run_command "test_cv_calc_hist 3 1"
  run_command "test_cv_calc_hist 1 1 512 512 1 0 1 0"
  run_command "test_cv_calc_hist 1 2 8"
  run_command "test_cv_calc_hist 2 2 8 8"
  run_command "test_cv_calc_hist 1 1 128 512 3"
  run_command "test_cv_calc_hist 1 1 512 512 3 1"
  run_command "test_cv_calc_hist 1 1 512 1024 2 0 2"
  run_command "test_cv_calc_hist 1 1 1024 512 1 0 1 0"
  run_command "test_cv_calc_hist 1 1 1024 1024 2 0 1 1"
  run_command "test_cv_hist_balance 2 2 8 8"
  run_command "test_cv_hist_balance 1 2 512 512"
  run_command "test_cv_hist_balance 1 1 1024 1024"
  run_command "test_cv_hist_balance 1 1 8192 8192"
  for i in {1..5}; do
    run_command "test_cv_cmulp"
  done
  run_command "test_cv_cmulp 1 1 256 20"
  run_command "test_cv_cmulp 2 2 1 2"
  run_command "test_cv_cmulp 2 1 1 1980"
  run_command "test_cv_cmulp 1 2 4096 1"
  run_command "test_cv_cmulp 1 1 4096 1980"
  run_command "test_cv_convert_to"
  run_command "test_cv_convert_to 2 1 -1 0"
  run_command "test_cv_copy_to"
  run_command "test_cv_copy_to 2"
  run_command "test_cv_copy_to 1 2"
  run_command "test_cv_copy_to 2 2"
  for i in {1..5}; do
    run_command "test_cv_distance"
  done
  run_command "test_cv_distance 1 1 1 512 3"
  run_command "test_cv_distance 2 2 0 1 1"
  run_command "test_cv_distance 2 1 1 40960 1"
  run_command "test_cv_distance 1 2 0 1 4"
  run_command "test_cv_distance 1 1 1 40960 8"
  run_command "test_cv_distance 1 1 1 20480 2"
  for i in {1..5}; do
    run_command "test_cv_draw_lines"
  done
  run_command "test_cv_draw_lines 1 1 1080 1920 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin out/output_draw_lines.bin"
  run_command "test_cv_draw_lines 2"
  run_command "test_cv_draw_lines 1 2"
  run_command "test_cv_draw_lines 2 2 8"
  run_command "test_cv_draw_lines 2 1 8 8"
  run_command "test_cv_draw_lines 1 1 2000 2148 0"
  run_command "test_cv_draw_lines 1 1 4096 4096 4"
  run_command "test_cv_draw_lines 1 1 1000 1000 6"
  run_command "test_cv_draw_lines 1 1 512 512 14"
  run_command "test_cv_fft 1 1 100 100 0 0"
  run_command "test_cv_fft 1 1 100 100 0 1"
  run_command "test_cv_fft 1 1 100 100 1 0"
  run_command "test_cv_fft 1 1 100 100 1 1"
  run_command "test_cv_fft"
  run_command "test_cv_fft 1"
  run_command "test_cv_fft 1 2"
  run_command "test_cv_fft 2 1 64"
  run_command "test_cv_fft 2 2 8 512"
  run_command "test_cv_fft 1 1 512 1 0"
  run_command "test_cv_fft 1 1 1024 1 1"
  for i in {1..5}; do
    run_command "test_cv_gaussian_blur"
  done
  run_command "test_cv_gaussian_blur 2 1 1 512 512 8 3 0.5 0.5"
  run_command "test_cv_gaussian_blur 1 1 0 1920 1080 10 3 0.5 0.5 /opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin out/out_gaussian_blur.bin"
  run_command "test_cv_gaussian_blur 1 2 1 8"
  run_command "test_cv_gaussian_blur 1 1 1 512 512"
  run_command "test_cv_gaussian_blur 2 1 1 8 8 8"
  run_command "test_cv_gaussian_blur 2 2 1 8 8 9 3"
  run_command "test_cv_gaussian_blur 1 1 1 8 8 12 5 0"
  run_command "test_cv_gaussian_blur 1 1 1 8 8 13 7 0 0"
  run_command "test_cv_gaussian_blur 1 1 1 4096 8192 14 3 5 5"
  run_command "test_cv_gaussian_blur 1 1 1 2048 8192 8 5 0 5"
  run_command "test_cv_gaussian_blur 1 1 1 1500 8192 9 7 0 0"
  run_command "test_cv_gaussian_blur 1 1 1 1024 4096 12 5 2.5 2.5"
  run_command "test_cv_gaussian_blur 1 1 1 700 4096 13 7 2.5 4"
  run_command "test_cv_gemm 2"
  run_command "test_cv_gemm 2 2 1 1 1 -4.95 -4.95 0 0"
  run_command "test_cv_gemm 1 2 800 1 1 0 -4.95 1 1"
  run_command "test_cv_gemm 2 1 1 800 1 -4.95 0 0 1"
  run_command "test_cv_gemm 1 2 1 1 800 4.95 -4.95 1 1"
  run_command "test_cv_gemm 1 1 800 800 800 4.95 4.95 0 1"
  run_command "test_cv_hist_balance"
  run_command "test_cv_hist_balance 2 1 800 800"
  run_command "test_cv_hm_distance 1 1 8"
  for i in {1..5}; do
    run_command "test_cv_hm_distance"
  done
  run_command "test_cv_hm_distance 2 2 1 4"
  run_command "test_cv_hm_distance 1 2 1 8 1"
  run_command "test_cv_hm_distance 2 1 1 16 1"
  run_command "test_cv_hm_distance 1 1 1 32 16 100000"
  run_command "test_cv_jpeg"
  for i in {1..5}; do
    run_command "test_cv_laplace"
  done
  run_command "test_cv_laplace 2 1 1 0 512 512 14"
  run_command "test_cv_laplace 2 1 3 1 1080 1920 14 /opt/sophon/libsophon-current/bin/res/1920x1080_gray.bin out/laplace_output.bin"
  run_command "test_cv_laplace 2 2 1 0 2 2 14"
  run_command "test_cv_laplace 2 1 3 0 4096 2 14"
  run_command "test_cv_laplace 1 2 1 0 2 4096 14"
  run_command "test_cv_laplace 1 1 3 0 2048 2048 14"
  run_command "test_cv_laplace 1 1 1 0 4096 4096 14"
  for i in {1..5}; do
    run_command "test_cv_matmul"
  done
  run_command "test_cv_matmul 2"
  run_command "test_cv_matmul 1 1 0 0 0 0 1 50 100"
  run_command "test_cv_matmul 1 1 1 1 1 2 5 50 100"
  run_command "test_cv_matmul 2 2 0 0 0 0 1 1 1"
  run_command "test_cv_matmul 2 1 1 0 0 1 6 1 1"
  run_command "test_cv_matmul 1 2 0 1 0 2 1 512 1"
  run_command "test_cv_matmul 1 1 0 0 1 1 1 1 9216"
  run_command "test_cv_matmul 1 1 1 1 0 0 6 512 9216"
  for i in {1..5}; do
    run_command "test_cv_min_max"
  done
  run_command "test_cv_min_max 2 1 512"
  run_command "test_cv_min_max 2 2 8"
  run_command "test_cv_min_max 1 2 1000"
  run_command "test_cv_min_max 1 1 10000"
  run_command "test_cv_min_max 1 1 1000049"
  run_command "test_cv_nms 2 1 100 0.7"
  run_command "test_cv_nms 1 1 100 0.7"
  for i in {1..5}; do
    run_command "test_cv_put_text"
  done
  run_command "test_cv_put_text 1 1 1080 1920 0 /opt/sophon/libsophon-current/bin/res/1920x1080_yuv420.bin out/output_put_text.bin"
  run_command "test_cv_put_text 2 1 8"
  run_command "test_cv_put_text 2 2 8 8"
  run_command "test_cv_put_text 1 2 100 1024 0"
  run_command "test_cv_put_text 1 1 2048 2048 6"
  run_command "test_cv_put_text 1 1 4096 8 3"
  run_command "test_cv_put_text 1 1 8 8192 14"
  for i in {1..5}; do
    run_command "test_cv_pyramid"
  done
  run_command "test_cv_pyramid 1 1 0 512 512"
  run_command "test_cv_pyramid 1 1 1 1080 1920 /opt/sophon/libsophon-current/bin/res/1920x1080_gray.bin out/pyramid_output.bin"
  run_command "test_cv_pyramid 2 1 0 8"
  run_command "test_cv_pyramid 2 2 0 8 8"
  run_command "test_cv_pyramid 1 2 0 2048 1022"
  run_command "test_cv_pyramid 1 1 0 4096 2043"
  for i in {1..5}; do
    run_command "test_cv_quantify"
  done
  run_command "test_cv_quantify 2"
  run_command "test_cv_quantify 1 1 0 512 512"
  run_command "test_cv_quantify 2 1 0 8"
  run_command "test_cv_quantify 2 2 0 8 8"
  run_command "test_cv_quantify 1 2 0 2048 2048 0"
  run_command "test_cv_quantify 1 1 0 4096 4096 9"
  run_command "test_cv_quantify 1 1 0 8192 4096 8"
  run_command "test_cv_quantify 1 1 0 8 8192 9"
  for i in {1..5}; do
    run_command "test_cv_sort"
  done
  run_command "test_cv_sort 2"
  run_command "test_cv_sort 1 1 1000 1000"
  run_command "test_cv_sort 2 2"
  run_command "test_cv_sort 2 1 1 1"
  run_command "test_cv_sort 1 2 2 1"
  run_command "test_cv_sort 1 1 600000 50000"
  run_command "test_cv_sort 1 1 700000 100000"
  run_command "test_cv_sort 1 1 1000000 1000000"
  run_command "test_cv_stft 1"
  run_command "test_cv_stft 1 1 1 200 10 0 256 128 256 0 0 0 1"
  run_command "test_cv_stft 2 2 128 1 2 1 2 1 2 0 1 1 0"
  run_command "test_cv_stft 2 1 64 3 9 0 27 26 27 1 2 1 0"
  run_command "test_cv_stft 1 2 32 16 16 1 2 1 2 1 3 0 0"
  run_command "test_cv_stft 1 1 16 125 25 0 80 1 100 0 4 1 0"
  run_command "test_cv_stft 1 1 8 64 8 0 100 50 512 0 0 1 0"
  run_command "test_cv_stft 1 1 3 81 3 0 10 1 500 1 1 0 0"
  run_command "test_cv_istft 1"
  run_command "test_cv_istft 1 1 4096 1 1 1 1 1 4096 1024"
  run_command "test_cv_istft"
  run_command "test_cv_istft 2 1"
  run_command "test_cv_istft 2 2 8"
  run_command "test_cv_istft 1 2 64 1"
  run_command "test_cv_istft 1 1 128 16 1"
  run_command "test_cv_istft 1 1 512 8 1 0"
  run_command "test_cv_istft 1 1 1024 4 0 0"
  run_command "test_cv_istft 1 1 2048 2 0 0 1 1 16 8"
  run_command "test_cv_istft 1 1 4096 1 0 1 1 0 4096 4096"
  run_command "test_cv_istft 1 1 8192 1 0 1 0 1 512 256"
  for i in {1..5}; do
    run_command "test_cv_threshold"
  done
  run_command "test_cv_threshold 2"
  run_command "test_cv_threshold 1 1 1 1920 1080 2 /opt/sophon/libsophon-current/bin/res/1920x1080_gray.bin out/threshold_output.bin"
  run_command "test_cv_threshold 1 2"
  run_command "test_cv_threshold 2 1 0"
  run_command "test_cv_threshold 1 1 0 64"
  run_command "test_cv_threshold 2 1 0 8 8"
  run_command "test_cv_threshold 1 1 0 4096 4096 0"
  run_command "test_cv_threshold 1 1 0 8192 8192 4"
  for i in {1..5}; do
    run_command "test_cv_transpose"
  done
  run_command "test_cv_transpose 2 1 0 1 3 512 512"
  run_command "test_cv_transpose 1 1 1 4 1 1080 1920 /opt/sophon/libsophon-current/bin/res/1920x1080_gray.bin out/transpose_output.bin"
  run_command "test_cv_transpose 2 2 0 1 3"
  run_command "test_cv_transpose 1 2 0 4 1 8"
  run_command "test_cv_transpose 2 1 0 4 3 8 8"
  run_command "test_cv_transpose 1 1 0 1 1 4096 8"
  run_command "test_cv_transpose 1 1 0 4 3 8 4096"
  run_command "test_cv_transpose 1 1 0 1 1 4096 4096"
  run_command "test_cv_warp_affine"
  run_command "test_cv_warp_affine 2"
  run_command "test_cv_warp_affine 0 0 8 8 8 8"
  run_command "test_cv_warp_affine 0 1 8 8 8 8"
  run_command "test_cv_warp_affine 0 0 4096 4096 4096 4096"
  run_command "test_cv_warp_affine 0 1 2048 3840 2048 2048"
  run_command "test_cv_warp_affine_padding"
  run_command "test_cv_warp_affine_padding 2"
  run_command "test_cv_warp_affine_padding 0 0 8 8 8 8"
  run_command "test_cv_warp_affine_padding 0 1 8 8 8 8"
  run_command "test_cv_warp_affine_padding 0 0 4096 4096 4096 4096"
  run_command "test_cv_warp_affine_padding 0 1 2048 3840 2048 2048"
  run_command "test_cv_warp_perspective"
  run_command "test_cv_warp_perspective 1"
  run_command "test_cv_warp_perspective 0 0 8 8 8 8"
  run_command "test_cv_warp_perspective 0 1 8 8 8 8"
  run_command "test_cv_warp_perspective 0 0 4096 4096 4096 4096"
  run_command "test_cv_warp_perspective 0 1 2048 3840 2048 2048"
  run_command "test_cv_width_align 1 1 0 0 0"
  run_command "test_cv_width_align 1 1 0 1 5 8"
  run_command "test_cv_width_align 2 2 0 0 8 8 8"
  run_command "test_cv_width_align 1 2 0 0 8 1024 8 1030"
  run_command "test_cv_width_align 2 2 0 1 9 2048 8 2054 12"
  run_command "test_cv_width_align 1 1 0 0 10 1024 512 1024 512"
  run_command "test_cv_width_align 1 1 0 1 11 3072 3072 3081 3080"
  run_command "test_cv_width_align 1 1 0 0 14 4096 4096 4105 4105"
  for i in {1..5}; do
    run_command "test_faiss_indexflatIP"
  done
  run_command "test_faiss_indexflatIP 2 2"
  run_command "test_faiss_indexflatIP 2 1 1 1 1"
  run_command "test_faiss_indexflatIP 1 1 2 1 1 1 0 1 9"
  run_command "test_faiss_indexflatIP 1 1 100079 50 30 128 0 3 5"
  run_command "test_faiss_indexflatIP 1 1 1500 20 10 512 1 5 3"
  run_command "test_faiss_indexflatIP 1 1 2000 25 15 756 1 5 5"
  run_command "test_faiss_indexflatIP 1 1 300 30 20 900 1 3 3"
  for i in {1..5}; do
    run_command "test_faiss_indexflatL2"
  done
  run_command "test_faiss_indexflatL2 2"
  run_command "test_faiss_indexflatL2 2 1 1 1 1"
  run_command "test_faiss_indexflatL2 1 1 3 1 1 1 0 5 3"
  run_command "test_faiss_indexflatL2 1 1 100120 50 30 128 0 3 5"
  run_command "test_faiss_indexflatL2 1 1 1500 20 10 512 1 5 3"
  run_command "test_faiss_indexflatL2 1 1 2000 25 15 756 1 5 5"
  run_command "test_faiss_indexflatL2 1 1 300 30 20 1024 0 3 3"
  run_command "test_faiss_indexPQ"
  run_command "test_faiss_indexPQ 2 2"
  run_command "test_faiss_indexPQ 2 1 1000 1000 1"
  run_command "test_faiss_indexPQ 1 1 1000 100 1000 128"
  run_command "test_faiss_indexPQ 1 1 10000 1875 15 256 8 1 1 5 5 0"
  run_command "test_faiss_indexPQ 1 1 10000 128 25 100 64 1 1 5 3 0"
}

run_dpu(){
  if [ ! -d "dpu_data" ]; then
    echo "Error: Directory 'dpu_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  run_command "test_dpu_sgbm_thread 1 1 512 284 dpu_data/sofa_left_img_512x284.bin dpu_data/sofa_right_img_512x284.bin dpu_data/205pU8Disp_ref_512x284.bin dpu_data/205pU8Disp_ref_512x284_output.bin"
  run_command "test_dpu_sgbm_thread 1 2 1920 1080 dpu_data/pendulum_left_img_1920x1080.bin dpu_data/pendulum_right_img_1920x1080.bin dpu_data/pU8Disp_ref1_1920x1080.bin dpu_data/pU8Disp_ref1_1920x1080_output.bin"
  run_command "test_dpu_sgbm_thread 2 1 804 540 dpu_data/804_left_img.bin dpu_data/804_right_img.bin dpu_data/804_sgbm_u8_median_res.bin dpu_data/804_sgbm_u8_median_res_output.bin"
  run_command "test_dpu_sgbm_thread 2 2 1608 1080 dpu_data/1608_left_img.bin dpu_data/1608_right_img.bin dpu_data/1608_sgbm_u8_median_res.bin dpu_data/1608_sgbm_u8_median_res_output.bin"
  run_command "test_dpu_sgbm_thread 2 2 64 64"
  run_command "test_dpu_sgbm_thread 2 1 64 540"
  run_command "test_dpu_sgbm_thread 1 2 960 64"
  run_command "test_dpu_sgbm_thread 1 1 516 1080"
  run_command "test_dpu_sgbm_thread 1 1 1920 1080"
  run_command "test_dpu_online_thread 1 1 512 284 dpu_data/sofa_left_img_512x284.bin dpu_data/sofa_right_img_512x284.bin dpu_data/fgs_512x284_res.bin dpu_data/fgs_512x284_res_output.bin"
  run_command "test_dpu_online_thread 1 2 1920 1080 dpu_data/pendulum_left_img_1920x1080.bin dpu_data/pendulum_right_img_1920x1080.bin dpu_data/fgs_res_1920x1080.bin dpu_data/fgs_res_1920x1080_output.bin"
  run_command "test_dpu_online_thread 2 2 64 64"
  run_command "test_dpu_online_thread 2 1 64 540"
  run_command "test_dpu_online_thread 1 2 960 64"
  run_command "test_dpu_online_thread 1 1 976 1080"
  run_command "test_dpu_online_thread 1 1 1920 1080"
  run_command "test_dpu_fgs_thread 1 1 512 284 dpu_data/sofa_left_img_512x284.bin dpu_data/205pU8Disp_ref_512x284.bin dpu_data/fgs_512x284_res.bin dpu_data/fgs_512x284_res_output.bin"
  run_command "test_dpu_fgs_thread 1 2 1920 1080 dpu_data/pendulum_left_img_1920x1080.bin dpu_data/pU8Disp_ref1_1920x1080.bin dpu_data/fgs_res_1920x1080.bin dpu_data/fgs_res_1920x1080_output.bin"
  run_command "test_dpu_fgs_thread 2 1 804 540 dpu_data/804_left_img.bin dpu_data/804_sgbm_u8_median_res.bin dpu_data/fgs_804_res.bin dpu_data/fgs_804_res_output.bin"
  run_command "test_dpu_fgs_thread 2 2 1608 1080 dpu_data/1608_left_img.bin dpu_data/1608_sgbm_u8_median_res.bin dpu_data/fgs_1608_res.bin dpu_data/fgs_1608_res_output.bin"
  run_command "test_dpu_fgs_thread 2 2 64 64"
  run_command "test_dpu_fgs_thread 2 1 64 540"
  run_command "test_dpu_fgs_thread 1 2 964 64"
  run_command "test_dpu_fgs_thread 1 1 964 1080"
  run_command "test_dpu_fgs_thread 1 1 1920 1080"
}

run_ive(){
  if [ ! -d "ive_data" ]; then
    echo "Error: Directory 'ive_data' does not exist"
    echo "please prepare test data"
    return 1
  fi
  run_command "test_ive_add_thread 352 288 14 14 19584 45952 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Add.yuv 0 1 1 0"
  run_command "test_ive_add_thread 64 64 14 14 1 65525 ive_data/data_add/00_64x64_u8.bin ive_data/data_add/01_64x64_u8.bin 047c1d188c1a350c74c9a55c5c6beb4b 0 2 2 0"
  run_command "test_ive_add_thread 256 512 14 14 65525 1 ive_data/data_add/00_256x512_u8.bin ive_data/data_add/01_256x512_u8.bin 340a356e7134b1c610bd30616e169ce0 0 2 1 0"
  run_command "test_ive_add_thread 1280 720 14 14 32763 32763 ive_data/data_add/00_1280x720_u8.bin ive_data/data_add/01_1280x720_u8.bin 304b5607f1c64d2dba4693a414d8caa9 0 1 2 0"
  run_command "test_ive_add_thread 1920 1080 14 14 49145 16381 ive_data/data_add/00_1920x1080_u8.bin ive_data/data_add/01_1920x1080_u8.bin 27f0a39f80205583dbda1c152e19205a 0 1 1 0"
  run_command "test_ive_and_thread 352 288 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_And.yuv 0 1 1 0"
  run_command "test_ive_and_thread 64 64 14 14 ive_data/data_add/00_64x64_u8.bin ive_data/data_add/01_64x64_u8.bin 1e8d77c71f8240886ead7242c4c44164 0 2 2 0"
  run_command "test_ive_and_thread 256 512 14 14 ive_data/data_add/00_256x512_u8.bin ive_data/data_add/01_256x512_u8.bin 222d15413407819c2b477ecaa6bfe419 0 1 2 0"
  run_command "test_ive_and_thread 1280 720 14 14 ive_data/data_add/00_1280x720_u8.bin ive_data/data_add/01_1280x720_u8.bin 6d8f96b780a8c0f63ee1ad65fc1a77d6 0 2 1 0"
  run_command "test_ive_and_thread 1920 1080 14 14 ive_data/data_add/00_1920x1080_u8.bin ive_data/data_add/01_1920x1080_u8.bin b6b1860c51bb18205f61f713c71b9f78 0 1 1 0"
  run_command "test_ive_dilate_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Dilate_3x3.yuv 0 1 1 0"
  run_command "test_ive_dilate_thread 640 480 1 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Dilate_5x5.yuv 0 1 1 0"
  run_command "test_ive_dilate_thread 352 288 0 ive_data/bin_352x288_y.yuv ive_data/result/sample_Dilate_3x3_dilate_only.bin 0 1 1 0"
  run_command "test_ive_dilate_thread 352 288 1 ive_data/bin_352x288_y.yuv ive_data/result/sample_Dilate_5x5_dilate_only.bin 0 1 1 0"
  run_command "test_ive_dilate_thread 64 64 0 ive_data/data_add/64x64_binary.raw bc757e7061ef4969404f95d14eaaf4d0 0 2 2 0"
  run_command "test_ive_dilate_thread 1920 540 1 ive_data/data_add/1920x540_binary.raw 149794d1bea7f5a83bcf1eff408fea64 0 2 1 0"
  run_command "test_ive_dilate_thread 960 1080 0 ive_data/data_add/960x1080_binary.raw 65afe67ed2dd904f6c96b2b9b2ac4986 0 1 2 0"
  run_command "test_ive_dilate_thread 1920 1080 1 ive_data/data_add/1920x1080_binary.raw 254c3eb83756002ba5210cf941a96677 0 1 1 0"
  run_command "test_ive_dma_thread 352 288 0 0 0 0 14 14 ive_data/00_352x288_y.yuv ive_data/result/sample_DMA_Direct.bin 0 1 1 0"
  run_command "test_ive_dma_thread 960 1080 0 8 6 60 14 14 ive_data/data_add/960x1080_u8.bin eab2d351dbb9f1479d665dba6f66e43e 0 2 1 0"
  run_command "test_ive_dma_thread 1920 1080 0 16 15 34 14 14 ive_data/data_add/1920x1080_u8.bin 3de54c208d3c084558b7936dbb7b4516 0 1 1 0"
  run_command "test_ive_dma_set_thread 64 64 0 0 14 ive_data/data_add/64x64_u8.bin d04e99a0df6e501c680fdf5f3bd44de8 0 2 2 0"
  run_command "test_ive_dma_set_thread 64 1080 1 0 14 ive_data/data_add/64x1080_u8.bin bdd20c09a9dda09d06a02bc4737749a2 0 2 1 0"
  run_command "test_ive_dma_set_thread 1920 64 0 0 14 ive_data/data_add/1920x64_u8.bin 9dced363f7aab45536db322853f9fe3d  0 1 2 0"
  run_command "test_ive_dma_set_thread 1920 1080 1 0 14 ive_data/data_add/1920x1080_u8.bin 3de54c208d3c084558b7936dbb7b4516 0 1 1 0"
  run_command "test_ive_erode_thread 352 288 0 ive_data/bin_352x288_y.yuv ive_data/result/sample_Erode_3x3.bin.only_erode 0 1 1 0"
  run_command "test_ive_erode_thread 352 288 1 ive_data/bin_352x288_y.yuv ive_data/result/sample_Erode_5x5.bin.only_erode 0 1 1 0"
  run_command "test_ive_erode_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Erode_3x3.yuv 0 1 1 0"
  run_command "test_ive_erode_thread 640 480 1 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Erode_5x5.yuv 0 1 1 0"
  run_command "test_ive_erode_thread 64 64 0 ive_data/data_add/64x64_binary.raw 7db8fbc726e6be564cb2aaa4609252dc 0 2 2 0"
  run_command "test_ive_erode_thread 1920 540 1 ive_data/data_add/1920x540_binary.raw 56d9a1169402f63a418881a10b827069 0 1 2 0"
  run_command "test_ive_erode_thread 960 1080 0 ive_data/data_add/960x1080_binary.raw 252bf30bc236b2edde90e5d672075736 0 2 1 0"
  run_command "test_ive_erode_thread 1920 1080 1 ive_data/data_add/1920x1080_binary.raw e15b7673e54b1f1901a9fc8f3eb06be4 0 1 1 0"
  run_command "test_ive_hist_thread 352 288 ive_data/00_352x288_y.yuv ./hist_res.bin ive_data/result/sample_Hist.bin 0 1 1 0"
  run_command "test_ive_hist_thread 64 64 ive_data/data_add/64x64_u8.bin result/64x64_u8_hist_res.bin fc8cd78d9c99d133b8e65d9957cd5415 0 2 2 0"
  run_command "test_ive_hist_thread 480 512 ive_data/data_add/480x512_u8.bin result/480x512_u8_hist_res.bin 010fbd662a7e631f58accdcf5d4c35e0 0 1 2 0"
  run_command "test_ive_hist_thread 512 256 ive_data/data_add/512x256_u8.bin result/512x256_u8_hist_res.bin aea78406d5453dae372d91f4133f39cc 0 2 1 0"
  run_command "test_ive_hist_thread 1920 1080 ive_data/data_add/1920x1080_u8.bin result/1920x1080_u8_hist_res.bin 27bc755322c783481502d0d1c0190482 0 1 1 0"
  run_command "test_ive_intg_thread 352 288 0 0 ive_data/00_352x288_y.yuv intge_combine.bin ive_data/result/sample_Integ_Combine.yuv 0 1 1"
  run_command "test_ive_intg_thread 352 288 2 1 ive_data/00_352x288_y.yuv integ_sqsum.bin ive_data/result/sample_Integ_Sqsum.yuv 0 1 1"
  run_command "test_ive_intg_thread 352 288 2 1 ive_data/00_352x288_y.yuv integ_sqsum.bin ive_data/result/sample_Integ_Sqsum.yuv 0 1 1"
  run_command "test_ive_intg_thread 32 16 0 0 ive_data/data_add/32x16_u8.bin ive_data/data_add/result/32x16_u32_intg_res.bin 87483909af2926399d2d8a51cd500cbd 0 2 2 0"
  run_command "test_ive_intg_thread 480 512 1 0 ive_data/data_add/480x512_u8.bin ive_data/data_add/result/480x512_u32_intg_res.bin 04d87137d24eabdc496acca9f860319c 0 2 1 0"
  run_command "test_ive_intg_thread 960 540 2 1 ive_data/data_add/960x540_u8.bin ive_data/data_add/result/960x540_u64_intg_res.bin daff64590c6a264e84793b5eed37d2b8 0 1 2 0"
  run_command "test_ive_intg_thread 1920 1080 0 1 ive_data/data_add/1920x1080_u8.bin ive_data/data_add/result/1920x1080_u64_intg_res.bin 48fe598b244e666541e2655d10b73b9c 0 1 1 0"
  run_command "test_ive_lbp_thread 352 288 0 ive_data/00_352x288_y.yuv ive_data/result/sample_LBP_Normal.yuv 0 1 1 0"
  run_command "test_ive_lbp_thread 352 288 1 ive_data/00_352x288_y.yuv ive_data/result/sample_LBP_Abs.yuv 0 1 1 0"
  run_command "test_ive_lbp_thread 64 64 0 ive_data/data_add/64x64_u8.bin 1afa12a02a7891ae56783737fd610c76 0 2 2 0"
  run_command "test_ive_lbp_thread 64 1080 0 ive_data/data_add/64x1080_u8.bin 2b214e4d58ca23ffac5c09cd8adcb259 0 1 2 0"
  run_command "test_ive_lbp_thread 1280 720 0 ive_data/data_add/1280x720_u8.bin 57b9a0869aa34593d95ed6c82938885e 0 2 1 0"
  run_command "test_ive_lbp_thread 1920 1080 0 ive_data/data_add/1920x1080_u8.bin c6a49706a7a31c1d4db904f4646830e8 0 1 1 0"
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
  run_command "test_ive_mag_and_ang_thread 64 64 ive_data/data_add/64x64_u8.bin 0 0 0 8b83d662528f3805ed15990ed613642c null 0 2 2 0"
  run_command "test_ive_mag_and_ang_thread 480 512 ive_data/data_add/480x512_u8.bin 0 0 1 29e8e236af93b125926a7af6f499d9ea 92267bb688e7008d11982f0f1dc74435 0 2 1 0"
  run_command "test_ive_mag_and_ang_thread 960 540 ive_data/data_add/960x540_u8.bin 0 1 0 72169179c9c3e3a885adaa4280f876cd null 0 1 2 0"
  run_command "test_ive_mag_and_ang_thread 1920 1080 ive_data/data_add/1920x1080_u8.bin 0 1 1 94ec03c75c5fa6bd908c05724a28a968 cb0eb79dc771bde42711d0f5fa9e05a6 0 1 1 0"
  run_command "test_ive_map_thread 352 288 0 14 14 ive_data/00_352x288_y.yuv ive_data/result/sample_Map.yuv 0 1 1 0"
  run_command "test_ive_map_thread 64 64 0 14 14 ive_data/data_add/64x64_u8.bin 40e3c1b484a6abe7e3a39010f32ee64f 0 2 2 0"
  run_command "test_ive_map_thread 256 256 1 14 14 ive_data/data_add/256x256_u8.bin 2842fac7914a0ebe9882f0d2ad70385c 0 2 1 0"
  run_command "test_ive_map_thread 368 288 2 14 14 ive_data/data_add/368x288_u8.bin 8ab5d9d80bd4645ad7b1efc3f1d1b2f8 0 1 2 0"
  run_command "test_ive_map_thread 480 512 0 14 14 ive_data/data_add/480x512_u8.bin cae07296e15a256a8a3e1f460c907223 0 1 1 0"
  run_command "test_ive_ncc_thread 352 288 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv dst_bin ive_data/result/sample_NCC_Mem.bin 0 1 1"
  run_command "test_ive_ncc_thread 32 32 ive_data/data_add/00_32x32_u8.bin ive_data/data_add/01_32x32_u8.bin result/32x32_u8_ncc_res.bin 18e0f693e08ff46b4f265a5c6f87986a 0 2 2 0"
  run_command "test_ive_ncc_thread 512 284 ive_data/data_add/00_512x284_u8.bin ive_data/data_add/01_512x284_u8.bin result/512x284_u8_ncc_res.bin 1fe64d6880273f22e89e5fba9f2b61ae 0 2 1 0"
  run_command "test_ive_ncc_thread 1280 720 ive_data/data_add/00_1280x720_u8.bin ive_data/data_add/01_1280x720_u8.bin result/1280x720_u8_ncc_res.bin db1d07cf18d93a65542030056f6d49f4 0 1 2 0"
  run_command "test_ive_ncc_thread 1920 1080 ive_data/data_add/00_1920x1080_u8.bin ive_data/data_add/01_1920x1080_u8.bin result/1920x1080_u8_ncc_res.bin 39fdcfa7b2da0e40afe9a5544d1447e8 0 1 1 0"
  run_command "test_ive_or_thread 352 288 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Or.yuv 0 1 1 0"
  run_command "test_ive_or_thread 64 64 14 14 ive_data/data_add/00_64x64_u8.bin ive_data/data_add/01_64x64_u8.bin 60b6c165cbb1e54a23cebc57f9ad153f 0 2 2 0"
  run_command "test_ive_or_thread 256 512 14 14 ive_data/data_add/00_256x512_u8.bin ive_data/data_add/01_256x512_u8.bin 3ac9062fe518f0e9cf5d851eec400e64 0 1 2 0"
  run_command "test_ive_or_thread 1280 720 14 14 ive_data/data_add/00_1280x720_u8.bin ive_data/data_add/01_1280x720_u8.bin a7812c5d4cba8b42c6d81ff25b7a9468 0 2 1 0"
  run_command "test_ive_or_thread 1920 1080 14 14 ive_data/data_add/00_1920x1080_u8.bin ive_data/data_add/01_1920x1080_u8.bin 6314c983ad49e45ee8c74017a513e460 0 1 1 0"
  run_command "test_ive_ordstatfilter_thread 352 288 0 ive_data/00_352x288_y.yuv ive_data/result/sample_OrdStaFilter_Median.yuv 0 1 1 0"
  run_command "test_ive_ordstatfilter_thread 352 288 1 ive_data/00_352x288_y.yuv ive_data/result/sample_OrdStaFilter_Max.yuv 0 1 1 0"
  run_command "test_ive_ordstatfilter_thread 352 288 2 ive_data/00_352x288_y.yuv ive_data/result/sample_OrdStaFilter_Min.yuv 0 1 1 0"
  run_command "test_ive_ordstatfilter_thread 64 64 0 ive_data/data_add/64x64_u8.bin f195f914c97d26ac38b8be0015d4dc33 0 2 2 0"
  run_command "test_ive_ordstatfilter_thread 960 540 1 ive_data/data_add/960x540_u8.bin 5488362f5ef2dfed4ca6bd56a56addbb 0 1 2 0"
  run_command "test_ive_ordstatfilter_thread 1280 720 2 ive_data/data_add/1280x720_u8.bin d2241f3c924e5a40e53fa7b80e47c036 0 2 1 0"
  run_command "test_ive_ordstatfilter_thread 1920 1080 1 ive_data/data_add/1920x1080_u8.bin 38c311094bdac9215b88861b6a82f9aa 0 1 1 0"
  run_command "test_ive_sub_thread 352 288 0 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Sub_Abs.yuv 0 1 1 0"
  run_command "test_ive_sub_thread 352 288 1 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Sub_Shift.yuv 0 1 1 0"
  run_command "test_ive_sub_thread 64 64 0 14 14 ive_data/data_add/00_64x64_u8.bin ive_data/data_add/01_64x64_u8.bin 4a7a009f257fb0c9855ab492a5999095 0 2 2 0"
  run_command "test_ive_sub_thread 480 512 1 14 14 ive_data/data_add/00_480x512_u8.bin ive_data/data_add/01_480x512_u8.bin 8c73f17e4a2b90d5214c4565166bdfc3 0 2 1 0"
  run_command "test_ive_sub_thread 1280 720 0 14 14 ive_data/data_add/00_1280x720_u8.bin ive_data/data_add/01_1280x720_u8.bin 7d158ed5ab82b038920fa1fe984473d9 0 1 2 0"
  run_command "test_ive_sub_thread 1920 1080 1 14 14 ive_data/data_add/00_1920x1080_u8.bin ive_data/data_add/01_1920x1080_u8.bin 13bd9f48639960ce3a31b5fc90aa0643 0 1 1 0"
  run_command "test_ive_thresh_s16_thread 352 288 14 14 8 41 105 -63 -5 -98 ive_data/00_352x288_s8_to_s16_reverse.yuv ive_data/result/sample_Thresh_S16_To_S8_MinMidMax_352x288.yuv 0 1 1 0"
  run_command "test_ive_thresh_s16_thread 64 64 14 14 8 -32768 32767 -128 0 127 ive_data/data_add/64x64_s16.s16 620f0b67a91f7f74151bc5be745b7110 0 2 2 0"
  run_command "test_ive_thresh_s16_thread 1280 720 14 14 16 -32768 32767 0 125 255 ive_data/data_add/1280x720_s16.s16 8a583ac5b931b18c3e6af5d16cd7c1f3 0 2 1 0"
  run_command "test_ive_thresh_s16_thread 1920 1080 14 14 17 -1 255 75 125 200 ive_data/data_add/1920x1080_s16.s16 b92967ea0ae7d96af1891dbae502f03c 0 1 2 0"
  run_command "test_ive_thresh_thread 352 288 14 14 3 236 249 166 219 60 ive_data/00_352x288_y.yuv ive_data/result/sample_Thresh_MinMidMax.yuv 0 1 1 0"
  run_command "test_ive_thresh_thread 64 64 14 14 0 0 null 0 null 255 ive_data/data_add/64x64_u8.bin 6ae59e64850377ee5470c854761551ea 0 2 2 0"
  run_command "test_ive_thresh_thread 64 64 14 14 1 25 null null null 225 ive_data/data_add/64x64_u8.bin a177f3a9c512b89e4f82d089172d6461 0 1 2 0"
  run_command "test_ive_thresh_thread 480 512 14 14 2 50 null 25 null null ive_data/data_add/480x512_u8.bin 29eaeba86bcc992b8070a1c2c1e14989 0 2 1 0"
  run_command "test_ive_thresh_thread 480 512 14 14 3 75 255 0 125 200 ive_data/data_add/480x512_u8.bin 796f19256f7b56878779cb0acb5857de 0 1 2 0"
  run_command "test_ive_thresh_thread 1280 720 14 14 4 100 200 null 50 175 ive_data/data_add/1280x720_u8.bin b7ae772eef354dfcf125729fc6ce1ed7 0 2 1 0"
  run_command "test_ive_thresh_thread 1280 720 14 14 5 125 225 75 175 null ive_data/data_add/1280x720_u8.bin 202507e959a72222d2ffd443b2344962 0 1 1 0"
  run_command "test_ive_thresh_thread 1920 1080 14 14 6 150 200 100 null 150 ive_data/data_add/1920x1080_u8.bin adf172359091340784a5b447e673a2e8 0 1 1 0"
  run_command "test_ive_thresh_thread 1920 1080 14 14 7 175 175 null 255 null ive_data/data_add/1920x1080_u8.bin 3de54c208d3c084558b7936dbb7b4516 0 1 1 0"
  run_command "test_ive_thresh_u16_thread 352 288 14 14 18 41 105 190 132 225 ive_data/00_352x288_u8_to_u16_reverse.yuv ive_data/result/sample_Thresh_U16_To_U8_MinMidMax_352x288.yuv 0 1 1 0"
  run_command "test_ive_thresh_u16_thread 64 64 14 14 18 16383 32767 0 175 255 ive_data/data_add/64x64_u16.u16 dafbb17b2aad2cf7b20015fdda6ffa5c 0 2 1 0"
  run_command "test_ive_thresh_u16_thread 480 512 14 14 19 0 255 0 null 255 ive_data/data_add/480x512_u16.u16 b17461e776ddd0007197b063bc4796ac 0 1 2 0"
  run_command "test_ive_thresh_u16_thread 1280 720 14 14 19 175 255 50 null 150 ive_data/data_add/1280x720_u16.u16 d319dea362256fe4dd2a1b7a35aac343 0 2 1 0"
  run_command "test_ive_thresh_u16_thread 1920 1080 14 14 18 0 65535 50 150 200 ive_data/data_add/1920x1080_u16.u16 35679e8cab549438d0b813a75291f6f0 0 1 1 0"
  run_command "test_ive_xor_thread 352 288 14 14 ive_data/00_352x288_y.yuv ive_data/01_352x288_y.yuv ive_data/result/sample_Xor.yuv 0 1 1 0"
  run_command "test_ive_xor_thread 64 64 14 14 ive_data/data_add/00_64x64_u8.bin ive_data/data_add/01_64x64_u8.bin 21a7a5ad2c4b1ffd9443d5952a92f4c9 0 2 2 0"
  run_command "test_ive_xor_thread 480 512 14 14 ive_data/data_add/00_480x512_u8.bin ive_data/data_add/01_480x512_u8.bin 766b3d9560315456f7470cc500313c57 0 1 2 0"
  run_command "test_ive_xor_thread 1280 720 14 14 ive_data/data_add/00_1280x720_u8.bin ive_data/data_add/01_1280x720_u8.bin d9937bfeb33bb09e0c32b7556831456e 0 2 1 0"
  run_command "test_ive_xor_thread 1920 1080 14 14 ive_data/data_add/00_1920x1080_u8.bin ive_data/data_add/01_1920x1080_u8.bin d1e0a685e56c1c8b963548e8797add37 0 1 1 0"
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
  run_command "test_ive_sobel_thread 16 16 ive_data/data_add/16x16_u8.bin 0 1 141193893d69fd317fc2f62a3193b1a8 null 0 2 2 0"
  run_command "test_ive_sobel_thread 480 512 ive_data/data_add/480x512_u8.bin 1 0 7ee2b201484344f7b438e2ebe400d71c db1174788c4c8923361187580ac874a4 0 2 1 0"
  run_command "test_ive_sobel_thread 960 540 ive_data/data_add/960x540_u8.bin 1 2 null f38e5175def1f2bf811eda464741496b 0 1 2 0"
  run_command "test_ive_sobel_thread 1920 1080 ive_data/data_add/1920x1080_u8.bin 0 0 fd125b20ca5e8da912e2dfa01bdad80f 45c7cbca381949ef2130d219b483a537 0 1 1 0"
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
  run_command "test_ive_normgrad_thread 64 64 ive_data/data_add/64x64_u8.bin 0 0 500a629d01c17c0937cc2e9877ad7489 7cf62fee9454fbea1f1b453a9e00ecc5 null 0 2 2 0"
  run_command "test_ive_normgrad_thread 256 512 ive_data/data_add/256x512_u8.bin 1 1 c9f203b70392ce16245d67270786e6eb null null 0 1 2 0"
  run_command "test_ive_normgrad_thread 960 1080 ive_data/data_add/960x1080_u8.bin 0 2 null dbb6f30dc22719435e15f2db50b594ab null 0 2 1 0"
  run_command "test_ive_normgrad_thread 1920 1080 ive_data/data_add/1920x1080_u8.bin 1 3 null null a736d131cbb012a5d92ac3907f947bf8 0 1 1 0"
  run_command "test_ive_canny_thread 640 480 ive_data/sky_640x480.yuv 1 ive_data/result/sample_tile_CannyEdge_5x5.yuv 0 1 1 0 out/tile_dst5x5_edge.bin"
  run_command "test_ive_canny_thread 640 480 ive_data/sky_640x480.yuv 0 ive_data/result/sample_tile_CannyEdge_3x3.yuv"
  run_command "test_ive_canny_thread 352 288 ive_data/00_352x288_y.yuv 1 ive_data/result/sample_CannyEdge_5x5.yuv 0 1 1"
  run_command "test_ive_canny_thread 352 288 ive_data/00_352x288_y.yuv 0 ive_data/result/sample_CannyEdge_3x3.yuv"
  run_command "test_ive_canny_thread 64 64 ive_data/data_add/64x64_u8.bin 0 7899de5d29bde9934ee644adad20877c 0 2 2 0"
  run_command "test_ive_canny_thread 1920 64 ive_data/data_add/1920x64_u8.bin 1 e6941ccc1a389f29545d698f8d275e93 0 1 2 0"
  run_command "test_ive_canny_thread 960 540 ive_data/data_add/960x540_u8.bin 0 ccedca2336b9b72825d77c8cd56fd2d0 0 2 1 0"
  run_command "test_ive_canny_thread 1920 1080 ive_data/data_add/1920x1080_u8.bin 1 282143eacb58745edbeb0abddb6ea3c0 0 1 1 0"
  run_command "test_ive_gmm_thread 352 288 14 ive_data/campus.u8c1.1_100.raw ive_data/result/sample_GMM_U8C1_fg_31.yuv ive_data/result/sample_GMM_U8C1_bg_31.yuv"
  run_command "test_ive_gmm_thread 64 64 14 ive_data/data_add/64x64_u8_50.raw 620f0b67a91f7f74151bc5be745b7110 7482e85eb68cd6b8197f648646d6275b 0 2 2 0"
  run_command "test_ive_gmm_thread 256 512 10 ive_data/data_add/256x512_prgb_75.raw d6fc250f58c3c17758ce09480f035e56 34588542702c20020b47662ce376c28a 0 1 2 0"
  run_command "test_ive_gmm2_thread 352 288 14 0 0 ive_data/campus.u8c1.1_100.raw null ive_data/result/sample_GMM2_U8C1_fg_31.yuv ive_data/result/sample_GMM2_U8C1_bg_31.yuv"
  run_command "test_ive_gmm2_thread 352 288 14 1 0 ive_data/campus.u8c1.1_100.raw ive_data/sample_GMM2_U8C1_PixelCtrl_Factor.raw ive_data/result/sample_GMM2_U8C1_PixelCtrl_fg_31.yuv ive_data/result/sample_GMM2_U8C1_PixelCtrl_bg_31.yuv ive_data/result/sample_GMM2_U8C1_PixelCtrl_match_31.yuv"
  run_command "test_ive_gmm2_thread 64 64 14 0 0 ive_data/data_add/64x64_u8_50.raw null 620f0b67a91f7f74151bc5be745b7110 0822c20bf65eb754d679139698907fff null 0 2 2 0"
  run_command "test_ive_gmm2_thread 256 512 10 1 0 ive_data/data_add/256x512_prgb_75.raw ive_data/data_add/256x512_factor_u16.bin 75b3752be50789e5b8e7b112e4aba45c fd7f5505a66c8e5f7af7f10cb8d415c5 97aaaf362a24705038c5de79a29e74ef 0 2 1 0"
  run_command "test_ive_filter_thread 352 288 14 ive_data/00_352x288_y.yuv 0 4 ive_data/result/sample_Filter_Y3x3.yuv"
  run_command "test_ive_filter_thread 352 288 14 ive_data/00_352x288_y.yuv 1 7 ive_data/result/sample_Filter_Y5x5.yuv"
  run_command "test_ive_filter_thread 256 512 14 ive_data/data_add/256x512_u8.bin 0 5 6cbe6e6b7e69dd1cbf2fcfb87127ee15 0 1 2 0"
  run_command "test_ive_filter_thread 1280 720 6 ive_data/data_add/1280x720.nv61 1 13 7abd86d632ef3a05ed01f6b3308e97b6 0 2 1 0"
  run_command "test_ive_filter_thread 1920 1080 14 ive_data/data_add/1920x1080_u8.bin 1 7 31ad7138b319c1e0aff0df89785c226c 0 1 1 0"
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
  run_command "test_ive_csc_thread 64 64 4 0 8 ive_data/data_add/64x64.nv21 d985e4bf225dd4634fe4a34d0f29c8f6 0 2 2 0"
  run_command "test_ive_csc_thread 256 512 12 5 4 ive_data/data_add/256x512_s.rgb 3709ded3b5a4f295b091f03804b215a2 0 1 2 0"
  run_command "test_ive_resize_thread 1 ive_data/result/sample_Resize_Bilinear_rgb.rgb ive_data/result/sample_Resize_Bilinear_gray.yuv ive_data/result/sample_Resize_Bilinear_240p.rgb"
  run_command "test_ive_resize_thread 3 ive_data/result/sample_Resize_Area_rgb.rgb ive_data/result/sample_Resize_Area_gray.yuv ive_data/result/sample_Resize_Area_240p.rgb"
  run_command "test_ive_stcandicorner_thread 352 288 25 ive_data/penguin_352x288.gray.shitomasi.raw ive_data/result/sample_Shitomasi_CandiCorner.yuv 0 1 1"
  run_command "test_ive_stcandicorner_thread 640 480 25 ive_data/sky_640x480.yuv ive_data/result/sample_tile_Shitomasi_sky_640x480.yuv 0 1 1"
  run_command "test_ive_stcandicorner_thread 64 64 255 ive_data/data_add/64x64_u8.bin 620f0b67a91f7f74151bc5be745b7110 0 2 2 0"
  run_command "test_ive_stcandicorner_thread 480 512 1 ive_data/data_add/480x512_u8.bin aae1fe17f54f6fa051e4f7071fc8bb20 0 1 2 0"
  run_command "test_ive_stcandicorner_thread 960 540 64 ive_data/data_add/960x540_u8.bin 28ab4055de434c9976be2b5237860cce 0 2 1 0"
  run_command "test_ive_stcandicorner_thread 1920 1080 191 ive_data/data_add/1920x1080_u8.bin f8681c0a960e747db1450e1f3b84ed05 0 1 1 0"
  run_command "test_ive_gradfg_thread 352 288 0 ive_data/00_352x288_y.yuv ive_data/result/sample_GradFg_USE_CUR_GRAD.out"
  run_command "test_ive_gradfg_thread 352 288 1 ive_data/00_352x288_y.yuv ive_data/result/sample_GradFg_FIND_MIN_GRAD.out"
  run_command "test_ive_gradfg_thread 640 480 0 ive_data/sky_640x480.yuv ive_data/result/sample_tile_GradFg_USE_CUR_GRAD.out"
  run_command "test_ive_gradfg_thread 640 480 1 ive_data/sky_640x480.yuv ive_data/result/sample_tile_GradFg_FIND_MIN_GRAD.out"
  run_command "test_ive_sad_thread 352 288 1 0 0 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode1_out0.bin ive_data/result/sample_Sad_thr_mode1_out0.bin 0 1 1 0"
  run_command "test_ive_sad_thread 352 288 2 0 0 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode2_out0.bin ive_data/result/sample_Sad_thr_mode2_out0.bin 0 1 1 0"
  run_command "test_ive_sad_thread 352 288 0 0 0 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode0_out0.bin ive_data/result/sample_Sad_thr_mode0_out0.bin 0 1 1 0"
  run_command "test_ive_sad_thread 352 288 0 0 1 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode0_out1.bin ive_data/result/sample_Sad_thr_mode0_out1.bin 0 1 1 0"
  run_command "test_ive_sad_thread 352 288 1 0 1 2048 2 30 ive_data/00_352x288_y.yuv ive_data/bin_352x288_y.yuv ive_data/result/sample_Sad_sad_mode1_out1.bin ive_data/result/sample_Sad_thr_mode1_out1.bin 0 1 1 0"
  run_command "test_ive_sad_thread 64 64 0 1 1 0 0 255 ive_data/data_add/00_64x64_u8.bin ive_data/data_add/01_64x64_u8.bin 78cade9af6465eff4861f78e735a15f0 null 0 2 2 0 "
  run_command "test_ive_sad_thread 480 512 1 1 0 125 125 125 ive_data/data_add/00_480x512_u8.bin ive_data/data_add/01_480x512_u8.bin c78f1666d61b4a3b18e5577235cabc3a null 0 2 1 0 "
  run_command "test_ive_sad_thread 1280 720 2 0 1 37767 125 255 ive_data/data_add/00_1280x720_u8.bin ive_data/data_add/01_1280x720_u8.bin dc9ccf15070ba6265e9b1c69a92224ad f90141c2eb840eeafbfa7387a27e7faa 0 1 2 0 "
  run_command "test_ive_sad_thread 1920 1080 2 0 0 65535 0 125 ive_data/data_add/00_1920x1080_u8.bin ive_data/data_add/01_1920x1080_u8.bin 497459e54b2792dbdaf2ace80bba12a8 19afb1a10078e901994a1e1e2c058652 0 1 1 0 "
  run_command "test_ive_ccl_thread 720 576 1 4 2 ive_data/ccl_raw_1.raw ive_data/result/sample_CCL_1.bin 0 1 1 0"
  run_command "test_ive_ccl_thread 1280 720 0 4 2 ive_data/ccl_raw_0.raw ive_data/result/sample_CCL_0.bin 0 1 1 0"
  run_command "test_ive_ccl_thread 64 64 0 0 65535 ive_data/data_add/64x64_binary.raw 72fc4fdc9a4cb15c71476ea2056f1608 0 2 2 0"
  run_command "test_ive_ccl_thread 64 1080 1 65535 1 ive_data/data_add/64x1080_binary.raw 7575350fc2a5ff450dbc5d5ff7ac7132 0 1 2 0"
  run_command "test_ive_ccl_thread 1280 720 0 32768 32767 ive_data/data_add/1280x720_binary.raw fb63055e6347105ccd87cf817307e182 0 2 1 0"
  run_command "test_ive_ccl_thread 1920 1080 1 4 2 ive_data/data_add/1920x1080_binary.raw 4d9060fc9431913c61fbcd9741cab767 0 1 1 0"
  run_command "test_ive_bgmodel_thread 352 288 ive_data/campus.u8c1.1_100.raw ive_data/result/sample_BgModelSample2_BgMdl_100.bin 0 1 1"
  run_command "test_ive_bgmodel_thread 64 64 ive_data/data_add/64x64_u8_50.raw 0b1733356892d6485bd122d42d8ccbaf 0 2 2 0 null 50"
  run_command "test_ive_bgmodel_thread 960 540 ive_data/data_add/960x540_u8_75.raw 7b1379e8a2e462a49496a77fac9d0668 0 2 1 0 null 75"
  run_command "test_ive_bgmodel_thread 1280 720 ive_data/data_add/1280x720_u8_150.raw 0d8c569b46b14d2a4fbef62c33dd43cb 0 1 2 0 null 150"
  run_command "test_ive_bgmodel_thread 1920 1080 ive_data/data_add/1920x1080_u8_200.raw 0524a67e25b0b9387bbddf2e1ed521e8 0 1 1 0 null 200"
  run_command "test_ive_bernsen_thread 352 288 0 5 ive_data/00_352x288_y.yuv ive_data/result/sample_Bernsen_5x5.yuv 0 1 1 0"
  run_command "test_ive_bernsen_thread 352 288 1 3 ive_data/00_352x288_y.yuv ive_data/result/sample_Bernsen_3x3_Thresh.yuv"
  run_command "test_ive_bernsen_thread 352 288 2 5 ive_data/00_352x288_y.yuv ive_data/result/sample_Bernsen_5x5_Paper.yuv"
  run_command "test_ive_bernsen_thread 16 16 0 5 ive_data/data_add/16x16_u8.bin 77e36bfc2978bbc60334bbea6c8c5631 0 2 2 0"
  run_command "test_ive_bernsen_thread 512 16 1 3 ive_data/data_add/512x16_u8.bin 2e9355620a5a88f06dbbc4f99da5c4a8 0 1 2 0"
  run_command "test_ive_bernsen_thread 960 540 2 3 ive_data/data_add/960x540_u8.bin f2c8e7392f2943e29c2136ea30909832 0 2 1 0"
  run_command "test_ive_bernsen_thread 1920 1080 2 5 ive_data/data_add/1920x1080_u8.bin 23ba18d7d4e3a1bb53273f0dad1e4562 0 1 1 0"
  run_command "test_ive_filterandcsc_thread 352 288 1 0 4 4 8 ive_data/00_352x288_SP420.yuv ive_data/result/sample_FilterAndCSC_420SPToVideoPlanar3x3.yuv"
  run_command "test_ive_filterandcsc_thread 352 288 1 1 7 4 8 ive_data/00_352x288_SP420.yuv ive_data/result/sample_FilterAndCSC_420SPToVideoPlanar5x5.yuv"
  run_command "test_ive_filterandcsc_thread 64 64 0 0 0 4 10 ive_data/data_add/64x64.nv21 775aab02f89965208d1bc4d2f7297b96 0 2 2 0"
  run_command "test_ive_filterandcsc_thread 960 540 0 1 5 6 8 ive_data/data_add/960x540.nv61 0e119a018d05b60a2721089860617b77 0 1 2 0"
  run_command "test_ive_filterandcsc_thread 1280 720 3 0 13 6 8 ive_data/data_add/1280x720.nv61 14ccef94b471a701abe62a728a39add4 0 2 1 0"
  run_command "test_ive_filterandcsc_thread 1920 1080 3 1 7 4 10 ive_data/data_add/1920x1080.nv21 db3ff8f2b61b7cdc00dbe71f710d5c9e 0 1 1 0"
  run_command "test_ive_16bitto8bit_thread 352 288 0 41 18508 0 6 2 ive_data/00_704x576.s16 ive_data/result/sample_16BitTo8Bit_S16ToS8.yuv"
  run_command "test_ive_16bitto8bit_thread 352 288 1 190 26690 0 6 1 ive_data/00_704x576.s16 ive_data/result/sample_16BitTo8Bit_Abs.yuv"
  run_command "test_ive_16bitto8bit_thread 352 288 2 225 15949 -42 6 1 ive_data/00_704x576.s16 ive_data/result/sample_16BitTo8Bit_Shift.yuv"
  run_command "test_ive_16bitto8bit_thread 352 288 3 174 27136 0 5 1 ive_data/00_704x576.u16 ive_data/result/sample_16BitTo8Bit_U16ToU8.yuv"
  run_command "test_ive_16bitto8bit_thread 16 16 0 1 1 -128 6 2 ive_data/data_add/16x16_s16.s16 348a9791dc41b89796ec3808b5b5262f 0 2 2 0"
  run_command "test_ive_16bitto8bit_thread 512 256 1 255 65535 0 6 1 ive_data/data_add/512x256_s16.s16 300289d8b5026883f9f81b9b4f14f12c 0 2 1 0"
  run_command "test_ive_16bitto8bit_thread 960 540 2 125 37767 127 6 1 ive_data/data_add/960x540_s16.s16 06515ed3ee0bc8b58eb5c02af086a3b3 0 1 2 0"
  run_command "test_ive_16bitto8bit_thread 1920 1080 3 75 49151 0 5 1 ive_data/data_add/1920x1080_u16.u16 a60f124cb2c20f1e5968f8159da60656 0 1 1 0"
  run_command "test_ive_framediffmotion_thread 480 480 ive_data/md1_480x480.yuv ive_data/md2_480x480.yuv ive_data/result/sample_FrameDiffMotion.yuv"
  run_command "test_ive_framediffmotion_thread 960 480 ive_data/input1_960x480.yuv ive_data/input2_960x480.yuv ive_data/result/sample_tile_FrameDiffMotion.yuv"
  run_command "test_ive_framediffmotion_thread 64 64 ive_data/data_add/00_64x64_u8.bin ive_data/data_add/01_64x64_u8.bin 20c9cd747b94cd34b4a29913de0b274c 0 2 2 0"
  run_command "test_ive_framediffmotion_thread 512 284 ive_data/data_add/00_512x284_u8.bin ive_data/data_add/01_512x284_u8.bin 43dbc25b00374d46e1fa750f759e0d49 0 1 2 0"
  run_command "test_ive_framediffmotion_thread 960 540 ive_data/data_add/00_960x540_u8.bin ive_data/data_add/01_960x540_u8.bin aa14155c40ebd557e573276e1b789564 0 2 1 0"
  run_command "test_ive_framediffmotion_thread 1920 1080 ive_data/data_add/00_1920x1080_u8.bin ive_data/data_add/01_1920x1080_u8.bin 53e9a3e78883d8b1d942c6911cd3caec 0 1 1 0"
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

run_all(){
  run_vpss
  run_tpu
  run_dpu
  run_ldc
  run_dwa
  run_blend
  run_ive
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