/**
 * @file    plat_tick.h
 * @brief   公共时基操作抽象 — 所有需要 tick 的模块共享
 * @author  GS_Mark
 *
 * @par dependencies
 * - stdint.h
 *
 * @note    消除各 plat_xxx.h 中重复定义的 pf_get_tick 类型。
 *          仅含 pf_get_tick 的 ops 类型（LED/Key/Flash time）直接使用本类型。
 *          含额外成员的 ops 类型（sys/uart）保留独立定义，内嵌 pf_get_tick。
 */

#ifndef PLAT_TICK_H
#define PLAT_TICK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** @brief  通用时基操作 — 获取系统 tick（毫秒） */
typedef struct
{
    uint32_t (*pf_get_tick)(void);
} tick_operations_t;

#ifdef __cplusplus
}
#endif

#endif /* PLAT_TICK_H */
