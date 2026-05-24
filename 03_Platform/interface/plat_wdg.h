/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_wdg.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author  GS_Mark
 *
 * @brief   Platform Watchdog interface
 *
 * @version V1.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_WDG_H
#define PLAT_WDG_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Declaring ********************************//
void BspWdg_Init(void *p_hwdg);
void BspWdg_Feed(void);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_WDG_H */
