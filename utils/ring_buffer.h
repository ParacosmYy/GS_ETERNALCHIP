/**
 * @file    ring_buffer.h
 * @brief   通用环形缓冲区（适用于串口 DMA 等不定长数据场景）
 * @author  GS_Mark
 *
 * @par dependencies
 * - stdint.h
 * - stdbool.h
 *
 * DMA + IDLE 模式下，ISR 推进 head，应用层消费 tail。
 * 只要应用层处理够快，小 Buffer 能收大量数据。
 * 判满牺牲一个位置：(head + 1) % size == tail 区分空和满。
 * 缓冲区由外部分配，本模块只管理指针。
 *
 * @version V1.0 2026-05-22
 *
 * @note 1 tab == 4 spaces
 *
 * */
#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

//******************************* Includes **********************************//
#include <stdint.h>
#include <stdbool.h>
//******************************* Includes **********************************//

//******************************* Types *************************************//

/** @brief  环形缓冲区描述符 */
typedef struct
{
    uint8_t *p_buffer;  /* 数据缓冲区（外部分配） */
    uint32_t head;      /* 写指针（ISR 推进） */
    uint32_t tail;      /* 读指针（应用消费） */
    uint32_t size;      /* 缓冲区总大小 */
} circular_buffer_t;

//******************************* Declaring *********************************//

/**
 * @brief  初始化环形缓冲区。
 *
 * @param[in] p_buf  : 环形缓冲区结构体指针。
 * @param[in] p_data : 底层数据数组。
 * @param[in] size   : 缓冲区大小（字节）。
 * */
void ring_buffer_init(circular_buffer_t *p_buf, uint8_t *p_data, uint32_t size);

/**
 * @brief  检查环形缓冲区是否为空。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return true 为空，false 为非空。
 * */
bool ring_buffer_is_empty(const circular_buffer_t *p_buf);

/**
 * @brief  检查环形缓冲区是否已满。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return true 为满，false 为未满。
 * */
bool ring_buffer_is_full(const circular_buffer_t *p_buf);

/**
 * @brief  获取环形缓冲区中已使用的数据长度。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return 可读字节数。
 * */
uint32_t ring_buffer_available(const circular_buffer_t *p_buf);

/**
 * @brief  获取环形缓冲区中剩余可写空间。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return 剩余可写字节数。
 * */
uint32_t ring_buffer_remaining(const circular_buffer_t *p_buf);

/**
 * @brief  从环形缓冲区读取一个字节（pop）。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 *
 * @return 读取的字节值（0~255），-1 表示缓冲区为空。
 * */
int16_t ring_buffer_get_byte(circular_buffer_t *p_buf);

/**
 * @brief  向环形缓冲区写入一个字节（push）。
 *
 * @param[in] p_buf : 环形缓冲区结构体指针。
 * @param[in] byte  : 待写入的字节。
 *
 * @return true 成功，false 缓冲区已满或参数无效。
 * */
bool ring_buffer_put_byte(circular_buffer_t *p_buf, uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif /* __RING_BUFFER_H__ */
