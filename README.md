# LED Key Polling Demo

这是一个面向工程框架演示的 STM32F411 示例项目，重点展示应用层、BSP 层、HAL 层和 RTOS 之间的组织方式，而不是单一外设的控制逻辑。

## 项目定位

- 这是一个框架优先的基础工程
- 适合观察 STM32CubeMX 生成代码与自定义代码如何协作
- 适合后续继续扩展为按键、LED、串口、定时器、状态机等模块
- 适合作为你后续 BSP 和应用架构的起点

## 框架结构

- `Core`：CubeMX 生成的系统初始化、时钟、外设和 RTOS 入口
- `Drivers`：STM32F4 HAL 与 CMSIS 等底层驱动
- `Middlewares`：FreeRTOS 等中间件支持
- `BSP`：你自己维护的板级封装层，用来承载可复用接口

## 当前能力

- `FreeRTOS` 默认任务已经接入
- `USART1` 已完成 `printf` 重定向，便于调试输出
- `GPIO` 已完成 LED 和按键相关基础配置
- `BSP/LED/driver` 已开始抽象 LED 驱动接口
- `HAL`、`RTOS`、`BSP` 三层关系已经建立

## 设计重点

- 保留 CubeMX 生成代码的稳定性
- 把自定义逻辑放进 `USER CODE` 区和 `BSP` 目录
- 通过接口抽象降低板级和业务逻辑的耦合
- 为后续复用 `time base`、`delay`、`LED ops` 等能力预留空间

## 关键文件

- `Core/Src/main.c`
- `Core/Src/main.h`
- `Core/Src/stm32f4xx_hal_msp.c`
- `Core/Src/stm32f4xx_it.c`
- `Core/Inc/FreeRTOSConfig.h`
- `BSP/LED/driver/inc/bsp_led_driver.h`
- `BSP/LED/driver/src/bsp_led_driver.c`

## 工程环境

- MCU: STM32F411CEUx
- 开发方式: STM32CubeMX + Keil MDK
- RTOS: FreeRTOS
- HAL 库: STM32CubeF4

## 使用方法

1. 打开工程
2. 编译并下载到开发板
3. 通过串口观察运行输出
4. 在此基础上继续扩展自己的 BSP 和业务模块

## 备注

- 这个分支的重点是框架演进，不是单一硬件演示
- README 里的描述应该优先服务于项目观察者快速理解工程分层和扩展性
