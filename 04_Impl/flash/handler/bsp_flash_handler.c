/**
 * @file    bsp_flash_handler.c
 * @brief   Flash Handler — 向后兼容的 Flash Driver 门面
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_flash_handler.h
 * - bsp_flash_driver.h
 * - FreeRTOS.h / task.h（临时，Phase 8 迁移到 system_adaption）
 * - stm32f4xx_hal.h（临时 HAL 胶水）
 * - string.h
 *
 * @note    本文件暴露与原始 bsp_flash.c 过程式 API 相同的函数签名。
 *          上层调用 BspFlash_Init()、BspFlash_Write() 等无需修改。
 */

#include "bsp_flash_handler.h"
#include "bsp_flash_driver.h"
#include "system_adaption.h"
#include "stm32f4xx_hal.h"
#include <string.h>

//*** 静态实例 ***//

static bsp_flash_driver_t  s_flash_driver;
static bsp_flash_handler_t s_flash_handler;

//*** Handler API 实现（向后兼容） ***//

/**
 * @brief  初始化 Flash 驱动和 Handler。
 *
 * Steps:
 *  1. 使用配置和 HAL ops 初始化驱动。
 *  2. 使用驱动实例初始化 Handler。
 *
 * @note  启动时调用一次。清除残留的 Flash 错误标志。
 * */
void BspFlash_Init(void)
{
    BspFlash_DriverInit(&s_flash_driver,
                        &g_flash_config,
                        &g_flash_hal_ops,
                        &g_flash_os_ops,
                        &g_flash_time_ops);

    BspFlashHandler_Init(&s_flash_handler, &s_flash_driver);
}

/**
 * @brief  用已配置的驱动实例初始化 Handler。
 *
 * @param[out] p_handler : Handler 实例。
 * @param[in]  p_driver  : 已初始化的驱动实例。
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
 * @brief  擦除 OTA 槽位所属的所有扇区（逐扇区擦除）。
 *
 * Steps:
 *  1. 将槽位映射到扇区范围。
 *  2. 通过 HW ops 解锁 Flash，清除错误标志。
 *  3. 逐扇区独立擦除，每个扇区使用独立临界区
 *     （避免跨扇区擦除期间长时间屏蔽中断 3~8 秒）。
 *  4. 锁定 Flash，返回结果。
 *
 * @param[in] slot : OTA 槽位（OTA_SLOT_A 或 OTA_SLOT_B）。
 *
 * @return   0 : 擦除成功。
 * @return  -1 : 无效槽位或擦除失败。
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

    /* 槽位 A: Sector 2-5 (224KB), 槽位 B: Sector 6-7 (256KB) */
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
 * @brief  向 Flash 写入数据（4 字节对齐，末尾用 0xFF 填充）。
 *
 * @param[in] addr   : 目标 Flash 地址。
 * @param[in] p_data : 待写入数据缓冲区。
 * @param[in] len    : 数据长度（字节）。
 *
 * @return   0 : 写入成功。
 * @return  -1 : 无效参数或写入失败。
 * */
int BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len)
{
    return BspFlash_DriverWrite(s_flash_handler.p_driver, addr, p_data, len);
}

/**
 * @brief  从 Flash 读取数据（内存映射直接读取）。
 *
 * @param[in]  addr   : Flash 起始地址。
 * @param[out] p_data : 输出数据缓冲区。
 * @param[in]  len    : 读取长度（字节）。
 * */
void BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len)
{
    BspFlash_DriverRead(s_flash_handler.p_driver, addr, p_data, len);
}

//*** 配置读写 ***//

/** @brief  Sector 1 有效数据区大小：Config + Trace + Crash Dump */
#define SECTOR1_USED_SIZE  ((CRASH_DUMP_ADDR - FLASH_ADDR_CONFIG) + CRASH_DUMP_SIZE)

/* 前向声明 — CRC 计算位于 bsp_flash_driver.c */
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

/**
 * @brief  清除 Trace 区域为 0xFF，保留 Config 和 Crash Dump。
 *
 * Steps:
 *  1. 读取 Sector 1 全部有效数据到静态缓冲区。
 *  2. 将 trace 部分填充 0xFF。
 *  3. 擦除 Sector 1，写回全部数据。
 *
 * @return   0 : 成功。
 * @return  -1 : 擦除或写入失败。
 *
 * */
int BspFlash_ClearTrace(void)
{
    static uint8_t s_buf[SECTOR1_USED_SIZE];

    /* 读取 Sector 1 全部有效数据 */
    memcpy(s_buf, (const void *)FLASH_ADDR_CONFIG, SECTOR1_USED_SIZE);

    /* 将 trace 部分填充 0xFF（保留 Config + Crash Dump） */
    memset(s_buf + (OTA_TRACE_ADDR - FLASH_ADDR_CONFIG),
           0xFF,
           OTA_TRACE_MAX_ENTRIES * OTA_TRACE_ENTRY_SIZE);

    /* 擦除 Sector 1，写回全部数据 */
    if (BspFlash_DriverEraseSector(s_flash_handler.p_driver,
                                    FLASH_SECTOR_1) != 0)
    {
        return -1;
    }

    if (BspFlash_Write(FLASH_ADDR_CONFIG, s_buf, SECTOR1_USED_SIZE) != 0)
    {
        return -1;
    }

    return 0;
}

//*** 工具函数 ***//

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
