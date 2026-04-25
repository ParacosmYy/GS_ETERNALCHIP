# Freertos_Uart

基于 STM32F4 + FreeRTOS 的 UART DMA 收发工程，演示 **乒乓缓冲 + IDLE 线检测** 的非阻塞串口接收方案。

---

## 硬件配置

| 项目 | 配置 |
|------|------|
| MCU | STM32F4 系列 (Cortex-M4) |
| 外部晶振 | HSE 12 MHz |
| 系统时钟 | SYSCLK 96 MHz (PLL: M=12, N=96, P=2) |
| AHB / APB1 / APB2 | 96 MHz / 48 MHz / 96 MHz |
| 调试接口 | SWD |

### 引脚分配

| 引脚 | 功能 | 说明 |
|------|------|------|
| PA9 | USART1_TX | 串口发送 |
| PA10 | USART1_RX | 串口接收 |
| PC13 | GPIO_Output | LED（默认高电平） |

### USART1 参数

| 参数 | 值 |
|------|-----|
| 波特率 | 115200 |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验 | None |
| 流控 | None |

---

## 软件架构

### 技术栈

- **HAL 库**: STM32F4xx HAL Driver
- **RTOS**: FreeRTOS V10.3.1 (CMSIS-OS v2 封装)
- **IDE**: STM32CubeMX 生成 + Keil MDK / VSCode

### DMA 配置

| DMA 通道 | 方向 | 模式 | 用途 |
|----------|------|------|------|
| DMA2 Stream2 (Ch4) | 外设→内存 | **Circular** | USART1_RX |
| DMA2 Stream7 (Ch4) | 内存→外设 | Normal | USART1_TX |

### FreeRTOS 任务

| 任务名 | 栈大小 | 优先级 | 功能 |
|--------|--------|--------|------|
| defaultTask | 128×4 字 | osPriorityNormal | 启动 UART DMA 接收，空闲循环 1s 延时 |

### 串口接收方案（两种模式）

#### DEBUG 模式（编译宏 `DEBUG`）

简单的 **echo 回显**：收到数据后原样发回，单缓冲 256 字节。

#### Release 模式（默认）

**乒乓缓冲 + DMA Circular + IDLE 线检测**：

```
rx_buf[512] = { half_A[256] | half_B[256] }
```

| 事件 | 回调 | 说明 |
|------|------|------|
| HT (Half Transfer) | 前半区填满 | DMA 正在写后半，前半可安全读取 |
| TC (Transfer Complete) | 后半区填满 | DMA 回绕前半，后半可安全读取 |
| IDLE | 总线空闲 | 变长帧结束，按 Size 定位数据 |

DMA Circular 模式自动持续接收，无需手动重启。

### 中断优先级

| 中断 | 优先级 |
|------|--------|
| USART1_IRQn | 5 |
| DMA2_Stream2_IRQn (RX) | 5 |
| DMA2_Stream7_IRQn (TX) | 5 |

---

## 工程目录

```
├── Core/
│   ├── Inc/          # 头文件
│   └── Src/          # 源文件 (main, usart, freertos, gpio, dma, ...)
├── Drivers/          # STM32 HAL / CMSIS 驱动
├── Middlewares/      # FreeRTOS 内核
├── MDK-ARM/          # Keil 工程文件
└── Freertos_Uart.ioc # CubeMX 配置文件
```

## 使用

1. 用 STM32CubeMX 打开 `Freertos_Uart.ioc` 查看或修改配置
2. 编译并烧录到 STM32F4 开发板
3. 连接串口工具 (115200, 8N1) 即可与设备通信
