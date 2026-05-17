# GS_ETERNALCHIP — Bootloader

基于 STM32F411CEUx 的裸机 Bootloader，上电自动读取 OTA 配置并跳转到 APP。

---

## 功能

- 读取 Sector 1 的 OTA Config（CRC-32 校验）
- 按 OTA 状态机分支处理：直接跳转 / Slot B→A 复制升级 / 回滚 / 等待恢复
- UART 串口状态输出（115200，PA9/PA10）
- Cortex-M 标准跳转（VTOR 重定位 + MSP 设置）

## 目录结构

```
boot/
├── bootloader/        # Bootloader 应用逻辑
│   ├── bootloader.h
│   └── bootloader.c   # 状态机 + JumpToApp + Boot_CopySlot
├── bsp/               # 板级驱动
│   ├── uart/          #   简化版 UART（blocking TX + Printf）
│   └── flash/         #   Flash 操作（无 FreeRTOS 依赖）
├── Core/              # CubeMX 生成代码
├── Drivers/           # HAL + CMSIS
└── MDK-ARM/           # Keil 工程文件 + Scatter File
```

## Flash 分区

| 区域 | 地址 | 大小 | 说明 |
|------|------|------|------|
| Bootloader | 0x08000000 | 16KB | Sector 0，本工程 |
| OTA Config | 0x08004000 | 16KB | Sector 1，状态 + CRC |
| Slot A (App) | 0x08008000 | 224KB | Sectors 2-5，运行区 |
| Slot B (Download) | 0x08040000 | 256KB | Sectors 6-7，下载区 |

## 启动流程

```
上电 → Boot_Run()
  → 读 OTA Config
  → IDLE / CONFIRMED → 直接跳转 Slot A
  → UPGRADE_PENDING  → 复制 Slot B→A → 跳转
  → CONFIRMING       → boot_count++ → 跳转（≥3 则 ROLLBACK）
  → ROLLBACK / 无APP → Boot_RecoveryLoop()（LED 闪烁等待恢复）
```

## 开发环境

- **IDE**: Keil MDK-ARM (ARMCC V5) + MicroLIB
- **配置工具**: STM32CubeMX
- **编辑器**: VS Code + clangd
- **编译产物**: ROM 需 ≤ 16KB

## 编码规范

与 APP 工程一致：大驼峰 + 模块前缀，指针 `p_` 前缀，GS_Mark 分区注释。
