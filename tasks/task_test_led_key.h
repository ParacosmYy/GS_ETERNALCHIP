/**
 * @file    task_test_led_key.h
 * @brief   Test module for BSP LED & Key (lightweight, no dedicated RTOS task)
 */

#ifndef TASK_TEST_LED_KEY_H
#define TASK_TEST_LED_KEY_H

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief  初始化 LED 和 KEY 的 BSP 实例 */
    void TaskTestLedKey_Init(void);

    /** @brief  驱动按键扫描和 LED 闪烁（约 10ms 周期调用） */
    void TaskTestLedKey_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_TEST_LED_KEY_H */
