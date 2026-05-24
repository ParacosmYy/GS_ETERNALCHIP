/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_rtt.h
 *
 * @par dependencies
 * - (none)
 *
 * @author  GS_Mark
 *
 * @brief   Platform RTT debug output interface
 *
 * @version V1.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_RTT_H
#define PLAT_RTT_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Declaring ********************************//
void BspRtt_Init(void);
int  BspRtt_WriteString(const char *s);
int  BspRtt_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_RTT_H */
