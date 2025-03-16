CC      = gcc
OBJCOPY	= objcopy
CFLAGS  = -O3 -Wall -g
LDFLAGS = -lpthread


SOURCES = \
	mem.c \
	dev_UART.c \
	dev_ATH79.c \
	dev_gmac.c \
	dev_SPI.c \
	dev_SPIFlash.c \
	dev_SPIFlashParam.c \
	env_unix.c \
	logfile.c \
	misc.c \
	mainloop32.c \
	cp0.c \
	tlb.c \
	exception.c \
	terminal.c \
	monitor.c \
	rc.c \
	rc_helper.c \
	rc_mips16e_x64.c \
	rc_mips32r2_x64.c \
	main.c

TARGET    = exmips
SRC_DIR   = src
OBJ_DIR   = obj
SRCS      = $(addprefix $(SRC_DIR)/,$(SOURCES))
OBJS      = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(SOURCES)))


all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ 

depend: .depend

.depend: $(SRCS)
	@mkdir -p $(OBJ_DIR)
	$(CC) -MM $(SRCS) | sed 's/^\([a-zA-Z0-9_]\+\.o\):/obj\/\1:/g' > .depend

-include .depend

clean:
	rm -rf $(OBJ_DIR) $(TARGET) .depend

.PHONY: all depend clean
