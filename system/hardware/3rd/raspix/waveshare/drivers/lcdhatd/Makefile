include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

LCDHATD_OBJS = lcdhatd.o fbd.o

LCDHATD = $(TARGET_DIR)/drivers/raspix/lcdhatd

$(LCDHATD): $(LCDHATD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(LCDHATD_OBJS) -o $(LCDHATD) $(LDFLAGS) -larch_bcm283x -lfbd -lupng -lgraph -lbsp -lsconf $(EWOK_LIBC)

clean:
	rm -f $(LCDHATD_OBJS)
