/**
 * @file    bsp_rtt.h
 * @brief   RTT debug output wrapper for the project
 */

#ifndef BSP_RTT_H
#define BSP_RTT_H

#ifdef __cplusplus
extern "C" {
#endif

void BspRtt_Init(void);
int  BspRtt_WriteString(const char *s);
int  BspRtt_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* BSP_RTT_H */
