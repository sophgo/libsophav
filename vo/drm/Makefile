SHELL = /bin/bash

ifeq ($(PRODUCTFORM), soc)
	LIB_PATH = lib_aarch64
else
	LIB_PATH = lib_musl_riscv64
endif

DRM_INC = $(BMVID_TOP_DIR)/3rdparty/drm
LIBKMS_INC = $(BMVID_TOP_DIR)/3rdparty/drm/include/libkms
LIBDRM_INC = $(BMVID_TOP_DIR)/3rdparty/drm/include/libdrm
DRM_COMMON_INC = $(PWD)/common
LIBS = $(BMVID_TOP_DIR)/3rdparty/libdrm/$(LIB_PATH)

SDIR = $(PWD)
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(BMVID_TOP_DIR) -I$(DRM_INC) -I$(LIBKMS_INC) -I$(LIBDRM_INC) -I$(DRM_COMMON_INC)
OBJS = $(SRCS:.c=.o)

TARGET = $(SRCS:.c=)

EXTRA_CFLAGS = $(INCS)
EXTRA_LDFLAGS = -L$(LIBS) -ldrm -lkms -lm

.PHONY : clean all
all: $(TARGET)

$(SDIR)/%: $(SDIR)/%.c $(SDIR)/common/*.c
	@$(CC) -o $@ $^ $(CFLAGS) $(ELFFLAGS) $(EXTRA_CFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CC))] $(notdir $@)

clean:
	@rm -f $(OBJS) $(notdir $(TARGET))
