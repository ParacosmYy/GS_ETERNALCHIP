# GS_ETERNALCHIP — OTA 加密升级系统

## 项目简介

基于 STM32F411CEUx 的嵌入式 OTA 加密固件升级系统，采用 FreeRTOS 实时操作系统，
支持通过 USART1 + DMA 进行固件传输，内置独立看门狗（IWDG）保障系统可靠性。

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

## 软件架构

```
OTA/
├── Core/                    # CubeMX 生成的应用代码
│   ├── Inc/                 #   头文件 (main.h, gpio.h, usart.h ...)
│   └── Src/                 #   源文件 (main.c, freertos.c, gpio.c ...)
├── bsp/                     # 板级支持包 (BSP) — 可跨芯片复用
│   ├── led/                 #   LED 驱动：操作抽象 + 驱动实例化
│   │   ├── bsp_led.h
│   │   └── bsp_led.c
│   └── key/                 #   KEY 驱动：消抖状态机 + 长短按检测
│       ├── bsp_key.h
│       └── bsp_key.c
├── Drivers/                 # STM32 HAL 驱动库
├── Middlewares/             # FreeRTOS + CMSIS-RTOS V2
├── MDK-ARM/                 # Keil MDK 工程文件
└── OTA.ioc                  # STM32CubeMX 配置文件
```

---

## BSP 模块说明

### LED 驱动 (`bsp/led/`)

- **架构**：操作抽象层 (`led_operations_t`) + 驱动实例层 (`bsp_led_driver_t`)
- **功能**：开 / 关 / 翻转 / 非阻塞定时闪烁 (blink)
- **移植**：换芯片只需实现新的 `led_operations_t`，无需改动上层代码
- **闪烁驱动**：在定时器或任务中周期调用 `BspLed_TimebaseHook()`

### KEY 驱动 (`bsp/key/`)

- **架构**：3 状态消抖状态机 (IDLE → DEBOUNCING → STABLE)
- **功能**：硬件消抖 + 短按检测 + 长按检测 + 回调通知
- **扫描**：在定时器或任务中以 5-10 ms 周期调用 `BspKey_Scan()`
- **事件**：`PRESSED` / `RELEASED` / `SHORT_PRESS` / `LONG_PRESS`

### BSP 使用示例

```c
/* LED */
static const bsp_led_config_t led_cfg = { GPIOC, GPIO_PIN_13, 0 };
static bsp_led_driver_t led;
BspLed_Init(&led, &led_cfg);
BspLed_BlinkStart(&led, 500);

/* KEY */
void OnKey(bsp_key_event_t evt, void *p_user) { /* ... */ }
static const bsp_key_config_t key_cfg = {
    GPIOA, GPIO_PIN_0, 0, 20, 1000, OnKey, NULL
};
static bsp_key_driver_t key;
BspKey_Init(&key, &key_cfg);
```

---

## 开发环境

- **IDE**: Keil MDK-ARM (AC5)
- **配置工具**: STM32CubeMX
- **烧录器**: J-Link / ST-Link
- **添加 BSP 到工程**: Include Path 追加 `../bsp/led;../bsp/key`

---

## 分支说明

| 分支 | 说明 |
|------|------|
| `master` | 项目集成文档 |
| `OTA_Encrypted_Upgrade` | OTA 加密升级主开发分支 |
| `LED_Driver` / `LED_Bridge_Pattern` | LED 驱动开发 |
| `key_led` | 按键 + LED 联调 |
| `freertos_uart_output` | FreeRTOS 串口输出 |

---

## 编码规范

- 函数命名：大驼峰 + 模块前缀 (`BspLed_On`, `BspKey_Scan`)
- 类型后缀：`_t`（如 `bsp_led_driver_t`）
- 指针前缀：`p_`（如 `p_port`, `p_config`）
- 注释风格：GS_Mark 分区注释 (`//*** SECTION ***//`)
- 文档注释：Doxygen (`@brief`, `@param`, `@return`)

---

## OTA 企业级加密升级框架

本项目旨在构建一套面向嵌入式设备的**企业级 OTA 安全升级框架**，通过多层密码学机制协同保障固件传输与烧录的机密性、完整性与真实性。

### 设计目标

| 目标 | 说明 |
|------|------|
| **机密性** | 固件密文传输，防止逆向与窃取 |
| **完整性** | 逐包校验 + 全局校验，防止篡改与损坏 |
| **真实性** | 数字签名验签，确保固件来源可信 |
| **可靠性** | 双区（A/B）设计 + 回滚机制，升级失败可恢复 |
| **传输通道** | UART + DMA，适用于无网络连接的离线设备 |

### 安全协议栈

```
┌─────────────────────────────────────────┐
│          固件数据 (Firmware Bin)         │
├─────────────────────────────────────────┤
│    SHA-256 哈希 — 完整性校验（防篡改）    │
├─────────────────────────────────────────┤
│    ECDSA 签名 — 身份认证（防伪造）        │
├─────────────────────────────────────────┤
│    AES-256-CBC 加密 — 数据加密（防窃取）  │
├─────────────────────────────────────────┤
│    UART + DMA 传输通道                    │
└─────────────────────────────────────────┘
```

### 协议工作流程

1. **握手阶段** — 设备与上位机通过 UART 协商升级会话，交换加密参数与随机挑战值
2. **传输阶段** — 固件经 AES-256-CBC 分包加密后逐帧传输，每帧携带 CRC16 校验
3. **验签阶段** — 全部接收完成后，计算 SHA-256 摘要并用 ECDSA 公钥验证签名
4. **烧录阶段** — 验签通过后写入备份区，设置升级标志并重启
5. **生效阶段** — Bootloader 检测标志，将备份区固件搬运至运行区，启动新固件
6. **回滚机制** — 若新固件启动失败（看门狗超时 / 自检不通过），自动回滚至上一版本

### 固件包格式（规划）

```
+--------+--------+----------+----------+--------+
| Header | Nonce  | Firmware | SHA-256  | ECDSA  |
| 16 B   | 16 B   | N B      | 32 B     | 64 B   |
+--------+--------+----------+----------+--------+
 │        │        │          │          │
 魔术字   AES IV   AES密文    原文摘要    签名值
 版本号
 固件长度
```

### 模块规划

| 模块 | 职责 |
|------|------|
| `ota_transport` | UART 传输协议层，分包收发与重传 |
| `ota_crypto` | AES-256 加解密 + SHA-256 哈希计算 |
| `ota_verify` | ECDSA-P256 签名验证 |
| `ota_flash` | Flash 双区管理，读写擦除 |
| `ota_manager` | 升级状态机，协调上述模块完成全流程 |
| `ota_rollback` | 启动自检 + 失败回滚 |
