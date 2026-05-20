# ============================================================================
# APP Makefile - STM32F411CEUx OTA Application (ARM GCC)
# ============================================================================
# Usage:
#   make          - build firmware
#   make clean    - remove build artifacts
#   make flash    - flash via ST-Link (optional)
#   make size     - show firmware size
# ============================================================================

# ----------------------------- Toolchain ----------------------------------- #
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

# ----------------------------- Target -------------------------------------- #
TARGET  = OTA
BUILDDIR = build

# ----------------------------- MCU ----------------------------------------- #
MCU      = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# ----------------------------- Linker Script ------------------------------ #
LDSCRIPT = STM32F411CEUx_APP.ld

# ----------------------------- Defines ------------------------------------- #
DEFS = -DUSE_HAL_DRIVER \
       -DSTM32F411xE \
       -DuECC_SUPPORTS_secp160r1=0 \
       -DuECC_SUPPORTS_secp192r1=0 \
       -DuECC_SUPPORTS_secp224r1=0 \
       -DuECC_SUPPORTS_secp256k1=0 \
       -DuECC_SUPPORT_COMPRESSED_POINT=0 \
       -DtraceISR_ENTER\(x\)=\
       -DtraceISR_EXIT_TO_SCHEDULER\(\)=\
       -DtraceISR_EXIT\(\)=

# ----------------------------- Include Paths ------------------------------- #
INCDIRS = Core/Inc \
          Drivers/STM32F4xx_HAL_Driver/Inc \
          Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
          Middlewares/Third_Party/FreeRTOS/Source/include \
          Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 \
          Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F \
          Drivers/CMSIS/Device/ST/STM32F4xx/Include \
          Drivers/CMSIS/Include \
          bsp/led \
          bsp/key \
          bsp/uart \
          bsp/rtt \
          bsp/flash \
          App \
          Module/ota \
          Middlewares/Third_Party/SEGGER_RTT \
          Middlewares/Third_Party/SHA256 \
          Middlewares/Third_Party/YMODEM \
          Middlewares/Third_Party/EasyLogger/inc \
          Middlewares/Third_Party/EasyLogger/port \
          Middlewares/Third_Party/CmBacktrace \
          Middlewares/Third_Party/micro-ecc \
          Shared

INCS = $(addprefix -I,$(INCDIRS))

# ----------------------------- Compiler Flags ------------------------------ #
CFLAGS  = $(MCU) $(DEFS) $(INCS)
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -fno-common -fno-builtin -fno-exceptions
CFLAGS += -ffreestanding -nostdlib
CFLAGS += -O2 -g3

# GCC-specific warnings (stricter than Keil)
CFLAGS += -Wshadow -Wdouble-promotion -Wundef

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
          Core/Src/freertos.c \
          Core/Src/dma.c \
          Core/Src/iwdg.c \
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
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_iwdg.c \
          Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c

# FreeRTOS
C_SRCS += Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
          Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
          Middlewares/Third_Party/FreeRTOS/Source/list.c \
          Middlewares/Third_Party/FreeRTOS/Source/queue.c \
          Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c \
          Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
          Middlewares/Third_Party/FreeRTOS/Source/timers.c \
          Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c \
          Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c \
          Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c

# BSP
C_SRCS += bsp/key/bsp_key.c \
          bsp/led/bsp_led.c \
          bsp/uart/bsp_uart.c \
          bsp/rtt/bsp_rtt.c \
          bsp/flash/bsp_flash.c

# SEGGER RTT
C_SRCS += Middlewares/Third_Party/SEGGER_RTT/SEGGER_RTT.c \
          Middlewares/Third_Party/SEGGER_RTT/SEGGER_RTT_printf.c

# App
C_SRCS += App/task_ota.c \
          App/ota_confirm.c

# Module
C_SRCS += Module/ota/ota_transport.c \
          Module/ota/ota_verify.c \
          Module/ota/ota_ecdsa.c

# Third-party Middlewares
C_SRCS += Middlewares/Third_Party/micro-ecc/uECC.c \
          Middlewares/Third_Party/SHA256/sha256.c \
          Middlewares/Third_Party/YMODEM/ymodem.c \
          Middlewares/Third_Party/EasyLogger/src/elog.c \
          Middlewares/Third_Party/EasyLogger/src/elog_utils.c \
          Middlewares/Third_Party/EasyLogger/port/elog_port.c \
          Middlewares/Third_Party/CmBacktrace/cm_backtrace.c

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
