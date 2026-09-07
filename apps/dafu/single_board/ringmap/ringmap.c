/*
 * @Description: 打靶环段映射实现 (实测 271 / 9 环)
 */

#include "ringmap.h"

#include <stddef.h>

/* 每环灯珠范围(1 基, 闭区间), 物理实测 */
static const uint16_t s_lo[RINGMAP_RING_NUM] = { 1, 49, 91, 135, 175, 211, 231, 251, 263 };
static const uint16_t s_hi[RINGMAP_RING_NUM] = { 48, 90, 134, 174, 210, 230, 250, 262, 271 };

uint8_t RingMap_RingOfLed(uint16_t led)
{
    if ((led < 1u) || (led > RINGMAP_LED_NUM))
    {
        return 0u;
    }
    for (uint8_t r = 0; r < RINGMAP_RING_NUM; r++)
    {
        if (led <= s_hi[r])
        {
            return (uint8_t)(r + 1u);
        }
    }
    return 0u;
}

void RingMap_RingRange(uint8_t ring, uint16_t *lo, uint16_t *hi)
{
    if (lo != NULL)
    {
        *lo = 0;
    }
    if (hi != NULL)
    {
        *hi = 0;
    }
    if ((ring < 1u) || (ring > RINGMAP_RING_NUM))
    {
        return;
    }
    if (lo != NULL)
    {
        *lo = s_lo[ring - 1u];
    }
    if (hi != NULL)
    {
        *hi = s_hi[ring - 1u];
    }
}
