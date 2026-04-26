/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 * 
 * @file bsp_debug.h
 * 
 * @par dependencies 
 * - main.h
 * 
 * @author GS_Mark
 *
 * @brief 调试输出接口，提供 printf 重定向到 UART
 * 
 * Usage:
 * printf("...")      - 直接使用 printf 输出到 UART
 * 
 * @note Keil 配置必须勾选 "Use MicroLib"，否则 fputc 重定向不生效
 * 
 * @version V1.0 2025-04-26
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/
#ifndef __BSP_DEBUG_H__
#define __BSP_DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include "main.h"
//******************************* Includes **********************************//

#ifdef __cplusplus
}
#endif

#endif /* __BSP_DEBUG_H__ */
