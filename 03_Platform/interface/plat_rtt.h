/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    plat_rtt.h
 *
 * @par dependencies
 * - stdarg.h
 *
 * @author  GS_Mark
 *
 * @brief   Platform RTT debug output interface (OPS pattern)
 *
 * @version V2.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef PLAT_RTT_H
#define PLAT_RTT_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdarg.h>
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Types *************************************//

/** @brief Forward declaration of RTT driver */
typedef struct bsp_rtt_driver bsp_rtt_driver_t;

/**
 * @brief  RTT hardware operations (injected at init).
 *
 * @note   Platform layer only — ZERO HAL dependencies.
 * */
typedef struct
{
    int (*pf_init)(void);
    int (*pf_write_string)(const char *s);
    int (*pf_vprintf)(const char *fmt, va_list args);
} rtt_operations_t;

/**
 * @brief  RTT driver instance.
 *
 * @note   p_ops     : Hardware operations table.
 *         is_inited : 1 after BspRtt_Init(), 0 otherwise.
 * */
struct bsp_rtt_driver
{
    const rtt_operations_t *p_ops;
    uint8_t                 is_inited;
};

//******************************* Declaring **********************************/

/**
 * @brief  初始化 RTT BSP 驱动，注入硬件操作表。
 *
 * @param[in] p_drv : 驱动实例指针。
 * @param[in] p_ops : 硬件操作表指针。
 * */
void BspRtt_Init(bsp_rtt_driver_t *p_drv, const rtt_operations_t *p_ops);

/**
 * @brief  向 RTT 终端写入字符串。
 *
 * @param[in] p_drv : 驱动实例指针。
 * @param[in] s     : 待写入的字符串。
 *
 * @return  >=0 : 写入字节数。
 * @return  -1  : 失败。
 * */
int BspRtt_WriteString(bsp_rtt_driver_t *p_drv, const char *s);

/**
 * @brief  向 RTT 终端格式化输出。
 *
 * @param[in] p_drv : 驱动实例指针。
 * @param[in] fmt   : 格式化字符串。
 * @param[in] ...   : 可变参数。
 *
 * @return  >=0 : 写入字节数。
 * @return  -1  : 失败。
 * */
int BspRtt_Printf(bsp_rtt_driver_t *p_drv, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_RTT_H */
