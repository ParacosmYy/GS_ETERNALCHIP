/**
 * @file    crash_dump.h
 * @brief   Crash 寄存器转储 — HardFault 时将关键寄存器写入 Flash
 * @author  GS_Mark
 *
 * @par dependencies
 * - ota_config.h
 * - stdint.h
 *
 * 在 HardFault/MemManage/BusFault/UsageFault Handler 中直接操作 Flash
 * 寄存器写入 crash_dump_t 结构，不依赖 HAL/FreeRTOS/中断。
 * App 启动时调用 CrashDump_CheckAndPrint() 检查是否有上次崩溃记录。
 *
 * @version V1.0 2025-05-23
 *
 * @note 1 tab == 4 spaces
 *
 * */
#ifndef CRASH_DUMP_H
#define CRASH_DUMP_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* 头文件 ************************************//
#include "ota_config.h"
#include <stdint.h>
//******************************* 头文件 ************************************//

//******************************* 函数声明 **********************************//

/**
 * @brief  在 fault handler 中调用，将寄存器写入 Flash。
 *
 * 直接操作 STM32F4 Flash 寄存器，不依赖 HAL/FreeRTOS。
 * 只在 magic 为空（0xFFFFFFFF）时写入，防止重复覆盖。
 *
 * @param[in] p_sp    : fault 时的 SP（指向栈帧：R0-R3,R12,LR,PC,xPSR）。
 * @param[in] exc_lr  : EXC_RETURN 值（fault handler 入口时的 LR）。
 * */
void CrashDump_SaveFromFault(uint32_t p_sp, uint32_t exc_lr);

/**
 * @brief  App 启动时检查并打印上次的 crash 记录。
 *
 * 如果 Flash 中存在有效 crash 记录，打印寄存器信息，
 * 然后清除 magic 防止下次重复打印。
 *
 * @return  1 : 检测到 crash 记录（已打印并清除）。
 * @return  0 : 无 crash 记录。
 * */
int CrashDump_CheckAndPrint(void);

#ifdef __cplusplus
}
#endif

#endif /* CRASH_DUMP_H */
