#include "key_detect.h"

#include <string.h>

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "dafu_key"
#include "ulog_def.h"

static Key_t g_keys[DAFU_KEY_NUM];

/* 按键名映射 (调试打印用) */
static const char *const s_key_names[DAFU_KEY_NUM] = {
    "PA0", "PA1", "PA2", "PA3", "PA4", "PA5", "PA6", "PA7", "PB0",
};

const char *Key_Name(uint8_t idx)
{
    return (idx < DAFU_KEY_NUM) ? s_key_names[idx] : "??";
}

void Key_Init(void)
{
    memset(g_keys, 0, sizeof(g_keys));
}

static void key_process(Key_t *k, uint8_t idx, uint32_t now_ms)
{
    uint8_t raw = (Key_GetLevel(idx) != 0u) ? 1u : 0u;

    if (raw != k->debounced)
    {
        /* 电平正在变化: 进入消抖窗口 */
        if (raw != k->candidate)
        {
            k->candidate      = raw;
            k->cand_since_ms  = now_ms;
        }
        else if ((now_ms - k->cand_since_ms) >= DAFU_KEY_DEBOUNCE_MS)
        {
            /* 稳定越过消抖时间 → 电平跳变确认 */
            k->debounced = k->candidate;

            if (k->debounced)
            {
                /* 按下 */
                k->press_ms   = now_ms;
                k->long_fired = 0;
                LOG_I("KEY[%s] PRESS", Key_Name(idx));
            }
            else
            {
                /* 松开 */
                LOG_I("KEY[%s] RELEASE", Key_Name(idx));
                if (!k->long_fired && (now_ms - k->press_ms) < DAFU_KEY_LONG_MS)
                {
                    k->event       = KEY_EVENT_CLICK;
                    k->event_ready = 1;
                }
            }
        }
    }
    else if (k->debounced && !k->long_fired)
    {
        /* 稳定按下且超过长按时间 → 触发一次长按 */
        if ((now_ms - k->press_ms) >= DAFU_KEY_LONG_MS)
        {
            k->long_fired = 1;
            k->event      = KEY_EVENT_LONG;
            k->event_ready = 1;
            LOG_I("KEY[%s] LONG", Key_Name(idx));
        }
    }
}

void Key_Scan(uint32_t now_ms)
{
    for (uint8_t i = 0; i < DAFU_KEY_NUM; i++)
    {
        key_process(&g_keys[i], i, now_ms);
    }
}

Key_Event_e Key_GetEvent(uint8_t idx)
{
    Key_Event_e ev;

    if (idx >= DAFU_KEY_NUM)
    {
        return KEY_EVENT_NONE;
    }

    ev = g_keys[idx].event;
    g_keys[idx].event       = KEY_EVENT_NONE;
    g_keys[idx].event_ready = 0;
    return ev;
}

uint8_t Key_IsPressed(uint8_t idx)
{
    if (idx >= DAFU_KEY_NUM)
    {
        return 0;
    }
    return g_keys[idx].debounced;
}

/* ================================================================
 * 硬件钩子: 引脚映射 + 电平读取
 *   idx0~7 = PA0~PA7, idx8 = PB0 (CubeMX 已配成输入上拉)
 * ================================================================ */
#if defined(STM32F103xB)
#include "gpio.h"

static GPIO_TypeDef *const s_key_port[DAFU_KEY_NUM] = {
    GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOB,
};
static const uint16_t s_key_pin[DAFU_KEY_NUM] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4,
    GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7, GPIO_PIN_0,
};

uint8_t Key_GetLevel(uint8_t idx)
{
    GPIO_PinState st;
    uint8_t       lv;

    if (idx >= DAFU_KEY_NUM)
    {
        return 0;
    }
    st = HAL_GPIO_ReadPin(s_key_port[idx], s_key_pin[idx]);
    lv = (st == GPIO_PIN_SET) ? 1u : 0u;              /* 物理高=1 */
    if (DAFU_KEY_ACTIVE_LEVEL == 0)
    {
        return (lv == 0u) ? 1u : 0u;                  /* 低有效: 按到低=按下 */
    }
    return (lv != 0u) ? 1u : 0u;
}

#else
__attribute__((weak)) uint8_t Key_GetLevel(uint8_t idx)
{
    (void)idx;
    return 0;
}
#endif
