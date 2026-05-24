/**
 * @file    system_adaption.c
 * @brief   系统适配层 — 所有 HAL/FreeRTOS 封装函数 + 操作接口表
 * @author  GS_Mark
 *
 * @par dependencies
 * - system_adaption.h
 * - stm32f4xx_hal.h
 * - FreeRTOS.h / task.h
 * - SEGGER_RTT.h / SEGGER_RTT_Printf.h
 * - bsp_flash_driver.h（用于 BSP_FLASH_VOLTAGE_RANGE）
 * - ota_config.h（用于 FLASH_ADDR_* 常量）
 *
 * @note    所有 HAL 胶水代码的唯一编译单元。
 *          其他 .c 文件不应直接调用 HAL_* 或 FreeRTOS API
 *          （ISR 上下文中的 HAL 回调和 Core/ 中的 CubeMX 代码除外）。
 */

//*** 头文件 ***//
#include "system_adaption.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "SEGGER_RTT.h"
#include "bsp_flash_driver.h"
#include "ota_config.h"
#include <string.h>
#include <stdarg.h>

//*** 私有宏 ***//

/** @brief  将 plat_gpio_t 不透明端口指针转换为 HAL GPIO_TypeDef* */
#define HAL_GPIO_PORT(p) ((GPIO_TypeDef *)(((const plat_gpio_t *)(p))->port))

/** @brief  从 plat_gpio_t 提取 HAL GPIO 引脚位掩码（1 << pin） */
#define HAL_GPIO_PIN(p)  ((uint16_t)(1U << ((const plat_gpio_t *)(p))->pin))

/** @brief  将 plat_gpio_t 的 port 字段转换为 HAL GPIO_TypeDef*（按值） */
#define HAL_GPIO_PORT_V(gpio) ((GPIO_TypeDef *)((gpio).port))

//*** LED HAL 操作接口 ***//

/**
 * @brief  HAL 实现：点亮 LED。
 *         低有效 LED 写 RESET，高有效 LED 写 SET。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
static void HalLed_On(bsp_led_driver_t *p_drv)
{
    const plat_gpio_t *p_gpio = &p_drv->p_config->gpio;

    if (p_gpio->active_level == 0)
    {
        HAL_GPIO_WritePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio), GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio), GPIO_PIN_SET);
    }
}

/**
 * @brief  HAL 实现：熄灭 LED。
 *         极性与 On 相反。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
static void HalLed_Off(bsp_led_driver_t *p_drv)
{
    const plat_gpio_t *p_gpio = &p_drv->p_config->gpio;

    if (p_gpio->active_level == 0)
    {
        HAL_GPIO_WritePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio), GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio), GPIO_PIN_RESET);
    }
}

/**
 * @brief  HAL 实现：翻转 LED 状态。
 *
 * @param[in] p_drv : LED 驱动实例指针。
 * */
static void HalLed_Toggle(bsp_led_driver_t *p_drv)
{
    const plat_gpio_t *p_gpio = &p_drv->p_config->gpio;

    HAL_GPIO_TogglePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio));
}

/**
 * @brief  LED HAL 操作接口实例（命名匹配 plat_led.h 的 extern 声明）。
 * */
const led_operations_t bsp_led_hal_ops = {
    .p_On     = HalLed_On,
    .p_Off    = HalLed_Off,
    .p_Toggle = HalLed_Toggle,
};

//*** 按键 HAL 操作接口 ***//

/**
 * @brief  读取原始引脚电平并转换为逻辑电平（0/1）。
 *         active_level=0（低有效）：引脚 RESET → 逻辑 1（按下）。
 *         active_level=1（高有效）：引脚 SET   → 逻辑 1（按下）。
 *
 * @param[in] p_cfg : 按键硬件配置指针。
 *
 * @return  1 : 按下。
 * @return  0 : 释放。
 * */
static uint8_t HalKey_ReadPin(const bsp_key_config_t *p_cfg)
{
    GPIO_PinState pin;

    pin = HAL_GPIO_ReadPin(HAL_GPIO_PORT_V(p_cfg->gpio), p_cfg->gpio.pin);

    if (p_cfg->gpio.active_level == 0)
    {
        if (pin == GPIO_PIN_RESET)
        {
            return 1;
        }
        return 0;
    }

    if (pin == GPIO_PIN_SET)
    {
        return 1;
    }
    return 0;
}

/**
 * @brief  获取系统 tick（HAL_GetTick 封装）。
 *
 * @return  当前 tick 计数（毫秒）。
 * */
static uint32_t HalKey_GetTick(void)
{
    return HAL_GetTick();
}

/** @brief  按键硬件操作接口实例 */
const key_hw_operations_t g_key_hal_ops = {
    .pf_read_pin = HalKey_ReadPin,
};

/** @brief  按键 OS 操作接口实例 */
const key_os_operations_t g_key_os_ops = {
    .pf_get_tick = HalKey_GetTick,
};

//*** 看门狗 HAL 操作接口 ***//

/**
 * @brief  刷新 IWDG 看门狗计数器。
 *
 * @param[in] p_handle : IWDG 句柄（从 void* 转换）。
 * */
static void HalWdg_Refresh(void *p_handle)
{
    IWDG_HandleTypeDef *p_hiwdg = (IWDG_HandleTypeDef *)p_handle;

    if (p_hiwdg != NULL && p_hiwdg->Instance != NULL)
    {
        HAL_IWDG_Refresh(p_hiwdg);
    }
}

/** @brief  看门狗硬件操作接口实例 */
const wdg_hw_operations_t g_wdg_hal_ops = {
    .pf_refresh = HalWdg_Refresh,
};

//*** 系统 HAL 操作接口 ***//

/**
 * @brief  获取系统 tick。
 *
 * @return  当前 tick 计数（毫秒）。
 * */
static uint32_t HalSys_GetTick(void)
{
    return HAL_GetTick();
}

/**
 * @brief  通过 NVIC 执行软件复位。
 * */
static void HalSys_Reboot(void)
{
    NVIC_SystemReset();
}

/**
 * @brief  从 SCB->VTOR 检测当前运行的 OTA Bank。
 *
 * @param[out] p_slot : 检测到的运行 Bank。
 *
 * @return   0 : 检测成功。
 * @return  -1 : 检测失败。
 * */
static int HalSys_GetRunningBank(ota_slot_t *p_slot)
{
    uint32_t vtor;

    if (p_slot == NULL)
    {
        return -1;
    }

    vtor = SCB->VTOR;
    if (vtor >= FLASH_ADDR_SLOT_B && vtor < (FLASH_ADDR_SLOT_B + SLOT_B_SIZE))
    {
        *p_slot = OTA_SLOT_B;
        return 0;
    }

    if (vtor >= FLASH_ADDR_SLOT_A && vtor < (FLASH_ADDR_SLOT_A + SLOT_A_SIZE))
    {
        *p_slot = OTA_SLOT_A;
        return 0;
    }

    return -1;
}

/** @brief  系统硬件操作接口实例 */
const sys_operations_t g_sys_hal_ops = {
    .pf_get_tick         = HalSys_GetTick,
    .pf_reboot           = HalSys_Reboot,
    .pf_get_running_bank = HalSys_GetRunningBank,
};

//*** RTT HAL 操作接口 ***//

/**
 * @brief  初始化 SEGGER RTT。
 *
 * @return  0 : 成功。
 * */
static int HalRtt_Init(void)
{
    SEGGER_RTT_Init();
    return 0;
}

/**
 * @brief  向 RTT 终端写入字符串（通道 0）。
 *
 * @param[in] s : 待写入的字符串。
 *
 * @return  >=0 : 写入字节数。
 * */
static int HalRtt_WriteString(const char *s)
{
    return SEGGER_RTT_Write(0, s, strlen(s));
}

/**
 * @brief  向 RTT 终端格式化输出（通道 0）。
 *
 * @param[in] fmt  : 格式化字符串。
 * @param[in] args : 可变参数列表。
 *
 * @return  >=0 : 写入字节数。
 * */
static int HalRtt_Vprintf(const char *fmt, va_list args)
{
    return SEGGER_RTT_vprintf(0, fmt, &args);
}

/** @brief  RTT 硬件操作接口实例 */
const rtt_operations_t g_rtt_hal_ops = {
    .pf_init         = HalRtt_Init,
    .pf_write_string = HalRtt_WriteString,
    .pf_vprintf      = HalRtt_Vprintf,
};

//*** Flash HAL 操作接口 ***//

/**
 * @brief  解锁 Flash，允许写入/擦除操作。
 *
 * @return  0 : 成功。
 * */
static int HalFlash_Unlock(void)
{
    HAL_FLASH_Unlock();
    return 0;
}

/**
 * @brief  锁定 Flash，写入/擦除操作完成后调用。
 *
 * @return  0 : 成功。
 * */
static int HalFlash_Lock(void)
{
    HAL_FLASH_Lock();
    return 0;
}

/**
 * @brief  清除残留的 Flash 错误标志。
 * */
static void HalFlash_ClearErrors(void)
{
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR |
                           FLASH_FLAG_RDERR);
}

/**
 * @brief  擦除单个 Flash 扇区。
 *
 * @param[in] sector        : 扇区编号（FLASH_SECTOR_x）。
 * @param[in] voltage_range : 擦除电压范围。
 *
 * @return   0 : 擦除成功。
 * @return  -1 : 擦除失败。
 * */
static int HalFlash_EraseSector(uint32_t sector, uint32_t voltage_range)
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

/**
 * @brief  向 Flash 编程一个 32 位字。
 *
 * @param[in] addr : 目标 Flash 地址（4 字节对齐）。
 * @param[in] data : 32 位数据值。
 *
 * @return   0 : 编程成功。
 * @return  -1 : 编程失败。
 * */
static int HalFlash_ProgramWord(uint32_t addr, uint32_t data)
{
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

/** @brief  Flash 硬件操作接口实例 */
const flash_hw_operations_t g_flash_hal_ops = {
    .pf_unlock       = HalFlash_Unlock,
    .pf_lock         = HalFlash_Lock,
    .pf_clear_errors = HalFlash_ClearErrors,
    .pf_erase_sector = HalFlash_EraseSector,
    .pf_program_word = HalFlash_ProgramWord,
};

/**
 * @brief  进入 FreeRTOS 临界区。
 * */
static void HalCriticalEnter(void)
{
    taskENTER_CRITICAL();
}

/**
 * @brief  退出 FreeRTOS 临界区。
 * */
static void HalCriticalExit(void)
{
    taskEXIT_CRITICAL();
}

/** @brief  Flash OS 操作接口实例（临界区管理） */
const flash_os_operations_t g_flash_os_ops = {
    .pf_critical_enter = HalCriticalEnter,
    .pf_critical_exit  = HalCriticalExit,
};

/**
 * @brief  获取系统 tick，用于 Flash 超时跟踪。
 *
 * @return  当前 tick 计数（毫秒）。
 * */
static uint32_t HalGetTick(void)
{
    return HAL_GetTick();
}

/** @brief  Flash 时间操作接口实例 */
const flash_time_operations_t g_flash_time_ops = {
    .pf_get_tick = HalGetTick,
};

/**
 * @brief  STM32F411CEUx Flash 布局配置。
 * */
const bsp_flash_config_t g_flash_config = {
    .boot_addr   = FLASH_ADDR_BOOT,
    .config_addr = FLASH_ADDR_CONFIG,
    .slot_a_addr = FLASH_ADDR_SLOT_A,
    .slot_b_addr = FLASH_ADDR_SLOT_B,
    .slot_a_size = SLOT_A_SIZE,
    .slot_b_size = SLOT_B_SIZE,
};

//*** UART HAL 操作接口 ***//

/**
 * @brief  启动 DMA + IDLE 接收。
 *
 * @param[in] p_huart : HAL UART 句柄（void*）。
 * @param[in] buf     : 接收缓冲区。
 * @param[in] size    : 接收缓冲区大小。
 *
 * @return   0 : HAL_OK。
 * @return  -1 : HAL 错误。
 * */
static int HalUart_StartDmaRx(void *p_huart, uint8_t *buf, uint16_t size)
{
    return (HAL_UARTEx_ReceiveToIdle_DMA((UART_HandleTypeDef *)p_huart, buf, size) == HAL_OK) ? 0 : -1;
}

/**
 * @brief  中止 UART 接收。
 *
 * @param[in] p_huart : HAL UART 句柄（void*）。
 * */
static void HalUart_StopRx(void *p_huart)
{
    HAL_UART_AbortReceive((UART_HandleTypeDef *)p_huart);
}

/**
 * @brief  非阻塞 DMA 发送。
 *
 * @param[in] p_huart : HAL UART 句柄（void*）。
 * @param[in] data    : 发送数据。
 * @param[in] len     : 发送长度。
 *
 * @return   0 : HAL_OK。
 * @return  -1 : HAL 错误。
 * */
static int HalUart_SendDma(void *p_huart, const uint8_t *data, uint16_t len)
{
    return (HAL_UART_Transmit_DMA((UART_HandleTypeDef *)p_huart, data, len) == HAL_OK) ? 0 : -1;
}

/**
 * @brief  阻塞式发送（带超时）。
 *
 * @param[in] p_huart : HAL UART 句柄（void*）。
 * @param[in] data    : 发送数据。
 * @param[in] len     : 发送长度。
 * @param[in] timeout : 超时时间（毫秒）。
 *
 * @return   0 : HAL_OK。
 * @return  -1 : HAL 错误。
 * */
static int HalUart_SendBlocking(void *p_huart, const uint8_t *data, uint16_t len, uint32_t timeout)
{
    return (HAL_UART_Transmit((UART_HandleTypeDef *)p_huart, data, len, timeout) == HAL_OK) ? 0 : -1;
}

/**
 * @brief  清空 UART 数据寄存器。
 *
 * @param[in] p_huart : HAL UART 句柄（void*）。
 * */
static void HalUart_FlushDr(void *p_huart)
{
    __HAL_UART_FLUSH_DRREGISTER((UART_HandleTypeDef *)p_huart);
}

/** @brief  UART 硬件操作接口实例 */
const uart_hw_operations_t g_uart_hal_ops = {
    .pf_start_dma_rx  = HalUart_StartDmaRx,
    .pf_stop_rx       = HalUart_StopRx,
    .pf_send_dma      = HalUart_SendDma,
    .pf_send_blocking = HalUart_SendBlocking,
    .pf_flush_dr      = HalUart_FlushDr,
};

/**
 * @brief  获取系统 tick，用于 UART 超时跟踪。
 *
 * @return  当前 tick 计数（毫秒）。
 * */
static uint32_t HalUart_GetTick(void)
{
    return HAL_GetTick();
}

/**
 * @brief  毫秒级延时（FreeRTOS osDelay 封装）。
 *
 * @param[in] ms : 延时时长（毫秒）。
 * */
static void HalUart_DelayMs(uint32_t ms)
{
    osDelay(ms);
}

/** @brief  UART OS 操作接口实例 */
const uart_os_operations_t g_uart_os_ops = {
    .pf_get_tick = HalUart_GetTick,
    .pf_delay_ms = HalUart_DelayMs,
};
