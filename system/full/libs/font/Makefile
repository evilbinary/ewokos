include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_font_OBJS = src/font.o

LIB_font = $(TARGET_DIR)/lib/libfont.a

$(LIB_font): $(LIB_font_OBJS)
	$(AR) rT $(LIB_font) $(LIB_font_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_font_OBJS)
	rm -f $(LIB_font)

