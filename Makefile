TARGET_EXEC ?= wifimon.bin
TARGET_ELF ?= $(TARGET_EXEC:.bin=.elf)

BUILD_DIR ?= ./build

SRCS := startup/entry.S startup/start_load.c startup/system_bl602.c \
	hal/src/bl602_glb.c hal/src/bl602_uart.c hal/src/bl602_common.c \
	hal/src/bl602_hbn.c hal/src/bl602_ef_ctrl.c hal/src/bl602_pds.c \
	board.c intc.c mac.c main.c phy.c rf.c rf_data.c rx.c utility.c

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find . -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CROSS := riscv64-unknown-elf-
CC := $(CROSS)gcc

CFLAGS := -DARCH_RISCV $(INC_FLAGS) -march=rv32imafc -mabi=ilp32f -Os -g3 \
	-fno-jump-tables -fshort-enums -fno-common -fms-extensions -ffunction-sections \
	-fdata-sections -fstrict-volatile-bitfields -std=c99 -MMD -MP
LDFLAGS := -Tflash.ld -Wl,-Map=$(BUILD_DIR)/$(TARGET_EXEC).map -Wl,--cref -Wl,--gc-sections \
	-nostartfiles -g3 -fms-extensions -ffunction-sections -fdata-sections \
	--specs=nano.specs -march=rv32imafc -mabi=ilp32f -Wl,-m,elf32lriscv

$(BUILD_DIR)/$(TARGET_ELF): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/$(TARGET_EXEC): $(BUILD_DIR)/$(TARGET_ELF)
	$(CROSS)objcopy -Obinary $< $@
	$(CROSS)size $<

$(BUILD_DIR)/%.S.o: %.S
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR ?= mkdir -p
