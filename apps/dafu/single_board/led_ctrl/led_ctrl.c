/*
 * @Description: WS2812 三条带高层控制 (状态切换才写一帧)
 *
 * 内部维护每带最近一次目标色; Led_SetStrip/Led_SetAll 在颜色变化时,
 * 才调用 Ws2812_Fill + Ws2812_Commit 把整带写一次。颜色不变则不动硬件,
 * 契合 WS2812 锁存保持特性。
 */

#include "led_ctrl.h"
#include "ws2812.h"

#include <string.h>

#define LOG_LVL LOG_LVL_WARNING
#define LOG_TAG "dafu_led"
#include "ulog_def.h"

static uint32_t s_last_color[DAFU_STRIP_NUM];
static uint8_t  s_valid[DAFU_STRIP_NUM];

void Led_Init(void)
{
    Ws2812_Init();
    memset(s_last_color, 0, sizeof(s_last_color));
    memset(s_valid, 0, sizeof(s_valid));
}

static void led_set(uint8_t strip, uint32_t argb)
{
    if (strip >= DAFU_STRIP_NUM)
    {
        return;
    }
    /* 颜色未变化 → 不重刷 (WS2812 锁存保持) */
    if (s_valid[strip] && (s_last_color[strip] == argb))
    {
        return;
    }
    s_last_color[strip] = argb;
    s_valid[strip]      = 1;

    Ws2812_Fill(strip, argb);
    Ws2812_Commit(strip); /* 状态切换才写一帧 */
}

void Led_SetStrip(uint8_t strip, uint32_t argb)
{
    led_set(strip, argb);
}

void Led_SetAll(uint32_t argb)
{
    for (uint8_t s = 0; s < DAFU_STRIP_NUM; s++)
    {
        led_set(s, argb);
    }
}
