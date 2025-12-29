##############################################################################
#
#    Copyright (c) 2016-2022 by Bitmain Technologies Inc. All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Bitmain Technologies Inc. This is proprietary information owned by
#    Bitmain Technologies Inc. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Bitmain Technologies Inc.
#
##############################################################################

SHELL           := /bin/bash
CHIP            ?= bm1688
PRODUCTFORM     ?= soc
TOOLCHAIN_PATH  ?= $(shell dirname $(shell which aarch64-linux-gnu-gcc))
DEBUG           ?= 1
# LIBSOPHAV_ROOT   ?= $(SG_MEDIA_TOP_DIR)
# OUT_DIR         ?= $(SG_OUTPUT_DIR)
LIBSOPHAV_ROOT    = ..
OUT_DIR          = ../bmcv/release
JPU_DIR			 = ../jpeg/install/$(PRODUCTFORM)

ifneq ($(PRODUCTFORM), soc)
	PCIE_MODE_ENABLE_CPU := 1
	SOC_MODE := 0
else
	PCIE_MODE_ENABLE_CPU := 0
	SOC_MODE := 1
endif

ifeq ($(PRODUCTFORM), soc)
	CROSS_COMPILE = $(TOOLCHAIN_PATH)/aarch64-linux-gnu-
else
	CROSS_COMPILE = $(TOOLCHAIN_PATH)/riscv64-unknown-linux-gnu-
endif

CC = $(CROSS_COMPILE)gcc
CXX= $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar

CFLAGS      = -Wall -Werror -Wno-error=deprecated-declarations $(CONFIG_FLAGS)\
 			  -ffunction-sections -fdata-sections \
 			  -fPIC -Wno-unused-function \
 			  -funwind-tables -fno-short-enums \
			  -ffp-contract=off

LDFLAGS     += -pthread \
               -Wl, --check--secrets \
		       -Wl, --gc--secrets

DYN_LDFLAGS += -L$(LIBSOPHAV_ROOT)/3rdparty/libbmcv/lib/$(PRODUCTFORM) \
			   -L$(JPU_DIR)/lib \
			   -ldl -lm -lbmlib -lbmjpeg \

ifeq ($(PRODUCTFORM), soc)
	CFLAGS   += -fsigned-char
	CXXFLAGS += -fsigned-char
else
	CFLAGS   +=
	CXXFLAGS +=
endif

ifeq ($(DEBUG), 1)
	CFLAGS   += -g -DDEBUG
	CXXFLAGS += -g -DEBUG
	LDFLAGS  += -rdynamic
else
    CFLAGS   += -O3
	CXXFLAGS += -O3
endif

ifeq ($(PCIE_MODE_ENABLE_CPU), 1)
	CFLAGS   += -DPCIE_MODE_ENABLE_CPU
	CXXFLAGS += -DPCIE_MODE_ENABLE_CPU
endif


define SOURCES_TO_OBJS
    $(patsubst %.$(2),%.o,$(filter %.$(2),$(1)))
endef

BMCV_SRC_DIR     = $(LIBSOPHAV_ROOT)/bmcv
BMCV_SOURCE    = $(wildcard $(BMCV_SRC_DIR)/src/*.c $(BMCV_SRC_DIR)/src/bm1688/*.c $(BMCV_SRC_DIR)/src/cmodel/*.c)
BMCV_CC_OBJS1  = $(call SOURCES_TO_OBJS, $(BMCV_SOURCE),c)
BMCV_CC_OBJS2  = $(patsubst $(BMCV_SRC_DIR)/%.o,%.o, $(BMCV_CC_OBJS1))
BMCV_CC_OBJS   = $(addprefix $(BMCV_SRC_DIR)/obj/,$(BMCV_CC_OBJS2))
BMCV_OBJ_DIR   = $(sort $(dir $(BMCV_CC_OBJS)))

BMCV_STATIC      =  $(OUT_DIR)/$(CHIP)/lib/libbmcv.a
BMCV_DYN         =  $(OUT_DIR)/$(CHIP)/lib/libbmcv.so
BMCV_DYN_NAME    =  libbmcv.so
INSTALL_LIBS    +=  $(BMCV_DYN)
RELEASE_LIBS    +=  $(BMCV_DYN)
RELEASE_HEADERS +=  $(BMCV_SRC_DIR)/include/bmcv_api_ext.h

INCLUDE_DIR  	+= -I$(LIBSOPHAV_ROOT)/3rdparty/libbmcv/include
INCLUDE_DIR  	+= -I$(LIBSOPHAV_ROOT)/3rdparty/osdrv
INCLUDE_DIR  	+= -I$(BMCV_SRC_DIR)/include
INCLUDE_DIR  	+= -I$(BMCV_SRC_DIR)/src/bm1688
INCLUDE_DIR  	+= -I$(BMCV_SRC_DIR)/src/cmodel
INCLUDE_DIR		+= -I$(LIBSOPHAV_ROOT)/3rdparty/libjpeg-turbo/include
INCLUDE_DIR		+= -I$(JPU_DIR)/include

ifeq ($(PRODUCTFORM), soc)
	LIB_TURBO_JPEG_PATH = $(LIBSOPHAV_ROOT)/3rdparty/libjpeg-turbo/lib/soc/libturbojpeg.a
endif

# include $(BMCV_SRC_DIR)/test/Makefile
-include $(LIBSOPHAV_ROOT)/build/version.mak
ifneq ($(SO_NAME),)
	BMCV_DYN_SONAME    = $(BMCV_DYN)$(SO_NAME)
	BMCV_DYN_SOVERSION = $(BMCV_DYN)$(SO_VERSION)
endif

$(BMCV_SRC_DIR)/obj/%.o: $(BMCV_SRC_DIR)/%.c
	@mkdir -p $(BMCV_OBJ_DIR)
	@echo $(CC) -c $< -o $@
	@$(CC) $(INCLUDE_DIR) -c $(CFLAGS) $< -o $@

$(BMCV_DYN): jpeg $(BMCV_CC_OBJS) $(BMCV_STATIC)
	@echo $(CC) $(BMCV_DYN)
	@mkdir -p $(OUT_DIR)/$(CHIP)/lib
	@mkdir -p $(OUT_DIR)/$(CHIP)/test
	@$(CC) $(BMCV_CC_OBJS) $(CFLAGS) $(DYN_LDFLAGS) -shared -Wl,-soname,$(BMCV_DYN_NAME)$(SO_NAME) -o $(BMCV_DYN) -Wl,--whole-archive $(LIB_TURBO_JPEG_PATH) -Wl,--no-whole-archive
ifneq ($(BMCV_DYN_SOVERSION), )
	mv $@ $(BMCV_DYN_SOVERSION)
	ln -sf $(BMCV_DYN_NAME)$(SO_VERSION) $(BMCV_DYN_SONAME)
	ln -sf $(BMCV_DYN_NAME)$(SO_NAME) $@
endif
	@rm -r $(BMCV_SRC_DIR)/obj

$(BMCV_STATIC):$(BMCV_CC_OBJS)
	@echo $(AR) $(BMCV_STATIC)
	@mkdir -p $(OUT_DIR)/$(CHIP)/lib
	@$(AR) rcs $(BMCV_STATIC) $(BMCV_CC_OBJS)

BMCV_TEST := $(wildcard $(BMCV_SRC_DIR)/test/*.c)
BMCV_TEST_OBJ := $(patsubst %.c, %, $(BMCV_TEST))
BMCV_TEST_OBJ := $(patsubst $(BMCV_SRC_DIR)/test%, %, $(BMCV_TEST_OBJ))
BMCV_TEST_OBJ := $(addprefix $(OUT_DIR)/$(CHIP)/test, $(BMCV_TEST_OBJ))
INCLUDE_DIR += -I$(OUT_DIR)/$(CHIP)/include
$(OUT_DIR)/$(CHIP)/test/%: $(BMCV_SRC_DIR)/test/%.c
	@echo $(CC) -o $@ $<
	@$(CC) $< -o $@ $(CFLAGS) $(DYN_LDFLAGS) $(INCLUDE_DIR) -lbmcv -L $(OUT_DIR)/$(CHIP)/lib -O3 -Wl,--gc-sections -rdynamic -pthread

jpeg:
	make -C $(LIBSOPHAV_ROOT)/jpeg/
	make -C $(LIBSOPHAV_ROOT)/jpeg/ install

ifeq ($(CHIP), bm1688)
bmcv: $(BMCV_DYN)
	@mkdir -p $(OUT_DIR)/$(CHIP)/include
	@cp -f $(RELEASE_HEADERS) $(OUT_DIR)/$(CHIP)/include

doc:
	make -C $(LIBSOPHAV_ROOT)/bmcv/document/bmcv_demo LC_ALL=en_US.UTF-8 pdf
	mkdir -p $(OUT_DIR)/$(CHIP)/doc
	mv $(LIBSOPHAV_ROOT)/bmcv/document/bmcv_demo/build/*.pdf $(OUT_DIR)/$(CHIP)/doc

html:
	make -C $(LIBSOPHAV_ROOT)/bmcv/document/bmcv_demo LC_ALL=en_US.UTF-8 web
	mkdir -p $(OUT_DIR)/$(CHIP)/html
	cp -r $(LIBSOPHAV_ROOT)/bmcv/document/bmcv_demo/build/html/* $(OUT_DIR)/$(CHIP)/html

test: $(BMCV_TEST_OBJ)

clean:
	@echo rm -rf $(OUT_DIR)
	@rm -rf $(OUT_DIR)
	@rm -rf $(BMCV_OBJ_DIR) $(BMCV_DYN) $(BMCV_STATIC)
	@rm -rf $(BMCV_DYN_SOVERSION) $(BMCV_DYN_SONAME)
	@rm -rf $(BMCV_TEST_OBJ)
	@rm -rf $(LIBSOPHAV_ROOT)/jpeg/install
endif

