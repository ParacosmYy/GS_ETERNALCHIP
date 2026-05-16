/**
 * @file    cmb_cfg.h
 * @brief   CmBacktrace 项目定制配置
 *
 * STM32F411CEUx + FreeRTOS + EasyLogger (UART 输出)
 */

#ifndef CMB_CFG_H
#define CMB_CFG_H

#include "elog.h"

/* 输出方式: 使用 elog_raw 绕过 mutex，避免 Fault 时死锁 */
#define cmb_println(...)                do { \
                                            elog_raw(__VA_ARGS__); \
                                            elog_raw("\r\n"); \
                                        } while(0)

#define CMB_USING_OS_PLATFORM
#define CMB_OS_PLATFORM_TYPE            CMB_OS_PLATFORM_FREERTOS
#define CMB_CPU_PLATFORM_TYPE           CMB_CPU_ARM_CORTEX_M4

/* English output (ARM Compiler V5 does not support UTF-8 Chinese) */
#define CMB_PRINT_LANGUAGE              CMB_PRINT_LANGUAGE_ENGLISH

/* 使能栈 dump */
#define CMB_USING_DUMP_STACK_INFO

#endif /* CMB_CFG_H */
