/**
 * @file    ota_config.h
 * @brief   OTA 双 Bank 分区布局、配置结构与状态定义
 * @author  GS_Mark
 *
 * @par dependencies
 * - stdint.h
 * - stddef.h
 *
 * Boot 和 APP 共用的唯一定义源。
 *
 * Flash 分区布局 (STM32F411CEUx, 512 KB):
 *   Sector 0  (16KB,  0x08000000)  Bootloader
 *   Sector 1  (16KB,  0x08004000)  OTA Config (ota_config_t + CRC-32)
 *   Sector 2-5(224KB, 0x08008000)  Bank A
 *   Sector 6-7(224KB, 0x08040000)  Bank B（实际 256KB，限制 224KB）
 *
 * 状态机（双 Bank 直接切换，无拷贝）:
 *   IDLE/CONFIRMED --(下载到非活跃 Bank)--> UPGRADE_PENDING
 *                                              |
 *                                        reset |
 *                                              v
 *                                     Boot 直接跳转新 Bank
 *                                              |
 *                                              v
 *                                      CONFIRMING (boot_count++)
 *                                         |            |
 *                                    count < 3     count >= 3
 *                                         |            |
 *                                    跳转新Bank    ROLLBACK
 *                                         |       切回旧 Bank
 *                                         v            |
 *                                    App confirm   跳转旧 Bank
 *                                         |            |
 *                                         v            v
 *                                      CONFIRMED    CONFIRMED
 *
 * @version V2.0 2025-05-21
 *
 * @note ARM Compiler V5 兼容。
 * @note 1 tab == 4 spaces
 *
 * */
#ifndef OTA_CONFIG_H
#define OTA_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
#include <stddef.h>
//******************************* Includes **********************************//

//******************************* Defines ***********************************//

/** @brief Flash 分区基地址 */
#define FLASH_ADDR_BOOT   0x08000000u
#define FLASH_ADDR_CONFIG 0x08004000u
#define FLASH_ADDR_BANK_A 0x08008000u
#define FLASH_ADDR_BANK_B 0x08040000u

/* 兼容旧代码的别名 */
#define FLASH_ADDR_SLOT_A FLASH_ADDR_BANK_A
#define FLASH_ADDR_SLOT_B FLASH_ADDR_BANK_B

/** @brief Bank 大小（两个 Bank 统一 224KB） */
#define BANK_A_SIZE       (224u * 1024u)
#define BANK_B_SIZE       (224u * 1024u)
#define BANK_APP_SIZE     BANK_A_SIZE
#define SLOT_MAX_APP_SIZE BANK_APP_SIZE

/* 兼容旧代码 */
#define SLOT_A_SIZE       BANK_A_SIZE
#define SLOT_B_SIZE       (256u * 1024u)

/** @brief SRAM 地址范围（STM32F411CEUx, 128 KB） */
#define SRAM_SIZE         (128u * 1024u)
#define SRAM_END          (SRAM_BASE + SRAM_SIZE)

/** @brief OTA Config 魔数与版本 */
#define OTA_CONFIG_MAGIC   0x4F544131u     /* "OTA1" */
#define OTA_CONFIG_VERSION 3u

/** @brief 未确认启动次数阈值，超过则回退 */
#define OTA_BOOT_COUNT_THRESHOLD 3u

/** @brief OTA 故障追踪区（Sector 1 内，ota_config_t 之后） */
#define OTA_TRACE_ADDR        (FLASH_ADDR_CONFIG + 0x100u)
#define OTA_TRACE_MAX_ENTRIES 32u
#define OTA_TRACE_ENTRY_SIZE  16u

/** @brief 追踪记录结构（16 字节，Flash 存储） */
typedef struct
{
    uint32_t event;      /**< 事件类型 */
    uint32_t timestamp;  /**< 时间戳（ms） */
    uint32_t result;     /**< 结果（0 = 成功） */
    uint32_t data;       /**< 附加数据 */
} ota_trace_entry_t;

/** @brief Crash dump 区（Sector 1 内，OTA Trace 之后） */
#define CRASH_DUMP_ADDR       (OTA_TRACE_ADDR + OTA_TRACE_MAX_ENTRIES * OTA_TRACE_ENTRY_SIZE)
#define CRASH_DUMP_MAGIC      0x43524153u  /* "CRAS" */
#define CRASH_DUMP_SIZE       64u          /* crash_dump_t 结构大小 */

//******************************* Types *************************************//

/** @brief OTA Bank 标识 */
typedef enum
{
    OTA_SLOT_A = 0,  /**< Bank A @ 0x08008000 */
    OTA_SLOT_B = 1,  /**< Bank B @ 0x08040000 */
} ota_slot_t;

/** @brief OTA 状态机状态 */
typedef enum
{
    OTA_STATE_IDLE             = 0,  /**< 正常运行 */
    OTA_STATE_UPGRADE_PENDING  = 1,  /**< 新固件已下载，等待 Boot 切换 */
    OTA_STATE_CONFIRMED        = 3,  /**< App 确认启动成功 */
    OTA_STATE_CONFIRMING       = 4,  /**< Boot 已切换，等待 App 确认 */
    OTA_STATE_ROLLBACK         = 5,  /**< 启动失败，回退到旧 Bank */
} ota_state_t;

/** @brief OTA 配置结构（存储在 Sector 1，CRC-32 保护） */
typedef struct
{
    uint32_t    magic;          /**< 必须等于 OTA_CONFIG_MAGIC */
    uint32_t    version;        /**< 配置结构版本 */
    ota_slot_t  active_slot;    /**< 当前活跃 Bank（真正切换） */
    ota_state_t state;          /**< 当前 OTA 状态 */
    uint32_t    fw_size;        /**< 固件大小（字节） */
    uint8_t     fw_sha256[32];  /**< 固件 SHA-256 哈希 */
    uint32_t    boot_count;     /**< 未确认启动计数 */
    uint32_t    upgrade_count;  /**< 总升级次数 */
    char        prev_fw_version[16]; /**< 升级前固件版本 */
    uint32_t    crc32;          /**< 以上所有字段的 CRC-32 */
} ota_config_t;

/** @brief Crash 寄存器转储结构（64 字节，写入 Flash） */
typedef struct
{
    uint32_t magic;    /**< CRASH_DUMP_MAGIC, 标记有效记录 */
    uint32_t cfsr;     /**< Configurable Fault Status */
    uint32_t hfsr;     /**< HardFault Status */
    uint32_t mmfar;    /**< MemManage Fault Address */
    uint32_t bfar;     /**< BusFault Address */
    uint32_t psp;      /**< Process Stack Pointer (fault context) */
    uint32_t msp;      /**< Main Stack Pointer */
    uint32_t pc;       /**< Program Counter (fault instruction) */
    uint32_t lr;       /**< Link Register (return address) */
    uint32_t xpsr;     /**< Program Status Register */
    uint32_t r0;       /**< General purpose register R0 */
    uint32_t r1;       /**< General purpose register R1 */
    uint32_t r2;       /**< General purpose register R2 */
    uint32_t r3;       /**< General purpose register R3 */
    uint32_t r12;      /**< General purpose register R12 */
    uint32_t exc_lr;   /**< EXC_RETURN value */
} crash_dump_t;

//******************************* Declaring *********************************//

/**
 * @brief  获取指定 Bank 的 Flash 基地址。
 *
 * @param[in] slot : Bank 标识（OTA_SLOT_A 或 OTA_SLOT_B）。
 *
 * @return Flash 基地址。
 * */
static inline uint32_t OtaConfig_BankAddr(ota_slot_t slot)
{
    if (slot == OTA_SLOT_B)
    {
        return FLASH_ADDR_BANK_B;
    }
    return FLASH_ADDR_BANK_A;
}

/**
 * @brief  获取另一个 Bank（非活跃 Bank）。
 *
 * @param[in] slot : 当前 Bank 标识。
 *
 * @return 另一个 Bank 的标识。
 * */
static inline ota_slot_t OtaConfig_Other(ota_slot_t slot)
{
    if (slot == OTA_SLOT_A)
    {
        return OTA_SLOT_B;
    }
    return OTA_SLOT_A;
}

#ifdef __cplusplus
}
#endif

#endif /* OTA_CONFIG_H */
