/*
 * @Description: 打靶环段映射 (实测 271 颗 / 9 环)
 *
 *   环段(1 基, 闭区间):
 *     1环   1- 48     2环  49- 90     3环  91-134
 *     4环 135-174     5环 175-210     6环 211-230
 *     7环 231-250     8环 251-262     9环 263-271(靶心)
 *   供显示(命中某环点亮对应灯段)与上报(灯珠号→环数)共用。
 */

#ifndef __RINGMAP_H__
#define __RINGMAP_H__

#include <stdint.h>

#define RINGMAP_LED_NUM  271 /* 中间靶环带灯珠总数 */
#define RINGMAP_RING_NUM 9   /* 环数 */

/* LED(1..271) → 环(1..9); 越界返回 0 */
uint8_t RingMap_RingOfLed(uint16_t led);

/* 取某环(1..9)灯珠范围(1 基, 闭区间); 非法返回 lo=hi=0 */
void RingMap_RingRange(uint8_t ring, uint16_t *lo, uint16_t *hi);

#endif /* __RINGMAP_H__ */
