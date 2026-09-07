/*
 * @Description: 板载 LED 闪灯线程 (最小链路验证)
 *
 * 行为:
 *   - 独立 TX_THREAD 周期性翻转板载 LED (亮/灭各 ONBOARD_BLINK_HALF_PERIOD_MS);
 *   - 启动时打一条日志, 之后以 LED 本身作为运行指示。
 *
 * 硬件适配:
 *   - OnboardLed_Set() 弱实现默认空操作, 不点灯也能编译运行(仅看串口日志);
 *   - 真机覆盖(本文件底部按芯片条件编译):
 *       * STM32F103xB (f103_c8 蓝丸): PC13 GPIO 直接翻转, 低电平点亮
 *       * STM32F407xx / STM32H723xx: 走 BSP_LED_Show() 控制板上 RGB
 */

#include "onboard_blink.h"

#include "tx_api.h"
#include "bsp_def.h" /* APPS_STACK_SECTION */

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "onboard_blink"
#include "ulog_def.h"

#ifndef ONBOARD_BLINK_STACK_SIZE
#define ONBOARD_BLINK_STACK_SIZE 512
#endif
#ifndef ONBOARD_BLINK_PRIORITY
#define ONBOARD_BLINK_PRIORITY 16
#endif
/* 半周期: 亮多久/灭多久 (ms), 完整闪烁周期 = 2 * ONBOARD_BLINK_HALF_PERIOD_MS */
#ifndef ONBOARD_BLINK_HALF_PERIOD_MS
#define ONBOARD_BLINK_HALF_PERIOD_MS 500
#endif

static TX_THREAD                  onboard_blink_thread;
APPS_STACK_SECTION static uint8_t onboard_blink_thread_stack[ONBOARD_BLINK_STACK_SIZE];

static void onboard_blink_task(ULONG thread_input)
{
    (void)thread_input;

    uint8_t on = 0;

    /* tx_user.h: TX_TIMER_TICKS_PER_SECOND = 1000 → tx_time_get() 单位就是 ms */
    LOG_I("onboard blink task started (half period %u ms)", (unsigned)ONBOARD_BLINK_HALF_PERIOD_MS);

    while (1)
    {
        on = !on;
        OnboardLed_Set(on);
        tx_thread_sleep(ONBOARD_BLINK_HALF_PERIOD_MS);
    }
}

void OnboardBlink_Init(void)
{
    UINT status;

    status = tx_thread_create(&onboard_blink_thread, "onboard_blink", onboard_blink_task, 0,
                              onboard_blink_thread_stack, ONBOARD_BLINK_STACK_SIZE,
                              ONBOARD_BLINK_PRIORITY, ONBOARD_BLINK_PRIORITY,
                              TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        LOG_E("onboard_blink thread create failed! status=0x%02X", (unsigned)status);
        return;
    }

    LOG_I("onboard blink thread created, prio=%u stack=%u", (unsigned)ONBOARD_BLINK_PRIORITY,
          (unsigned)ONBOARD_BLINK_STACK_SIZE);
}

/* ================================================================
 * 硬件驱动钩子: 各芯片真机实现 (强实现覆盖上面的弱实现)
 * ================================================================ */

#if defined(STM32F103xB)
/* f103_c8 (蓝丸): PC13 板载 LED, 低电平点亮 (CubeMX 已配 GPIO_Output) */
#include "gpio.h"

#ifndef ONBOARD_LED_ACTIVE_LEVEL
#define ONBOARD_LED_ACTIVE_LEVEL 0 /* 0 = 低电平点亮, 1 = 高电平点亮 */
#endif

void OnboardLed_Set(uint8_t on)
{
    GPIO_PinState st = (ONBOARD_LED_ACTIVE_LEVEL == 0)
                           ? (on ? GPIO_PIN_RESET : GPIO_PIN_SET)
                           : (on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, st);
}

#elif defined(STM32F407xx) || defined(STM32H723xx)
/* dji_c / damiao_h7: 板上 RGB 灯走 BSP_LED_Show() */
#include "bsp_led.h"

void OnboardLed_Set(uint8_t on)
{
    BSP_LED_Show(on ? LED_Green : LED_Black);
}

#else
/* 其他芯片: 暂无实现, 保留弱实现占位 (不点灯, 仅看日志) */
__attribute__((weak)) void OnboardLed_Set(uint8_t on)
{
    (void)on;
}
#endif
