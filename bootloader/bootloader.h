/**
 * @file    bootloader.h
 * @brief   Bootloader application — state machine, flash copy, jump to app
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>

//*** Bootloader Version ***//

#define BOOT_FW_MAJOR  2
#define BOOT_FW_MINOR  0
#define BOOT_FW_PATCH  0
#define BOOT_FW_VERSION "V2.0.0"

//*** Public API ***//

/** @brief  Bootloader 主入口，初始化外设并执行 OTA 状态机 */
void Boot_Run(void);

/** @brief  恢复等待循环，LED 闪烁 + UART 输出 */
void Boot_RecoveryLoop(void);

#endif /* BOOTLOADER_H */
