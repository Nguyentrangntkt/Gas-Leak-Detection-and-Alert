# Makefile phu tro cho c8t6_producer (STM32F103C8T6).
#
# Day la duong build ngoai IDE de kiem chung nhanh. STM32CubeIDE van build
# project nay bang .cproject nhu binh thuong; hai duong dung chung nguon,
# cung co dinh nghia va cung linker script.
#
#   make            -> build Debug vao build/
#   make TOOLCHAIN=/duong/dan/bin/   neu arm-none-eabi-gcc khong o PATH

TARGET    := c8t6_producer
BUILD_DIR := build

TOOLCHAIN ?=
CC   := $(TOOLCHAIN)arm-none-eabi-gcc
SZ   := $(TOOLCHAIN)arm-none-eabi-size
OBJCOPY := $(TOOLCHAIN)arm-none-eabi-objcopy

C_SOURCES := \
  Src/main.c \
  Src/bsp_clock.c \
  Src/bsp_gpio.c \
  Src/bsp_systick.c \
  Src/drv_adc.c \
  Src/drv_uart.c \
  Src/drv_i2c.c \
  Src/i2c_bus_clear.c \
  Src/drv_lcd1602.c \
  Src/mq2_sensor.c \
  Src/protocol.c \
  Src/app_producer.c \
  Src/stm32f1xx_it.c \
  Src/syscalls.c \
  Src/sysmem.c

ASM_SOURCES := Startup/startup_stm32f103c8tx.s

INCLUDES := -IInc \
            -IDrivers/CMSIS/Include \
            -IDrivers/CMSIS/Device/ST/STM32F1xx/Include

DEFS := -DSTM32F103xB -DSTM32F1 -DSTM32F103C8Tx

MCU := -mcpu=cortex-m3 -mthumb

CFLAGS := $(MCU) $(DEFS) $(INCLUDES) -Og -g3 -std=gnu11 \
          -Wall -Wextra -Werror -Wshadow -Wconversion \
          -ffunction-sections -fdata-sections

ASFLAGS := $(MCU) -g3 -x assembler-with-cpp

LDSCRIPT := STM32F103C8TX_FLASH.ld
LDFLAGS  := $(MCU) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs \
            -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -lm

OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# syscalls.c / sysmem.c do STM32CubeIDE sinh ra - khong sua file nha cung cap,
# chi noi long canh bao rieng cho hai file do.
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
