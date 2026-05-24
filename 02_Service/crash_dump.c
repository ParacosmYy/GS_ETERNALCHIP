/**
 * @file    crash_dump.c
 * @brief   Crash 寄存器转储实现
 * @author  GS_Mark
 *
 * @par dependencies
 * - crash_dump.h
 * - stm32f4xx.h (寄存器定义)
 * - elog.h (打印)
 *
 * SaveFromFault 在 fault handler 上下文中运行，直接操作 Flash 寄存器，
 * 绝不调用 HAL/FreeRTOS/中断相关函数。
 *
 * @version V1.0 2025-05-23
 *
 * @note 1 tab == 4 spaces
 *
 * */

#define LOG_TAG "CRASH"
#include "elog.h"
#include "crash_dump.h"
#include "stm32f4xx.h"
#include <string.h>

//*** STM32F4 Flash 寄存器操作（绕过 HAL） ***//

#define FLASH_KEY1  0x45670123u
#define FLASH_KEY2  0xCDEF89ABu

static void flash_unlock_raw(void)
{
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
}

static void flash_lock_raw(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void flash_clear_errors_raw(void)
{
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_SOP |
                FLASH_SR_WRPERR | FLASH_SR_PGAERR |
                FLASH_SR_PGPERR | FLASH_SR_PGSERR;
}

static int flash_write_word_raw(uint32_t addr, uint32_t data)
{
    uint32_t timeout;

    /* 等待 BSY 清零 */
    timeout = 100000u;
    while ((FLASH->SR & FLASH_SR_BSY) != 0)
    {
        if (--timeout == 0)
        {
            return -1;
        }
    }

    /* 编程 x32 */
    FLASH->CR &= ~FLASH_CR_PSIZE;
    FLASH->CR |= FLASH_CR_PSIZE_1;  /* x32 */
    FLASH->CR |= FLASH_CR_PG;

    *(volatile uint32_t *)addr = data;

    /* 等待完成 */
    timeout = 100000u;
    while ((FLASH->SR & FLASH_SR_BSY) != 0)
    {
        if (--timeout == 0)
        {
            FLASH->CR &= ~FLASH_CR_PG;
            return -1;
        }
    }

    /* 清除 EOP 标志 */
    if (FLASH->SR & FLASH_SR_EOP)
    {
        FLASH->SR = FLASH_SR_EOP;
    }

    FLASH->CR &= ~FLASH_CR_PG;
    return 0;
}

/**
 * @brief  将 crash_dump_t 写入 Flash（逐 word，裸操作）。
 *
 * @param[in] p_dump : 填充好的 crash dump 结构。
 *
 * @return 0 成功，-1 失败。
 * */
static int crash_write_raw(const crash_dump_t *p_dump)
{
    uint32_t i;
    const uint32_t *p_words = (const uint32_t *)p_dump;

    flash_unlock_raw();
    flash_clear_errors_raw();

    for (i = 0; i < sizeof(crash_dump_t) / 4; i++)
    {
        if (flash_write_word_raw(CRASH_DUMP_ADDR + i * 4, p_words[i]) != 0)
        {
            flash_lock_raw();
            return -1;
        }
    }

    flash_lock_raw();
    return 0;
}

//*** 公共 API ***//

/**
 * @brief  在 fault handler 中调用，将寄存器写入 Flash。
 *
 * Cortex-M4 在进入 fault handler 前硬件自动压栈：
 *   [SP+0]  R0
 *   [SP+4]  R1
 *   [SP+8]  R2
 *   [SP+12] R3
 *   [SP+16] R12
 *   [SP+20] LR  (返回地址)
 *   [SP+24] PC  (出错指令)
 *   [SP+28] xPSR
 *
 * @param[in] p_sp : fault 时的 SP（指向硬件栈帧）。
 * */
void CrashDump_SaveFromFault(uint32_t p_sp, uint32_t exc_lr)
{
    crash_dump_t dump;
    volatile uint32_t *frame = (volatile uint32_t *)p_sp;

    /* 检查是否已有记录（防止覆盖） */
    if (*(volatile uint32_t *)CRASH_DUMP_ADDR != 0xFFFFFFFFu)
    {
        return;
    }

    memset(&dump, 0xFF, sizeof(dump));

    dump.magic = CRASH_DUMP_MAGIC;

    /* SCB 故障寄存器 */
    dump.cfsr  = SCB->CFSR;
    dump.hfsr  = SCB->HFSR;
    dump.mmfar = SCB->MMFAR;
    dump.bfar  = SCB->BFAR;

    /* 当前栈指针 */
    dump.psp = p_sp;
    dump.msp = __get_MSP();

    /* 从硬件栈帧提取 */
    dump.r0   = frame[0];
    dump.r1   = frame[1];
    dump.r2   = frame[2];
    dump.r3   = frame[3];
    dump.r12  = frame[4];
    dump.lr   = frame[5];
    dump.pc   = frame[6];
    dump.xpsr = frame[7];

    /* EXC_RETURN */
    dump.exc_lr = exc_lr;

    crash_write_raw(&dump);
}

/**
 * @brief  App 启动时检查并打印上次的 crash 记录。
 *
 * Steps:
 *  1. 读取 magic 字段，检查是否为 CRASH_DUMP_MAGIC。
 *  2. 打印所有保存的寄存器值。
 *  3. 清除 magic 防止下次重复打印。
 *
 * @return  1 : 检测到 crash 记录（已打印并清除）。
 * @return  0 : 无 crash 记录。
 * */
int CrashDump_CheckAndPrint(void)
{
    crash_dump_t dump;

    memcpy(&dump, (const void *)CRASH_DUMP_ADDR, sizeof(dump));

    if (dump.magic != CRASH_DUMP_MAGIC)
    {
        return 0;
    }

    log_w("=== Crash Dump Detected ===");
    log_w("PC   = 0x%08lX  (fault instruction)", dump.pc);
    log_w("LR   = 0x%08lX  (return address)",    dump.lr);
    log_w("XPSR = 0x%08lX",                       dump.xpsr);
    log_w("CFSR = 0x%08lX",                       dump.cfsr);
    log_w("HFSR = 0x%08lX",                       dump.hfsr);
    log_w("MMFAR= 0x%08lX",                       dump.mmfar);
    log_w("BFAR = 0x%08lX",                       dump.bfar);
    log_w("R0   = 0x%08lX  R1 = 0x%08lX",        dump.r0, dump.r1);
    log_w("R2   = 0x%08lX  R3 = 0x%08lX",        dump.r2, dump.r3);
    log_w("R12  = 0x%08lX",                       dump.r12);
    log_w("PSP  = 0x%08lX  MSP= 0x%08lX",        dump.psp, dump.msp);
    log_w("EXC_LR=0x%08lX",                       dump.exc_lr);
    log_w("=== Use: addr2line -e OTA_B.elf 0x%08lX ===", dump.pc);

    /* 清除 magic：写入 0 到 magic 字段 */
    flash_unlock_raw();
    flash_clear_errors_raw();
    flash_write_word_raw(CRASH_DUMP_ADDR, 0x00000000u);
    flash_lock_raw();

    return 1;
}
