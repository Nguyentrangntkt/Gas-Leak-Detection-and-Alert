# Makefile phu tro cho ccu6_consumer (STM32F401CCU6).
# Xem ghi chu trong c8t6_producer/Makefile.

TARGET    := ccu6_consumer
BUILD_DIR := build

TOOLCHAIN ?=
CC      := $(TOOLCHAIN)arm-none-eabi-gcc
SZ      := $(TOOLCHAIN)arm-none-eabi-size
OBJCOPY := $(TOOLCHAIN)arm-none-eabi-objcopy

C_SOURCES := \
  Src/main.c \
  Src/bsp_clock.c \
  Src/bsp_gpio.c \
  Src/bsp_systick.c \
  Src/drv_uart.c \
  Src/drv_tim.c \
  Src/hal_output.c \
  Src/hal_switch.c \
  Src/debounce.c \
  Src/drv_iwdg.c \
  Src/protocol.c \
  Src/alarm_fsm.c \
  Src/app_consumer.c \
  Src/stm32f4xx_it.c \
  Src/syscalls.c \
  Src/sysmem.c

ASM_SOURCES := Startup/startup_stm32f401ccux.s

INCLUDES := -IInc \
            -IDrivers/CMSIS/Include \
            -IDrivers/CMSIS/Device/ST/STM32F4xx/Include

DEFS := -DSTM32F401xC -DSTM32F4 -DSTM32F401CCUx

MCU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS := $(MCU) $(DEFS) $(INCLUDES) -Og -g3 -std=gnu11 \
          -Wall -Wextra -Werror -Wshadow -Wconversion \
          -ffunction-sections -fdata-sections

ASFLAGS := $(MCU) -g3 -x assembler-with-cpp

LDSCRIPT := STM32F401CCUX_FLASH.ld
LDFLAGS  := $(MCU) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs \
            -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -lm

OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# File do STM32CubeIDE sinh ra - khong sua, chi noi long canh bao.
VENDOR_OBJS := $(BUILD_DIR)/syscalls.o $(BUILD_DIR)/sysmem.o
$(VENDOR_OBJS): CFLAGS := $(filter-out -Werror -Wconversion,$(CFLAGS))

all: $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/$(TARGET).bin
	$(SZ) $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
-include $(wildcard $(BUILD_DIR)/*.d)
