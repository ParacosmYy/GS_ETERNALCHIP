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

/** @brief  Initialise LED & KEY BSP instances. Call once at startup. */
void TaskTestLedKey_Init(void);

/** @brief  Drive key scan and LED blink. Call periodically (~10 ms). */
void TaskTestLedKey_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_TEST_LED_KEY_H */
