/**
 * @file    elog_port.c
 * @brief   EasyLogger port layer — UART output + FreeRTOS mutex
 *
 * STM32F411CEUx + FreeRTOS + UART1 (polling transmit)
 */

#include "elog.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

static SemaphoreHandle_t s_log_mutex;
static char s_time_buf[16];

ElogErrCode elog_port_init(void)
{
    s_log_mutex = xSemaphoreCreateMutex();
    return (s_log_mutex != NULL) ? ELOG_NO_ERR : ELOG_NO_ERR;
}

ElogErrCode elog_port_deinit(void)
{
    if (s_log_mutex != NULL) {
        vSemaphoreDelete(s_log_mutex);
        s_log_mutex = NULL;
    }
    return ELOG_NO_ERR;
}

void elog_port_output(const char *log, size_t size)
{
    HAL_UART_Transmit(&huart1, (const uint8_t *)log, (uint16_t)size, 100);
}

void elog_port_output_lock(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xSemaphoreTake(s_log_mutex, portMAX_DELAY);
    }
}

void elog_port_output_unlock(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xSemaphoreGive(s_log_mutex);
    }
}

const char *elog_port_get_time(void)
{
    snprintf(s_time_buf, sizeof(s_time_buf), "%05lu",
             (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
    return s_time_buf;
}

const char *elog_port_get_p_info(void)
{
    return "";
}

const char *elog_port_get_t_info(void)
{
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();
    if (handle != NULL) {
        return pcTaskGetName(handle);
    }
    return "idle";
}
