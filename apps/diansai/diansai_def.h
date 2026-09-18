#ifndef __diansai_def_h__
#define __diansai_def_h__

#include <stdint.h>
#include "main.h" /* GPIOC / GPIO_PIN_13 等板级引脚宏 */

/* ── 板载 LED ──
 * 在 board/f4_diansai/base.ioc 中配置为 PC13 开漏输出，低电平点亮
 */
#define DIANSAI_LED_PORT           GPIOC
#define DIANSAI_LED_PIN            GPIO_PIN_13

/* ── 心跳任务 ── */
#define DIANSAI_HEARTBEAT_MS       500 /* 心跳周期 (ms) */
#define DIANSAI_HEARTBEAT_STACK    512 /* 心跳任务栈大小 (byte) */
#define DIANSAI_HEARTBEAT_PRIORITY 8   /* 心跳任务优先级 */

#endif /* __diansai_def_h__ */
