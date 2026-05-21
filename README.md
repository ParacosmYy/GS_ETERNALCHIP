# GS_ETERNALCHIP — OTA 加密升级系统

基于 **STM32F411CEUx** 的嵌入式 OTA 固件升级系统，支持 **ECDSA-P256 签名验证** + **SHA-256 完整性校验** + **YMODEM-CRC 传输** + **双 Bank 热切换**。

`master` 分支仅保留项目说明文档，不包含应用代码。各功能分支按开发阶段独立维护，最终汇聚到 `OTA_Encrypted_Upgrade` 主开发分支。

---

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F411CEUx (Cortex-M4F, 96 MHz, 512KB Flash, 128KB SRAM) |
| 通信 | USART1 (PA9/PA10) @ 2Mbps，DMA + IDLE + 环形缓冲区 |
| LED | PC13，推挽输出，低电平点亮 |
| KEY | PA0，上拉输入，按下低电平 |
| 调试 | SWD (SWDIO/SWCLK) + SEGGER RTT |

---

## 系统架构

```
┌─────────────────────────────────────────────────┐
│                   PC 上位机                       │
│  firmware_sign.py (签名) + ota_upgrade.py (传输)  │
└────────────────────┬────────────────────────────┘
                     │ YMODEM-CRC / UART 2Mbps
┌────────────────────▼────────────────────────────┐
│              STM32F411CEUx 设备端                  │
│                                                   │
│  ┌─ Bootloader (16KB, Sector 0) ──────────────┐  │
│  │  状态机 → Slot B→A 复制 → SHA-256 校验 → 跳转  │  │
│  └────────────────────────────────────────────┘  │
│                     ↓ 跳转                        │
│  ┌─ App (Slot A 或 Slot B) ───────────────────┐  │
│  │  FreeRTOS + UART DMA + 环形缓冲区            │  │
│  │  YMODEM 接收 → ECDSA 验签 → SHA-256 → Flash  │  │
│  │  LED 状态指示 + 按键触发 + IWDG 看门狗        │  │
│  └────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────┘
```

---

## Flash 分区

| 区域 | 地址 | 大小 | 说明 |
|------|------|------|------|
| Bootloader | 0x08000000 | 16 KB | Sector 0，裸机引导 |
| OTA Config | 0x08004000 | 16 KB | Sector 1，状态 + SHA-256 + CRC-32 |
| Slot A (App) | 0x08008000 | 224 KB | Sectors 2-5，运行区 |
| Slot B (Download) | 0x08040000 | 256 KB | Sectors 6-7，下载区 |

---

## 核心功能

- **双 Bank 热切换** — 同一源码编译 Bank A / Bank B 两个固件，支持 A→B 和 B→A 互相升级
- **ECDSA-P256 签名验证** — micro-ecc 轻量椭圆曲线库，设备端验签防止恶意固件
- **SHA-256 完整性校验** — 传输后 + Boot 复制后双重校验
- **UART DMA + 环形缓冲区** — DMA + IDLE 接收，ISR push，应用层 pop，CPU 零阻塞
- **YMODEM-CRC 协议** — 128/1024 字节数据包，CRC-16 校验，支持 2Mbps 高速传输
- **FreeRTOS 多任务** — OTA 任务 + 按键扫描 + LED 状态机
- **安全回退** — boot_count 超限自动 ROLLBACK，长按强制回退
- **双构建系统** — Keil ARMCC v5 + GCC arm-none-eabi-gcc 13.3

---

## 分支介绍

| 分支 | 说明 | 阶段 |
|------|------|------|
| `master` | 项目说明文档（本分支） | — |
| `freertos_uart_output` | FreeRTOS 移植 + UART DMA + printf | 基础 |
| `LED_Initial` | LED GPIO 基础验证 | 基础 |
| `LED_Driver` | LED BSP 驱动层抽象 | 驱动 |
| `LED_Bridge_Pattern` | LED 桥接模式重构 | 架构 |
| `key_led` | 按键 + LED 联调 + 任务框架 | 集成 |
| `OTA_Encrypted_Upgrade` | **主开发分支**，完整 OTA 系统 | 产品 |
| `OTA_Bootloader` | Bootloader 独立工程（裸机） | 产品 |

### 分支演进路线

```
freertos_uart_output          LED_Initial
       │                          │
       ▼                          ▼
       └────────► LED_Driver ◄───┘
                      │
                      ▼
              LED_Bridge_Pattern
                      │
                      ▼
                  key_led
                      │
                      ▼
            OTA_Encrypted_Upgrade
                      │
               ┌──────┴──────┐
               ▼             ▼
         APP 工程分支    OTA_Bootloader
        (FreeRTOS)       (裸机引导)
```

---

## 项目目录

```
OTA/
├── APP/                        # 应用工程 (FreeRTOS)
│   ├── App/                    #   任务入口 (task_ota, ota_confirm)
│   ├── Module/ota/             #   OTA 业务模块 (transport, verify, ecdsa, led)
│   ├── bsp/                    #   板级驱动 (uart, flash, led, key, rtt)
│   ├── utils/                  #   通用组件 (ring_buffer)
│   ├── Shared/                 #   Boot + App 共享 (ota_config.h)
│   ├── Middlewares/            #   FreeRTOS + micro-ecc + YMODEM + SHA256 + ...
│   ├── STM32F411CEUx_BANK_A.ld #   GCC 链接脚本 (0x08008000)
│   ├── STM32F411CEUx_BANK_B.ld #   GCC 链接脚本 (0x08040000)
│   ├── Makefile                #   GCC 构建系统
│   └── MDK-ARM/                #   Keil 工程文件
│
├── Boot/                       # Bootloader 工程 (裸机)
│   ├── bootloader/             #   状态机 + JumpToApp + CopySlot
│   ├── bsp/                    #   板级驱动 (uart, flash)
│   ├── Shared/                 #   与 APP 共享 (ota_config.h)
│   ├── Middlewares/SHA256/     #   SHA-256 校验
│   ├── STM32F411CEUx_BOOT.ld   #   GCC 链接脚本 (0x08000000)
│   ├── Makefile                #   GCC 构建系统
│   └── MDK-ARM/                #   Keil 工程文件
│
├── tools/                      # PC 端工具
│   ├── firmware_sign.py        #   ECDSA 签名工具
│   └── ota_upgrade.py          #   YMODEM 上位机
│
└── scripts/                    # 构建脚本 (Windows + Linux)
```

---

## 快速开始

```bash
# 1. 编译 Bootloader
cd Boot && make clean && make

# 2. 编译 App（双 Bank）
cd APP && make clean && make all

# 3. 签名固件
python tools/firmware_sign.py --sign APP/build/OTA_B.bin --keydir tools/keys/ -o tools/upload_B.bin

# 4. OTA 传输
python tools/ota_upgrade.py
```

---

## 开发环境

| 工具 | 说明 |
|------|------|
| Keil MDK-ARM | ARMCC v5 + MicroLIB，主 IDE |
| GCC | arm-none-eabi-gcc 13.3 + nano libc |
| STM32CubeMX | 外设配置 + 代码生成 |
| VS Code + clangd | 辅助编辑，`tools/gen-clangd/` 生成配置 |
| J-Link / ST-Link | 烧录调试 |

---

## 编码规范

- **大括号**: Allman 风格（独占一行）
- **函数命名**: 大驼峰 + 模块前缀 (`BspLed_On`, `OtaEcdsa_Verify`)
- **类型后缀**: `_t` | **指针前缀**: `p_` | **静态变量**: `s_` | **全局变量**: `g_`
- **注释**: GS_Mark 分区注释 + Doxygen
- **禁止**: 三目运算符、VLA、malloc、递归、K&R 风格大括号

---

## 参考资源

- [STM32F411 参考手册 (RM0383)](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [micro-ecc 轻量椭圆曲线库](https://github.com/kmackay/micro-ecc)
- [MCUboot 开源安全 Bootloader](https://docs.mcuboot.com/)
- [FreeRTOS 官方文档](https://www.freertos.org/Documentation)
