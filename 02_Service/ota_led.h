/******************************************************************************
 * Copyright (C) 2025 GS_Mark. All rights reserved.
 *
 * @file    ota_led.h
 * @brief   OTA 升级 LED 状态指示模块
 * @author  GS_Mark
 *
 * @par dependencies
 * - stdint.h
 *
 * 提供 OTA 升级过程中的 LED 视觉反馈。
 * 每个 OTA 阶段对应一种 LED 模式，错误状态产生 N 次闪烁错误码
 * （连闪 N 次 + 暂停 2s，循环）。
 *
 * @version V1.0 2025-05-20
 *
 * @note 1 tab == 4 spaces
 *
 *****************************************************************************/
#ifndef OTA_LED_H
#define OTA_LED_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
//******************************* Includes **********************************//

//******************************* Defines ***********************************//

/** @brief 错误码：Slot B 擦除失败 */
#define OTA_ERR_ERASE_FAILED    2

/** @brief 错误码：YMODEM 接收失败 */
#define OTA_ERR_YMODEM_FAILED   3

/** @brief 错误码：ECDSA 签名验证失败 */
#define OTA_ERR_ECDSA_FAILED    4

/** @brief 错误码：SHA-256 校验失败 */
#define OTA_ERR_SHA256_FAILED   5

/** @brief 错误码：OTA Config 更新失败 */
#define OTA_ERR_CONFIG_FAILED   6

//******************************* Types *************************************//

/** @brief OTA LED 显示模式 */
typedef enum
{
    OTA_LED_IDLE,       /**< 慢闪 1Hz — 等待按键触发 */
    OTA_LED_WORKING,    /**< 快闪 4Hz — 擦除/验签/校验/更新配置 */
    OTA_LED_RECEIVING,  /**< 常亮 — 接收固件中 */
    OTA_LED_REBOOTING,  /**< 极快闪 10Hz — 即将重启 */
    OTA_LED_ERROR,      /**< 错误码闪烁 — 连闪 N 次 + 暂停 2s */
    OTA_LED_OFF,        /**< LED 关闭 */
} ota_led_mode_t;

//******************************* Declaring *********************************//

/**
 * @brief  初始化 OTA LED 模块（PC13，低有效）。
 *
 * Steps:
 *  1. 初始化 BSP LED 驱动。
 *  2. 设置默认模式为 OTA_LED_IDLE。
 *
 * */
void OtaLed_Init(void);

/**
 * @brief  设置 LED 显示模式。
 *
 * Steps:
 *  1. 停止当前闪烁模式。
 *  2. 应用新模式（闪烁/常亮/关闭）。
 *
 * @param[in] mode : 目标 LED 模式。
 *
 * */
void OtaLed_SetMode(ota_led_mode_t mode);

/**
 * @brief  设置错误码并切换到 ERROR 模式。
 *
 * Steps:
 *  1. 记录错误码（2~6）。
 *  2. 切换到 OTA_LED_ERROR 模式。
 *  3. 日志输出错误码描述。
 *
 * @param[in] code : 错误码（OTA_ERR_xxx）。
 *
 * */
void OtaLed_SetErrorCode(uint8_t code);

/**
 * @brief  获取当前错误码（0 = 无错误）。
 *
 * @return 当前错误码。
 *
 * */
uint8_t OtaLed_GetErrorCode(void);

/**
 * @brief  LED 闪烁定时钩子函数。
 *
 * 必须在任务或定时器上下文中每 50ms 调用一次。
 *
 * */
void OtaLed_TimebaseHook(void);

/**
 * @brief  反初始化 LED 模块（关闭 LED）。
 *
 * */
void OtaLed_Deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_LED_H */
