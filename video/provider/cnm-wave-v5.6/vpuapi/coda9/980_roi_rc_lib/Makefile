#-----------------------------------------------------------------------------
# COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
# 
# This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
# SPDX License Identifier: BSD-3-Clause
# SPDX License Identifier: LGPL-2.1-only
# 
# The entire notice above must be reproduced on all authorized copies.
# 
# Description  : 
#-----------------------------------------------------------------------------

.PHONY: clean

CROSS_CC_PREFIX=
PLATFORM_CFLAGS=
TARGET   = libroirc.a
ifeq ("$(BUILD_CONFIGURATION)", "EmbeddedLinux")
    CROSS_CC_PREFIX = arm-none-linux-gnueabi-
    TARGET   = libroirc_arm.a
endif
ifeq ("$(BUILD_CONFIGURATION)", "NonOS")
    CROSS_CC_PREFIX = arm-none-eabi-
    PLATFORM_CFLAGS =
    TARGET   = libroirc_nonos.a
endif

CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
AR  = $(CROSS_CC_PREFIX)ar

CFLAGS   = $(PLATFORM_CFLAGS) -g -O3 -c -DCLIP_PIC_DELTA_QP
CXXFLAGS = $(PLATFORM_CFLAGS) -g -O3 -c -DCLIP_PIC_DELTA_QP
ARFLAGS  = cru
LDFLAGS  = 


SOURCES = frm_rc.c hevc_enc_rc.c util_float.c
OBJECTS = $(SOURCES:.c=.o)
VPATH   = src

$(TARGET): $(OBJECTS) 
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

clean:
	rm -f $(TARGET)
	rm -f *.o
	rm -f *.dep

-include $(OBJECTS:.o=.dep)

.c.o: Makefile
	$(CC) -c $(CFLAGS) -o $@ $< -MD -MF $(@:.o=.dep) 


