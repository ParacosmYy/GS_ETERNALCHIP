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
 * @brief   平台 RTT 调试输出接口（OPS 模式）
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

/** @brief RTT 驱动前向声明 */
typedef struct bsp_rtt_driver bsp_rtt_driver_t;

/**
 * @brief  RTT 硬件操作（初始化时注入）。
 *
 * @note   仅平台层 — 零 HAL 依赖。
 * */
typedef struct
{
    int (*pf_init)(void);
    int (*pf_write_string)(const char *s);
    int (*pf_vprintf)(const char *fmt, va_list args);
} rtt_operations_t;

/**
 * @brief  RTT 驱动实例。
 *
 * @note   p_ops     : 硬件操作表。
 *         is_inited : BspRtt_Init() 后为 1，否则为 0。
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
