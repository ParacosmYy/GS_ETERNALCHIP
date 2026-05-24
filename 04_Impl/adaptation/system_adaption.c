/**
 * @file    system_adaption.c
 * @brief   System Adaption Layer — ALL HAL/FreeRTOS wrapper functions + ops tables
 * @author  GS_Mark
 *
 * @par dependencies
 * - system_adaption.h
 * - stm32f4xx_hal.h
 * - FreeRTOS.h / task.h
 * - SEGGER_RTT.h / SEGGER_RTT_Printf.h
 * - bsp_flash_driver.h (for BSP_FLASH_VOLTAGE_RANGE)
 * - ota_config.h (for FLASH_ADDR_* constants)
 *
 * @note    This is the SINGLE compilation unit for all HAL glue.
 *          No other .c file should call HAL_* or FreeRTOS APIs directly
 *          (except HAL callbacks in ISR context and CubeMX code in Core/).
 */

//*** Includes ***//
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

//*** Private Macros ***//

/** @brief  Cast plat_gpio_t opaque port pointer to HAL GPIO_TypeDef* */
#define HAL_GPIO_PORT(p) ((GPIO_TypeDef *)(((const plat_gpio_t *)(p))->port))

/** @brief  Extract HAL GPIO pin bitmask (1 << pin) from plat_gpio_t */
#define HAL_GPIO_PIN(p)  ((uint16_t)(1U << ((const plat_gpio_t *)(p))->pin))

/** @brief  Cast plat_gpio_t port field to HAL GPIO_TypeDef* (by value) */
#define HAL_GPIO_PORT_V(gpio) ((GPIO_TypeDef *)((gpio).port))

//*** LED HAL Ops ***//

/**
 * @brief  HAL implementation: turn LED on.
 *         Low-active LED writes RESET, high-active writes SET.
 *
 * @param[in] p_drv : LED driver instance pointer.
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
 * @brief  HAL implementation: turn LED off.
 *         Polarity inverted from On.
 *
 * @param[in] p_drv : LED driver instance pointer.
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
 * @brief  HAL implementation: toggle LED state.
 *
 * @param[in] p_drv : LED driver instance pointer.
 * */
static void HalLed_Toggle(bsp_led_driver_t *p_drv)
{
    const plat_gpio_t *p_gpio = &p_drv->p_config->gpio;

    HAL_GPIO_TogglePin(HAL_GPIO_PORT(p_gpio), HAL_GPIO_PIN(p_gpio));
}

/**
 * @brief  LED HAL ops instance (name matches plat_led.h extern declaration).
 * */
const led_operations_t bsp_led_hal_ops = {
    .p_On     = HalLed_On,
    .p_Off    = HalLed_Off,
    .p_Toggle = HalLed_Toggle,
};

//*** Key HAL Ops ***//

/**
 * @brief  Read raw pin level and convert to logical level (0/1).
 *         active_level=0 (low-active): pin RESET -> logical 1 (pressed).
 *         active_level=1 (high-active): pin SET   -> logical 1 (pressed).
 *
 * @param[in] p_cfg : Key hardware config pointer.
 *
 * @return  1 : pressed.
 * @return  0 : released.
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
 * @brief  Get system tick (wrapper for HAL_GetTick).
 *
 * @return  Current tick count in ms.
 * */
static uint32_t HalKey_GetTick(void)
{
    return HAL_GetTick();
}

/** @brief  Key hardware ops instance */
const key_hw_operations_t g_key_hal_ops = {
    .pf_read_pin = HalKey_ReadPin,
};

/** @brief  Key OS ops instance */
const key_os_operations_t g_key_os_ops = {
    .pf_get_tick = HalKey_GetTick,
};

//*** WDG HAL Ops ***//

/**
 * @brief  Refresh IWDG watchdog counter.
 *
 * @param[in] p_handle : IWDG handle (cast from void*).
 * */
static void HalWdg_Refresh(void *p_handle)
{
    IWDG_HandleTypeDef *p_hiwdg = (IWDG_HandleTypeDef *)p_handle;

    if (p_hiwdg != NULL && p_hiwdg->Instance != NULL)
    {
        HAL_IWDG_Refresh(p_hiwdg);
    }
}

/** @brief  WDG hardware ops instance */
const wdg_hw_operations_t g_wdg_hal_ops = {
    .pf_refresh = HalWdg_Refresh,
};

//*** SYS HAL Ops ***//

/**
 * @brief  Get system tick.
 *
 * @return  Current tick count in ms.
 * */
static uint32_t HalSys_GetTick(void)
{
    return HAL_GetTick();
}

/**
 * @brief  Software reset via NVIC.
 * */
static void HalSys_Reboot(void)
{
    NVIC_SystemReset();
}

/**
 * @brief  Detect running OTA bank from SCB->VTOR.
 *
 * @param[out] p_slot : Detected running bank.
 *
 * @return   0 : detection success.
 * @return  -1 : detection failed.
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

/** @brief  SYS hardware ops instance */
const sys_operations_t g_sys_hal_ops = {
    .pf_get_tick         = HalSys_GetTick,
    .pf_reboot           = HalSys_Reboot,
    .pf_get_running_bank = HalSys_GetRunningBank,
};

//*** RTT HAL Ops ***//

/**
 * @brief  Initialize SEGGER RTT.
 *
 * @return  0 : success.
 * */
static int HalRtt_Init(void)
{
    SEGGER_RTT_Init();
    return 0;
}

/**
 * @brief  Write string to RTT terminal (channel 0).
 *
 * @param[in] s : String to write.
 *
 * @return  >=0 : bytes written.
 * */
static int HalRtt_WriteString(const char *s)
{
    return SEGGER_RTT_Write(0, s, strlen(s));
}

/**
 * @brief  Formatted output to RTT terminal (channel 0).
 *
 * @param[in] fmt  : Format string.
 * @param[in] args : Variadic argument list.
 *
 * @return  >=0 : bytes written.
 * */
static int HalRtt_Vprintf(const char *fmt, va_list args)
{
    return SEGGER_RTT_vprintf(0, fmt, &args);
}

/** @brief  RTT hardware ops instance */
const rtt_operations_t g_rtt_hal_ops = {
    .pf_init         = HalRtt_Init,
    .pf_write_string = HalRtt_WriteString,
    .pf_vprintf      = HalRtt_Vprintf,
};

//*** Flash HAL Ops ***//

/**
 * @brief  Unlock Flash for write/erase operations.
 *
 * @return  0 : success.
 * */
static int HalFlash_Unlock(void)
{
    HAL_FLASH_Unlock();
    return 0;
}

/**
 * @brief  Lock Flash after write/erase operations.
 *
 * @return  0 : success.
 * */
static int HalFlash_Lock(void)
{
    HAL_FLASH_Lock();
    return 0;
}

/**
 * @brief  Clear residual Flash error flags.
 * */
static void HalFlash_ClearErrors(void)
{
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR |
                           FLASH_FLAG_RDERR);
}

/**
 * @brief  Erase a single Flash sector.
 *
 * @param[in] sector        : Sector number (FLASH_SECTOR_x).
 * @param[in] voltage_range : Voltage range for erase.
 *
 * @return   0 : erase success.
 * @return  -1 : erase failure.
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
 * @brief  Program a 32-bit word to Flash.
 *
 * @param[in] addr : Target Flash address (4-byte aligned).
 * @param[in] data : 32-bit data value.
 *
 * @return   0 : program success.
 * @return  -1 : program failure.
 * */
static int HalFlash_ProgramWord(uint32_t addr, uint32_t data)
{
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

/** @brief  Flash hardware ops instance */
const flash_hw_operations_t g_flash_hal_ops = {
    .pf_unlock       = HalFlash_Unlock,
    .pf_lock         = HalFlash_Lock,
    .pf_clear_errors = HalFlash_ClearErrors,
    .pf_erase_sector = HalFlash_EraseSector,
    .pf_program_word = HalFlash_ProgramWord,
};

/**
 * @brief  Enter FreeRTOS critical section.
 * */
static void HalCriticalEnter(void)
{
    taskENTER_CRITICAL();
}

/**
 * @brief  Exit FreeRTOS critical section.
 * */
static void HalCriticalExit(void)
{
    taskEXIT_CRITICAL();
}

/** @brief  Flash OS ops instance (critical section management) */
const flash_os_operations_t g_flash_os_ops = {
    .pf_critical_enter = HalCriticalEnter,
    .pf_critical_exit  = HalCriticalExit,
};

/**
 * @brief  Get system tick for Flash timeout tracking.
 *
 * @return  Current tick count in ms.
 * */
static uint32_t HalGetTick(void)
{
    return HAL_GetTick();
}

/** @brief  Flash time ops instance */
const flash_time_operations_t g_flash_time_ops = {
    .pf_get_tick = HalGetTick,
};

/**
 * @brief  Flash layout configuration for STM32F411CEUx.
 * */
const bsp_flash_config_t g_flash_config = {
    .boot_addr   = FLASH_ADDR_BOOT,
    .config_addr = FLASH_ADDR_CONFIG,
    .slot_a_addr = FLASH_ADDR_SLOT_A,
    .slot_b_addr = FLASH_ADDR_SLOT_B,
    .slot_a_size = SLOT_A_SIZE,
    .slot_b_size = SLOT_B_SIZE,
};

//*** UART HAL Ops ***//

/**
 * @brief  Start DMA + IDLE reception.
 *
 * @param[in] p_huart : HAL UART handle (void*).
 * @param[in] buf     : RX buffer.
 * @param[in] size    : RX buffer size.
 *
 * @return   0 : HAL_OK.
 * @return  -1 : HAL error.
 * */
static int HalUart_StartDmaRx(void *p_huart, uint8_t *buf, uint16_t size)
{
    return (HAL_UARTEx_ReceiveToIdle_DMA((UART_HandleTypeDef *)p_huart, buf, size) == HAL_OK) ? 0 : -1;
}

/**
 * @brief  Abort UART reception.
 *
 * @param[in] p_huart : HAL UART handle (void*).
 * */
static void HalUart_StopRx(void *p_huart)
{
    HAL_UART_AbortReceive((UART_HandleTypeDef *)p_huart);
}

/**
 * @brief  Non-blocking DMA transmit.
 *
 * @param[in] p_huart : HAL UART handle (void*).
 * @param[in] data    : TX data.
 * @param[in] len     : TX length.
 *
 * @return   0 : HAL_OK.
 * @return  -1 : HAL error.
 * */
static int HalUart_SendDma(void *p_huart, const uint8_t *data, uint16_t len)
{
    return (HAL_UART_Transmit_DMA((UART_HandleTypeDef *)p_huart, data, len) == HAL_OK) ? 0 : -1;
}

/**
 * @brief  Blocking transmit with timeout.
 *
 * @param[in] p_huart : HAL UART handle (void*).
 * @param[in] data    : TX data.
 * @param[in] len     : TX length.
 * @param[in] timeout : Timeout in ms.
 *
 * @return   0 : HAL_OK.
 * @return  -1 : HAL error.
 * */
static int HalUart_SendBlocking(void *p_huart, const uint8_t *data, uint16_t len, uint32_t timeout)
{
    return (HAL_UART_Transmit((UART_HandleTypeDef *)p_huart, data, len, timeout) == HAL_OK) ? 0 : -1;
}

/**
 * @brief  Flush UART data register.
 *
 * @param[in] p_huart : HAL UART handle (void*).
 * */
static void HalUart_FlushDr(void *p_huart)
{
    __HAL_UART_FLUSH_DRREGISTER((UART_HandleTypeDef *)p_huart);
}

/** @brief  UART hardware ops instance */
const uart_hw_operations_t g_uart_hal_ops = {
    .pf_start_dma_rx  = HalUart_StartDmaRx,
    .pf_stop_rx       = HalUart_StopRx,
    .pf_send_dma      = HalUart_SendDma,
    .pf_send_blocking = HalUart_SendBlocking,
    .pf_flush_dr      = HalUart_FlushDr,
};

/**
 * @brief  Get system tick for UART timeout tracking.
 *
 * @return  Current tick count in ms.
 * */
static uint32_t HalUart_GetTick(void)
{
    return HAL_GetTick();
}

/**
 * @brief  Delay in ms (FreeRTOS osDelay wrapper).
 *
 * @param[in] ms : Delay duration in ms.
 * */
static void HalUart_DelayMs(uint32_t ms)
{
    osDelay(ms);
}

/** @brief  UART OS ops instance */
const uart_os_operations_t g_uart_os_ops = {
    .pf_get_tick = HalUart_GetTick,
    .pf_delay_ms = HalUart_DelayMs,
};
