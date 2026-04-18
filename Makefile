# Top-level Makefile for STM8S207K8 using SDCC + stm8flash
PROJECT := stm8_blink
SRCDIR := src
BUILDDIR := build

CC := sdcc-sdcc
HEX := $(BUILDDIR)/$(PROJECT).ihx
BIN := $(BUILDDIR)/$(PROJECT).bin
MCU := stm8s207k8
FLASH_TOOL := stm8flash
FLASH_OPTS := -c stlinkv21 -p $(MCU) -w $(HEX)

CFLAGS := -mstm8 --opt-code-size --std-c23 -I$(SRCDIR)
LDFLAGS := -lstm8 
#--stack-loc 0x400

SRCS := $(SRCDIR)/main.c
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.rel,$(SRCS))

.PHONY: all clean flash

all: $(HEX)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.rel: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(HEX): $(OBJS)
	# SDCC produces .ihx automatically when linking; invoke sdcc to link
	$(CC) $(CFLAGS) $(OBJS) -o $(HEX)
	# SDCC places .ihx in current directory build dir; ensure it exists
	@echo "Output: $(HEX)"

clean:
	rm -rf $(BUILDDIR)

flash: all
	$(FLASH_TOOL) $(FLASH_OPTS)

