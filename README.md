# GS_ETERNALCHIP — OTA 加密升级系统

本仓库是一个基于 STM32F411CEUx 的嵌入式 OTA 加密升级项目，采用循序渐进的分支策略管理开发过程。`master` 分支仅保留项目说明文档，不包含应用代码。各功能分支按开发阶段独立维护，最终汇聚到 `OTA_Encrypted_Upgrade` 主开发分支。

## 分支介绍

| 分支 | 说明 |
|------|------|
| `OTA_Encrypted_Upgrade` | **主开发分支** — 完整 OTA 加密升级系统（FreeRTOS + ECDSA-P256 + SHA-256 + YMODEM + 双 Bank） |
| `OTA_Bootloader` | 裸机 Bootloader（≤16KB）— Slot 复制 + SHA-256 校验 + 跳转 + 自动回滚 |
| `key_led` | 按键 + LED 联调 — 消抖状态机 + 长短按 + 依赖注入时间接口 |
| `LED_Bridge_Pattern` | 桥接模式重构 — LED 控制接口与具体实现解耦 |
| `LED_Driver` | LED BSP 驱动层抽象 — 结构化驱动模块 + OS 条件编译 |
| `LED_Initial` | LED GPIO 基础验证 — 输出控制 + 按键中断翻转 |
| `freertos_uart_output` | FreeRTOS 移植 + UART DMA + printf 重定向 |
