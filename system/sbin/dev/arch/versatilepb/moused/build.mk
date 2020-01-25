VERSATILE_MOUSED_OBJS = $(ROOT_DIR)/sbin/dev/arch/versatilepb/moused/moused.o

VERSATILE_MOUSED = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/versatilepb/moused

PROGS += $(VERSATILE_MOUSED)
CLEAN += $(VERSATILE_MOUSED_OBJS)

$(VERSATILE_MOUSED): $(VERSATILE_MOUSED_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(VERSATILE_MOUSED_OBJS) -o $(VERSATILE_MOUSED) $(LDFLAGS) -lewokc
