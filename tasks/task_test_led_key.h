/**
 * @file    task_test_led_key.h
 * @brief   Test module for BSP LED & Key (lightweight, no dedicated RTOS task)
 *
 * Usage:
 *   Call TaskTestLedKey_Init() once at startup,
 *   then call TaskTestLedKey_Process() every ~10 ms in an existing task loop.
 */

#ifndef TASK_TEST_LED_KEY_H
#define TASK_TEST_LED_KEY_H

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief  初始化 LED 和 KEY 的 BSP 实例，启动时调用一次 */
    void TaskTestLedKey_Init(void);

    /** @brief  驱动按键扫描和 LED 闪烁，周期性调用（约 10ms） */
    void TaskTestLedKey_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_TEST_LED_KEY_H */
