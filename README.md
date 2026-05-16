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
