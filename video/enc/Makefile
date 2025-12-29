########################################################################
#
# The API of BM VPU SDK in Linux
#
# CHIP:        bm1684
# PRODUCTFORM: soc, pcie, pcie_arm64
# SUBTYPE:     palladium, fpga, asic
########################################################################

CHIP ?= bm1684
PRODUCTFORM ?= soc
SUBTYPE ?= asic
INSTALL_DIR ?= ./release
DEBUG ?= 0
ION ?= 0
ION_DIR ?= ./release
BMVID_TOP_DIR ?= ./
BMVID_ROOT ?= $(BMVID_TOP_DIR)

ifeq ($(PRODUCTFORM),pcie) # pcie mode
    CROSS_CC_PREFIX =
else ifeq ($(PRODUCTFORM),pcie_mips64) # pcie mode
    CROSS_CC_PREFIX = mips-linux-gnu-
else ifeq ($(PRODUCTFORM),pcie_loongarch64) # pcie mode
    CROSS_CC_PREFIX = loongarch64-linux-gnu-
else ifeq ($(PRODUCTFORM),pcie_sw64) # pcie mode
    CROSS_CC_PREFIX = sw_64-sunway-linux-gnu-
else # pcie_arm64 and soc mode
    CROSS_CC_PREFIX = aarch64-linux-gnu-
endif

BM_CHIP_TYPE = CHIP_BM1684

ifeq ($(SUBTYPE), palladium)
    BM_SUB_TYPE = PALLADIUM_SIM
else ifeq ($(SUBTYPE), fpga)
    BM_SUB_TYPE = FPGA_SIM
else ifeq ($(SUBTYPE), asic)
    BM_SUB_TYPE = BM_ASIC
else
    $(error Only palladium/fpga/asic are valid sub-types)
endif

CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
AR  = $(CROSS_CC_PREFIX)ar

CFLAGS += -fPIC -Wall -Wl,--fatal-warning
# CFLAGS +=  -I/home/hantao/work/a2-sdk/ramdisk/rootfs/public/libcrypto/include
# CFLAGS +=  -I/home/hantao/work/a2-sdk/ramdisk/rootfs/public/libz/include
# CFLAGS +=  -I/home/hantao/work/a2-sdk/ramdisk/rootfs/public/secure_image/include
# CFLAGS +=  -I/home/hantao/work/a2-sdk/ramdisk/rootfs/public/crontabs/include
# CFLAGS +=  -I/home/hantao/work/a2-sdk/ramdisk/rootfs/public/gatord/include
# CFLAGS +=  -I/home/hantao/work/a2-sdk/middleware/v2/include
# CFLAGS +=  -I/home/hantao/work/a2-sdk/middleware/v2/modules/isp/include/bm1688

# CFLAGS += -I$(KERNEL_PATH)/build/$(PROJECT_FULLNAME)/arm64/usr/include
# CFLAGS +=  -I/home/hantao/work/a2-sdk/middleware/v2/modules/sys/include

# CFLAGS += -I$(KERNEL_PATH)/include
# CFLAGS += -I$(KERNEL_PATH)/build/$(PROJECT_FULLNAME)/arch/arm64/include/generated
# CFLAGS += -I$(KERNEL_PATH)/arch/arm64/include/
# CFLAGS += -I$(BMVID_TOP_DIR)/inc
CFLAGS += -D$(BM_CHIP_TYPE) -D$(BM_SUB_TYPE)
ifneq ($(PRODUCTFORM), soc)
    CFLAGS += -DBM_PCIE_MODE
endif
ifeq ($(DEBUG), 0)
    CFLAGS += -O2
else
    CFLAGS += -g
endif


LDFLAGS += -L$(BMVID_TOP_DIR)/soc/
LDFLAGS += -L$(BMVID_ROOT)/../sophon_media/libsophav/3rdparty/libbmcv/lib/$(PRODUCTFORM)/
LDFLAGS += -L$(INSTALL_DIR)/lib
# LDFLAGS += -L./$(PRODUCTFORM) -L$(INSTALL_DIR)/lib -L$(ION_DIR)/lib
LDLIBS   = -lpthread -lm -lrt -ldl -lbmlib
CFLAGS  += -I$(BMVID_TOP_DIR)/inc
CFLAGS  += -I./inc
CFLAGS  += -I$(BMVID_ROOT)/../sophon_media/libsophav/3rdparty/libbmcv/include
CFLAGS  += -I$(BMVID_ROOT)/../sophon_media/libsophav/3rdparty/osdrv

ifeq ($(PRODUCTFORM),pcie_mips64)
    CFLAGS += -mips64r2 -mabi=64 -march=gs464e -D_GLIBCXX_USE_CXX11_ABI=0
    LDFLAGS += -Wl,-melf64ltsmip
endif

ifeq ($(PRODUCTFORM), pcie_loongarch64)
    LDFLAGS += -Wl,-melf64loongarch
    LDLIBS += -lstdc++
endif

LDLIBS += -lpthread -lm -lrt
LDLIBSTEST = ${LDLIBS} -lbmapi
TARGET_NAME=libbmapi.so
TESTDEC_NAME=bmjpegdec
TESTENC_NAME=bmjpegenc
TESTMULTI_NAME=bmjpegmulti
TEST_VPUDEC_NAME=bm_test
TEST_VPUENC_NAME=bmvpuenc

TARGET=$(PRODUCTFORM)/$(TARGET_NAME)
TESTDEC=$(PRODUCTFORM)/$(TESTDEC_NAME)
TESTENC=$(PRODUCTFORM)/$(TESTENC_NAME)
TESTMULTI=$(PRODUCTFORM)/$(TESTMULTI_NAME)
TESTVPUDEC=$(PRODUCTFORM)/$(TEST_VPUDEC_NAME)
TESTVPUENC=$(PRODUCTFORM)/$(TEST_VPUENC_NAME)

-include $(BMVID_ROOT)/build/version.mak
ifneq ($(SO_NAME),)
    TARGET_SONAME=$(TARGET)$(SO_NAME)
    TARGET_SOVERSION=$(TARGET)$(SO_VERSION)
endif

MAKEFILE=Makefile
OBJDIR=$(PRODUCTFORM)/obj
ALLOBJS=*.o
ALLDEPS=*.dep
RM ?= rm -rf
CP ?= cp -f
MKDIR ?= mkdir -p

# LIB_SRC = src/bm_vpudec_interface.c \
#           src/bm_vpuenc_interface.c \
#           src/bmjpu_api.c
LIB_SRC = src/bm_ioctl.c \
          src/bm_vpuenc_interface.c

LIB_OBJS=$(patsubst %.c,%.o,$(LIB_SRC))
LIB_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(LIB_OBJS)))

# ENC_SRC = $(BMVID_TOP_DIR)/example/cvi_api_test.c

ENC_OBJS=$(patsubst %.c,%.o,$(ENC_SRC))
ENC_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(ENC_OBJS)))




DEC_SRC = example/jpeg_dec.c \
          example/jpeg_common.c

DEC_OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(DEC_SRC)))
DEC_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(DEC_OBJS)))

ENC_SRC = example/jpeg_enc.c \
          example/jpeg_common.c

ENC_OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(ENC_SRC)))
ENC_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(ENC_OBJS)))

MULTI_SRC = example/jpeg_multi.c \
            example/jpeg_common.c \
            example/bmjpurun.c

MULTI_OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(MULTI_SRC)))
MULTI_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(MULTI_OBJS)))


VPUDEC_SRC = example/bm_test.c
VPUDEC_OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(VPUDEC_SRC)))
VPUDEC_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(VPUDEC_OBJS)))


VPUENC_SRC = example/bmvpuenc.c \
             example/queue.c
VPUENC_OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(VPUENC_SRC)))
VPUENC_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(VPUENC_OBJS)))



OBJECTPATHS=$(LIB_PATHS) $(DEC_PATHS) $(ENC_PATHS) $(MULTI_PATHS) $(VPUDEC_PATHS) $(VPUENC_PATHS)
$(info LIB_PATHS=$(LIB_PATHS))


all: $(OBJDIR) $(TARGET) $(TESTVPUENC)
# all: $(OBJDIR) $(TARGET) $(TESTDEC) $(TESTENC) $(TESTMULTI) $(TESTVPUDEC) $(TESTVPUENC)

# $(TESTDEC): $(TARGET) $(DEC_PATHS)
# 	$(CC) $(CFLAGS) -o $@ $(DEC_PATHS) $(LDFLAGS) $(LDLIBSTEST)

# $(TESTENC): $(TARGET) $(DEC_PATHS) $(ENC_PATHS)
# 	$(CC) $(CFLAGS) -o $@ $(ENC_PATHS) $(LDFLAGS) $(LDLIBSTEST)
#
# $(TESTMULTI):$(TARGET) $(DEC_PATHS) $(MULTI_PATHS)
# 	$(CC) $(CFLAGS) -o $@ $(MULTI_PATHS) $(LDFLAGS) $(LDLIBSTEST)
#
# $(TESTVPUDEC):$(TARGET) $(VPUDEC_PATHS)
# 	$(CC) $(CFLAGS) -o $@ $(VPUDEC_PATHS) $(LDFLAGS) $(LDLIBSTEST)

$(TESTVPUENC):$(TARGET) $(VPUENC_PATHS)
	$(CC) $(CFLAGS) -o $@ $(VPUENC_PATHS) $(LDFLAGS) $(LDLIBSTEST)

$(TARGET): $(LIB_PATHS)
	$(CC) $(CFLAGS) -o $@ -shared -Wl,-soname,$(TARGET_NAME)$(SO_NAME) $(LIB_PATHS) $(LDFLAGS) ${LDLIBS}

ifneq ($(TARGET_SOVERSION), )
	mv $@ $(TARGET_SOVERSION)
	ln -sf $(TARGET_NAME)$(SO_VERSION) $(TARGET_SONAME)
	ln -sf $(TARGET_NAME)$(SO_NAME) $@
endif

-include $(OBJECTPATHS:.o=.dep)

clean:
	$(RM) $(TARGET) $(TARGET_SOVERSION) $(TARGET_SONAME)
	$(RM) $(OBJDIR)/$(ALLDEPS)
	$(RM) $(OBJDIR)/$(ALLOBJS)

install: $(TARGET)
	install -d $(INSTALL_DIR)/bin
	install -d $(INSTALL_DIR)/lib
	install -d $(INSTALL_DIR)/include
	install $(BMVID_TOP_DIR)/soc/$(TARGET_NAME) $(INSTALL_DIR)/lib
	# install -m 0644 $(BMVID_TOP_DIR)/inc/bm_vpudec_interface.h $(INSTALL_DIR)/include
	install -m 0644 $(BMVID_TOP_DIR)/inc/bm_vpuenc_interface.h $(INSTALL_DIR)/include
ifeq ($(TARGET_SOVERSION), )
	install $(TARGET) $(INSTALL_DIR)/lib
else
	install $(TARGET_SOVERSION) $(INSTALL_DIR)/lib
	cp -ap $(TARGET_SONAME) $(TARGET) $(INSTALL_DIR)/lib
endif

uninstall:
	$(RM) $(INSTALL_DIR)/lib/$(TARGET_NAME)
	$(RM) $(INSTALL_DIR)/lib/$(TARGET_NAME)$(SO_NAME)
	$(RM) $(INSTALL_DIR)/lib/$(TARGET_NAME)$(SO_VERSION)
	# $(RM) $(INSTALL_DIR)/include/bm_vpudec_interface.h

$(OBJDIR)/bm_ioctl.o : src/bm_ioctl.c $(MAKEFILE)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
# $(OBJDIR)/jpu_logging.o : src/jpu_logging.c $(MAKEFILE)
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

# $(OBJDIR)/bm_vpudec_interface.o : src/bm_vpudec_interface.c $(MAKEFILE)
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
$(OBJDIR)/bm_vpuenc_interface.o : src/bm_vpuenc_interface.c $(MAKEFILE)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
# $(OBJDIR)/bmjpu_api.o : src/bmjpu_api.c $(MAKEFILE)
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

# $(OBJDIR)/jpeg_common.o : example/jpeg_common.c
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
#
# $(OBJDIR)/jpeg_dec.o : example/jpeg_dec.c
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
#
# $(OBJDIR)/jpeg_enc.o : example/jpeg_enc.c
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
#
# $(OBJDIR)/bmjpurun.o : example/bmjpurun.c
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
#
# $(OBJDIR)/jpeg_multi.o : example/jpeg_multi.c
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
#
# $(OBJDIR)/bm_test.o : example/bm_test.c
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
#
# $(OBJDIR)/vdi_osal.o : src/vdi_osal.c
# 	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/queue.o : example/queue.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/bmvpuenc.o : example/bmvpuenc.c
	$(CC) $(CFLAGS) -I./ -c $< -o $@ -MD -MF $(@:.o=.dep)




$(OBJDIR):
	-mkdir -p $(PRODUCTFORM)
	-mkdir -p $(OBJDIR)
	-mkdir -p $(INSTALL_DIR)

force_dependency:
	true
