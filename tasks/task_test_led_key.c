/**
 * @file    task_test_led_key.c
 * @brief   LED/KEY 测试模块（已停用，功能已合并到 task_ota.c）
 */

#define LOG_TAG "KEY"
#include "elog.h"
#include "task_test_led_key.h"

void TaskTestLedKey_Init(void)
{
    log_i("--- LED/KEY Test Disabled (moved to task_ota) ---");
}

void TaskTestLedKey_Process(void)
{
}
