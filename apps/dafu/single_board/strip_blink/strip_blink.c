/*
 * @Description: 灯带两态显示控制(空闲/按键按下)
 *   - 空闲: 中间 255 圈 按 DAFU_MIDDLE_PATTERN 显示(0=全亮 / 1=十字), 外圈(上86/下92)同色
 *   - 按下: 中间全灭, 外圈变蓝
 *   - 由主任务 robot_control 在"是否按着"变化时调用一次; 无自建线程, 不抢灯。
 */

#include "strip_blink.h"

#include "led_ctrl.h"
#include "ws2812.h"
#include "bsp_dwt.h"
#include "ringmap.h"
#include "dafu_def.h"

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "strip_blink"
#include "ulog_def.h"

/* 空闲颜色(默认红灯) */
#ifndef STRIP_BLINK_COLOR
#define STRIP_BLINK_COLOR DAFU_LED_COLOR_RED
#endif
/* 中间圈模式: 0 = 全亮; 1 = 十字(横+纵两条直径) */
#ifndef DAFU_MIDDLE_PATTERN
#define DAFU_MIDDLE_PATTERN 0 /* 默认全亮; 需要十字版改 1 */
#endif
/* 亮度: 0 = 全亮; N = RGB 右移 N 位(1/2^N) */
#ifndef STRIP_BLINK_DIM_BITS
#define STRIP_BLINK_DIM_BITS 3
#endif

/* 按 STRIP_BLINK_DIM_BITS 降亮度(保留 A 通道) */
static uint32_t strip_dim_argb(uint32_t argb)
{
#if (STRIP_BLINK_DIM_BITS) > 0
    uint8_t a = (uint8_t)(argb >> 24);
    uint8_t r = (uint8_t)(argb >> 16) >> (STRIP_BLINK_DIM_BITS);
    uint8_t g = (uint8_t)(argb >> 8) >> (STRIP_BLINK_DIM_BITS);
    uint8_t b = (uint8_t)(argb) >> (STRIP_BLINK_DIM_BITS);
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
#else
    return argb;
#endif
}

/* 实测十字灯号(1 基物理号, 覆盖到内圈), 仅 DAFU_MIDDLE_PATTERN=1 时用 */
#if (DAFU_MIDDLE_PATTERN) == 1
static const uint16_t s_horiz[] = { /* 横轴 */
    1, 2, 25, 26, 49, 50, 70, 71, 92, 93, 113, 114, 136, 137, 155, 156,
    175, 176, 192, 193, 211, 212, 221, 231, 241, 250, 251, 257, 263, 268,
};
static const uint16_t s_vert[] = { /* 纵轴 */
    12, 13, 36, 37, 60, 61, 79, 80, 102, 103, 123, 124, 145, 146, 165,
    166, 183, 184, 201, 202, 216, 226, 235, 236, 246, 254, 260, 270,
};
#endif

/* 按模式把中间 255 圈画成 color */
static void draw_middle(uint32_t color)
{
    Ws2812_Fill(0, DAFU_LED_COLOR_OFF);

#if (DAFU_MIDDLE_PATTERN) == 0
    Ws2812_Fill(0, color); /* 全亮 */
#else
    for (uint8_t i = 0; i < (uint8_t)(sizeof(s_horiz) / sizeof(s_horiz[0])); i++)
    {
        if ((s_horiz[i] >= 1u) && (s_horiz[i] <= DAFU_STRIP0_LED_NUM))
        {
            Ws2812_SetLed(0, (uint16_t)(s_horiz[i] - 1u), color);
        }
    }
    for (uint8_t i = 0; i < (uint8_t)(sizeof(s_vert) / sizeof(s_vert[0])); i++)
    {
        if ((s_vert[i] >= 1u) && (s_vert[i] <= DAFU_STRIP0_LED_NUM))
        {
            Ws2812_SetLed(0, (uint16_t)(s_vert[i] - 1u), color);
        }
    }
#endif
    Ws2812_Commit(0);
}

/* 空闲态: 中间 + 外圈 同色 */
static void draw_idle(void)
{
    uint32_t color = strip_dim_argb(STRIP_BLINK_COLOR);

    draw_middle(color);
    Led_SetStrip(1, color); /* 外圈上(86) */
    Led_SetStrip(2, color); /* 外圈下(92) */
    LOG_I("strip idle: middle pattern=%d, outer=%u", (int)DAFU_MIDDLE_PATTERN,
          (unsigned)(STRIP_BLINK_COLOR & 0xFFFFFF));
}

/* 按下态: 中间灭 + 外圈蓝 */
static void draw_held(void)
{
    Ws2812_Fill(0, DAFU_LED_COLOR_OFF);
    Ws2812_Commit(0);

    uint32_t blue = strip_dim_argb(DAFU_LED_COLOR_BLUE);
    Led_SetStrip(1, blue);
    Led_SetStrip(2, blue);
    LOG_I("strip held: middle off, outer BLUE");
}

/* 打中显示: 中间只亮被命中那环, 其余灭; 外圈变蓝 */
static void draw_hit_ring(uint8_t ring)
{
    uint16_t lo = 0, hi = 0;
    RingMap_RingRange(ring, &lo, &hi);

    Ws2812_Fill(0, DAFU_LED_COLOR_OFF);
    if ((lo >= 1u) && (hi >= lo) && (lo <= DAFU_STRIP0_LED_NUM))
    {
        uint32_t c = STRIP_BLINK_COLOR; /* 命中环用全亮色凸显 */
        if (hi > DAFU_STRIP0_LED_NUM)
        {
            hi = DAFU_STRIP0_LED_NUM;
        }
        for (uint16_t i = lo; i <= hi; i++)
        {
            Ws2812_SetLed(0, (uint16_t)(i - 1u), c);
        }
    }
    Ws2812_Commit(0);

    uint32_t blue = strip_dim_argb(DAFU_LED_COLOR_BLUE);
    Led_SetStrip(1, blue);
    Led_SetStrip(2, blue);
    LOG_I("strip hit ring=%u led=%u..%u", (unsigned)ring, (unsigned)lo, (unsigned)hi);
}

void StripBlink_Init(void)
{
    draw_idle();
}

void StripBlink_SetKeyHold(uint8_t held)
{
    if (held != 0u)
    {
        draw_held();
    }
    else
    {
        draw_idle();
    }
}

void StripBlink_SetHit(uint8_t ring)
{
    if ((ring < 1u) || (ring > RINGMAP_RING_NUM))
    {
        draw_idle(); /* 非法环号回空闲 */
        return;
    }
    draw_hit_ring(ring);
}

/* ================= 边界标定扫描(找 灯珠号↔环数分档) ================= */
/* 中间 255 螺带: 单颗依次点亮(1 → 2 → … → 255 → 循环), 全亮色便于看清.
 * 步进用 DWT 真实毫秒(非 OS tick), 各板计时一致. */
#ifndef STRIP_MAP_COLOR
#define STRIP_MAP_COLOR DAFU_LED_COLOR_RED
#endif
#ifndef STRIP_MAP_STEP_MS
#define STRIP_MAP_STEP_MS 180U /* 每跳到下一颗的真实毫秒数(太快/太慢调这里) */
#endif

static uint16_t s_map_pos = 0; /* 当前点亮位置(0 基) */

void StripBlink_MapReset(void)
{
    s_map_pos = 0;
    Ws2812_Fill(0, DAFU_LED_COLOR_OFF);
    Ws2812_Commit(0);
}

void StripBlink_MapTick(void)
{
    uint32_t color = STRIP_MAP_COLOR; /* 标定用全亮色(不降亮度, 否则单颗看不清) */
    static uint32_t t_last = 0;
    uint32_t now_ms = (uint32_t)BSP_DWT_GetTimeline_ms();

    if ((now_ms - t_last) < STRIP_MAP_STEP_MS)
    {
        return;
    }
    t_last = now_ms;

    /* 一次只亮一颗: 先清空整条, 再点亮当前颗 */
    Ws2812_Fill(0, DAFU_LED_COLOR_OFF);
    Ws2812_SetLed(0, s_map_pos, color);
    Ws2812_Commit(0);
    LOG_I("map n=%u", (unsigned)(s_map_pos + 1u)); /* 打印 1 基灯号 */

    s_map_pos++;
    if (s_map_pos >= DAFU_STRIP0_LED_NUM)
    {
        s_map_pos = 0; /* 走完 1..255 后从 1 重新开始 */
    }
}
