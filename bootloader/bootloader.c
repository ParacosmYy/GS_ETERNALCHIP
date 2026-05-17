/**
 * @file    bootloader.c
 * @brief   Bootloader application — state machine, flash copy, jump to app
 */

//*** Includes ***//
#include "bootloader.h"
#include "bsp_uart.h"
#include "bsp_flash.h"
#include "ota_config.h"
#include "usart.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"
#include <string.h>

//*** Jump to Application ***//

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

//*** Copy Slot B to Slot A ***//

static int Boot_CopySlot(uint32_t fw_size)
{
    uint8_t  buf[256];
    uint32_t offset = 0;
    uint32_t chunk;
    uint32_t count  = 0;

    if (fw_size == 0 || fw_size > SLOT_MAX_APP_SIZE)
    {
        BspUart_Printf("[BOOT] Invalid fw_size: %lu\r\n", fw_size);
        return -1;
    }

    BspUart_Printf("[BOOT] Erasing Slot A ...\r\n");
    if (BspFlash_EraseSlot(OTA_SLOT_A) != 0)
    {
        BspUart_Printf("[BOOT] Erase Slot A failed!\r\n");
        return -1;
    }

    BspUart_Printf("[BOOT] Copying Slot B -> Slot A (%lu bytes) ...\r\n", fw_size);
    while (offset < fw_size)
    {
        chunk = fw_size - offset;
        if (chunk > sizeof(buf))
        {
            chunk = sizeof(buf);
        }

        BspFlash_Read(FLASH_ADDR_SLOT_B + offset, buf, chunk);
        if (BspFlash_Write(FLASH_ADDR_SLOT_A + offset, buf, chunk) != 0)
        {
            BspUart_Printf("[BOOT] Write failed at offset 0x%08lX!\r\n", offset);
            return -1;
        }

        offset += chunk;
        count++;

        if ((count % 64) == 0)
        {
            BspUart_Printf("[BOOT]   %lu / %lu bytes\r\n", offset, fw_size);
        }
    }

    BspUart_Printf("[BOOT] Copy complete. Verifying ...\r\n");
    offset = 0;
    while (offset < fw_size)
    {
        chunk = fw_size - offset;
        if (chunk > sizeof(buf))
        {
            chunk = sizeof(buf);
        }

        BspFlash_Read(FLASH_ADDR_SLOT_A + offset, buf, chunk);
        if (memcmp(buf, (const void *)(FLASH_ADDR_SLOT_B + offset), chunk) != 0)
        {
            BspUart_Printf("[BOOT] Verify failed at offset 0x%08lX!\r\n", offset);
            return -1;
        }

        offset += chunk;
    }

    BspUart_Printf("[BOOT] Verify OK.\r\n");
    return 0;
}

//*** Public API ***//

void Boot_Run(void)
{
    ota_config_t cfg;
    int          ret;

    BspUart_Init(&huart1);
    BspFlash_Init();

    BspUart_Printf("\r\n========================================\r\n");
    BspUart_Printf("[BOOT] GS_ETERNALCHIP Bootloader v1.0\r\n");
    BspUart_Printf("[BOOT] STM32F411CEUx @ 96MHz\r\n");
    BspUart_Printf("========================================\r\n");

    ret = BspFlash_ReadConfig(&cfg);

    if (ret != 0)
    {
        BspUart_Printf("[BOOT] Config %s (ret=%d)\r\n",
                       ret == -1 ? "magic mismatch" : "CRC error", ret);

        if (BspFlash_IsEmpty(FLASH_ADDR_SLOT_A, 8))
        {
            BspUart_Printf("[BOOT] Slot A empty, no app to boot.\r\n");
        }
        else
        {
            BspUart_Printf("[BOOT] Jumping to Slot A directly.\r\n");
            JumpToApp(FLASH_ADDR_SLOT_A);
        }
        return;
    }

    BspUart_Printf("[BOOT] State=%d, boot_count=%lu, fw_size=%lu\r\n",
                   cfg.state, cfg.boot_count, cfg.fw_size);

    switch (cfg.state)
    {
        case OTA_STATE_UPGRADE_PENDING:
            if (Boot_CopySlot(cfg.fw_size) == 0)
            {
                cfg.state      = OTA_STATE_CONFIRMING;
                cfg.boot_count = 0;
                if (BspFlash_WriteConfig(&cfg) != 0)
                {
                    BspUart_Printf("[BOOT] Config write failed!\r\n");
                }
                BspUart_Printf("[BOOT] Upgrade done. Jumping to app.\r\n");
                JumpToApp(FLASH_ADDR_SLOT_A);
            }
            break;

        case OTA_STATE_CONFIRMING:
            cfg.boot_count++;
            if (cfg.boot_count >= OTA_BOOT_COUNT_THRESHOLD)
            {
                cfg.state = OTA_STATE_ROLLBACK;
                if (BspFlash_WriteConfig(&cfg) != 0)
                {
                    BspUart_Printf("[BOOT] Config write failed!\r\n");
                }
                BspUart_Printf("[BOOT] ROLLBACK: boot count exceeded!\r\n");
            }
            else
            {
                if (BspFlash_WriteConfig(&cfg) != 0)
                {
                    BspUart_Printf("[BOOT] Config write failed!\r\n");
                }
                BspUart_Printf("[BOOT] Confirming (%lu/%u). Jumping to app.\r\n",
                               cfg.boot_count, OTA_BOOT_COUNT_THRESHOLD);
                JumpToApp(FLASH_ADDR_SLOT_A);
            }
            break;

        case OTA_STATE_CONFIRMED:
        case OTA_STATE_IDLE:
            BspUart_Printf("[BOOT] Jumping to Slot A.\r\n");
            JumpToApp(FLASH_ADDR_SLOT_A);
            break;

        case OTA_STATE_ROLLBACK:
            BspUart_Printf("[BOOT] ROLLBACK state. Waiting for recovery.\r\n");
            break;

        default:
            BspUart_Printf("[BOOT] Unknown state %d.\r\n", cfg.state);
            break;
    }
}

void Boot_RecoveryLoop(void)
{
    static uint32_t tick = 0;
    uint32_t        now  = HAL_GetTick();

    if (now - tick >= 200)
    {
        tick = now;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        BspUart_Printf("[BOOT] Waiting for recovery ...\r\n");
    }
}
