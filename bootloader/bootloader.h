/**
 * @file    bootloader.h
 * @brief   Bootloader application — state machine, flash copy, jump to app
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>

//*** Public API ***//

/**
 * @brief  Bootloader 主入口
 *         初始化 UART + Flash，打印启动信息，执行 OTA 状态机。
 *         有效 APP 时跳转后不返回；无有效 APP 或 ROLLBACK 状态时返回。
 */
void Boot_Run(void);

/**
 * @brief  恢复等待循环（在 main while(1) 中调用）
 *         当 Bootloader 无法跳转到 APP 时（无有效 APP / ROLLBACK 状态），
 *         每 200ms 翻转 LED 并通过 UART 输出等待恢复信息。
 */
void Boot_RecoveryLoop(void);

#endif /* BOOTLOADER_H */
