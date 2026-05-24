# ============================================================================
# APP Makefile - STM32F411CEUx Dual Bank OTA Application (ARM GCC)
# ============================================================================
# Usage:
#   make            - build both Bank A + Bank B firmware
#   make build_a    - build Bank A only (0x08008000)
#   make build_b    - build Bank B only (0x08040000)
#   make clean      - remove build artifacts
#   make size       - show firmware size
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

# ----------------------------- Linker Scripts ------------------------------ #
LDSCRIPT_A = STM32F411CEUx_BANK_A.ld
LDSCRIPT_B = STM32F411CEUx_BANK_B.ld

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
          04_Impl/led \
          04_Impl/key \
          04_Impl/uart \
          04_Impl/rtt \
          04_Impl/flash \
          04_Impl/wdg \
          04_Impl/sys \
          03_Platform/common \
          03_Platform/interface \
          01_App \
          02_Service \
          Vendor/tiny-AES-c \
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
LDFLAGS  = $(MCU)
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
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

# 04_Impl (BSP implementations)
C_SRCS += 04_Impl/key/bsp_key_driver.c \
          04_Impl/led/bsp_led_driver.c \
          04_Impl/uart/bsp_uart_driver.c \
          04_Impl/uart/bsp_uart_handler.c \
          04_Impl/rtt/bsp_rtt_driver.c \
           04_Impl/flash/bsp_flash_driver.c \
           04_Impl/flash/bsp_flash_handler.c \
          04_Impl/wdg/bsp_wdg_driver.c \
          04_Impl/sys/bsp_sys_driver.c

# 03_Platform/common
C_SRCS += 03_Platform/common/ring_buffer.c

# SEGGER RTT
C_SRCS += Middlewares/Third_Party/SEGGER_RTT/SEGGER_RTT.c \
          Middlewares/Third_Party/SEGGER_RTT/SEGGER_RTT_printf.c

# 01_App
C_SRCS += 01_App/task_ota.c \
          01_App/ota_confirm.c

# 02_Service
C_SRCS += 02_Service/ota_transport.c \
          02_Service/ota_verify.c \
          02_Service/ota_led.c \
          02_Service/ota_trace.c \
          02_Service/ota_aes.c \
          02_Service/ota_aes_key.c \
          02_Service/ota_ecdsa.c \
          02_Service/crash_dump.c

# Third-party Middlewares
C_SRCS += Middlewares/Third_Party/micro-ecc/uECC.c \
          Vendor/tiny-AES-c/aes.c \
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
.PHONY: all build_a build_b clean size

all: build_a build_b

# --- Bank A (0x08008000) ---
build_a: $(BUILDDIR)/OTA_A.elf $(BUILDDIR)/OTA_A.hex $(BUILDDIR)/OTA_A.bin

$(BUILDDIR)/OTA_A.elf: $(OBJS) $(LDSCRIPT_A)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -T$(LDSCRIPT_A) -Wl,-Map=$(BUILDDIR)/OTA_A.map -o $@ $(OBJS)

$(BUILDDIR)/OTA_A.hex: $(BUILDDIR)/OTA_A.elf
	$(OBJCOPY) -O ihex $< $@

$(BUILDDIR)/OTA_A.bin: $(BUILDDIR)/OTA_A.elf
	$(OBJCOPY) -O binary $< $@

# --- Bank B (0x08040000) ---
build_b: $(BUILDDIR)/OTA_B.elf $(BUILDDIR)/OTA_B.hex $(BUILDDIR)/OTA_B.bin

$(BUILDDIR)/OTA_B.elf: $(OBJS) $(LDSCRIPT_B)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -T$(LDSCRIPT_B) -Wl,-Map=$(BUILDDIR)/OTA_B.map -o $@ $(OBJS)

$(BUILDDIR)/OTA_B.hex: $(BUILDDIR)/OTA_B.elf
	$(OBJCOPY) -O ihex $< $@

$(BUILDDIR)/OTA_B.bin: $(BUILDDIR)/OTA_B.elf
	$(OBJCOPY) -O binary $< $@

# --- Size ---
size: $(BUILDDIR)/OTA_A.elf $(BUILDDIR)/OTA_B.elf
	@echo "=== Bank A ==="
	$(SIZE) --format=berkeley $(BUILDDIR)/OTA_A.elf
	@echo "=== Bank B ==="
	$(SIZE) --format=berkeley $(BUILDDIR)/OTA_B.elf

# ----------------------------- Compile Rules ------------------------------- #
MKDIR_P = mkdir

$(OBJDIR)/%.o: %.c
	@$(MKDIR_P) $(subst /,\,$(dir $@)) 2>nul || exit 0
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.s
	@$(MKDIR_P) $(subst /,\,$(dir $@)) 2>nul || exit 0
	$(CC) $(MCU) -c -o $@ $<

# ----------------------------- Clean --------------------------------------- #
clean:
	@if exist $(BUILDDIR) rmdir /s /q $(BUILDDIR)

# ----------------------------- Dependencies -------------------------------- #
-include $(C_OBJS:.o=.d)
