/**
 * @file    bootloader.h
 * @brief   Bootloader application — state machine, flash copy, jump to app
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>

//*** Public API ***//

/**
 * @brief  Bootloader main entry point.
 *         Initializes UART + Flash, prints banner, runs OTA state machine.
 *         Jumps to app if valid — never returns on success.
 *         Returns only if no valid app or ROLLBACK state.
 */
void Boot_Run(void);

/**
 * @brief  Recovery loop handler. Call from main while(1) when bootloader
 *         cannot jump to app (no valid app / ROLLBACK state).
 *         Toggles LED and prints status via UART.
 */
void Boot_RecoveryLoop(void);

#endif /* BOOTLOADER_H */
