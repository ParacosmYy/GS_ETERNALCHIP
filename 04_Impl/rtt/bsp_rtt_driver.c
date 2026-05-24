/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    bsp_rtt_driver.c
 *
 * @par dependencies
 * - bsp_rtt_driver.h
 * - SEGGER_RTT.h
 *
 * @author  GS_Mark
 *
 * @brief   RTT BSP 驱动实现 — 通过 OPS 表封装 SEGGER RTT
 *
 * 处理流程:
 *   BspRtt_Init() 将 ops 表存入驱动实例。
 *   所有公共函数委托到 ops 表函数指针。
 *
 * @version V2.0 2025-05-24
 *
 * @note    1 tab == 4 spaces
 *
 *****************************************************************************/

//******************************** 头文件 *********************************//
#include "bsp_rtt_driver.h"
#include <stdint.h>
#include <stddef.h>
//******************************** 头文件 *********************************//

//******************************** 实现 ********************************//

/**
 * @brief  初始化 RTT BSP 驱动，注入硬件操作表。
 *
 * Steps:
 *  1. 校验参数。
 *  2. 存储 ops 表。
 *  3. 若提供 pf_init 则调用之。
 *  4. 标记为已初始化。
 *
 * @param[in] p_drv : 驱动实例指针。
 * @param[in] p_ops : 硬件操作表指针。
 * */
void BspRtt_Init(bsp_rtt_driver_t *p_drv, const rtt_operations_t *p_ops)
{
    if (p_drv == NULL || p_ops == NULL)
    {
        return;
    }

    p_drv->p_ops = p_ops;

    if (p_ops->pf_init != NULL)
    {
        p_ops->pf_init();
    }

    p_drv->is_inited = 1u;
}

/**
 * @brief  向 RTT 终端写入字符串。
 *
 * Steps:
 *  1. 校验驱动、ops 和字符串。
 *  2. 委托到 pf_write_string。
 *
 * @param[in] p_drv : 驱动实例指针。
 * @param[in] s     : 待写入的字符串。
 *
 * @return  >=0 : 写入字节数。
 * @return  -1  : 失败。
 * */
int BspRtt_WriteString(bsp_rtt_driver_t *p_drv, const char *s)
{
    if (p_drv == NULL || p_drv->p_ops == NULL || s == NULL)
    {
        return -1;
    }

    if (p_drv->p_ops->pf_write_string != NULL)
    {
        return p_drv->p_ops->pf_write_string(s);
    }

    return -1;
}

/**
 * @brief  向 RTT 终端格式化输出。
 *
 * Steps:
 *  1. 校验驱动、ops 和格式化字符串。
 *  2. 封装 va_list 并委托到 pf_vprintf。
 *
 * @param[in] p_drv : 驱动实例指针。
 * @param[in] fmt   : 格式化字符串。
 * @param[in] ...   : 可变参数。
 *
 * @return  >=0 : 写入字节数。
 * @return  -1  : 失败。
 * */
int BspRtt_Printf(bsp_rtt_driver_t *p_drv, const char *fmt, ...)
{
    va_list args;
    int     result;

    if (p_drv == NULL || p_drv->p_ops == NULL || fmt == NULL)
    {
        return -1;
    }

    if (p_drv->p_ops->pf_vprintf == NULL)
    {
        return -1;
    }

    va_start(args, fmt);
    result = p_drv->p_ops->pf_vprintf(fmt, args);
    va_end(args);

    return result;
}

//******************************** 实现 ********************************//
