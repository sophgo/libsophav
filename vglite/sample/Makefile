SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../Makefile.param
	include $(PARAM_FILE)
endif
include ../sample.mk
export SDK_DIR = $(PWD)
export LDFLAGS = -L$(MW_PATH)/lib/3rd
export GPU_CHIP_ID_GCNanoUltraV = 1

.phony: all clean

all: test_util test_cases

test_util:
	@$(MAKE) -C samples/util

test_cases: test_util
	@$(MAKE) -C samples/character
	@$(MAKE) -C samples/clear
	@$(MAKE) -C samples/clear_params
	@$(MAKE) -C samples/clock
	@$(MAKE) -C samples/rotate
	@$(MAKE) -C samples/rotate_params
	@$(MAKE) -C samples/vector
	@$(MAKE) -C samples/vector_upload
	@$(MAKE) -C samples/sft_255
	@$(MAKE) -C samples/multi_draw
	@$(MAKE) -C samples/tiger
	@$(MAKE) -C samples/ui
	@$(MAKE) -C samples/tiled
	@$(MAKE) -C samples/linearGrad
	@$(MAKE) -C samples/radialGrad
	@$(MAKE) -C samples/patternFill
	@$(MAKE) -C samples/glyphs2
	@$(MAKE) -C samples/unit_test
	@$(MAKE) -C samples/unit_test1
	@$(MAKE) -C samples/imgA4
	@$(MAKE) -C samples/imgA8
	@$(MAKE) -C samples/imgIndex
	@$(MAKE) -C samples/parking
	@$(MAKE) -C samples/global_alpha
	@$(MAKE) -C samples/rgba8_etc2
	@$(MAKE) -C samples/colorkey
#	@$(MAKE) -C samples/gfx
#	@$(MAKE) -C samples/lossy
#	@$(MAKE) -C samples/double_image
	@$(MAKE) -C samples/format24bit
	@$(MAKE) -C samples/ARGB2222
	@$(MAKE) -C samples/dither
	@$(MAKE) -C samples/yuv_input
	@$(MAKE) -C samples/arc_path
	@$(MAKE) -C samples/decnano_compress
	@$(MAKE) -C samples/stroke
	@$(MAKE) -C samples/linearGradient
	@$(MAKE) -C samples/mask
	@$(MAKE) -C samples/scissor
	@$(MAKE) -C samples/mirror
	@$(MAKE) -C samples/gamma
	@$(MAKE) -C samples/new_blend_mode
	@$(MAKE) -C samples/color_transformation
	@$(MAKE) -C samples/premultiply
	@$(MAKE) -C samples/24bit_planar
	@$(MAKE) -C samples/imgIndex_endian
	@$(MAKE) -C samples/recolor
	@$(MAKE) -C samples/blend_lvgl
	@$(MAKE) -C samples/pixel_matrix
	@$(MAKE) -C samples/gaussian_blur3x3
	@$(MAKE) -C samples/stencil
	@$(MAKE) -C samples/16pixels_align
	@$(MAKE) -C samples/slt

clean:
	@$(MAKE) -C samples/util clean
	@$(MAKE) -C samples/character clean
	@$(MAKE) -C samples/clear clean
	@$(MAKE) -C samples/clear_params clean
	@$(MAKE) -C samples/clock clean
	@$(MAKE) -C samples/rotate clean
	@$(MAKE) -C samples/rotate_params clean
	@$(MAKE) -C samples/vector clean
	@$(MAKE) -C samples/vector_upload clean
	@$(MAKE) -C samples/sft_255 clean
	@$(MAKE) -C samples/multi_draw clean
	@$(MAKE) -C samples/tiger clean
	@$(MAKE) -C samples/ui clean
	@$(MAKE) -C samples/tiled clean
	@$(MAKE) -C samples/linearGrad clean
	@$(MAKE) -C samples/patternFill clean
	@$(MAKE) -C samples/glyphs2 clean
	@$(MAKE) -C samples/unit_test clean
	@$(MAKE) -C samples/unit_test1 clean
	@$(MAKE) -C samples/imgA4 clean
	@$(MAKE) -C samples/imgA8 clean
	@$(MAKE) -C samples/imgIndex clean
	@$(MAKE) -C samples/parking clean
	@$(MAKE) -C samples/global_alpha clean
	@$(MAKE) -C samples/rgba8_etc2 clean
	@$(MAKE) -C samples/colorkey clean
#	@$(MAKE) -C samples/gfx clean
#	@$(MAKE) -C samples/lossy clean
#	@$(MAKE) -C samples/double_image clean
	@$(MAKE) -C samples/format24bit clean
	@$(MAKE) -C samples/ARGB2222 clean
	@$(MAKE) -C samples/dither clean
	@$(MAKE) -C samples/yuv_input clean
	@$(MAKE) -C samples/arc_path clean
	@$(MAKE) -C samples/decnano_compress clean
	@$(MAKE) -C samples/stroke clean
	@$(MAKE) -C samples/linearGradient clean
	@$(MAKE) -C samples/mask clean
	@$(MAKE) -C samples/scissor clean
	@$(MAKE) -C samples/mirror clean
	@$(MAKE) -C samples/gamma clean
	@$(MAKE) -C samples/new_blend_mode clean
	@$(MAKE) -C samples/color_transformation clean
	@$(MAKE) -C samples/premultiply clean
	@$(MAKE) -C samples/24bit_planar clean
	@$(MAKE) -C samples/imgIndex_endian clean
	@$(MAKE) -C samples/recolor clean
	@$(MAKE) -C samples/blend_lvgl clean
	@$(MAKE) -C samples/pixel_matrix clean
	@$(MAKE) -C samples/gaussian_blur3x3 clean
	@$(MAKE) -C samples/stencil clean
	@$(MAKE) -C samples/16pixels_align clean
	@$(MAKE) -C samples/slt clean
