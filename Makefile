PREFIX := $(MSP_GNU_ROOT)/bin/msp430-elf-
SUPPORT_FILE_DIRECTORY := $(MSP_GNU_ROOT)/include

OUTPUT_DIR := _build
SRC_DIR := src
LINKER_SCRIPT:= linker.ld

DEVICE  = MSP430FR2433

CSRC_FILES += \
  main.c \
  uart.c \
  printf.c

ASM_FILES += \
  spiloop.s

OBJ_FILES = $(CSRC_FILES:.c=.o) $(ASM_FILES:.s=.o)

INC_FOLDERS += \
	$(SUPPORT_FILE_DIRECTORY) \
	./include

INCLUDES = $(INC_FOLDERS:%=-I%)

CFLAGS += -Og -g
CFLAGS += ${INCLUDES}
CFLAGS += -Wall
CFLAGS += -mlarge
CFLAGS += -mmcu=$(DEVICE)

LDFLAGS += ${CFLAGS}
LDFLAGS += -L $(SUPPORT_FILE_DIRECTORY)
LDFLAGS += -Wl,-Map,$(OUTPUT_DIR)/build.map,--gc-sections
LDFLAGS += -T$(LINKER_SCRIPT)

.PHONY: clean all debug

all: ${OUTPUT_DIR}/build.hex ${OUTPUT_DIR}/build.elf

${OUTPUT_DIR}/%.o: ${SRC_DIR}/%.c
	${PREFIX}gcc ${CFLAGS} -c $< -o $@
	@echo "CC $<"

${OUTPUT_DIR}/%.o: ${SRC_DIR}/%.s
	${PREFIX}gcc ${CFLAGS} -c $< -o $@
	@echo "AS $<"

${OUTPUT_DIR}/build.elf: $(OBJ_FILES:%=${OUTPUT_DIR}/%)
	${PREFIX}gcc ${LDFLAGS} $^ -o $@
	@${PREFIX}size $@

${OUTPUT_DIR}/build.hex: ${OUTPUT_DIR}/build.elf
	@${PREFIX}objcopy -O ihex $< $@
	@echo "Preparing $@"

flash: ${OUTPUT_DIR}/build.hex
	@echo Flashing: $<
	mspdebug tilib 'prog $<'

clean:
	rm -rf _build/*

debug: all
	$(GDB) $(DEVICE).out