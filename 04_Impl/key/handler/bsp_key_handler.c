/**
 * @file    bsp_key_handler.c
 * @brief   按键实例注册表实现 — 多按键统一管理
 * @author  GS_Mark
 *
 * @par dependencies
 * - bsp_key_handler.h
 */

//*** 头文件 ***//
#include "bsp_key_handler.h"
#include <string.h>

//*** 静态实例 ***//

static key_instance_table_t s_key_table;

//*** 公共 API ***//

/**
 * @brief  初始化按键实例注册表（清零所有槽位）。
 * */
void BspKeyHandler_Init(void)
{
    memset(&s_key_table, 0, sizeof(s_key_table));
}

/**
 * @brief  将按键驱动实例注册到注册表。
 *
 * @param[in] p_drv : 按键驱动实例指针。
 * */
void BspKeyHandler_Register(bsp_key_driver_t *p_drv)
{
    uint32_t i;

    if (p_drv == NULL)
    {
        return;
    }

    /* 避免重复注册 */
    for (i = 0; i < s_key_table.count; i++)
    {
        if (s_key_table.instances[i] == p_drv)
        {
            return;
        }
    }

    if (s_key_table.count < BSP_KEY_MAX_INSTANCES)
    {
        s_key_table.instances[s_key_table.count] = p_drv;
        s_key_table.count++;
    }
}

/**
 * @brief  从注册表注销按键驱动实例。
 *
 * @param[in] p_drv : 按键驱动实例指针。
 * */
void BspKeyHandler_Unregister(bsp_key_driver_t *p_drv)
{
    uint32_t i;
    uint32_t j;

    if (p_drv == NULL)
    {
        return;
    }

    for (i = 0; i < s_key_table.count; i++)
    {
        if (s_key_table.instances[i] == p_drv)
        {
            /* 后面的实例前移 */
            for (j = i; j < s_key_table.count - 1; j++)
            {
                s_key_table.instances[j] = s_key_table.instances[j + 1];
            }
            s_key_table.count--;
            s_key_table.instances[s_key_table.count] = NULL;
            return;
        }
    }
}

/**
 * @brief  批量扫描所有已注册按键。
 *
 *         需在主循环中周期调用（建议 1~10ms）。
 * */
void BspKeyHandler_ScanAll(void)
{
    uint32_t i;

    for (i = 0; i < s_key_table.count; i++)
    {
        if (s_key_table.instances[i] != NULL)
        {
            BspKey_Scan(s_key_table.instances[i]);
        }
    }
}

/**
 * @brief  通过索引查找已注册的按键驱动实例。
 *
 * @param[in] index : 实例索引（0 ~ count-1）。
 *
 * @return  按键驱动实例指针，索引越界返回 NULL。
 * */
bsp_key_driver_t *BspKeyHandler_GetByIndex(uint32_t index)
{
    if (index >= s_key_table.count)
    {
        return NULL;
    }
    return s_key_table.instances[index];
}
