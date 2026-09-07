/*
 * @Description: 板载 LED 闪灯线程 (最小链路验证)
 *
 * 用途:
 *   - 新建一个独立 TX_THREAD, 周期性翻转板载 LED;
 *   - 用于确认整套工程可用: ThreadX 建线程/调度 + 板载 LED 驱动路径正常;
 *   - 不依赖 led_ctrl / key_detect 等业务模块, 是独立的冒烟测试模块。
 *
 * 硬件适配:
 *   - OnboardLed_Set() 是弱钩子(默认空操作), 真机实现见 onboard_blink.c 底部:
 *       * STM32F103xB (f103_c8 蓝丸)      : PC13 GPIO 直接翻转 (低电平点亮)
 *       * STM32F407xx / STM32H723xx       : 走 BSP_LED_Show() 板上 RGB
 */

#ifndef __ONBOARD_BLINK_H__
#define __ONBOARD_BLINK_H__

#include <stdint.h>

/* 创建并启动板载 LED 闪灯线程 (TX_AUTO_START, 建议在 robot_control_init 中调用) */
void OnboardBlink_Init(void);

/* 硬件钩子: on=1 点亮 / on=0 熄灭 (弱实现, 各芯片覆盖) */
void OnboardLed_Set(uint8_t on);

#endif /* __ONBOARD_BLINK_H__ */
