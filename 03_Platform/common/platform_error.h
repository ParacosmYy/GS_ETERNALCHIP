/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    platform_error.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author  GS_Mark
 *
 * @brief   平台层统一错误码
 *
 * 用法：
 *   所有 Platform/Service/App 层函数返回 platform_err_t。
 *   实现层将 HAL 错误映射为这些错误码。
 *
 * @version V1.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLATFORM_ERROR_H
#define PLATFORM_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Types *************************************//
typedef enum
{
    PLATFORM_ERR_OK              = 0,
    PLATFORM_ERR_GENERAL         = 1,
    PLATFORM_ERR_TIMEOUT         = 2,
    PLATFORM_ERR_PARAM           = 3,
    PLATFORM_ERR_NO_MEMORY       = 4,
    PLATFORM_ERR_NO_RESOURCE     = 5,
    PLATFORM_ERR_NOT_SUPPORTED   = 6,
    PLATFORM_ERR_NOT_INITIALIZED = 7,
    PLATFORM_ERR_ALREADY_INIT    = 8,
    PLATFORM_ERR_BUSY            = 9,
    PLATFORM_ERR_FAIL            = 10,
    PLATFORM_ERR_RESERVED        = 0x7FFFFFFF
} platform_err_t;

//******************************* Defines ***********************************//
#define PLATFORM_IS_ERR(err)    ((err) != PLATFORM_ERR_OK)
#define PLATFORM_IS_OK(err)     ((err) == PLATFORM_ERR_OK)

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_ERROR_H */
