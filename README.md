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
│   ├── key/                 #   KEY 驱动：消抖状态机 + 长短按检测
│   │   ├── bsp_key.h
│   │   └── bsp_key.c
│   ├── flash/               #   Flash 操作（双区读写擦除）
│   ├── uart/                #   UART 适配层
│   └── rtt/                 #   SEGGER RTT 调试输出
├── tasks/                   # FreeRTOS 任务入口
│   ├── task_ota.c           #   OTA 升级任务
│   └── task_test_led_key.c  #   LED/KEY 测试任务
├── Drivers/                 # STM32 HAL 驱动库 + CMSIS
├── Middlewares/             # FreeRTOS + 第三方中间件
│   └── Third_Party/
│       ├── FreeRTOS/        #   FreeRTOS 内核 + CMSIS-RTOS V2
│       ├── SEGGER_RTT/      #   SEGGER RTT 调试终端
│       ├── SHA256/          #   SHA-256 哈希库
│       ├── YMODEM/          #   YMODEM 文件传输协议
│       ├── CmBacktrace/     #   Cortex-M 故障追踪
│       └── EasyLogger/      #   轻量级日志框架
├── tools/                   # 开发工具脚本
│   └── gen-clangd/          #   VS Code IntelliSense 配置生成器
│       ├── gen-clangd.ps1   #     主脚本（解析 .uvprojx）
│       └── run.bat          #     双击运行入口
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

- **IDE**: Keil MDK-ARM (AC5 / ARMCC v5)
- **编辑器**: VS Code + clangd / C/C++ 扩展（IntelliSense）
- **配置工具**: STM32CubeMX
- **烧录器**: J-Link / ST-Link
- **工具链**: ARM GCC（仅用于 VS Code IntelliSense，实际编译由 Keil ARMCC 完成）
- **添加 BSP 到工程**: Include Path 追加 `../bsp/led;../bsp/key`

### VS Code IntelliSense 配置

Keil ARMCC 是闭源编译器，VS Code 无法直接调用。项目提供自动化工具，从 `.uvprojx` 提取配置生成 IntelliSense 所需文件：

```bash
# 双击运行，或命令行执行：
tools/gen-clangd/run.bat
```

自动生成：
- `.clangd` — clangd 扩展配置
- `.vscode/c_cpp_properties.json` — Microsoft C/C++ 扩展配置

**特性**：自动解析 include 路径、宏定义、检测 ARM GCC 编译器、添加 `__GNUC__` 解决 CMSIS 兼容性。

**迁移**：将 `tools/gen-clangd/` 文件夹复制到新 Keil 项目，双击 `run.bat` 即可。

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

本项目旨在构建一套面向嵌入式设备的**企业级 OTA 安全升级框架**，通过多层密码学机制协同保障固件传输与烧录的机密性、完整性与真实性。传输通道基于 UART + DMA，适用于无网络连接的离线设备。

### 整体进度

- [x] UART + DMA 通信基础（`freertos_uart_output` 分支已完成）
- [ ] OTA 传输协议层（分包、重传、CRC16 帧校验）
- [ ] AES-256-CBC 固件加解密
- [ ] SHA-256 完整性校验（逐包 + 全局摘要）
- [ ] ECDSA-P256 数字签名验签
- [ ] Flash 双区管理（A/B 分区读写擦除）
- [ ] Bootloader 引导程序
- [ ] 升级状态机（握手→传输→验签→烧录→生效）
- [ ] 升级失败自动回滚机制

### 协议工作流程

```
上位机                              设备端
  │                                   │
  │──── 握手请求（版本/挑战值） ────────>│  1. 协商升级会话
  │<─── 握手响应（设备信息/随机数）─────│
  │                                   │
  │──── 加密固件帧（AES-CBC + CRC）──>│  2. 分包加密传输
  │<─── 帧确认 / 重传 ─────────────────│
  │              ...                  │
  │                                   │
  │──── 传输结束 ──────────────────────>│  3. SHA-256 摘要计算
  │<─── 验签结果 ──────────────────────│  4. ECDSA 签名验证
  │                                   │
  │                                   │  5. 写入备份区 → 重启
  │                                   │  6. Bootloader 搬运 → 启动新固件
  │                                   │  7. 自检失败 → 自动回滚
```

### 固件包格式

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

### 模块实现状态

| 模块 | 职责 | 状态 |
|------|------|------|
| `ota_transport` | UART 传输协议层，分包收发与重传 | ❌ 待开发 |
| `ota_crypto` | AES-256 加解密 + SHA-256 哈希计算 | ❌ 待开发 |
| `ota_verify` | ECDSA-P256 签名验证 | ❌ 待开发 |
| `ota_flash` | Flash 双区管理，读写擦除 | ❌ 待开发 |
| `ota_manager` | 升级状态机，协调上述模块完成全流程 | ❌ 待开发 |
| `ota_rollback` | 启动自检 + 失败回滚 | ❌ 待开发 |
