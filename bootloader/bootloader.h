/**
 * @file    bootloader.h
 * @brief   Bootloader application — state machine, flash copy, jump to app
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>

//*** Public API ***//

/** @brief  Bootloader 主入口，初始化外设并执行 OTA 状态机 */
void Boot_Run(void);

/** @brief  恢复等待循环，LED 闪烁 + UART 输出 */
void Boot_RecoveryLoop(void);

#endif /* BOOTLOADER_H */
