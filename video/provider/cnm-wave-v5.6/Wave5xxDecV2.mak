#-----------------------------------------------------------------------------
# COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
#
# This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
# SPDX License Identifier: BSD-3-Clause
# SPDX License Identifier: LGPL-2.1-only
#
# The entire notice above must be reproduced on all authorized copies.
#
# Description  : C&M Video decoder sample
#-----------------------------------------------------------------------------

.PHONY: CREATE_DIR

PRODUCT := WAVE521C
PRODUCT := WAVE517

$(shell cp sample_v2/component_list_decoder.h sample_v2/component/component_list.h)

USE_FFMPEG  = yes
USE_PTHREAD = yes
USE_RTL_SIM = no
USE_THEORA  = yes
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
ifneq ($(GCC_VERSION_CHANGE),)
CROSS_CC_PREFIX     = $(GCC_VERSION_CHANGE)
endif
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

DEFINES  += -DCNM_WAVE_SERIES

CC     = $(CROSS_CC_PREFIX)gcc
CXX    = $(CROSS_CC_PREFIX)g++
LINKER = $(CC)
AR     = $(CROSS_CC_PREFIX)ar

INCLUDES  = -I./vpuapi -I./ffmpeg/include -I./theoraparser/include -I./sample_v2/helper -I./sample_v2/helper/misc -I./sample_v2/component -I./vdi
INCLUDES += -I./sample_v2/component_decoder
ifeq ($(USE_RTL_SIM), yes)
DEFINES  += -DCNM_SIM_PLATFORM -DCNM_SIM_DPI_INTERFACE -DSUPPORT_DECODER -DUSE_SINGLE_THREAD
DEFINES  += -D$(PRODUCT) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
MM_C      = vdi/mm.c
else
DEFINES  += -D$(PRODUCT) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
endif	# USE_SIM_PLATFORM

ifeq ($(SCALER), 1)
DEFINES  += -DSUPPORT_MINI_PIPPEN
endif


ifeq ($(CSC), 1)
DEFINES  += -DSUPPORT_COLOR_SPACE_CONVERSION_YUV2RGB
endif


CFLAGS   += -g -I. -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning $(INCLUDES) $(DEFINES) $(PLATFORM_FLAGS)
ifeq ($(USE_RTL_SIM), yes)
ifeq ($(IUS), 1)
CFLAGS   += -fPIC # ncverilog is 64bit version
endif
endif
ARFLAGS  += cru

LDFLAGS  = $(PLATFORM_FLAGS)


LDFLAGS  += -L./vpuapi/coda9/980_roi_rc_lib/
ifeq ("$(BUILD_CONFIGURATION)", "NonOS")
LDLIBS   += -lroirc_nonos
else ifeq ("$(BUILD_CONFIGURATION)", "EmbeddedLinux")
LDLIBS   += -lroirc_arm
else
LDLIBS   += -lroirc
endif

ifeq ($(USE_FFMPEG), yes)
LDLIBS   += -lavformat -lavcodec -lavutil -lswresample -laom
LDFLAGS  += -L./ffmpeg/lib/$(PLATFORM)
ifneq ("$(BUILD_CONFIGURATION)", "EmbeddedLinux")
ifneq ($(USE_32BIT), yes)
LDLIBS   += -lz
endif #Not USE_32BIT
endif #Not EmbeddedLinux
endif #USE_FFMPEG

ifeq ($(USE_PTHREAD), yes)
LDLIBS   += -lpthread
endif
LDLIBS   += -lm

ifeq ($(USE_THEORA), yes)
ifneq ($(wildcard theoraparser), )
LDFLAGS  += -L./theoraparser
LDLIBS  += -ltheoraparser
BUILDLIST+=LIBTHEORA
endif
endif

BUILDLIST+=APPEND
MAKEFILE=Wave5xxDecV2.mak
APPEND=w5_dec_test

OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES_COMMON =main_helper.c                   vpuhelper.c                 bitstreamfeeder.c           \
                bitstreamreader.c               bsfeeder_fixedsize_impl.c   bsfeeder_framesize_impl.c   \
                bsfeeder_size_plus_es_impl.c    host_dec_stream.c           host_bsfeeder_framesize_impl.c \
                bin_comparator_impl.c           comparator.c                \
                md5_comparator_impl.c           yuv_comparator_impl.c       \
                cfgParser.c                     decoder_listener.c          \
                cnm_video_helper.c              container.c                 \
                datastructure.c                 debug.c                     \
                bw_monitor.c                    pf_monitor.c                \
                cnm_app.c                       cnm_task.c                  component.c                 \
                component_dec_decoder.c         component_dec_feeder.c      component_dec_renderer.c    \
                product.c                       vpuapifunc.c                vpuapi.c                    \
                coda9.c                         wave5.c                     wave6.c                     \
                $(VDI_C)                        $(VDI_OSAL_C)               $(VDI_DEBUG_C)              $(MM_C)\
                $(VDI_DEVICE_C)


VPATH = sample_v2:
VPATH += sample_v2/component_encoder:
VPATH += sample_v2/component_decoder:
VPATH += sample_v2/helper:
VPATH += sample_v2/helper/bitstream:
VPATH += sample_v2/helper/bitstream/host_dec_stream:
VPATH += sample_v2/helper/comparator:
VPATH += sample_v2/helper/display:sample_v2/helper/misc:sample_v2/helper/yuv:sample_v2/component:
VPATH += vdi:
VPATH += $(VDI_VPATH):vpuapi:vpuapi/coda9:vpuapi/wave

VPATH2=$(patsubst %,-I%,$(subst :, ,$(VPATH)))

OBJECTNAMES_COMMON=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES_COMMON)))
OBJECTPATHS_COMMON=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES_COMMON)))

SOURCES_DECTEST = sample_v2/main_dec_test.c
ifeq ($(USE_RTL_SIM), yes)
	SOURCES_DECTEST += sample_v2/main_sim.c
endif

OBJECTNAMES_DECTEST=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES_DECTEST)))
OBJECTPATHS_DECTEST=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES_DECTEST))) $(OBJECTPATHS_COMMON)

all: $(BUILDLIST)

ifeq ($(USE_RTL_SIM), yes)
APPEND: CREATE_DIR $(OBJECTPATHS_DECTEST)
else
APPEND: CREATE_DIR $(OBJECTPATHS_DECTEST)
	$(LINKER) -o $(APPEND) $(LDFLAGS) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS_DECTEST) $(LDLIBS) -Wl,--end-group
endif

-include $(OBJECTPATHS:.o=.dep)

clean:
	$(RM) $(APPEND)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)
ifeq ($(USE_THEORA), yes)
ifneq ($(wildcard theoraparser), )
	make -C theoraparser BUILD_CONFIGURATION=$(BUILD_CONFIGURATION) clean;
endif
endif

LIBTHEORA:
ifneq ($(wildcard theoraparser), )
	make -C theoraparser BUILD_CONFIGURATION=$(BUILD_CONFIGURATION)
endif

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c $(MAKEFILE)
	$(CC) $(CFLAGS) -Wall -Werror -c $< -o $@ -MD -MF $(@:.o=.dep)


lint:
	"$(LINT_HOME)/flint" -i"$(LINT_HOME)" $(DEFINES) $(INCLUDES) $(VPATH2) linux_std.lnt $(HAPS_RULE) $(NONOS_RULE)  $(SOURCES_COMMON)

