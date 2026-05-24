/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_sys.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author  GS_Mark
 *
 * @brief   Platform system utilities — tick, reboot
 *
 * @version V1.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_SYS_H
#define PLAT_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
#include "ota_config.h"
//******************************* Includes **********************************//

//******************************* Declaring ********************************//
uint32_t BspSys_GetTick(void);
void     BspSys_Reboot(void);
int      BspSys_GetRunningBank(ota_slot_t *p_slot);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_SYS_H */
