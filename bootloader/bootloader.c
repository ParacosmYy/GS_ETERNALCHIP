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
#include "sha256.h"

//*** Utility ***//

/**
 * @brief  OTA 状态枚举转可读字符串
 * @param  state  OTA 状态值
 * @return 状态名称字符串
 */
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
 * @brief  跳转到 APP（Cortex-M 标准跳转流程）
 *
 *         从指定地址读取栈顶指针和复位向量，校验栈指针合法性后：
 *         关中断 → HAL 反初始化 → 停 SysTick → 重定位 VTOR → 设 MSP → 跳转。
 *         成功后不返回。
 *
 * @param  app_addr  APP 起始地址（向量表所在 Flash 地址）
 * @note   栈指针范围校验使用 SRAM_BASE ~ SRAM_END（128KB SRAM）
 */
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

/**
 * @brief  复制 Slot B 固件到 Slot A（带校验）
 *
 *         流程：校验 fw_size → 擦除 Slot A → 逐块复制（256 字节） → 逐块读回对比校验
 *         每 16KB 打印一次进度。
 *
 * @param  fw_size  固件大小（字节）
 * @retval 0   复制并校验成功
 * @retval -1  fw_size 非法、擦除失败、写入失败或校验不通过
 */
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

//*** SHA-256 Verification ***//

/**
 * @brief  计算 Flash 区域的 SHA-256 并与期望值比对
 *
 *         分块读取 Flash 内容（256 字节/块），计算 SHA-256 摘要，
 *         与 OTA Config 中存储的哈希值比对。
 *         不匹配则拒绝启动，防止损坏固件运行。
 *
 * @param  addr      Flash 起始地址
 * @param  fw_size   固件大小（字节）
 * @param  expected  期望的 32 字节 SHA-256 哈希值
 * @retval 0   哈希匹配
 * @retval -1  哈希不匹配
 *
 * @note   栈开销约 420 字节（SHA256_CTX 112 + buf 256 + hash 32）
 */
static int Boot_VerifySHA256(uint32_t addr, uint32_t fw_size,
                              const uint8_t expected[32])
{
    SHA256_CTX ctx;
    uint8_t    buf[256];
    uint8_t    hash[32];
    uint32_t   offset = 0;
    uint32_t   chunk;
    uint32_t   count  = 0;

    BspUart_Printf("[BOOT] SHA-256 verifying (%lu bytes) ...\r\n", fw_size);

    sha256_init(&ctx);

    while (offset < fw_size)
    {
        chunk = fw_size - offset;
        if (chunk > sizeof(buf))
        {
            chunk = sizeof(buf);
        }

        BspFlash_Read(addr + offset, buf, chunk);
        sha256_update(&ctx, buf, chunk);

        offset += chunk;
        count++;

        if ((count % 64) == 0)
        {
            BspUart_Printf("[BOOT]   %lu / %lu bytes\r\n", offset, fw_size);
        }
    }

    sha256_final(&ctx, hash);

    /* 打印计算哈希（调试用） */
    // clang-format off
    BspUart_Printf("[BOOT] SHA-256: "
                   "%02X%02X%02X%02X%02X%02X%02X%02X"
                   "%02X%02X%02X%02X%02X%02X%02X%02X"
                   "%02X%02X%02X%02X%02X%02X%02X%02X"
                   "%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                   hash[0],hash[1],hash[2],hash[3],hash[4],hash[5],hash[6],hash[7],
                   hash[8],hash[9],hash[10],hash[11],hash[12],hash[13],hash[14],hash[15],
                   hash[16],hash[17],hash[18],hash[19],hash[20],hash[21],hash[22],hash[23],
                   hash[24],hash[25],hash[26],hash[27],hash[28],hash[29],hash[30],hash[31]);
    // clang-format on

    if (memcmp(hash, expected, 32) != 0)
    {
        BspUart_Printf("[BOOT] SHA-256 MISMATCH!\r\n");
        return -1;
    }

    BspUart_Printf("[BOOT] SHA-256 OK.\r\n");
    return 0;
}

//*** Public API ***//

/**
 * @brief  Bootloader 主入口
 *
 *         初始化 UART + Flash，打印启动信息，读取 OTA Config。
 *         根据 OTA 状态机分支处理：
 *         - IDLE / CONFIRMED → 直接跳转 Slot A
 *         - UPGRADE_PENDING  → 复制 Slot B→A → 跳转
 *         - CONFIRMING       → boot_count++ → 跳转（≥3 则 ROLLBACK）
 *         - ROLLBACK         → 停留在 Boot，等待恢复
 *         Config 无效时尝试直接跳转 Slot A。
 *         跳转成功后不返回；无有效 APP 时返回。
 */
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

    BspUart_Printf("[BOOT] State=%s, boot_count=%lu, fw_size=%lu\r\n",
                   StateStr(cfg.state), cfg.boot_count, cfg.fw_size);

    switch (cfg.state)
    {
        case OTA_STATE_UPGRADE_PENDING:
            if (Boot_CopySlot(cfg.fw_size) == 0)
            {
                /* SHA-256 完整性校验 */
                if (Boot_VerifySHA256(FLASH_ADDR_SLOT_A, cfg.fw_size,
                                      cfg.fw_sha256) != 0)
                {
                    cfg.state = OTA_STATE_ROLLBACK;
                    BspFlash_WriteConfig(&cfg);
                    BspUart_Printf("[BOOT] SHA-256 failed! Recovery mode.\r\n");
                    break;
                }

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

/**
 * @brief  恢复等待循环（在 main while(1) 中调用）
 *
 *         当 Bootloader 无法跳转到 APP 时（无有效 APP / ROLLBACK 状态），
 *         每 200ms 翻转 PC13 LED 并通过 UART 输出等待恢复信息。
 */
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
