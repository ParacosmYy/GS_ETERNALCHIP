/**
 * @file    bsp_flash_handler.c
 * @brief   Flash Handler — backward-compatible facade over Flash Driver
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_flash_handler.h
 * - bsp_flash_driver.h
 * - FreeRTOS.h / task.h (temporary, moves to system_adaption in Phase 8)
 * - stm32f4xx_hal.h (temporary HAL glue)
 * - string.h
 *
 * @note    This file exposes the SAME function signatures as the original
 *          bsp_flash.c procedural API. Upper layers call BspFlash_Init(),
 *          BspFlash_Write(), etc. — no changes needed in callers.
 */

#include "bsp_flash_handler.h"
#include "bsp_flash_driver.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

//*** Temporary HAL glue — will move to system_adaption.c in Phase 8 ***//

static int hal_flash_unlock(void)
{
    HAL_FLASH_Unlock();
    return 0;
}

static int hal_flash_lock(void)
{
    HAL_FLASH_Lock();
    return 0;
}

static void hal_flash_clear_errors(void)
{
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR |
                           FLASH_FLAG_RDERR);
}

static int hal_flash_erase_sector(uint32_t sector, uint32_t voltage_range)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t               sector_error = 0;
    HAL_StatusTypeDef      status;

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.Sector       = sector;
    erase.NbSectors    = 1;
    erase.VoltageRange = voltage_range;

    status = HAL_FLASHEx_Erase(&erase, &sector_error);

    if (status == HAL_OK && sector_error == 0xFFFFFFFFu)
    {
        return 0;
    }
    return -1;
}

static int hal_flash_program_word(uint32_t addr, uint32_t data)
{
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

static const flash_hw_operations_t s_flash_hal_ops = {
    .pf_unlock       = hal_flash_unlock,
    .pf_lock         = hal_flash_lock,
    .pf_clear_errors = hal_flash_clear_errors,
    .pf_erase_sector = hal_flash_erase_sector,
    .pf_program_word = hal_flash_program_word,
};

static void hal_critical_enter(void)
{
    taskENTER_CRITICAL();
}

static void hal_critical_exit(void)
{
    taskEXIT_CRITICAL();
}

static const flash_os_operations_t s_flash_os_ops = {
    .pf_critical_enter = hal_critical_enter,
    .pf_critical_exit  = hal_critical_exit,
};

static uint32_t hal_get_tick(void)
{
    return HAL_GetTick();
}

static const flash_time_operations_t s_flash_time_ops = {
    .pf_get_tick = hal_get_tick,
};

//*** Static Configuration (F411 layout from ota_config.h) ***//

static const bsp_flash_config_t s_flash_config = {
    .boot_addr   = FLASH_ADDR_BOOT,
    .config_addr = FLASH_ADDR_CONFIG,
    .slot_a_addr = FLASH_ADDR_SLOT_A,
    .slot_b_addr = FLASH_ADDR_SLOT_B,
    .slot_a_size = SLOT_A_SIZE,
    .slot_b_size = SLOT_B_SIZE,
};

//*** Static Instances ***//

static bsp_flash_driver_t  s_flash_driver;
static bsp_flash_handler_t s_flash_handler;

//*** Handler API Implementation (backward-compatible) ***//

/**
 * @brief  Initialize Flash driver and handler.
 *
 * Steps:
 *  1. Initialize driver with config + HAL ops.
 *  2. Initialize handler with driver instance.
 *
 * @note  Called once at startup. Clears residual Flash error flags.
 * */
void BspFlash_Init(void)
{
    BspFlash_DriverInit(&s_flash_driver,
                        &s_flash_config,
                        &s_flash_hal_ops,
                        &s_flash_os_ops,
                        &s_flash_time_ops);

    BspFlashHandler_Init(&s_flash_handler, &s_flash_driver);
}

/**
 * @brief  Initialize handler with a pre-configured driver.
 *
 * @param[out] p_handler : Handler instance.
 * @param[in]  p_driver  : Pre-initialized driver instance.
 * */
void BspFlashHandler_Init(bsp_flash_handler_t *p_handler,
                          bsp_flash_driver_t *p_driver)
{
    if (p_handler == NULL || p_driver == NULL)
    {
        return;
    }

    p_handler->p_driver  = p_driver;
    p_handler->is_inited = 1;
}

/**
 * @brief  Erase all sectors belonging to an OTA slot (one-by-one).
 *
 * Steps:
 *  1. Map slot to sector range.
 *  2. Unlock Flash via HW ops, clear error flags.
 *  3. Erase each sector independently with its own critical section
 *     (avoid masking interrupts for 3~8 seconds across multi-sector erase).
 *  4. Lock Flash, return result.
 *
 * @param[in] slot : OTA slot (OTA_SLOT_A or OTA_SLOT_B).
 *
 * @return   0 : Erase success.
 * @return  -1 : Invalid slot or erase failure.
 * */
int BspFlash_EraseSlot(ota_slot_t slot)
{
    bsp_flash_driver_t *p_drv = s_flash_handler.p_driver;
    uint32_t start_sector;
    uint32_t nb_sectors;
    uint32_t i;
    int      ret;

    if (slot != OTA_SLOT_A && slot != OTA_SLOT_B)
    {
        return -1;
    }

    /* Slot A: Sector 2-5 (224KB), Slot B: Sector 6-7 (256KB) */
    if (slot == OTA_SLOT_A)
    {
        start_sector = FLASH_SECTOR_2;
        nb_sectors   = 4;
    }
    else
    {
        start_sector = FLASH_SECTOR_6;
        nb_sectors   = 2;
    }

    ret = p_drv->p_hw_ops->pf_unlock();
    if (ret != 0)
    {
        return -1;
    }

    p_drv->p_hw_ops->pf_clear_errors();

    /* 逐扇区擦除：每个扇区独立临界区，避免长时间屏蔽中断 */
    for (i = 0; i < nb_sectors; i++)
    {
        p_drv->p_os_ops->pf_critical_enter();
        ret = p_drv->p_hw_ops->pf_erase_sector(start_sector + i,
                                                BSP_FLASH_VOLTAGE_RANGE);
        p_drv->p_os_ops->pf_critical_exit();

        if (ret != 0)
        {
            p_drv->p_hw_ops->pf_lock();
            return -1;
        }
    }

    p_drv->p_hw_ops->pf_lock();
    return 0;
}

/**
 * @brief  Write data to Flash (4-byte aligned, tail padded with 0xFF).
 *
 * @param[in] addr   : Target Flash address.
 * @param[in] p_data : Data buffer to write.
 * @param[in] len    : Data length in bytes.
 *
 * @return   0 : Write success.
 * @return  -1 : Invalid params or write failure.
 * */
int BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len)
{
    return BspFlash_DriverWrite(s_flash_handler.p_driver, addr, p_data, len);
}

/**
 * @brief  Read data from Flash (memory-mapped direct read).
 *
 * @param[in]  addr   : Flash start address.
 * @param[out] p_data : Output data buffer.
 * @param[in]  len    : Read length in bytes.
 * */
void BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len)
{
    BspFlash_DriverRead(s_flash_handler.p_driver, addr, p_data, len);
}

//*** Config R/W ***//

/** @brief  Sector 1 有效数据区大小：Config + Trace + Crash Dump */
#define SECTOR1_USED_SIZE  ((CRASH_DUMP_ADDR - FLASH_ADDR_CONFIG) + CRASH_DUMP_SIZE)

/* Forward declaration — CRC calc lives in bsp_flash_driver.c */
extern uint32_t BspFlash_DriverCalcCrc32(const uint8_t *p_data, uint32_t len);

/**
 * @brief  从 Flash Config 区读取 OTA 配置，并校验 CRC-32。
 *
 * Steps:
 *  1. 从 FLASH_ADDR_CONFIG 复制 ota_config_t 结构。
 *  2. 校验 magic 字段，不匹配则清零返回 -1。
 *  3. 计算并校验 CRC-32，不匹配返回 -2。
 *
 * @param[out] p_cfg : 输出 OTA 配置结构。
 *
 * @return   0 : 读取成功且 CRC 校验通过。
 * @return  -1 : 参数无效或 magic 不匹配。
 * @return  -2 : CRC 校验失败。
 * */
int BspFlash_ReadConfig(ota_config_t *p_cfg)
{
    uint32_t crc;

    if (p_cfg == NULL)
    {
        return -1;
    }

    memcpy(p_cfg, (const void *)FLASH_ADDR_CONFIG, sizeof(ota_config_t));

    /* 首次上电或 Config 区未初始化 */
    if (p_cfg->magic != OTA_CONFIG_MAGIC)
    {
        memset(p_cfg, 0, sizeof(ota_config_t));
        return -1;
    }

    /* CRC 校验覆盖除 crc32 字段外的所有字段 */
    crc = BspFlash_DriverCalcCrc32((const uint8_t *)p_cfg,
                                   offsetof(ota_config_t, crc32));
    if (crc != p_cfg->crc32)
    {
        return -2;
    }

    return 0;
}

/**
 * @brief  将 OTA 配置写入 Flash Config 区，保留 Trace 和 Crash Dump。
 *
 * Steps:
 *  1. 读取 Sector 1 全部有效数据（Config + Trace + Crash Dump）到静态缓冲区。
 *  2. 计算新 Config 的 CRC-32，更新缓冲区中的 Config 部分。
 *  3. 擦除 Sector 1。
 *  4. 将缓冲区整体写回 Flash。
 *  5. 读回 Config 验证 CRC。
 *
 * @param[in] p_cfg : OTA 配置结构指针。
 *
 * @return   0 : 写入成功且读回验证通过。
 * @return  -1 : 参数无效、擦除或写入失败。
 * @return  -2 : 读回验证失败。
 * */
int BspFlash_WriteConfig(const ota_config_t *p_cfg)
{
    static uint8_t s_buf[SECTOR1_USED_SIZE]; /* static: 避免栈溢出 */
    ota_config_t tmp;
    ota_config_t verify;
    int          ret;

    if (p_cfg == NULL)
    {
        return -1;
    }

    /* 计算 CRC，覆盖除 crc32 字段外的所有字段 */
    tmp       = *p_cfg;
    tmp.crc32 = BspFlash_DriverCalcCrc32((const uint8_t *)&tmp,
                                         offsetof(ota_config_t, crc32));

    /* 读取 Sector 1 全部有效数据（Config + Trace + Crash Dump） */
    memcpy(s_buf, (const void *)FLASH_ADDR_CONFIG, SECTOR1_USED_SIZE);

    /* 只更新 Config 部分，Trace 和 Crash Dump 保持原样 */
    memcpy(s_buf, &tmp, sizeof(ota_config_t));

    /* 擦除 Sector 1，写回全部数据 */
    if (BspFlash_DriverEraseSector(s_flash_handler.p_driver,
                                   FLASH_SECTOR_1) != 0)
    {
        return -1;
    }

    ret = BspFlash_Write(FLASH_ADDR_CONFIG, s_buf, SECTOR1_USED_SIZE);
    if (ret != 0)
    {
        return -1;
    }

    /* 读回验证 Config 部分 */
    if (BspFlash_ReadConfig(&verify) != 0)
    {
        return -2;
    }

    return 0;
}

//*** Utility ***//

/**
 * @brief  获取扇区大小（字节）。
 *
 * @param[in] sector_num : 扇区编号（0 ~ 7）。
 *
 * @return  扇区大小（字节），无效编号返回 0。
 * */
uint32_t BspFlash_GetSectorSize(uint32_t sector_num)
{
    return BspFlash_DriverGetSectorSize(s_flash_handler.p_driver, sector_num);
}

/**
 * @brief  根据 Slot 编号获取 Flash 起始地址。
 *
 * @param[in] slot : OTA 槽位（OTA_SLOT_A 或 OTA_SLOT_B）。
 *
 * @return  Flash 起始地址，无效槽位返回 0。
 * */
uint32_t BspFlash_GetSlotAddress(ota_slot_t slot)
{
    if (slot != OTA_SLOT_A && slot != OTA_SLOT_B)
    {
        return 0;
    }

    if (slot == OTA_SLOT_A)
    {
        return FLASH_ADDR_SLOT_A;
    }
    return FLASH_ADDR_SLOT_B;
}

/**
 * @brief  检查 Flash 指定区域是否全为 0xFF（已擦除状态）。
 *
 * @param[in] addr : Flash 起始地址。
 * @param[in] len  : 检查长度（字节）。
 *
 * @return  1 : 区域为空（全 0xFF）。
 * @return  0 : 区域非空。
 * */
int BspFlash_IsEmpty(uint32_t addr, uint32_t len)
{
    const uint8_t *p;
    uint32_t       i;

    if (len == 0)
    {
        return 1;
    }

    p = (const uint8_t *)addr;

    for (i = 0; i < len; i++)
    {
        if (p[i] != 0xFF)
        {
            return 0;
        }
    }

    return 1;
}
