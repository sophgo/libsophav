#-----------------------------------------------------------------------------
# COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
#
# This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
# SPDX License Identifier: BSD-3-Clause
# SPDX License Identifier: LGPL-2.1-only
#
# The entire notice above must be reproduced on all authorized copies.
#
# Description  : C&M Video encoder sample v2
#-----------------------------------------------------------------------------

.PHONY: CREATE_DIR

PRODUCT :=CODA980

$(shell cp sample_v2/component_list_encoder.h sample_v2/component/component_list.h)

USE_FFMPEG  = yes
USE_PTHREAD = yes
USE_RTL_SIM = no
LINT_HOME   = etc/lint

UNAME = $(shell uname -a)
ifneq (,$(findstring i386, $(UNAME)))
    USE_32BIT = yes
endif

ifeq ($(RTL_SIM), 1)
USE_RTL_SIM = yes
endif

REFC    := 0

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
    VDI_DEVICE_C    =
    VDI_OSAL_C      = vdi/nonos/vdi_osal.c
    MM_C            = vdi/mm.c
    USE_FFMPEG      = no
    USE_PTHREAD     = no
    PLATFORM        = none
    DEFINES         = -DLIB_C_STUB
    PLATFORM_FLAGS  =
    VDI_VPATH       = vdi/nonos
    NONOS_RULE      = options_nonos.lnt
endif
ifeq ("$(BUILD_CONFIGURATION)", "EmbeddedLinux")
    CROSS_CC_PREFIX = arm-none-linux-gnueabi-
    PLATFORM        = armlinux
endif

CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
LINKER=$(CC)
AR  = $(CROSS_CC_PREFIX)ar

INCLUDES = -I./vpuapi -I./ffmpeg/include -I./sample_v2/helper -I./sample_v2/helper/misc -I./sample_v2/component -I./vdi
INCLUDES += -I./theoraparser/include
INCLUDES += -I./sample_v2/component_encoder
DEFINES += -D$(PRODUCT)

ifeq ($(USE_RTL_SIM), yes)
DEFINES += -DCNM_SIM_PLATFORM -DCNM_SIM_DPI_INTERFACE -DSUPPORT_ENCODER
MM_C            = vdi/mm.c
else

endif	#USE_RTL_SIM

CFLAGS  += -g -I. -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning $(INCLUDES) $(DEFINES) $(PLATFORM_FLAGS)
ifeq ($(USE_RTL_SIM), yes)
ifeq ($(IUS), 1)
CFLAGS  += -fPIC # ncverilog is 64bit version
endif
endif
ARFLAGS += cru

LDFLAGS  = $(PLATFORM_FLAGS)

LDFLAGS  += -L./vpuapi/coda9/980_roi_rc_lib/
ifeq ("$(BUILD_CONFIGURATION)", "NonOS")
LDLIBS   += -lroirc_nonos
else ifeq ("$(BUILD_CONFIGURATION)", "EmbeddedLinux")
LDLIBS   += -lroirc_arm
else
LDLIBS   += -lroirc
endif

LDFLAGS  += -L./theoraparser/
LDLIBS  += -ltheoraparser

ifeq ($(USE_FFMPEG), yes)
LDLIBS  += -lavformat -lavcodec -lavutil
LDFLAGS += -L./ffmpeg/lib/$(PLATFORM)
ifneq ($(USE_32BIT), yes)
LDLIBS  += -lz
endif #USE_32BIT
endif #USE_FFMPEG

ifeq ($(USE_PTHREAD), yes)
LDLIBS  += -lpthread
endif
LDLIBS  += -lm

TARGET =coda980_enc_test
MAKEFILE=Coda980EncV2.mak

OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = main_coda980_enc_test.c               \
          component_coda9_enc_encoder.c         \
          component_enc_feeder.c                \
          component_enc_reader.c                \
          encoder_listener.c                    \
          cnm_app.c                             \
          cnm_task.c                            \
          component.c                           \
          main_helper.c                         \
          vpuhelper.c                           \
          bitstreamfeeder.c                     \
          bsfeeder_fixedsize_impl.c             \
          bsfeeder_framesize_impl.c             \
          bsfeeder_size_plus_es_impl.c          \
          bitstreamreader.c                     \
          bin_comparator_impl.c                 \
          comparator.c                          \
          md5_comparator_impl.c                 \
          yuv_comparator_impl.c                 \
          cfgParser.c                           \
          cnm_video_helper.c                    \
          container.c                           \
          datastructure.c                       \
          debug.c                               \
          yuvfeeder.c                           \
          yuvLoaderfeeder.c                     \
          bw_monitor.c                          \
          pf_monitor.c

SOURCES += $(VDI_C)                                               \
          $(VDI_DEVICE_C)                                         \
          $(VDI_OSAL_C)                                           \
          $(VDI_DEBUG_C)                                          \
          $(MM_C)                                                 \
          vpuapi/product.c                                        \
          vpuapi/vpuapifunc.c                                     \
          vpuapi/vpuapi.c                                         \
          vpuapi/coda9/coda9.c                                    \
          vpuapi/wave/wave5.c                                     \
          vpuapi/wave/wave6.c
ifeq ($(USE_RTL_SIM), yes)
	SOURCES += sample/main_sim.c
endif

VPATH  = sample_v2:
VPATH += sample_v2/component_encoder:
VPATH += sample_v2/component_encoder/coda9:
VPATH += sample_v2/helper:
VPATH += sample_v2/helper/bitstream:
VPATH += sample_v2/helper/comparator:
VPATH += sample_v2/helper/display:sample_v2/helper/misc:sample_v2/helper/yuv:sample_v2/component:
VPATH += vdi:
VPATH += $(VDI_VPATH):vpuapi:vpuapi/coda9:vpuapi/wave

OBJECTNAMES=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))
OBJECTPATHS=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES)))

LINT_SRC_INCLUDES = -I./sample_v2 -I./sample_v2/component -I./sample_v2/component_decoder -I./sample_v2/component_encoder
LINT_SRC_INCLUDES += -I./sample_v2/component_decoder/coda9 -I./sample_v2/component_encoder/coda9
LINT_SRC_INCLUDES += -I./sample_v2/helper -I./sample_v2/helper/bitstream -I./sample_v2/helper/comparator -I./sample_v2/helper/display
LINT_SRC_INCLUDES += -I./sample_v2/helper/misc -I./sample_v2/helper/yuv -I./sample_v2/helper/comparator -I./sample_v2/helper/display

ifeq ($(USE_RTL_SIM), yes)
all: LIBTHEORA CREATE_DIR $(OBJECTPATHS)
else
all: LIBTHEORA CREATE_DIR $(OBJECTPATHS)
	$(LINKER) -o $(TARGET) $(LDFLAGS) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS) $(LDLIBS) -Wl,--end-group
endif

-include $(OBJECTPATHS:.o=.dep)

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)
	make -C theoraparser BUILD_CONFIGURATION=$(BUILD_CONFIGURATION) clean

LIBTHEORA:
	make -C theoraparser BUILD_CONFIGURATION=$(BUILD_CONFIGURATION)

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c $(MAKEFILE)
#$(CC) $(CFLAGS) -Wall -Werror -c $< -o $@ -MD -MF $(@:.o=.dep) #TODO You should include the flag -Werror after testing
	$(CC) $(CFLAGS) -Wall -c $< -o $@ -MD -MF $(@:.o=.dep)

lint:
	"$(LINT_HOME)/flint" -i"$(LINT_HOME)" $(DEFINES) $(INCLUDES) $(LINT_SRC_INCLUDES) linux_std.lnt $(HAPS_RULE) $(NONOS_RULE)  $(SOURCES)

