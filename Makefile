# ============================================================================
# Boot Makefile - STM32F411CEUx Bootloader (ARM GCC)
# ============================================================================
# Usage:
#   make          - build bootloader
#   make clean    - remove build artifacts
#   make size     - show firmware size
# ============================================================================

# ----------------------------- Toolchain ----------------------------------- #
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

# ----------------------------- Target -------------------------------------- #
TARGET   = Boot
BUILDDIR = build

# ----------------------------- MCU ----------------------------------------- #
MCU      = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# ----------------------------- Linker Script ------------------------------ #
LDSCRIPT = STM32F411CEUx_BOOT.ld

# ----------------------------- Defines ------------------------------------- #
DEFS = -DUSE_HAL_DRIVER \
       -DSTM32F411xE

# ----------------------------- Include Paths ------------------------------- #
INCDIRS = Core/Inc \
          Drivers/STM32F4xx_HAL_Driver/Inc \
          Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
          Drivers/CMSIS/Device/ST/STM32F4xx/Include \
          Drivers/CMSIS/Include \
          bsp/uart \
          bsp/flash \
          bootloader \
          Shared \
          Middlewares/SHA256

INCS = $(addprefix -I,$(INCDIRS))

# ----------------------------- Compiler Flags ------------------------------ #
CFLAGS  = $(MCU) $(DEFS) $(INCS)
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -fno-common -fno-builtin -fno-exceptions
CFLAGS += -ffreestanding -nostdlib
CFLAGS += -Os -g3

# ----------------------------- Linker Flags -------------------------------- #
LDFLAGS  = $(MCU) -T$(LDSCRIPT)
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -Wl,-Map=$(BUILDDIR)/$(TARGET).map
LDFLAGS += --specs=nano.specs --specs=nosys.specs
LDFLAGS += -Wl,--start-group -lc -lm -Wl,--end-group

# ----------------------------- Source Files -------------------------------- #
# Startup (GCC version)
STARTUP = Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s

# Core
C_SRCS  = Core/Src/main.c \
          Core/Src/gpio.c \
          Core/Src/usart.c \
          Core/Src/stm32f4xx_it.c \
          Core/Src/stm32f4xx_hal_msp.c \
          Core/Src/stm32f4xx_hal_timebase_tim.c \
          Core/Src/system_stm32f4xx.c

# HAL Drivers
C_SRCS += Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c

# BSP
C_SRCS += bsp/flash/bsp_flash.c \
          bsp/uart/bsp_uart.c

# Bootloader
C_SRCS += bootloader/bootloader.c

# SHA256
C_SRCS += Middlewares/SHA256/sha256.c

# ----------------------------- Object Files -------------------------------- #
OBJDIR   = $(BUILDDIR)/obj
C_OBJS   = $(patsubst %.c,$(OBJDIR)/%.o,$(C_SRCS))
ASM_OBJS = $(OBJDIR)/$(STARTUP:.s=.o)
OBJS     = $(C_OBJS) $(ASM_OBJS)

# ----------------------------- Targets ------------------------------------- #
.PHONY: all clean size

all: $(BUILDDIR)/$(TARGET).elf $(BUILDDIR)/$(TARGET).hex $(BUILDDIR)/$(TARGET).bin size

$(BUILDDIR)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(BUILDDIR)/$(TARGET).hex: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILDDIR)/$(TARGET).bin: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

size: $(BUILDDIR)/$(TARGET).elf
	$(SIZE) --format=berkeley $<

# ----------------------------- Compile Rules ------------------------------- #
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(MCU) -c -o $@ $<

# ----------------------------- Clean --------------------------------------- #
clean:
	rm -rf $(BUILDDIR)

# ----------------------------- Dependencies -------------------------------- #
-include $(C_OBJS:.o=.d)
