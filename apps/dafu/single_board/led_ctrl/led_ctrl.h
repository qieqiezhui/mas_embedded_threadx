/*
 * @Description: WS2812 三条带高层控制 (状态切换才写一帧)
 *
 * 说明:
 *   - 本层只维护"每条带当前要显示的颜色";
 *   - 真正发送走 ws2812 驱动 (Ws2812_Fill/Commit);
 *   - 颜色没变化时不会重复写硬件 (WS2812 锁存保持)。
 */

#ifndef __LED_CTRL_H__
#define __LED_CTRL_H__

#include <stdint.h>
#include "dafu_def.h"

/* 初始化(内部会调用 Ws2812_Init, 全灭) */
void Led_Init(void);

/* 设置单带目标色; argb = 0xAARRGGBB, 变化时才提交到硬件.
 * strip: 0 = 中间255带, 1 = 上86带, 2 = 下92带 */
void Led_SetStrip(uint8_t strip, uint32_t argb);

/* 三条带同一目标色 (全亮/全灭等) */
void Led_SetAll(uint32_t argb);

#endif /* __LED_CTRL_H__ */
