/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2026-05-10 17:40:19
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2026-05-11 15:42:38
 * @FilePath: /mas_embedded_threadx/modules/REMOTE/module_remote.h
 * @Description:
 */
#ifndef _MODULE_REMOTE_H_
#define _MODULE_REMOTE_H_

#include <stdint.h>

#ifndef REMOTE_UART
#define REMOTE_UART huart5 /* 遥控器串口 */
#endif
#ifndef REMOTE_VT_UART
#define REMOTE_VT_UART huart1 /* 图传串口 */
#endif

#ifndef REMOTE_SOURCE
#define REMOTE_SOURCE 1 /* 遥控器选择: 0=none, 1=sbus, 2=dt7 */
#endif
#ifndef REMOTE_VT_SOURCE
#define REMOTE_VT_SOURCE 0 /* 图传选择:   0=none, 1=vt02, 2=vt03 */
#endif
#ifndef REMOTE_DEAD_ZONE
#define REMOTE_DEAD_ZONE 10 /* 遥控器死区范围 */
#endif

/* 任务配置 */
#ifndef REMOTE_TASK_STACK_SIZE
#define REMOTE_TASK_STACK_SIZE 1024 /* 每个解码任务栈大小 */
#endif
#ifndef REMOTE_TASK_PRIORITY
#define REMOTE_TASK_PRIORITY 9 /* 遥控器解码任务优先级 */
#endif
#ifndef REMOTE_TASK_VT_PRIORITY
#define REMOTE_TASK_VT_PRIORITY 8 /* 图传任务优先级 */
#endif

/* 统一的按键类型定义 */
typedef enum
{
    KEY_W = 0,
    KEY_S,
    KEY_A,
    KEY_D,
    KEY_SHIFT,
    KEY_CTRL,
    KEY_Q,
    KEY_E,
    KEY_R,
    KEY_F,
    KEY_G,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_COUNT // 用于计数
} remote_key_e;

/* keyboard state structure */
typedef union
{
    uint16_t key_code;
    struct
    {
        uint16_t KEY_W     : 1;
        uint16_t KEY_S     : 1;
        uint16_t KEY_A     : 1;
        uint16_t KEY_D     : 1;
        uint16_t KEY_SHIFT : 1;
        uint16_t KEY_CTRL  : 1;
        uint16_t KEY_Q     : 1;
        uint16_t KEY_E     : 1;
        uint16_t KEY_R     : 1;
        uint16_t KEY_F     : 1;
        uint16_t KEY_G     : 1;
        uint16_t KEY_Z     : 1;
        uint16_t KEY_X     : 1;
        uint16_t KEY_C     : 1;
        uint16_t KEY_V     : 1;
        uint16_t KEY_B     : 1;
    } bit;
} keyboard_state_t;

typedef struct
{
    int16_t mouse_x;
    int16_t mouse_y;
    int16_t mouse_z;
    uint8_t mouse_l;
    uint8_t mouse_r;
    uint8_t mouse_m;
} mouse_state_t;

/**
 * @brief 统一遥控数据
 */
typedef struct
{
    /* 摇杆/通道数据 */
    int16_t channels[16]; /* ch[0..3]: 摇杆通道, ch[4..15]: 扩展通道 (SBUS) */
    int16_t wheel;        /* 滚轮 (DT7 / VT03) */
    uint8_t sw1;          /* 拨杆1 (DT7) */
    uint8_t sw2;          /* 拨杆2 (DT7) */
    uint8_t switch_pos;   /* 拨杆位置 (VT03) */

    /* 键鼠数据 */
    mouse_state_t    mouse;    /* 鼠标状态 */
    keyboard_state_t keyboard; /* 键盘状态 */
} Remote_Data_t;

/**
 * @brief 初始化遥控/图传模块
 */
void Module_Remote_init(void);

/**
 * @brief 获取统一遥控数据指针
 * @return 指向全局 Remote_Data_t 的指针（只读）
 */
const Remote_Data_t *Module_Remote_get_data(void);

/**
 * @brief 获取指定通道值
 * @param ch 通道号 (1-based, 1..16)
 * @return 通道值, 无效通道返回 0
 */
int16_t Module_Remote_get_channel(uint8_t ch);

/**
 * @brief 获取鼠标状态指针
 * @return 鼠标状态指针, 未初始化返回 NULL
 */
const mouse_state_t *Module_Remote_get_mouse(void);

/**
 * @brief 获取键盘状态指针
 * @return 键盘状态指针, 未初始化返回 NULL
 */
const keyboard_state_t *Module_Remote_get_keyboard(void);

/**
 * @brief 获取滚轮值
 * @return 滚轮值
 */
int16_t Module_Remote_get_wheel(void);

#endif // _MODULE_REMOTE_H_
