/*
 * @Description: 灯带显示状态控制(空闲/按键按下), 由主任务(robot_control)驱动切换
 *
 * 空闲态: 中间 255 圈 按 DAFU_MIDDLE_PATTERN(0=全亮红 / 1=十字红) + 外圈(上86/下92)红
 * 按下态: 中间 255 圈全灭 + 外圈变蓝
 *
 * 说明: 不再自建线程; Ws2812/Led 提交只在"状态变化"时发生一次(WS2812 锁存保持)。
 */

#ifndef __STRIP_BLINK_H__
#define __STRIP_BLINK_H__

#include <stdint.h>

/* 上电显示空闲态(需在 Led_Init 之后调用一次) */
void StripBlink_Init(void);

/* 按键按住状态变化时调用: held=1 → 中间灭+外圈蓝; held=0 → 恢复空闲态 */
void StripBlink_SetKeyHold(uint8_t held);

/* 打中某环(1..9)显示: 中间只保留该环灯珠亮、其余灭, 外圈变蓝(保持到下次切换) */
void StripBlink_SetHit(uint8_t ring);

/* ── 灯珠号↔环数分档边界标定(单颗循环点亮) ──
 * 中间 255 螺带每次只亮一颗: 1 → 2 → … → 255 → 循环, 每颗打印灯号,
 * 供肉眼对照物理靶面的环线位置确定分档边界。
 * 用法: Init 时调 StripBlink_MapReset() 清空, 之后主任务每周期调 MapTick()。
 */
void StripBlink_MapReset(void);
void StripBlink_MapTick(void); /* 用 DWT 真实时间定步进(内部计时), 主任务每周期调用即可 */

#endif /* __STRIP_BLINK_H__ */
