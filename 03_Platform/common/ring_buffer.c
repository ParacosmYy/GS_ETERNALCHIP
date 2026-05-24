/**
 * @file    ring_buffer.c
 * @brief   通用环形缓冲区实现
 * @author  GS_Mark
 *
 * @par dependencies
 * - ring_buffer.h
 *
 * @version V1.0 2026-05-22
 *
 * @note 1 tab == 4 spaces
 *
 * */

//******************************* Includes **********************************//
#include "ring_buffer.h"
#include <stddef.h>
//******************************* Includes **********************************//

//******************************* Implement ********************************//

/**
 * @brief  初始化环形缓冲区。
 *
 * @param[in] p_buf  : 环形缓冲区结构体指针。
 * @param[in] p_data : 底层数据数组。
 * @param[in] size   : 缓冲区大小（字节）。
 * */
void ring_buffer_init(circular_buffer_t *p_buf, uint8_t *p_data, uint32_t size)
{
    p_buf->p_buffer = p_data;
    p_buf->head     = 0;
    p_buf->tail     = 0;
    p_buf->size     = size;
}

/**
 * @brief  检查环形缓冲区是否为空。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return true 为空，false 为非空。
 * */
bool ring_buffer_is_empty(const circular_buffer_t *p_buf)
{
    if (p_buf == NULL)
    {
        return true;
    }

    return (p_buf->head % p_buf->size) == (p_buf->tail % p_buf->size);
}

/**
 * @brief  检查环形缓冲区是否已满。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return true 为满，false 为未满。
 * */
bool ring_buffer_is_full(const circular_buffer_t *p_buf)
{
    if (p_buf == NULL)
    {
        return false;
    }

    return ((p_buf->head + 1) % p_buf->size) == (p_buf->tail % p_buf->size);
}

/**
 * @brief  获取环形缓冲区中已使用的数据长度。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return 可读字节数。
 * */
uint32_t ring_buffer_available(const circular_buffer_t *p_buf)
{
    if (p_buf == NULL)
    {
        return 0;
    }

    return (p_buf->head - p_buf->tail + p_buf->size) % p_buf->size;
}

/**
 * @brief  获取环形缓冲区中剩余可写空间。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return 剩余可写字节数。
 * */
uint32_t ring_buffer_remaining(const circular_buffer_t *p_buf)
{
    if (p_buf == NULL)
    {
        return 0;
    }

    return p_buf->size - 1 - ring_buffer_available(p_buf);
}

/**
 * @brief  从环形缓冲区读取一个字节（pop）。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return 读取的字节值（0~255），-1 表示缓冲区为空。
 * */
int16_t ring_buffer_get_byte(circular_buffer_t *p_buf)
{
    uint8_t byte = 0;

    if (p_buf == NULL || ring_buffer_is_empty(p_buf))
    {
        return -1;
    }

    byte = p_buf->p_buffer[p_buf->tail % p_buf->size];
    p_buf->tail++;

    return (int16_t)byte;
}

/**
 * @brief  向环形缓冲区写入一个字节（push）。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 * @param[in] byte  : 待写入的字节。
 *
 * @return true 成功，false 缓冲区已满或参数无效。
 * */
bool ring_buffer_put_byte(circular_buffer_t *p_buf, uint8_t byte)
{
    if (p_buf == NULL || ring_buffer_is_full(p_buf))
    {
        return false;
    }

    p_buf->p_buffer[p_buf->head % p_buf->size] = byte;
    p_buf->head++;

    return true;
}
