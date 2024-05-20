########################################################################
#
# The API of JPEG in Linux
#
# CHIP:        bm1688
# PRODUCTFORM: soc
# SUBTYPE:     asic, palladium, fpga
########################################################################

CHIP ?= bm1688
PRODUCTFORM ?= soc
SUBTYPE ?= asic
INSTALL_DIR ?= ./install
ION_DIR ?= ./install
DEBUG ?= 0

ifeq ($(PRODUCTFORM),pcie) # pcie mode
    CROSS_CC_PREFIX =
else ifeq ($(PRODUCTFORM),pcie_mips64) # pcie mode
    CROSS_CC_PREFIX = mips-linux-gnu-
else ifeq ($(PRODUCTFORM),pcie_sw64) # pcie mode
    CROSS_CC_PREFIX = sw_64-sunway-linux-gnu-
else ifeq ($(PRODUCTFORM),pcie_loongarch64) # pcie mode
    CROSS_CC_PREFIX = loongarch64-linux-gnu-
else # pcie_arm64 and soc mode
    CROSS_CC_PREFIX = aarch64-linux-gnu-
endif


CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
AR  = $(CROSS_CC_PREFIX)ar

CFLAGS += -fPIC -Wall -Wl,--fatal-warning
CFLAGS += -I./inc -I../3rdparty/libbmcv/include -I../3rdparty/osdrv

ifeq ($(CHIP), bm1688)
    CFLAGS += -DCHIP_BM1684  # change to CHIP_BM1688?
else
    $(error UNDEFINED CHIP)
endif

ifneq ($(PRODUCTFORM),soc)
    CFLAGS += -DBM_PCIE_MODE
endif

ifeq ($(SUBTYPE), fpga)
    CFLAGS += -DBOARD_FPGA
endif

ifeq ($(DEBUG), 0)
    CFLAGS += -O2
else
    CFLAGS += -g
endif

LDFLAGS += -L$(INSTALL_DIR)/$(PRODUCTFORM)/lib -L../3rdparty/libbmcv/lib/$(PRODUCTFORM)
LIBS = -lpthread -lm -lrt -ldl
#LIBS0 = -lbmlib
LIBS0 =

ifeq ($(PRODUCTFORM),pcie_mips64)
    CFLAGS  += -mips64r2 -mabi=64 -march=gs464e -D_GLIBCXX_USE_CXX11_ABI=0
    LDFLAGS += -Wl,-melf64ltsmip
endif
ifeq ($(PRODUCTFORM), pcie_loongarch64)
    LDFLAGS += -Wl,-melf64loongarch
    LIBS0   += -lstdc++
endif
LIBS += $(LIBS0)


TARGET_NAME=libbmjpeg.so
TESTDEC_NAME=bmjpegdec
TESTENC_NAME=bmjpegenc
TESTMULTI_NAME=bmjpegmulti

TARGET=$(INSTALL_DIR)/$(PRODUCTFORM)/lib/$(TARGET_NAME)
TESTDEC=$(INSTALL_DIR)/$(PRODUCTFORM)/test/$(TESTDEC_NAME)
TESTENC=$(INSTALL_DIR)/$(PRODUCTFORM)/test/$(TESTENC_NAME)
TESTMULTI=$(INSTALL_DIR)/$(PRODUCTFORM)/test/$(TESTMULTI_NAME)

ifneq ($(SO_NAME),)
    TARGET_SONAME=$(TARGET)$(SO_NAME)
    TARGET_SOVERSION=$(TARGET)$(SO_VERSION)
endif

MAKEFILE=Makefile
OBJDIR=./obj
ALLOBJS=*.o
ALLDEPS=*.dep
RM ?= rm -f
CP ?= cp -f
MKDIR ?= mkdir -p
RMDIR ?= rm -rf

LIB_SRC = src/bm_jpeg_interface.c \
          src/bm_ioctl.c \
          src/bm_jpeg_logging.c
LIB_OBJS=$(patsubst %.c,%.o,$(LIB_SRC))
LIB_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(LIB_OBJS)))

DEC_SRC = example/jpeg_dec_test.c \
          example/jpeg_dec_common.c \
          example/helper/md5.c
DEC_OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(DEC_SRC)))
DEC_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(DEC_OBJS)))

ENC_SRC = example/jpeg_enc_test.c \
          example/jpeg_enc_common.c \
          example/helper/md5.c
ENC_OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(ENC_SRC)))
ENC_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(ENC_OBJS)))

MULTI_SRC = example/jpeg_multi_test.c \
            example/bmjpurun.c \
            example/helper/md5.c
MULTI_OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(MULTI_SRC)))
MULTI_PATHS=$(addprefix $(OBJDIR)/,$(notdir $(MULTI_OBJS)))


OBJECTPATHS=$(LIB_PATHS) $(DEC_PATHS) $(ENC_PATHS) $(MULTI_PATHS)

all: prepare $(OBJDIR) $(TARGET) $(TESTDEC) $(TESTENC) $(TESTMULTI)

$(TESTDEC): $(TARGET) $(DEC_PATHS)
	$(CC) $(CFLAGS) -o $@ $(DEC_PATHS) $(LDFLAGS) $(LIBS) -lbmlib -lbmjpeg

$(TESTENC): $(TARGET) $(ENC_PATHS)
	$(CC) $(CFLAGS) -o $@ $(ENC_PATHS) $(LDFLAGS) $(LIBS) -lbmlib -lbmjpeg

$(TESTMULTI): $(TARGET) $(MULTI_PATHS)
	$(CC) $(CFLAGS) -o $@ $(MULTI_PATHS) $(LDFLAGS) $(LIBS) -lbmlib -lbmjpeg -I./example

$(TARGET): $(LIB_PATHS)
	$(CC) $(CFLAGS) -o $@ -shared -Wl,-soname,$(TARGET_NAME)$(SO_NAME) $(LIB_PATHS) $(LIBS0)
ifneq ($(TARGET_SOVERSION), )
	mv $@ $(TARGET_SOVERSION)
	ln -sf $(TARGET_NAME)$(SO_VERSION) $(TARGET_SONAME)
	ln -sf $(TARGET_NAME)$(SO_NAME) $@
endif

-include $(OBJECTPATHS:.o=.dep)

clean:
	$(RM) $(TESTDEC)
	$(RM) $(TESTENC)
	$(RM) $(TESTMULTI)
	$(RM) $(TARGET) $(TARGET_SOVERSION) $(TARGET_SONAME)
	$(RM) $(OBJDIR)/$(ALLDEPS)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RMDIR) $(OBJDIR)

install: $(TARGET) $(TESTDEC) $(TESTENC) $(TESTMULTI)
#	install -d $(INSTALL_DIR)/$(PRODUCTFORM)/bin
#	install -d ${INSTALL_DIR}/$(PRODUCTFORM)/test
#	install -d $(INSTALL_DIR)/$(PRODUCTFORM)/lib
	install -d $(INSTALL_DIR)/$(PRODUCTFORM)/include
#	install $(TESTDEC) $(INSTALL_DIR)/$(PRODUCTFORM)/test
#	install $(TESTENC) $(INSTALL_DIR)/$(PRODUCTFORM)/test
#	install $(TESTMULTI) $(INSTALL_DIR)/$(PRODUCTFORM)/test
#	install $(TARGET)  $(INSTALL_DIR)/$(PRODUCTFORM)/lib
	install -m 0644 inc/bm_jpeg_interface.h $(INSTALL_DIR)/$(PRODUCTFORM)/include
	install -m 0644 inc/bm_jpeg_logging.h $(INSTALL_DIR)/$(PRODUCTFORM)/include
#ifeq ($(TARGET_SOVERSION), )
#	install $(TARGET) $(INSTALL_DIR)/$(PRODUCTFORM)/lib
#else
#	install $(TARGET_SOVERSION) $(INSTALL_DIR)/$(PRODUCTFORM)/lib
#	cp -ap $(TARGET_SONAME) $(TARGET) $(INSTALL_DIR)/$(PRODUCTFORM)/lib
#endif

uninstall:
	$(RM) $(INSTALL_DIR)/$(PRODUCTFORM)/test/$(TESTDEC_NAME)
	$(RM) $(INSTALL_DIR)/$(PRODUCTFORM)/test/$(TESTENC_NAME)
	$(RM) $(INSTALL_DIR)/$(PRODUCTFORM)/test/$(TESTMULTI_NAME)
	$(RM) $(INSTALL_DIR)/$(PRODUCTFORM)/lib/$(TARGET_NAME)
	$(RM) $(INSTALL_DIR)/$(PRODUCTFORM)/lib/$(TARGET_NAME)$(SO_NAME)
	$(RM) $(INSTALL_DIR)/$(PRODUCTFORM)/lib/$(TARGET_NAME)$(SO_VERSION)
	$(RM) $(INSTALL_DIR)/$(PRODUCTFORM)/include/bm_jpeg_interface.h
	$(RM) $(INSTALL_DIR)/$(PRODUCTFORM)/include/bm_jpeg_logging.h

$(OBJDIR)/md5.o : example/helper/md5.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/bm_ioctl.o : src/bm_ioctl.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/bm_jpeg_logging.o : src/bm_jpeg_logging.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/bm_jpeg_interface.o : src/bm_jpeg_interface.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/jpeg_dec_common.o : example/jpeg_dec_common.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/jpeg_enc_common.o : example/jpeg_enc_common.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/bmjpurun.o : example/bmjpurun.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/jpeg_dec_test.o : $(DEC_SRC) $(LIB_PATHS)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/jpeg_enc_test.o : $(ENC_SRC) $(LIB_PATHS)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR)/jpeg_multi_test.o : $(MULTI_SRC) $(LIB_PATHS)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

prepare:
	$(MKDIR) $(INSTALL_DIR)/$(PRODUCTFORM)/lib
	$(MKDIR) $(INSTALL_DIR)/$(PRODUCTFORM)/test

force_dependency:
	true
