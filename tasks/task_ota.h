/**
 * @file    task_ota.h
 * @brief   OTA upgrade task — receives firmware via YMODEM, writes to Slot B
 *
 * Flow:
 *   1. 5-second trigger window (key short press or UART "OTA" command)
 *   2. Erase Slot B
 *   3. YMODEM receive → write to Slot B
 *   4. SHA-256 verify
 *   5. Update Config (UPGRADE_PENDING)
 *   6. Software reset → Bootloader copies Slot B → Slot A
 */

#ifndef TASK_OTA_H
#define TASK_OTA_H

#include <stdint.h>

//*** OTA Task States ***//

typedef enum
{
    OTA_TASK_NORMAL,         /* 正常运行状态，OTA 未触发 */
    OTA_TASK_TRIGGER_WAIT,   /* 等待触发状态（5秒触发窗口） */
    OTA_TASK_PREPARING,      /* 准备阶段（擦除 Slot B） */
    OTA_TASK_RECEIVING,      /* 接收阶段（YMODEM 接收固件） */
    OTA_TASK_VERIFYING,      /* 校验阶段（SHA-256 验证） */
    OTA_TASK_UPDATING_CONFIG,/* 更新配置（写入 UPGRADE_PENDING） */
    OTA_TASK_REBOOT_PENDING, /* 重启待发（准备软件复位） */
    OTA_TASK_ERROR,          /* 错误状态（OTA 流程异常） */
} ota_task_state_t;

//*** Public API ***//

/** @brief  初始化 OTA 子系统和 EasyLogger，在 TaskOta_Run 前调用 */
void TaskOta_Init(void);

/**
 * @brief  OTA 任务入口函数（FreeRTOS 任务函数）
 *         执行完整 OTA 流程：等待触发 → 擦除 → YMODEM 接收 → 校验 → 重启
 * @param  p_argument  未使用（传 NULL）
 */
void TaskOta_Run(void *p_argument);

/**
 * @brief  获取当前 OTA 任务状态
 * @return 任务状态枚举值
 */
ota_task_state_t TaskOta_GetState(void);

/** @brief  通知 OTA 任务：检测到按键触发 */
void TaskOta_NotifyKeyTrigger(void);

/** @brief  通知 OTA 任务：收到 UART 触发命令 */
void TaskOta_NotifyUartTrigger(void);

/** @brief  通知 OTA 任务：请求退出（错误恢复或外部请求） */
void TaskOta_NotifyExit(void);

#endif /* TASK_OTA_H */