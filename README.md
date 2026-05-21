# GS_ETERNALCHIP — Bootloader

基于 STM32F411CEUx 的裸机 Bootloader，上电自动读取 OTA 配置，完成固件升级拷贝后跳转到 APP。
精简设计（≤ 16KB Flash），仅负责 SHA-256 校验 + Slot 复制 + 跳转，不做密码学运算。

---

## 功能

- 读取 Sector 1 的 OTA Config（CRC-32 校验）
- 按 OTA 状态机分支处理：直接跳转 / Slot B→A 复制升级 / 回滚 / 等待恢复
- Slot B→A 复制后二次 SHA-256 校验，确保数据完整性
- UART 串口状态输出（2Mbps，PA9/PA10）
- Cortex-M 标准跳转（VTOR 重定位 + MSP 设置）
- 固件版本号显示（Banner 打印 Boot 版本）

---

## Flash 分区

| 区域 | 地址 | 大小 | 说明 |
|------|------|------|------|
| Bootloader | 0x08000000 | 16KB | Sector 0，本工程 |
| OTA Config | 0x08004000 | 16KB | Sector 1，状态 + SHA-256 + CRC-32 |
| Slot A (App) | 0x08008000 | 224KB | Sectors 2-5，运行区 |
| Slot B (Download) | 0x08040000 | 256KB | Sectors 6-7，下载区 |

---

## 目录结构

```
Boot/
├── bootloader/              # Bootloader 应用逻辑
│   ├── bootloader.c/h       #   状态机 + JumpToApp + CopySlot + Recovery
├── bsp/                     # 板级驱动（裸机，无 FreeRTOS 依赖）
│   ├── uart/                #   UART blocking TX + Printf
│   └── flash/               #   Flash 操作 + CRC-32 Config 读写
├── Shared/                  # 与 APP 共享
│   └── ota_config.h         #   分区地址 + 状态枚举（单一真相源）
├── Middlewares/             # 中间件
│   └── SHA256/              #   SHA-256 哈希校验
├── Core/                    # CubeMX 生成代码
├── Drivers/                 # HAL + CMSIS
├── STM32F411CEUx_BOOT.ld    # GCC 链接脚本
├── Makefile                 # GCC 构建文件
└── MDK-ARM/                 # Keil 工程文件 + Scatter File
```

---

## 启动流程

```
上电 → Boot_Run()
  → 读 OTA Config (CRC-32 校验)
  → IDLE / CONFIRMED → 直接跳转 Slot A
  → UPGRADE_PENDING  → 擦除 Slot A → 复制 Slot B→A → SHA-256 校验 → 跳转
  → CONFIRMING       → boot_count++ → 跳转（≥3 则 ROLLBACK）
  → ROLLBACK / 无APP → Boot_RecoveryLoop()（LED 闪烁等待恢复）
```

### 升级日志示例

```
[BOOT] ========================================
[BOOT]   GS_ETERNALCHIP Bootloader
[BOOT]   STM32F411CEUx @ 96MHz (Dual Bank)
[BOOT]   Boot : V2.0.0
[BOOT] ========================================
[BOOT] State=UPGRADE_PENDING, boot_count=0, fw_size=42828
[BOOT] Erasing Slot A ...
[BOOT] Copying Slot B -> Slot A (42828 bytes) ...
[BOOT] Copy complete. Verifying ...
[BOOT] Verify OK.
[BOOT] SHA-256 verifying (42828 bytes) ...
[BOOT] SHA-256: 9B63D617...F7A9
[BOOT] SHA-256 OK.
[BOOT] Upgrade done. Jumping to app.
```

---

## 开发环境

### 双构建系统

| 项目 | Keil (ARMCC v5) | GCC (arm-none-eabi-gcc 13.3) |
|------|-----------------|-------------------------------|
| 构建工具 | Keil MDK-ARM | Makefile |
| 编译器 | ARMCC v5 + MicroLIB | ARM GCC 13.3 + nano libc |
| 链接脚本 | .sct (Scatter File) | .ld (Linker Script) |
| 烧录 | Keil 内置 | ST-Link / OpenOCD |

### 编译命令

```bash
# GCC 编译
cd Boot && make clean && make

# Windows 双击编译
scripts/windows/build_boot.bat

# 资源约束
#   Flash ≤ 16KB (Sector 0)，当前约 15.3KB (93.4%)
#   RAM = 128KB 共享
```

### 其他工具

- **配置工具**: STM32CubeMX
- **烧录器**: J-Link / ST-Link
- **编辑器**: VS Code + clangd

---

## 资源约束

Bootloader 受限于 Sector 0 的 16KB Flash，设计原则：

- **不做密码学运算** — ECDSA/AES 在 App 侧执行
- **最小化依赖** — 裸机运行，不依赖 FreeRTOS
- **仅做可信操作** — SHA-256 校验 + 扇区复制 + 跳转
- **安全回退** — boot_count 超限自动 ROLLBACK，Recovery 模式等待恢复

---

## 编码规范

与 APP 工程一致：Allman 大括号，大驼峰 + 模块前缀，指针 `p_` 前缀，GS_Mark 分区注释。
