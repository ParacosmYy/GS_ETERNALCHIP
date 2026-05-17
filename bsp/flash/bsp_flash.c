/**
 * @file    bsp_flash.c
 * @brief   Internal Flash BSP driver implementation
 */

#include "bsp_flash.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

//*** Private: CRC-32 Lookup Table (IEEE 802.3) ***//

static const uint32_t s_crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBBBD6, 0xACBCCB40, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7F89,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD706B3, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

static uint32_t calc_crc32(const uint8_t *p_data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        crc = s_crc32_table[(crc ^ p_data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

//*** Sector Info ***//
// STM32F411CEUx 擦除粒度为扇区(Sector)，无页(Page)概念
// Sector 0-3 各 16KB, Sector 4 = 64KB, Sector 5-7 各 128KB
// 合计: 4×16K + 64K + 3×128K = 64K + 64K + 384K = 512KB

uint32_t BspFlash_GetSectorSize(uint32_t sector_num)
{
    /* 参数校验: sector_num 有效范围 FLASH_SECTOR_0 ~ FLASH_SECTOR_7 */
    if (sector_num > FLASH_SECTOR_7)
    {
        return 0;
    }

    // clang-format off
    switch (sector_num)
    {
        case FLASH_SECTOR_0:
            return 16384u; /* 16KB */

        case FLASH_SECTOR_1:
            return 16384u; /* 16KB */

        case FLASH_SECTOR_2:
            return 16384u; /* 16KB */

        case FLASH_SECTOR_3:
            return 16384u; /* 16KB */

        case FLASH_SECTOR_4:
            return 65536u; /* 64KB */

        case FLASH_SECTOR_5:
            return 131072u; /* 128KB */

        case FLASH_SECTOR_6:
            return 131072u; /* 128KB */

        case FLASH_SECTOR_7:
            return 131072u; /* 128KB */

        default:
            return 0;
    }
    // clang-format on
}

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

//*** Init ***//

void BspFlash_Init(void)
{
    /* 清除上电/复位后可能残留的 FLASH_SR 错误标志 */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR |
                           FLASH_FLAG_RDERR);
}

//*** Erase ***//

int BspFlash_EraseSector(uint32_t sector_num)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t               sector_error = 0;
    HAL_StatusTypeDef      status;

    if (sector_num > FLASH_SECTOR_7)
    {
        return -1;
    }

    HAL_FLASH_Unlock();

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR |
                           FLASH_FLAG_RDERR);

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.Sector       = sector_num;
    erase.NbSectors    = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    taskENTER_CRITICAL();
    status = HAL_FLASHEx_Erase(&erase, &sector_error);
    taskEXIT_CRITICAL();

    HAL_FLASH_Lock();

    if (status == HAL_OK && sector_error == 0xFFFFFFFFu)
    {
        return 0;
    }
    return -1;
}

int BspFlash_EraseSlot(ota_slot_t slot)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t               sector_error = 0;
    HAL_StatusTypeDef      status;

    if (slot != OTA_SLOT_A && slot != OTA_SLOT_B)
    {
        return -1;
    }

    HAL_FLASH_Unlock();

    /* 清除残留错误标志，防止跨复位遗留 */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR |
                           FLASH_FLAG_RDERR);

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (slot == OTA_SLOT_A)
    {
        erase.Sector    = FLASH_SECTOR_2;
        erase.NbSectors = 4; /* Sector 2,3,4,5 */
    }
    else
    {
        erase.Sector    = FLASH_SECTOR_6;
        erase.NbSectors = 2; /* Sector 6,7 */
    }

    taskENTER_CRITICAL();
    status = HAL_FLASHEx_Erase(&erase, &sector_error);
    taskEXIT_CRITICAL();

    HAL_FLASH_Lock();

    if (status == HAL_OK && sector_error == 0xFFFFFFFFu)
    {
        return 0;
    }
    return -1;
}

//*** Write ***//

int BspFlash_Write(uint32_t addr, const uint8_t *p_data, uint32_t len)
{
    uint32_t i      = 0;
    int      result = 0;
    uint32_t word;

    if (p_data == NULL || len == 0)
    {
        return -1;
    }
    /* 地址范围校验: 必须在 Flash 区域内 */
    if (addr < FLASH_ADDR_BOOT || addr + len > FLASH_ADDR_SLOT_B + SLOT_B_SIZE)
    {
        return -1;
    }

    HAL_FLASH_Unlock();

    /* 清除残留错误标志 */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR |
                           FLASH_FLAG_RDERR);

    taskENTER_CRITICAL();

    /* Write 4-byte words (x32 parallelism, no VPP required) */
    while (i + 4 <= len)
    {
        memcpy(&word, &p_data[i], 4);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word) != HAL_OK)
        {
            result = -1;
            goto exit;
        }
        i += 4;
    }

    /* Pad remaining bytes with 0xFF */
    if (i < len)
    {
        uint8_t  buf[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
        uint32_t remain = len - i;

        memcpy(buf, &p_data[i], remain);
        memcpy(&word, buf, 4);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word) != HAL_OK)
        {
            result = -1;
            goto exit;
        }
    }

exit:
    taskEXIT_CRITICAL();
    HAL_FLASH_Lock();
    return result;
}

//*** Read ***//

void BspFlash_Read(uint32_t addr, uint8_t *p_data, uint32_t len)
{
    if (p_data == NULL || len == 0)
    {
        return;
    }
    memcpy(p_data, (const void *)addr, len);
}

//*** Config R/W ***//

int BspFlash_ReadConfig(ota_config_t *p_cfg)
{
    uint32_t crc;

    if (p_cfg == NULL)
    {
        return -1;
    }

    memcpy(p_cfg, (const void *)FLASH_ADDR_CONFIG, sizeof(ota_config_t));

    if (p_cfg->magic != OTA_CONFIG_MAGIC)
    {
        memset(p_cfg, 0, sizeof(ota_config_t));
        return -1;
    }

    crc = calc_crc32((const uint8_t *)p_cfg, offsetof(ota_config_t, crc32));
    if (crc != p_cfg->crc32)
    {
        return -2;
    }

    return 0;
}

int BspFlash_WriteConfig(const ota_config_t *p_cfg)
{
    ota_config_t tmp;

    if (p_cfg == NULL)
    {
        return -1;
    }

    /* Compute CRC over everything except the crc32 field */
    tmp       = *p_cfg;
    tmp.crc32 = calc_crc32((const uint8_t *)&tmp, offsetof(ota_config_t, crc32));

    /* Sector 1 专用存放 Config，直接擦除后写入即可 */
    if (BspFlash_EraseSector(FLASH_SECTOR_1) != 0)
    {
        return -1;
    }

    return BspFlash_Write(FLASH_ADDR_CONFIG, (const uint8_t *)&tmp, sizeof(ota_config_t));
}

//*** Utility ***//

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
