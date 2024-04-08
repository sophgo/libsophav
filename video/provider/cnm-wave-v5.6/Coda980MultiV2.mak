#-----------------------------------------------------------------------------
# COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
#
# This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
# SPDX License Identifier: BSD-3-Clause
# SPDX License Identifier: LGPL-2.1-only
#
# The entire notice above must be reproduced on all authorized copies.
#
# Description  : C&M Video Multi Instance #
#-----------------------------------------------------------------------------
.PHONY: TARGETLIBTHEORA CREATE_DIR

$(shell cp sample_v2/component_list_all.h sample_v2/component/component_list.h)

USE_FFMPEG  = yes
USE_PTHREAD = yes

UNAME = $(shell uname -a)
ifneq (,$(findstring i386, $(UNAME)))
    USE_32BIT = yes
endif


ifeq ($(USE_32BIT), yes)
PLATFORM    = nativelinux
else
PLATFORM    = nativelinux_64bit
endif

CROSS_CC_PREFIX =
VDI_C           = vdi/linux/vdi.c
VDI_DEVICE_C    = vdi/linux/device.c
VDI_OSAL_C      = vdi/linux/vdi_osal.c
VDI_DEBUG_C     = vdi/vdi_debug.c
MM_C            =
PLATFORM_FLAGS  =
VDI_VPATH       = vdi/linux

ifeq ("$(BUILD_CONFIGURATION)", "NonOS")
    CROSS_CC_PREFIX = arm-none-eabi-
    VDI_C           = vdi/nonos/vdi.c
    VDI_OSAL_C      = vdi/nonos/vdi_osal.c
    MM_C            = vdi/mm.c
    USE_FFMPEG      = no
    USE_PTHREAD     = no
    PLATFORM        = none
    DEFINES         = -DLIB_C_STUB
    PLATFORM_FLAGS  =
    VDI_VPATH       = vdi/nonos
endif
ifeq ("$(BUILD_CONFIGURATION)", "EmbeddedLinux")
    CROSS_CC_PREFIX = arm-none-linux-gnueabi-
    PLATFORM        = armlinux
endif

CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
AR  = $(CROSS_CC_PREFIX)ar

INCLUDES = -I./theoraparser/include -I./vpuapi -I./ffmpeg/include -I./sample_v2/helper -I./sample_v2/helper/misc -I./sample_v2/component -I./vdi
INCLUDES += -I./sample_v2/component_decoder -I./sample_v2/component_encoder

DEFINES += -DCODA980 -DSUPPORT_MULTI_INSTANCE_TEST
CFLAGS  += -g -I. -Wno-implicit-function-declaration -Wno-unused-result -Wno-format -Wl,--fatal-warning $(INCLUDES) $(DEFINES)
ARFLAGS += cru

LDFLAGS  = -L./theoraparser/
LDLIBS   = -ltheoraparser

LDFLAGS += -L./vpuapi/coda9/980_roi_rc_lib/
LDLIBS  += -lroirc

ifeq ($(USE_FFMPEG), yes)
LDLIBS  += -lavformat -lavcodec -lavutil -lswresample -laom
LDFLAGS += -L./ffmpeg/lib/$(PLATFORM)
ifneq ($(USE_32BIT), yes)
LDLIBS  += -lz
endif #USE_32BIT
endif #USE_FFMPEG

ifeq ($(USE_PTHREAD), yes)
LDLIBS  += -lpthread
endif
LDLIBS  += -lm

TARGET=multi_instance_test
MAKEFILE=Coda980MultiV2.mak
OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = main_multi_instance_test.c        \
          component_coda9_enc_encoder.c     \
          component_enc_feeder.c            \
          component_enc_reader.c            \
          encoder_listener.c                \
          component_coda9_dec_decoder.c     \
          component_dec_feeder.c            \
          component_dec_renderer.c          \
          decoder_listener.c           \
          cnm_app.c                    \
          cnm_task.c                   \
          component.c                  \
          main_helper.c                \
          vpuhelper.c                  \
          bitstreamfeeder.c            \
          bitstreamreader.c            \
          bsfeeder_fixedsize_impl.c    \
          bsfeeder_framesize_impl.c    \
          bsfeeder_size_plus_es_impl.c \
          bin_comparator_impl.c        \
          byframe_comparator_impl.c    \
          regout_comparator_impl.c     \
          comparator.c                 \
          md5_comparator_impl.c        \
          yuv_comparator_impl.c        \
          cfgParser.c                  \
          cnm_video_helper.c           \
          container.c                  \
          datastructure.c              \
          debug.c                      \
          yuvfeeder.c                  \
          yuvLoaderfeeder.c            \
          bw_monitor.c                 \
          pf_monitor.c
SOURCES += $(VDI_C)                                         \
          $(VDI_DEVICE_C)                                   \
          $(VDI_OSAL_C)                                     \
          $(VDI_DEBUG_C)                                    \
          $(MM_C)                                           \
          vpuapi/product.c                                  \
          vpuapi/vpuapifunc.c                               \
          vpuapi/vpuapi.c                                   \
          vpuapi/coda9/coda9.c                              \
          vpuapi/wave/wave5.c                               \
          vpuapi/wave/wave6.c
VPATH  = sample_v2:
VPATH += sample_v2/component_encoder:
VPATH += sample_v2/component_encoder/coda9:
VPATH += sample_v2/component_decoder:
VPATH += sample_v2/component_decoder/coda9:
VPATH += sample_v2/helper:
VPATH += sample_v2/helper/bitstream:
VPATH += sample_v2/helper/comparator:
VPATH += sample_v2/helper/display:sample_v2/helper/misc:sample_v2/helper/yuv:sample_v2/component:
VPATH += vdi:
VPATH += $(VDI_VPATH):vpuapi:vpuapi/coda9:vpuapi/wave

OBJECTNAMES=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))
OBJECTPATHS=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES)))

all: CREATE_DIR $(OBJECTPATHS) LIBTHEORA
	$(CC) -o $(TARGET) $(LDFLAGS) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS) $(LDLIBS) -Wl,--end-group

-include $(OBJECTPATHS:.o=.dep)

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)
	$(RM) theoraparser/$(ALLOBJS)
	$(RM) theoraparser/$(ALLLIBS)
	$(RM) theoraparser/$(ALLDEPS)

LIBTHEORA:
	cd theoraparser; make clean; make

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c $(MAKEFILE)
	$(CC) $(CFLAGS) -Wall -Werror -c $< -o $@ -MD -MF $(@:.o=.dep)
