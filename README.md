# key_led — STM32F4 FreeRTOS 按键与 UART 应用

基于 **STM32F407** + **FreeRTOS** + **HAL 库** 的嵌入式应用项目，实现按键扫描、LED 控制、UART DMA 收发及任务栈监测。

## 硬件连接

| 外设 | 引脚 | 模式 | 说明 |
|---|---|---|---|
| **LED** | PC13 | 推挽输出 | 低电平点亮 |
| **Key** | PA0 | 上拉输入 | 按下为低电平 |
| **USART1_TX** | PA9 | 复用推挽 (AF7) | UART 发送 |
| **USART1_RX** | PA10 | 复用推挽 (AF7) | UART 接收 |

### USART1 配置

| 参数 | 值 |
|---|---|
| 波特率 | 115200 |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | None |
| DMA RX | DMA2_Stream2 / Channel 4 |
| DMA TX | DMA2_Stream7 / Channel 4 |

## 项目结构

```
key_led/
├── APP/                        # 应用层（用户代码）
│   ├── app_key_task.c/h        #   按键扫描任务 + 消息队列
│   ├── app_uart_task.c/h       #   UART 收发任务 + 心跳
│   └── app_stack_monitor.c/h   #   任务栈 High Water Mark 监测
├── BSP/                        # 硬件抽象层（BSP）
│   ├── bsp_key.c/h             #   按键驱动（依赖注入延时接口）
│   └── bsp_led.c/h             #   LED 驱动（On/Off/Toggle）
├── Core/                       # STM32CubeMX 生成代码
│   ├── Inc/                    #   头文件（main.h, FreeRTOSConfig.h 等）
│   └── Src/                    #   源文件（main.c, freertos.c, usart.c 等）
├── Drivers/                    # STM32 HAL 库
├── Middlewares/                # FreeRTOS 内核
├── MDK-ARM/                    # Keil MDK 工程文件
├── EWARM/                      # IAR 工程文件
└── key_led.ioc                 # STM32CubeMX 配置文件
```

## 软件架构

### FreeRTOS 任务

| 任务 | 栈大小 | 优先级 | 功能 |
|---|---|---|---|
| `defaultTask` | 128 words (512B) | Normal | 空闲占位 |
| `keyTask` | 128 words (512B) | Normal | 按键扫描 + 消息队列发送 |
| `uartTask` | 256 words (1KB) | Normal | UART DMA 收发 + 心跳 + 栈监测 |

### 数据流

```
PA0 按键 ──► BSP_Key_Scan() ──► xQueue ──► uartTask ──► printf("LED Toggle")
                                                        │
PA9/PA10 ◄──► DMA + 空闲中断 ◄──► uartTask ──► echo 回显
```

### BSP 层依赖注入

BSP 层通过函数指针接口实现 RTOS 解耦：

```
APP 层 (知道 FreeRTOS)                    BSP 层 (RTOS 无关)
┌──────────────────────┐                ┌──────────────────────┐
│  bsp_key_time_ops_t  │─── 注入 ────►  │  gp_time_ops         │
│  {                   │                │  ├─ pf_delay_ms()    │
│    .pf_delay_ms      │ = osDelay      │  └─ pf_get_tick_ms() │
│    .pf_get_tick_ms   │ = GetTickCount │                      │
│  }                   │                │  BSP_Key_Scan()      │
└──────────────────────┘                └──────────────────────┘
```

裸机复用时只需替换注入的实现：

```c
static const bsp_key_time_ops_t bare_metal_ops = {
    .pf_get_tick_ms = HAL_GetTick,
    .pf_delay_ms    = HAL_Delay,
};
```

## 功能特性

### UART 收发

- **DMA + 空闲中断** 接收任意长度数据
- Echo 回显：收到什么发回什么
- printf 重定向到 USART1（MicroLib + fputc 重定向）

### 心跳输出

通过编译期开关控制（默认关闭）：

```c
/* app_uart_task.c */
#define UART_DEBUG_ENABLE_HEARTBEAT  0   /* 0=关闭  1=每秒输出 heartbeat */
```

### 栈 High Water Mark 监测

启动后 5 秒自动打印各任务栈使用情况，通过编译期开关控制：

```c
/* app_stack_monitor.h */
#define STACK_MONITOR_ENABLE  1   /* 0=完全剔除零开销  1=启用 */
```

串口输出示例：

```
=== Stack High Water Mark ===
defaultTask : total=128 words, hwm=105 words, used_peak=23 words
keyTask     : total=128 words, hwm=101 words, used_peak=27 words
uartTask    : total=256 words, hwm=182 words, used_peak=74 words
```

## 开发环境

| 工具 | 版本/说明 |
|---|---|
| MCU | STM32F407 系列 |
| IDE | Keil MDK-ARM v5 |
| 配置工具 | STM32CubeMX |
| RTOS | FreeRTOS V10.3.1 (CMSIS-RTOS2 封装) |
| 调试器 | ST-Link / J-Link |
| 串口工具 | 115200 8N1 |

### 编译步骤

1. 使用 STM32CubeMX 打开 `key_led.ioc` 生成代码（如需修改外设配置）
2. 使用 Keil MDK 打开 `MDK-ARM/key_led.uvprojx`
3. 勾选 **Options → Target → Use MicroLib**（printf 重定向需要）
4. 编译并下载

## 编码规范

遵循项目自定义嵌入式 C 编码规范：

- BSP/APP 分层架构，BSP 层零 RTOS 依赖
- 所有用户代码写在 `USER CODE BEGIN/END` 标记对内
- 全局变量 `g_` 前缀、指针 `p_` 前缀、函数指针 `pf_` 前缀
- FreeRTOS 原生 API（`xQueueSend` / `xQueueReceive`），禁止 CMSIS-OS2 队列封装
- 所有函数必须有 Doxygen 注释（`@brief` / `Steps` / `@param[in]` / `@return`）
- 禁止 `malloc` / `free` / VLA / 递归

## License

MIT License

## Author

GS_Mark
