include ../../Makefile.def

OUT = ../../bin
NAME = libvg_lite_util.so
NAME_A = libvg_lite_util.a
SOURCES = \
	load_png.c save_png.c fbdev.c vg_lite_util.c \
	$(wildcard libpng-1.6.1/*.c) \
	$(wildcard zlib-1.2.7/*.c)

OBJS = $(SOURCES:.c=.o)

LibpngDir = ./libpng-1.6.1
zlibDir = ./zlib-1.2.7


CFLAGS += -I$(SDK_DIR)/inc -I.
CFLAGS += -Ilibpng-1.6.1 -Izlib-1.2.7
CFLAGS += -DHAVE_FBDEV

LIBS += vg_lite m
LDFLAGS += -L$(SDK_DIR)/drivers -L$(OUT) $(addprefix -l,$(LIBS))
LDFLAGS += -fPIC -shared  -Wl,-z,defs

all : $(NAME_A) $(OUT)/$(NAME)

clean :
	@rm -f $(OUT)/$(NAME) $(NAME_A) $(MW_LIB)/3rd/libvg_lite_util.a $(MW_LIB)/3rd/libvg_lite_util.so $(OBJS)

$(OUT) :
	@mkdir $(OUT)

$(NAME_A): $(OBJS)
	@$(AR) $(ARFLAGS) $(NAME_A) $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M
	@cp $@ $(MW_LIB)/3rd/libvg_lite_util.a

./%.o: ./%.c
	@echo $(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

$(LibpngDir)/%.o: $(LibpngDir)/%.c
	@echo $(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)
	
$(zlibDir)/%.o: $(zlibDir)/%.c
	@echo $(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

$(OUT)/$(NAME) : $(OUT) $(SOURCES)
	@echo $(OBJS)
	@echo $(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)
	@$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)
	@cp $@ $(MW_LIB)/3rd/libvg_lite_util.so