# GS_ETERNALCHIP — OTA 加密升级系统

基于 STM32F411CEUx 的嵌入式 OTA 固件升级系统。FreeRTOS 实时操作系统，
ECDSA-P256 签名验证 + AES-256-CTR 加密传输 + SHA-256 完整性校验。

---

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F411CEUx (Cortex-M4F, 96 MHz, 512KB Flash, 128KB SRAM) |
| LED | PC13，推挽输出，低电平点亮 |
| KEY | PA0，上拉输入，按下低电平 |
| 通信 | USART1 (PA9/PA10) @ 2Mbps，DMA + IDLE + 环形缓冲区 |

## Flash 分区

| 区域 | 地址 | 大小 | 扇区 |
|------|------|------|------|
| Bootloader | 0x08000000 | 16 KB | Sector 0 |
| OTA Config | 0x08004000 | 16 KB | Sector 1 |
| Slot A (App) | 0x08008000 | 224 KB | Sector 2-5 |
| Slot B (Download) | 0x08040000 | 256 KB | Sector 6-7 |

## OTA 流程

```
PC:    firmware.bin → ECDSA-P256 签名 → AES-256-CTR 加密 → upload.bin
设备:  YMODEM 接收 → AES 解密 → ECDSA 验签 → SHA-256 → 写 Slot B → 重启
Boot:  检测 UPGRADE_PENDING → Slot B→A → SHA-256 校验 → 跳转
```

### 安全机制

- **ECDSA-P256**: 私钥签名（PC），公钥验签（设备），防篡改
- **AES-256-CTR**: 固件传输全程密文，防逆向
- **SHA-256**: 写入 Flash 前后双重校验
- **Boot Count**: 连续 crash 达阈值自动回退旧 Bank

### 签名工具链

```bash
python tools/firmware_sign.py --genkey --keydir tools/keys/           # 生成密钥对
python tools/firmware_sign.py --sign OTA.bin --keydir tools/keys/ -o upload.bin  # 签名
python tools/ota_upgrade.py                                            # YMODEM 传输
```

---

## 目录结构

```
APP/
├── 01_App/                   # 应用层 — FreeRTOS 任务入口
├── 02_Service/               # 服务层 — OTA 业务逻辑
├── 03_Platform/              # 平台层 — 硬件抽象接口 (plat_*.h) + 通用组件
│   ├── interface/            #   零 HAL 依赖的类型 + ops 接口定义
│   └── common/               #   ring_buffer 等纯数据结构
├── 04_Impl/                  # 实现层 — BSP Driver + Handler (OPS 函数指针)
│   ├── adaptation/           #   system_adaption.c (HAL/RTOS 胶合层)
│   ├── led/ key/ wdg/ sys/   #   单实例 Driver
│   ├── flash/ uart/          #   Driver + Handler (多实例管理器)
│   └── rtt/                  #   SEGGER RTT 调试输出
├── Core/ Drivers/ Middlewares/ Vendor/   # [CubeMX/第三方，不修改]
├── Shared/                   # [Boot + App 共享: ota_config.h]
├── Makefile                  # GCC 构建 (arm-none-eabi-gcc)
└── MDK-ARM/                  # Keil 工程
```

## 编译

```bash
cd APP && make all        # 编译 Bank A + Bank B
cd APP && make build_a    # 仅 Bank A
cd APP && make build_b    # 仅 Bank B
```

支持 Keil (ARMCC v5) 和 GCC (arm-none-eabi-gcc 13.3) 双构建系统。
