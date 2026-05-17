# GS_ETERNALCHIP - OTA 加密升级系统

本仓库是一个基于 STM32F411CEUx 的嵌入式 OTA 加密升级项目，采用循序渐进的分支策略管理开发过程。`master` 分支仅保留项目说明文档，不包含应用代码。各功能分支按开发阶段独立维护，最终汇聚到 `OTA_Encrypted_Upgrade` 主开发分支。

## 分支介绍

### `OTA_Encrypted_Upgrade`（主开发分支）

当前项目的主干分支，包含完整的 OTA 加密升级系统。集成了 FreeRTOS 实时操作系统、应用任务框架、BSP 板级驱动（LED/KEY）以及 OTA 加密升级功能。该分支是所有功能演进的最终汇聚点。

### `OTA_Bootloader`（Bootloader 分支）

基于 STM32F411CEUx 的裸机 Bootloader，上电自动读取 OTA 配置（CRC-32 校验）并按状态机跳转到 APP。支持 Slot B→A 复制升级、自动回滚、恢复等待等功能。ROM 限制在 Sector 0（16KB），使用 MicroLIB 精简体积。

### `freertos_uart_output`

FreeRTOS 移植与 UART 通信的基础分支。完成了 FreeRTOS 在 STM32 上的集成，实现了基于 DMA 乒乓缓冲的 UART 数据收发，并添加了 printf 重定向支持，为后续调试输出奠定了基础。

### `LED_Initial`

LED 驱动的初始验证分支。实现了基本的 LED GPIO 输出控制，并通过外部按键中断实现了 LED 状态翻转，属于最基础的硬件交互验证阶段。

### `LED_Driver`

LED 驱动层重构分支。将 LED 控制逻辑从裸机操作抽象为结构化的驱动模块（BSP 层），引入了 OS 条件编译支持，并配置了 VSCode 代码格式化规范（clang-format），提升了代码的可维护性。

### `LED_Bridge_Pattern`

LED 桥接模式分支。在驱动层基础上引入了桥接设计模式（Bridge Pattern），将 LED 的控制接口与具体实现解耦，分离了 BSP 处理层和底层驱动，实现了更灵活的硬件抽象。

### `key_led`

按键与 LED 联调分支。在 FreeRTOS 环境下实现了按键扫描任务与 LED 控制任务的协同工作，引入了依赖注入方式的时间操作接口、任务栈监测模块等功能，是应用层任务框架的雏形。

## Flash 分区

| 区域 | 地址 | 大小 | 说明 |
|------|------|------|------|
| Bootloader | 0x08000000 | 16KB | Sector 0 |
| OTA Config | 0x08004000 | 16KB | Sector 1 |
| Slot A (App) | 0x08008000 | 224KB | Sectors 2-5，运行区 |
| Slot B (Download) | 0x08040000 | 256KB | Sectors 6-7，下载区 |

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
