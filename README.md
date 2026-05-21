# GS_ETERNALCHIP - OTA 加密升级系统

本仓库是一个基于 STM32F411CEUx 的嵌入式 OTA 加密升级项目，采用循序渐进的分支策略管理开发过程。`master` 分支仅保留项目说明文档，不包含应用代码。各功能分支按开发阶段独立维护，最终汇聚到 `OTA_Encrypted_Upgrade` 主开发分支。

## 分支介绍

### `OTA_Encrypted_Upgrade`（主开发分支）

当前项目的主干分支，包含完整的 OTA 加密升级系统。集成了 FreeRTOS 实时操作系统、UART DMA 环形缓冲区、BSP 板级驱动（LED/KEY/Flash/UART）、ECDSA-P256 签名验证、SHA-256 完整性校验、YMODEM-CRC 传输、双 Bank 热切换。该分支是所有功能演进的最终汇聚点。

### `OTA_Bootloader`（Bootloader 分支）

基于 STM32F411CEUx 的裸机 Bootloader，上电自动读取 OTA 配置（CRC-32 校验）并按状态机跳转到 APP。支持 Slot B→A 复制升级、自动回滚、恢复等待等功能。ROM 限制在 Sector 0（16KB），使用 MicroLIB 精简体积。

### `freertos_uart_output`

FreeRTOS 移植与 UART 通信的基础分支。完成了 FreeRTOS 在 STM32F411 上的集成，实现了基于 DMA 乒乓缓冲的 UART 数据收发，并添加了 printf 重定向支持。

### `LED_Initial`

LED 驱动的初始验证分支。实现了基本的 LED GPIO 输出控制，并通过外部按键中断实现了 LED 状态翻转。

### `LED_Driver`

LED 驱动层重构分支。将 LED 控制逻辑从裸机操作抽象为结构化的驱动模块（BSP 层），引入了 OS 条件编译支持。

### `LED_Bridge_Pattern`

LED 桥接模式分支。在驱动层基础上引入了桥接设计模式（Bridge Pattern），将 LED 的控制接口与具体实现解耦。

### `key_led`

按键与 LED 联调分支。在 FreeRTOS 环境下实现了按键扫描任务与 LED 控制任务的协同工作，引入了依赖注入方式的时间操作接口。

## 分支演进路线

```
freertos_uart_output          LED_Initial
       |                          |
       v                          v
       +--------> LED_Driver <----+
                      |
                      v
              LED_Bridge_Pattern
                      |
                      v
                  key_led
                      |
                      v
            OTA_Encrypted_Upgrade
                      |
                      v
              OTA_Bootloader
```

## 目标平台

- **MCU**: STM32F411CEUx（Cortex-M4F, 96MHz, 512KB Flash, 128KB SRAM）
- **RTOS**: FreeRTOS（APP）/ 裸机（Bootloader）
- **开发环境**: Keil MDK-ARM (ARMCC V5) + STM32CubeMX + VS Code (clangd)
