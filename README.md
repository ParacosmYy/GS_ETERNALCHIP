# LED Key Interrupt Demo

这是一个基于 STM32F411 的按键中断控制 LED 示例工程。

## 功能说明

- 按下按键后触发外部中断
- 在中断回调中翻转 LED 状态
- 适合用于学习 GPIO 中断、EXTI 和 HAL 回调机制

## 硬件连接

- LED：PC13
- KEY：PA0
- 按键另一端接 GND
- 使用内部上拉输入，按下时产生下降沿中断

## 中断调用链

1. 按键按下，PA0 由高电平变为低电平
2. EXTI0 产生中断请求
3. 进入 `EXTI0_IRQHandler()`
4. 调用 `HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0)`
5. 进入 `HAL_GPIO_EXTI_Callback(GPIO_PIN_0)`
6. 翻转 LED

## 关键文件

- [Core/Src/main.c](Core/Src/main.c)
- [Core/Src/gpio.c](Core/Src/gpio.c)
- [Core/Src/stm32f4xx_it.c](Core/Src/stm32f4xx_it.c)
- [Core/Inc/main.h](Core/Inc/main.h)

## 工程环境

- MCU: STM32F411CEUx
- 开发方式: STM32CubeMX + Keil MDK
- HAL 库: STM32CubeF4

## 使用方法

1. 打开工程
2. 编译并下载到板子
3. 按下 KEY，观察 LED 翻转

## 备注

如果使用的是 PA0 按键，请确保中断函数配置为 `EXTI0_IRQHandler()`，而不是 `EXTI15_10_IRQHandler()`。
