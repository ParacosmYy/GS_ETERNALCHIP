# LED Key Polling Demo

这是一个基于 STM32F411 的 LED 与按键控制示例工程。

## 功能说明

- 使用 `FreeRTOS` 创建默认任务 `defaultTask`
- 通过 `USART1` 重定向 `printf` 输出调试信息
- 在任务中轮询读取按键状态
- 按下按键后控制 `PC13` 上的 LED 亮灭切换
- 便于学习 `GPIO`、`HAL`、`FreeRTOS` 和基础 BSP 封装写法

## 硬件连接

- LED：PC13
- KEY：PA0
- 按键另一端接 GND
- 按键输入使用内部上拉，按下时为低电平

## 运行逻辑

1. 系统初始化后启动 `defaultTask`
2. 任务中通过 `printf` 输出调试信息
3. 轮询检测 `PA0` 电平
4. 当按键按下时，`PC13` LED 进行亮灭切换
5. 任务中配合 `osDelay(1)` 让出 CPU

## 关键文件

- `Core/Src/main.c`
- `Core/Src/main.h`
- `Core/Src/stm32f4xx_it.c`
- `Core/Src/stm32f4xx_hal_msp.c`
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
3. 串口查看 `USART1` 输出
4. 按下 `KEY`，观察 `LED` 状态变化

## 备注

- 当前工程是按键轮询控制 LED，并不是外部中断回调版本
- 如果后续要改成中断方式，可以再把 `PA0` 配置为 `EXTI0_IRQHandler()` 对应的中断输入
