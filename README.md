# GS_ETERNALCHIP — OTA 加密升级系统

基于 STM32F411CEUx 的嵌入式 OTA 固件升级系统，FreeRTOS 实时操作系统，
ECDSA-P256 签名验证 + SHA-256 完整性校验 + YMODEM-CRC 传输。

---

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F411CEUx (Cortex-M4F, 96 MHz, 512KB Flash, 128KB SRAM) |
| LED | PC13，推挽输出，低电平点亮 |
| KEY | PA0，上拉输入，按下低电平 |
| 调试接口 | SWD (SWDIO/SWCLK) |
| 通信接口 | USART1 (PA9/PA10) @ 2Mbps，DMA + IDLE + 环形缓冲区 |

---

## Flash 分区

| 区域 | 地址 | 大小 | 扇区 |
|------|------|------|------|
| Bootloader | 0x08000000 | 16 KB | Sector 0 |
| OTA Config | 0x08004000 | 16 KB | Sector 1 |
| Slot A (App) | 0x08008000 | 224 KB | Sector 2-5 |
| Slot B (Download) | 0x08040000 | 256 KB | Sector 6-7 |

---

## OTA 安全升级流程

```
PC 端签名:
  OTA.bin → SHA-256(固件) → 32字节哈希
  私钥 + 哈希 → ECDSA-P256 签名(64字节)
  OTA.bin + 签名 = upload.bin (固件 + 签名包)

设备端验签:
  upload.bin → YMODEM 传输 → 写入 Slot B
  Slot B 末尾 64 字节 = 签名，前部 = 固件
  固件 → SHA-256 → 哈希A
  公钥(Flash 内嵌) + 哈希A + 签名 → ECDSA 验签
  验签通过 → SHA-256 记入 OTA Config → 重启

Bootloader:
  检测 UPGRADE_PENDING → Slot B 复制到 Slot A → SHA-256 二次校验 → 跳转
```

### 双固件构建

同一源码编译两次，仅链接脚本 FLASH ORIGIN 不同：

| 固件 | 链接脚本 | FLASH ORIGIN | 说明 |
|------|---------|-------------|------|
| `OTA_A.bin` | `STM32F411CEUx_BANK_A.ld` | 0x08008000 | 运行在 Bank A |
| `OTA_B.bin` | `STM32F411CEUx_BANK_B.ld` | 0x08040000 | 运行在 Bank B |

```bash
make build_a    # 编译 Bank A 固件
make build_b    # 编译 Bank B 固件
make all        # 编译两个 Bank 固件
```

### 签名工具链

```bash
# 1. 生成密钥对（仅首次）
python tools/firmware_sign.py --genkey --keydir tools/keys/

# 2. 编译固件
#    GCC  编译 → APP/build/OTA_A.bin / OTA_B.bin
#    Keil 编译 → APP/MDK-ARM/OTA/OTA.bin

# 3. 签名
python tools/firmware_sign.py --sign APP/build/OTA_B.bin --keydir tools/keys/ -o tools/upload_B.bin

# 4. OTA 传输
python tools/ota_upgrade.py
#    选择 upload_B.bin（签名包），YMODEM-CRC 发送
```

### 密钥安全

| 文件 | 用途 | 存放位置 | 安全级别 |
|------|------|----------|----------|
| `tools/keys/ec_private.pem` | 签名（PC 端） | 仅 PC，不进固件 | 机密 |
| `tools/keys/ec_public.pem` | 验签参考 | PC 备份 | 公开 |
| `ota_ecdsa.c: s_ecdsa_pubkey[64]` | 验签（设备端） | 编译进固件 | 公开 |

---

## OTA 状态机

```
IDLE --(YMODEM receive)--> UPGRADE_PENDING
                                    |
                              reset |
                                    v
                          Bootloader copies B->A
                                    |
                                    v
                             CONFIRMING ---- boot_count>=3 --> ROLLBACK
                                |                              |
                           App  |                        Recovery YMODEM
                        confirm |
                                v
                            CONFIRMED
```

---

## 目录结构

```
APP/
├── App/                     # FreeRTOS 任务
│   ├── task_ota.c/h         #   OTA 升级任务入口 + 7 阶段状态机
│   └── ota_confirm.c/h      #   启动确认（boot_count 计数）
├── Module/                  # 业务模块层
│   └── ota/
│       ├── ota_transport.c/h  # UART DMA ring buffer 适配 + YMODEM 数据回调
│       ├── ota_verify.c/h     # SHA-256 Flash 哈希校验
│       ├── ota_ecdsa.c/h      # ECDSA-P256 签名验证（micro-ecc）
│       └── ota_led.c/h        # OTA 状态 LED 控制
├── bsp/                     # 板级支持包
│   ├── led/                 #   LED 驱动（操作抽象 + 实例化）
│   ├── key/                 #   KEY 驱动（消抖状态机 + 长短按）
│   ├── flash/               #   Flash 双区管理 + CRC-32 Config
│   ├── uart/                #   UART DMA + IDLE + 环形缓冲区 + Printf
│   └── rtt/                 #   SEGGER RTT 调试输出
├── utils/                   # 通用工具组件
│   └── ring_buffer.c/h      #   环形缓冲区（无硬件依赖，纯数据结构）
├── Core/                    # CubeMX 生成代码
├── Drivers/                 # STM32 HAL + CMSIS
├── Middlewares/             # 中间件
│   └── Third_Party/
│       ├── FreeRTOS/        #   FreeRTOS + GCC/ARM_CM4F port
│       ├── micro-ecc/       #   ECDSA-P256 轻量椭圆曲线库
│       ├── YMODEM/          #   YMODEM-CRC 协议
│       ├── SHA256/          #   SHA-256 哈希
│       ├── CmBacktrace/     #   Cortex-M 故障诊断
│       └── EasyLogger/      #   日志框架
├── Shared/                  # Bootloader + App 共享
│   └── ota_config.h         #   分区地址 + 状态枚举（单一真相源）
├── STM32F411CEUx_BANK_A.ld  # GCC 链接脚本 (0x08008000)
├── STM32F411CEUx_BANK_B.ld  # GCC 链接脚本 (0x08040000)
├── Makefile                 # GCC 构建文件
└── MDK-ARM/                 # Keil 工程文件
```

---

## 数据流架构

### UART DMA + 环形缓冲区

```
PC → UART → DMA Normal + IDLE → ISR push to ring buffer (2048B)
                                        ↓
                               BspUart_ReadByte(pop tail)
                                        ↓
                               YMODEM 解包 → Flash 写入
               CPU 不阻塞，DMA 自动接收，应用层按需取数据
```

### 三层架构

```
Layer 1: utils/ring_buffer     纯数据结构，无硬件依赖
   ↓
Layer 2: bsp/uart              DMA + IDLE 写入 ring buffer (ISR push)
   ↓
Layer 3: Module/ota/transport  从 bsp_uart 读字节 (pop from ring buffer)
```

---

## 开发环境

### 双构建系统

| 项目 | Keil (ARMCC v5) | GCC (arm-none-eabi-gcc 13.3) |
|------|-----------------|-------------------------------|
| 构建工具 | Keil MDK-ARM | Makefile |
| 编译器 | ARMCC v5 + MicroLIB | ARM GCC 13.3 + nano libc |
| 链接脚本 | .sct (Scatter File) | .ld (Linker Script) |
| FreeRTOS port | portable/RVDS/ARM_CM4F | portable/GCC/ARM_CM4F |
| 烧录 | Keil 内置 | ST-Link / OpenOCD |

### 编译命令

```bash
# GCC 编译（APP — 双 Bank）
cd APP && make clean && make all

# GCC 编译（Boot）
cd Boot && make clean && make

# Windows 双击编译
scripts/windows/build_all.bat

# Linux / Git Bash
bash scripts/linux/build_all.sh
```

### 其他工具

- **配置工具**: STM32CubeMX
- **烧录器**: J-Link / ST-Link
- **编辑器**: VS Code + clangd
- **IntelliSense**: `tools/gen-clangd/run.bat` 自动从 `.uvprojx` 生成配置

---

## 编码规范

- 大括号: Allman 风格（独占一行）
- 函数命名: `BspLed_On`, `OtaEcdsa_Verify`（大驼峰 + 模块前缀）
- 类型后缀: `_t` | 指针前缀: `p_` | 静态变量: `s_`
- 注释: GS_Mark 分区 (`//*** SECTION ***//`) + Doxygen
- 禁止: 三目运算符、VLA、malloc、递归
- 变量声明: 函数顶部，声明即初始化

---

## 当前进度

- [x] UART + DMA 通信基础
- [x] LED/KEY BSP 驱动 + FreeRTOS 集成
- [x] Flash 双区管理 (A/B 读写擦除 + CRC-32)
- [x] SHA-256 完整性校验 + YMODEM-CRC 传输协议
- [x] OTA 升级任务 (App 侧完整流程)
- [x] App 适配 Bootloader (VTOR / 链接脚本 / 确认机制)
- [x] CmBacktrace 故障诊断 + 代码规范统一
- [x] Bootloader 引导程序 (独立工程)
- [x] ECDSA-P256 固件签名验证 (micro-ecc)
- [x] DJI 风格分层重构 (tasks → App + Module/ota)
- [x] ARM GCC 双构建系统 (Makefile + 链接脚本)
- [x] 编译脚本 (Windows PS + Linux Bash + 资源统计)
- [x] 环形缓冲区基础组件 (utils/ring_buffer)
- [x] UART DMA + 环形缓冲区接收模式
- [x] 双 Bank 固件构建 (Bank A + Bank B)
- [x] 固件版本号管理 (MAJOR.MINOR.PATCH)
- [x] LED 状态指示 + 按键强制回退
- [x] 上位机串口连接/断开管理

---

## 企业级演进路线图

### Phase 1 — AES-256 加密传输（固件防逆向）

> 目标：固件传输全程密文，防逆向 + 防篡改

Bootloader 仅 16KB，无法容纳 AES + ECDSA。企业级做法：App 侧做解密验签，Bootloader 保持精简。

```
PC: firmware.bin → ECDSA签名 → AES-256加密 → firmware_ota.bin
设备: YMODEM接收 → AES解密 → ECDSA验签 → SHA-256 → 写Slot B
Boot: 复制 Slot B→A → SHA-256校验 → 跳转
```

| 任务 | 说明 |
|------|------|
| AES-256-CBC 解密 | mbedTLS 裁剪版，App 侧解密后写 Slot B |
| 固件打包工具升级 | Python 端: 签名 → 加密 → 打包 |
| 密钥管理 | AES 密钥编译时嵌入或安全通道下发 |

### Phase 2 — 公共技术组件

| 任务 | 说明 |
|------|------|
| 栈水位监控 | FreeRTOS 任务栈使用率运行时检测 |
| 多任务看门狗 | 任务健康监控 + 超时复位 |
| 静态内存池 | 替代 malloc，固定大小块分配 |
| OSAL 抽象层 | 隔离 FreeRTOS，统一 IPC 接口 |

### Phase 3 — 应用层协议

| 任务 | 说明 |
|------|------|
| OTA 协议帧 | magic + cmd + seq + len + payload + CRC-16 |
| 命令集 | QUERY_VERSION / START_UPGRADE / VERIFY / GET_STATUS |
| 断点续传 | 记录已接收偏移，支持中断后继续 |

### Phase 4 — 五层架构

| 任务 | 说明 |
|------|------|
| 平台类型/错误码 | platform_type.h + platform_error.h 统一抽象 |
| 传感器 ops 接口 | sensor_hal_interface.h，驱动可插拔 |
| 传输接口抽象 | ota_transport_t 接口，支持 UART/SPI/BLE 扩展 |

### Phase 5 — 高级特性

| 任务 | 说明 |
|------|------|
| 差分升级 | bsdiff/hdiffpatch，只传 diff 节省 60-80% 传输量 |
| 多通道传输 | BLE / Wi-Fi / SPI 扩展 |
| 安全启动链 | Bootloader 嵌入根公钥哈希，完整 Chain of Trust |

---

## 参考资源

- [STM32F411 参考手册 (RM0383)](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [micro-ecc 轻量椭圆曲线库](https://github.com/kmackay/micro-ecc)
- [mbedTLS 嵌入式裁剪指南](https://mbed-tls.readthedocs.io/en/latest/kb/how-to/reduce-mbedtls-memory-and-code-size/)
- [MCUboot 开源安全 Bootloader](https://docs.mcuboot.com/)
- [SUIT Firmware Architecture (IETF)](https://datatracker.ietf.org/wg/suit/about/)
