/*
 * @Description: 按键检测模块 (软件消抖 + 短按/长按事件)
 *
 * 实现方式: 周期轮询 Key_GetLevel() 原始电平, 与硬件解耦;
 * 真正的引脚电平读取放在 Key_GetLevel() (文件底部默认弱实现), 按板子改即可。
 */

#ifndef __KEY_DETECT_H__
#define __KEY_DETECT_H__

#include <stdint.h>
#include "dafu_def.h"

typedef enum
{
    KEY_EVENT_NONE = 0, /* 无事件 */
    KEY_EVENT_CLICK,    /* 短按(单击) */
    KEY_EVENT_LONG,     /* 长按(只触发一次) */
    /* 需要双击/连击时在此扩展, 并在 key_detect.c 的状态机里补逻辑 */
} Key_Event_e;

typedef struct
{
    uint8_t     debounced;    /* 消抖后的稳定电平 (1=按下) */
    uint8_t     candidate;    /* 当前候选电平(去抖过程) */
    uint8_t     long_fired;   /* 本次按下是否已触发过长按 */
    uint32_t    cand_since_ms; /* 候选电平出现时刻 */
    uint32_t    press_ms;     /* 按下时刻 */
    Key_Event_e event;        /* 待消费事件 */
    uint8_t     event_ready;  /* 是否有事件待读 */
} Key_t;

/* 初始化(清零状态) */
void Key_Init(void);

/* 周期调用(周期 = DAFU_KEY_SCAN_MS, 通常放进主任务循环) */
void Key_Scan(uint32_t now_ms);

/* 读取并清除某一路按键的事件, 无事件返回 KEY_EVENT_NONE */
Key_Event_e Key_GetEvent(uint8_t idx);

/* 按键名 (用于日志): idx0~7=PA0~PA7, idx8=PB0; 供调试打印 */
const char *Key_Name(uint8_t idx);

/* 查询某键当前稳定(消抖后)是否按下: 1=按下 */
uint8_t Key_IsPressed(uint8_t idx);

/* 硬件钩子: 返回按键 idx 的原始电平(1=按下)
 * 默认弱实现返回 0, 请按实际板子改写, 例如:
 *   return (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET) ^ (1-DAFU_KEY_ACTIVE_LEVEL);
 */
uint8_t Key_GetLevel(uint8_t idx);

#endif /* __KEY_DETECT_H__ */
