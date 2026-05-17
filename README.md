# GS_ETERNALCHIP — OTA 加密升级系统

基于 STM32F411CEUx 的嵌入式 OTA 固件升级系统，FreeRTOS 实时操作系统，UART + DMA 传输。

---

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F411CEUx (Cortex-M4F, 96 MHz) |
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

## 进度

- [x] UART + DMA 通信基础
- [x] LED/KEY BSP 驱动 + FreeRTOS 集成
- [x] Flash 双区管理 (A/B 读写擦除 + CRC-32)
- [x] SHA-256 完整性校验 + YMODEM 传输协议
- [x] OTA 升级任务 (App 侧完整流程)
- [x] App 适配 Bootloader (VTOR / 链接脚本 / 确认机制)
- [x] CmBacktrace 故障诊断 + 代码规范统一
- [ ] Bootloader 引导程序 (独立工程)
- [ ] Bootloader YMODEM 恢复模式 + 自动回滚
- [ ] AES-256-CBC 固件加解密
- [ ] ECDSA-P256 数字签名验签
- [ ] OTA 传输协议层 (分包、重传、CRC16)
