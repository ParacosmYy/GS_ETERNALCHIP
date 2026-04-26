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

---

## Keil 调试：外设寄存器不显示的修复

### 问题描述

Keil MDK 新版本（DFP 3.x.x 起）不再自带 `.sfr` 文件，只提供 `.svd` 文件。调试时 **Peripherals** 菜单或 **System Viewer** 窗口为空，无法查看 USART、GPIO、DMA 等外设寄存器。

### 原因

Keil 调试器依赖 `.sfr` 文件来渲染外设寄存器视图，而新版 Pack 中只有 `.svd`（CMSIS 标准描述格式），需要手动转换。

### 修复步骤

#### 1. 找到 SVD 文件

路径格式：

```
<Keil安装目录>\ARM\PACK\Keil\<芯片系列>_DFP\<版本号>\CMSIS\SVD\<型号>.svd
```

本工程对应文件：

```
E:\Embedded\Keil_v5\ARM\PACK\Keil\STM32F4xx_DFP\3.1.1\CMSIS\SVD\STM32F411.svd
```

#### 2. 找到 SVDConv 工具

位于 Keil 安装目录下的 `UV4` 文件夹：

```
<Keil安装目录>\UV4\SVDConv.exe
```

#### 3. 生成 SFR 文件

在终端中执行：

```cmd
"<Keil安装目录>\UV4\SVDConv.exe" "<SVD文件路径>" --generate=sfr
```

本工程实际命令：

```cmd
"E:\Embedded\Keil_v5\UV4\SVDConv.exe" "E:\Embedded\Keil_v5\ARM\PACK\Keil\STM32F4xx_DFP\3.1.1\CMSIS\SVD\STM32F411.svd" --generate=sfr
```

> 注意：SFR 文件会生成在**当前工作目录**，需要手动复制到 SVD 文件所在目录。

#### 4. 将 SFR 文件放到 SVD 目录

```cmd
copy STM32F411.sfr "E:\Embedded\Keil_v5\ARM\PACK\Keil\STM32F4xx_DFP\3.1.1\CMSIS\SVD\"
```

#### 5. 修改 Keil 工程文件

打开 `MDK-ARM/Freertos_Uart.uvprojx`，找到 `<SFDFile>` 标签：

```xml
<!-- 修改前 -->
<SFDFile>$$Device:STM32F411CEUx$CMSIS\SVD\STM32F411.svd</SFDFile>

<!-- 修改后 -->
<SFDFile>$$Device:STM32F411CEUx$CMSIS\SVD\STM32F411.sfr</SFDFile>
```

#### 6. 验证

1. 重新打开 Keil 工程
2. 进入 Debug 模式
3. 菜单 **Peripherals** 或 **View → System Viewer** 应能看到 USART1、GPIOA、DMA2 等外设寄存器

### 参考链接

- [Keil 官方 SVDConv 文档](https://developer.arm.com/documentation/101407/0537/Utilities/System-View-Description-Converter)
- [CSDN 原文参考](https://blog.csdn.net/weixin_51686526/article/details/137756385)
