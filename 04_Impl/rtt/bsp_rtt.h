/**
 * @file    bsp_rtt.h
 * @brief   RTT debug output wrapper for the project
 * @author  GS_Mark
 */

#ifndef BSP_RTT_H
#define BSP_RTT_H

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief  初始化 SEGGER RTT 调试输出后端 */
    void BspRtt_Init(void);

    /** @brief  向 RTT 终端写入字符串 */
    int BspRtt_WriteString(const char *s);

    /** @brief  向 RTT 终端格式化输出 */
    int BspRtt_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* BSP_RTT_H */
