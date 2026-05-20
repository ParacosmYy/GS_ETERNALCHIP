# GS_ETERNALCHIP — OTA 加密升级系统

基于 STM32F411CEUx 的嵌入式 OTA 固件升级系统，FreeRTOS 实时操作系统，UART + DMA 传输。

---

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F411CEUx (Cortex-M4F, 96 MHz, 512KB Flash, 128KB SRAM) |
| LED | PC13，推挽输出，低电平点亮 |
| KEY | PA0，上拉输入，按下低电平 |
| 调试接口 | SWD (SWDIO/SWCLK) |
| 通信接口 | USART1 (PA9/PA10) + DMA |

---

## Flash 分区

| 区域 | 地址 | 大小 | 扇区 |
|------|------|------|------|
| Bootloader | 0x08000000 | 16 KB | Sector 0 |
| OTA Config | 0x08004000 | 16 KB | Sector 1 |
| Slot A (App) | 0x08008000 | 224 KB | Sector 2-5 |
| Slot B (Download) | 0x08040000 | 256 KB | Sector 6-7 |

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
├── Core/                    # CubeMX 生成代码
├── Drivers/                 # STM32 HAL + CMSIS
├── Middlewares/             # FreeRTOS, YMODEM, SHA256, SEGGER_RTT ...
├── bsp/                     # 板级支持包
│   ├── led/                 #   LED 驱动 (操作抽象 + 实例化)
│   ├── key/                 #   KEY 驱动 (消抖状态机 + 长短按)
│   ├── flash/               #   Flash 双区管理 + CRC-32 Config
│   ├── uart/                #   UART DMA + IDLE + Printf
│   └── rtt/                 #   SEGGER RTT 调试输出
├── tasks/                   # FreeRTOS 任务
│   ├── task_ota.c           #   OTA 升级 (YMODEM -> Slot B)
│   ├── ota_confirm.c        #   启动确认
│   └── task_test_led_key.c  #   LED/KEY 测试
├── Shared/                  # Bootloader + App 共享
│   └── ota_config.h         #   分区地址 + 状态枚举 (单一真相源)
├── tools/                   # 开发工具
│   └── gen-clangd/          #   VS Code IntelliSense 配置生成器
└── MDK-ARM/                 # Keil 工程文件
```

---

## BSP 模块

| 模块 | 功能 |
|------|------|
| `bsp/led` | 开/关/翻转/非阻塞闪烁，操作抽象层可跨芯片移植 |
| `bsp/key` | 3 状态消抖状态机，短按/长按检测，回调通知 |
| `bsp/flash` | 扇区擦除、双字节写入、CRC-32 Config 读写、Slot 管理 |
| `bsp/uart` | DMA + IDLE 接收，非阻塞发送，Printf 格式化输出 |
| `bsp/rtt` | SEGGER RTT 调试终端 |

---

## 开发环境

- **IDE**: Keil MDK-ARM (ARMCC v5)
- **编辑器**: VS Code + clangd
- **配置工具**: STM32CubeMX
- **烧录器**: J-Link / ST-Link
- **IntelliSense**: `tools/gen-clangd/run.bat` 自动从 `.uvprojx` 生成配置

---

## 编码规范

- 函数命名: `BspLed_On`, `BspKey_Scan` (大驼峰 + 模块前缀)
- 类型后缀: `_t` | 指针前缀: `p_`
- 注释: GS_Mark 分区 (`//*** SECTION ***//`) + Doxygen

---

## 当前进度

- [x] UART + DMA 通信基础
- [x] LED/KEY BSP 驱动 + FreeRTOS 集成
- [x] Flash 双区管理 (A/B 读写擦除 + CRC-32)
- [x] SHA-256 完整性校验 + YMODEM 传输协议
- [x] OTA 升级任务 (App 侧完整流程)
- [x] App 适配 Bootloader (VTOR / 链接脚本 / 确认机制)
- [x] CmBacktrace 故障诊断 + 代码规范统一
- [x] Bootloader 引导程序 (独立工程)

---

## 企业级演进路线图

### 架构约束：Bootloader 16KB 瓶颈

Bootloader 仅占 Sector 0 (16KB)，无法容纳 AES-256 + ECDSA-P256 等重量级密码学库
（mbedTLS 裁剪版 AES 模块约 8KB，ECDSA 约 15KB，合计远超 16KB）。

**核心策略：将安全验签职责移至 App 侧执行。**

这是企业级 OTA 的标准做法 —— Bootloader 保持精简可信（只做 hash 校验 + 复制 + 跳转），
App 侧拥有 224KB 空间，足以承载完整的密码学流水线。

```
传统方案（不可行）：                企业方案（本路线）：

  PC → [密文] → Bootloader           PC → [密文] → App (运行中)
       | 解密 + 验签                       | AES 解密
       | 16KB 放不下！                     | ECDSA 验签
       v                                    | SHA-256 校验
      复制 + 跳转                           v
                                        [明文] 写入 Slot B
                                             |
                                        通知 Bootloader
                                             v
                                        复制 + 跳转 (无需解密)
```

### Phase 1 — 安全基础：App 侧加密验签

> 目标：固件传输全程密文，防逆向 + 防篡改

| 任务 | 说明 | 预估工作量 |
|------|------|-----------|
| AES-256-CBC 解密模块 | App 侧接收密文固件，解密后写入 Slot B。使用 mbedTLS 裁剪版（仅保留 AES + CBC） | 3-4 天 |
| ECDSA-P256 签名验签 | App 侧验证固件签名（公钥内嵌），防止恶意固件运行 | 3-4 天 |
| 固件打包工具升级 | Python 端增加签名 + 加密流程：`firmware.bin` → 签名 → 加密 → `firmware_ota.bin` | 2 天 |
| 密钥管理方案 | 公钥编译进 App（常量数组），AES 密钥通过安全通道下发或编译时嵌入 | 1 天 |
| OTA Config 扩展 | `ota_config_t` 增加 `fw_signature[64]` + `fw_iv[16]` 字段，版本号升级到 V3 | 1 天 |

**完成后安全链路：**

```
PC 打包: firmware.bin → SHA-256 → ECDSA 签名 → AES 加密 → firmware_ota.bin
                              |
传输: YMODEM → App 接收密文 → AES 解密 → ECDSA 验签 → SHA-256 校验 → 写 Slot B
                                                                        |
Bootloader: 读 Slot B → SHA-256 校验（已有）→ 复制到 Slot A → 跳转
```

### Phase 2 — 产品化体验：LED + 按键交互

> 目标：用 LED 和按键让升级过程可感知、可操控

| 任务 | 说明 |
|------|------|
| LED 升级状态指示 | 快闪 = 传输中，慢闪 = 校验中，常亮 = 成功，N 次快闪 = 错误码 N |
| 按键强制回退 | 长按按键 > 3s 触发固件回退（OTA_STATE_ROLLBACK） |
| Bootloader LED 反馈 | Bootloader 阶段用 LED 闪烁表明状态（复制中 / 回滚中 / 等待恢复） |
| App 自检机制 | App 启动后自检关键外设（UART / Flash），失败则主动进入回退 |

### Phase 3 — 协议层设计

> 目标：在 YMODEM 之上建立应用层协议，支持更多交互

| 任务 | 说明 |
|------|------|
| OTA 协议帧格式 | 帧头（magic + cmd + seq + len）+ payload + CRC-16 |
| 命令定义 | QUERY_VERSION / START_UPGRADE / TRANSFER_DATA / VERIFY + REBOOT / GET_STATUS |
| 传输可靠性 | 丢包重传、超时检测、断点续传（记录已接收偏移） |
| Python 上位机 | 完整的 CLI 工具：查询版本、发起升级、进度显示、签名打包 |

**协议帧结构示例：**

```
+--------+------+-----+------+----------+--------+
| 0xAA55 | CMD  | SEQ | LEN  | PAYLOAD  | CRC16  |
| 2B     | 1B   | 2B  | 2B   | 0~1024B  | 2B     |
+--------+------+-----+------+----------+--------+
```

### Phase 4 — 架构演进：OSAL 抽象 + 五层重构

> 目标：提升可移植性，脱离具体 RTOS 和 MCU 平台

| 任务 | 说明 |
|------|------|
| OSAL 层 | 抽象 mutex / queue / task / delay / timer，隔离 FreeRTOS 调用 |
| 传输接口抽象 | `ota_transport_t` 接口（init / send / recv），当前实现 UART，未来可扩展 |
| 五层架构落地 | Vendor(HAL) → Impl(BSP) → Platform(OSAL) → Service(OTA) → App(Task) |
| 静态内存池 | 替代 malloc，所有 OTA 缓冲区编译时分配，避免碎片 |

### Phase 5 — 高级特性（毕业级）

| 任务 | 说明 |
|------|------|
| 差分升级 (Delta OTA) | bsdiff/hdiffpatch 算法，只传 diff 补丁，节省 60-80% 传输量 |
| 多通道传输 | 在 `ota_transport_t` 抽象上扩展 SPI / I2C / BLE / Wi-Fi |
| 固件版本管理 | semver (major.minor.patch) + 升级兼容性矩阵 |
| 安全启动链 | Bootloader 嵌入根公钥哈希，完整 Chain of Trust |

---

## Flash 分区规划（含 Phase 1 扩展）

当前分区保持不变，Phase 1 仅扩展 OTA Config 结构：

```
Sector 0  (16KB)   Bootloader  — 保持不变，仅 SHA-256 + 复制 + 跳转
Sector 1  (16KB)   OTA Config  — 增加 signature[64] + iv[16]，升级到 V3
Sector 2-5(224KB)  Slot A      — 运行区（含 AES/ECDSA 模块）
Sector 6-7(256KB)  Slot B      — 下载区（存储解密后的明文固件）
```

如未来 Bootloader 需要更多空间（如嵌入安全启动链），可调整为：

```
Sector 0-1 (32KB)  Bootloader  — 扩展后可容纳轻量级验签
Sector 2   (16KB)  OTA Config
Sector 3-5 (208KB) Slot A
Sector 6-7 (256KB) Slot B
```

---

## 参考资源

- [STM32F411 参考手册 (RM0383)](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [mbedTLS 嵌入式裁剪指南](https://mbed-tls.readthedocs.io/en/latest/kb/how-to/reduce-mbedtls-memory-and-code-size/)
- [SUIT Firmware Architecture (IETF)](https://datatracker.ietf.org/wg/suit/about/)
- [MCUboot 开源安全 Bootloader](https://docs.mcuboot.com/)
