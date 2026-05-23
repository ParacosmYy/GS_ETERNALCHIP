/**
 * @file    bootloader.c
 * @brief   Bootloader — 双 Bank 直接跳转，无拷贝
 * @author  GS_Mark
 *
 * @par dependencies
 * - bootloader.h
 * - bsp_uart.h
 * - bsp_flash.h
 * - ota_config.h
 * - usart.h
 * - gpio.h
 * - stm32f4xx_hal.h
 *
 * 读取 OTA Config，根据 active_slot 直接跳转到对应 Bank。
 * UPGRADE_PENDING: 切换到新 Bank。
 * CONFIRMING: 计数启动次数，超过阈值则 ROLLBACK（切回旧 Bank）。
 * ROLLBACK: 切回旧 Bank 跳转。
 * 全程不做 Flash 拷贝。
 *
 * @version V2.0 2025-05-21
 *
 * */

//*** Includes ***//
#include "bootloader.h"
#include "bsp_uart.h"
#include "bsp_flash.h"
#include "ota_config.h"
#include "usart.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"
#include <string.h>

//*** Utility ***//

/**
 * @brief  OTA 状态枚举转可读字符串。
 *
 * @param[in] state : OTA 状态值。
 *
 * @return 状态名称字符串。
 * */
static const char *StateStr(ota_state_t state)
{
    switch (state)
    {
        case OTA_STATE_IDLE:            return "IDLE";
        case OTA_STATE_UPGRADE_PENDING: return "UPGRADE_PENDING";
        case OTA_STATE_CONFIRMED:       return "CONFIRMED";
        case OTA_STATE_CONFIRMING:      return "CONFIRMING";
        case OTA_STATE_ROLLBACK:        return "ROLLBACK";
        default:                        return "UNKNOWN";
    }
}

//*** Jump to Application ***//

/**
 * @brief  跳转到指定 Bank（Cortex-M 标准跳转流程）。
 *
 *         从 Bank 基地址读取栈顶指针和复位向量，校验合法性后：
 *         关中断 → HAL 反初始化 → 停 SysTick → 重定位 VTOR → 设 MSP → 跳转。
 *         成功后不返回。
 *
 * @param[in] app_addr : Bank 基地址（向量表所在 Flash 地址）。
 * */
static void JumpToApp(uint32_t app_addr)
{
    uint32_t app_stack;
    uint32_t app_entry;
    typedef void (*pFunction)(void);
    pFunction JumpToApplication;

    app_stack = *(volatile uint32_t *)app_addr;
    app_entry = *(volatile uint32_t *)(app_addr + 4);

    if (app_stack < SRAM_BASE || app_stack > SRAM_END)
    {
        BspUart_Printf("[BOOT] Invalid app stack: 0x%08X\r\n", app_stack);
        return;
    }

    __disable_irq();
    HAL_DeInit();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    SCB->VTOR = app_addr;
    __set_MSP(app_stack);

    JumpToApplication = (pFunction)app_entry;
    JumpToApplication();
}

//*** Bank Address Helper ***//

/**
 * @brief  获取指定 Bank 的 Flash 基地址。
 *
 * @param[in] slot : Bank 标识。
 *
 * @return Flash 基地址。
 * */
static uint32_t BankAddr(ota_slot_t slot)
{
    if (slot == OTA_SLOT_B)
    {
        return FLASH_ADDR_BANK_B;
    }
    return FLASH_ADDR_BANK_A;
}

/**
 * @brief  获取另一个 Bank。
 *
 * @param[in] slot : 当前 Bank。
 *
 * @return 另一个 Bank 标识。
 * */
static ota_slot_t OtherBank(ota_slot_t slot)
{
    if (slot == OTA_SLOT_A)
    {
        return OTA_SLOT_B;
    }
    return OTA_SLOT_A;
}

//*** Trace Reader ***//

static const char *EventName(uint32_t event)
{
    switch (event)
    {
        case 0x01: return "APP_START";
        case 0x02: return "TRIGGER";
        case 0x03: return "ERASE_START";
        case 0x04: return "ERASE_DONE";
        case 0x05: return "YMODEM_START";
        case 0x06: return "YMODEM_DONE";
        case 0x07: return "ECDSA_START";
        case 0x08: return "ECDSA_DONE";
        case 0x09: return "SHA256_START";
        case 0x0A: return "SHA256_DONE";
        case 0x0B: return "CFG_WRITE";
        case 0x0C: return "REBOOT";
        case 0xFF: return "ERROR";
        default:   return "???";
    }
}

static void PrintTrace(void)
{
    const ota_trace_entry_t *entries = (const ota_trace_entry_t *)OTA_TRACE_ADDR;
    int count = 0;
    int i;

    for (i = 0; i < (int)OTA_TRACE_MAX_ENTRIES; i++)
    {
        if (entries[i].event == 0xFFFFFFFF)
        {
            break;
        }
        count++;
    }

    if (count == 0)
    {
        BspUart_Printf("[BOOT] Trace: (empty)\r\n");
        return;
    }

    BspUart_Printf("[BOOT] Trace: %d entries\r\n", count);

    for (i = 0; i < count; i++)
    {
        BspUart_Printf("[BOOT] #%02d %-14s t=%-5lu %s data=%lu\r\n",
                       i + 1,
                       EventName(entries[i].event),
                       entries[i].timestamp,
                       entries[i].result == 0 ? "OK " : "ERR",
                       entries[i].data);
    }
}

//*** Public API ***//

/**
 * @brief  Bootloader 主入口。
 *
 *         初始化 UART + Flash，打印启动信息，读取 OTA Config。
 *         根据 OTA 状态机分支处理：
 *         - IDLE / CONFIRMED → 直接跳转 active_slot
 *         - UPGRADE_PENDING  → 直接跳转新 Bank（无拷贝）
 *         - CONFIRMING       → boot_count++ → 跳转（>=3 则 ROLLBACK）
 *         - ROLLBACK         → 切回旧 Bank 跳转
 *         Config 无效时尝试跳转 Bank A。
 * */
void Boot_Run(void)
{
    ota_config_t cfg;
    int          ret;

    BspUart_Init(&huart1);
    BspFlash_Init();

    BspUart_Printf("\r\n");
    BspUart_Printf("========================================\r\n");
    BspUart_Printf("  GS_ETERNALCHIP Bootloader\r\n");
    BspUart_Printf("  STM32F411CEUx @ 96MHz (Dual Bank)\r\n");
    BspUart_Printf("  Boot  : %s\r\n", BOOT_FW_VERSION);
    BspUart_Printf("========================================\r\n");

    ret = BspFlash_ReadConfig(&cfg);

    if (ret != 0)
    {
        BspUart_Printf("[BOOT] Config %s (ret=%d)\r\n",
                       ret == -1 ? "magic mismatch" : "CRC error", ret);

        if (BspFlash_IsEmpty(FLASH_ADDR_BANK_A, 8))
        {
            BspUart_Printf("[BOOT] Bank A empty, no app to boot.\r\n");
        }
        else
        {
            BspUart_Printf("[BOOT] Jumping to Bank A directly.\r\n");
            JumpToApp(FLASH_ADDR_BANK_A);
        }
        return;
    }

    BspUart_Printf("[BOOT] State=%s, Bank=%c, boot_count=%lu, fw_size=%lu\r\n",
                   StateStr(cfg.state),
                   cfg.active_slot == OTA_SLOT_A ? 'A' : 'B',
                   cfg.boot_count, cfg.fw_size);

    PrintTrace();

    switch (cfg.state)
    {
        case OTA_STATE_UPGRADE_PENDING:
            cfg.state      = OTA_STATE_CONFIRMING;
            cfg.boot_count = 0;
            if (BspFlash_WriteConfig(&cfg) != 0)
            {
                BspUart_Printf("[BOOT] Config write failed!\r\n");
            }
            BspUart_Printf("[BOOT] Upgrade! Jumping to Bank %c.\r\n",
                           cfg.active_slot == OTA_SLOT_A ? 'A' : 'B');
            JumpToApp(BankAddr(cfg.active_slot));
            break;

        case OTA_STATE_CONFIRMING:
            cfg.boot_count++;
            if (cfg.boot_count >= OTA_BOOT_COUNT_THRESHOLD)
            {
                ota_slot_t old = cfg.active_slot;
                cfg.active_slot = OtherBank(old);
                cfg.state       = OTA_STATE_CONFIRMED;
                cfg.boot_count  = 0;
                if (BspFlash_WriteConfig(&cfg) != 0)
                {
                    BspUart_Printf("[BOOT] Config write failed!\r\n");
                }
                BspUart_Printf("[BOOT] ROLLBACK! %c -> %c.\r\n",
                               old == OTA_SLOT_A ? 'A' : 'B',
                               cfg.active_slot == OTA_SLOT_A ? 'A' : 'B');
                JumpToApp(BankAddr(cfg.active_slot));
            }
            else
            {
                if (BspFlash_WriteConfig(&cfg) != 0)
                {
                    BspUart_Printf("[BOOT] Config write failed!\r\n");
                }
                BspUart_Printf("[BOOT] Confirming (%lu/%u). Jumping to Bank %c.\r\n",
                               cfg.boot_count, OTA_BOOT_COUNT_THRESHOLD,
                               cfg.active_slot == OTA_SLOT_A ? 'A' : 'B');
                JumpToApp(BankAddr(cfg.active_slot));
            }
            break;

        case OTA_STATE_CONFIRMED:
        case OTA_STATE_IDLE:
            BspUart_Printf("[BOOT] Jumping to Bank %c.\r\n",
                           cfg.active_slot == OTA_SLOT_A ? 'A' : 'B');
            JumpToApp(BankAddr(cfg.active_slot));
            break;

        case OTA_STATE_ROLLBACK:
        {
            ota_slot_t old = cfg.active_slot;
            cfg.active_slot = OtherBank(old);
            cfg.state       = OTA_STATE_CONFIRMED;
            cfg.boot_count  = 0;
            if (BspFlash_WriteConfig(&cfg) != 0)
            {
                BspUart_Printf("[BOOT] Config write failed!\r\n");
            }
            BspUart_Printf("[BOOT] ROLLBACK! %c -> %c.\r\n",
                           old == OTA_SLOT_A ? 'A' : 'B',
                           cfg.active_slot == OTA_SLOT_A ? 'A' : 'B');
            JumpToApp(BankAddr(cfg.active_slot));
            break;
        }

        default:
            BspUart_Printf("[BOOT] Unknown state %d.\r\n", cfg.state);
            break;
    }
}

/**
 * @brief  恢复等待循环（在 main while(1) 中调用）。
 *
 *         当 Bootloader 无法跳转到 APP 时，
 *         每 1s 翻转 PC13 LED 并通过 UART 输出等待恢复信息。
 * */
void Boot_RecoveryLoop(void)
{
    static uint32_t tick = 0;
    uint32_t        now  = HAL_GetTick();

    if (now - tick >= 1000)
    {
        tick = now;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        BspUart_Printf("[BOOT] Waiting for recovery ...\r\n");
    }
}
