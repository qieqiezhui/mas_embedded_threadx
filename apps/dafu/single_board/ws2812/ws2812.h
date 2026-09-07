/*
 * @Description: WS2812(GRB, 800kHz) 灯带驱动 — 3 条带 (255/86/92)
 *
 * 设计:
 *   - 每条带维护一份 GRB 颜色缓冲(3B/LED, 驱动内部由 AARRGGBB 转 GRB);
 *   - Ws2812_Fill / Ws2812_SetLed 只改缓冲;
 *   - Ws2812_Commit() 才真正把整带发到硬件。WS2812 锁存保持,
 *     因此只在"状态切换"时调一次 Commit 即可, 无需周期刷新。
 *
 * 后端(见 ws2812.c 顶部配置, PCB 定了改那里):
 *   - Strip0(255): SPI 传输, 紧凑 3 SPI bit/WS bit 位打包(缓冲 = 9B/LED)
 *   - Strip1(86) / Strip2(92): GPIO 位带 (DWT 精确延时)
 */

#ifndef __WS2812_H__
#define __WS2812_H__

#include <stdint.h>
#include "dafu_def.h"

/* 初始化(清空各带缓冲, 全灭) */
void Ws2812_Init(void);

/* 整带同色: argb = 0xAARRGGBB (只写缓冲, 不发送) */
void Ws2812_Fill(uint8_t strip, uint32_t argb);

/* 单颗设色: idx = 0..(该带 LED 数-1) (只写缓冲, 不发送) */
void Ws2812_SetLed(uint8_t strip, uint16_t idx, uint32_t argb);

/* 把 strip 当前缓冲完整发到硬件(状态切换时调用; 内部会补 RESET≥80µs) */
void Ws2812_Commit(uint8_t strip);

/* 该带 LED 颗数 */
uint16_t Ws2812_LedNum(uint8_t strip);

/* 在 strip 上画"十字"(0/90/180/270° 四个方向点状线, 其余灭)。
 * 几何按"螺旋带: 索引0=最外圈1号(正右), 由外向里共 N 颗、绕 DAFU_SPIRAL_TURNS 圈"计算,
 * 参数见 ws2812.c 顶部(圈数/容差可调)。只写缓冲, 需自行 Ws2812_Commit。 */
void Ws2812_DrawCross(uint8_t strip, uint32_t argb);

#endif /* __WS2812_H__ */
