/**
 * @file    task_ota.h
 * @brief   OTA upgrade task — receives firmware via YMODEM, writes to Slot B
 * @author  GS_Mark
 */

#ifndef TASK_OTA_H
#define TASK_OTA_H

#include <stdint.h>

//*** Firmware Version ***//

#define OTA_FW_VERSION  "V1.0.0"

//*** OTA Task States ***//

/** @brief  OTA 任务内部状态（供外部查询当前进度） */
typedef enum
{
    OTA_TASK_NORMAL,          /* 正常运行，OTA 未触发 */
    OTA_TASK_TRIGGER_WAIT,    /* 等待按键触发 */
    OTA_TASK_PREPARING,       /* 擦除 Slot B */
    OTA_TASK_RECEIVING,       /* YMODEM 接收固件 */
    OTA_TASK_VERIFYING_SIG,   /* ECDSA-P256 签名验证 */
    OTA_TASK_VERIFYING,       /* SHA-256 校验 */
    OTA_TASK_UPDATING_CONFIG, /* 更新 OTA Config */
    OTA_TASK_REBOOT_PENDING,  /* 即将重启 */
    OTA_TASK_ERROR,           /* 出错，即将重启 */
} ota_task_state_t;

//*** Public API ***//

/** @brief  初始化 OTA 子系统和 EasyLogger */
void TaskOta_Init(void);

/** @brief  OTA 任务入口函数（FreeRTOS 任务） */
void TaskOta_Run(void *p_argument);

/** @brief  获取当前 OTA 任务状态 */
ota_task_state_t TaskOta_GetState(void);

#endif /* TASK_OTA_H */
