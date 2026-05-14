/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2026-05-13 15:10:40
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2026-05-13 15:10:58
 * @FilePath: /mas_embedded_threadx/modules/BOARDCOMM/module_boardcomm.h
 * @Description:
 */
#ifndef _MODULE_BOARDCOMM_H_
#define _MODULE_BOARDCOMM_H_

#include <stdint.h>

/* CAN ID 定义 */
#define BOARDCOMM_GIMBAL_ID  0x310 /* 云台 */
#define BOARDCOMM_CHASSIS_ID 0x311 /* 底盘 */

/* 消息类型 (首字节) */
#define BOARDCOMM_TYPE_CMD   0x01
#define BOARDCOMM_TYPE_UI    0x02

/* 使用的 CAN 总线 */
#ifndef BOARDCOMM_CAN
#define BOARDCOMM_CAN BSP_CAN_HANDLE2
#endif

#pragma pack(1)

typedef struct
{
    uint8_t type;         /* 消息类型 = 0x01 */
    int8_t  vx;           /* 前进速度 (×100使用) */
    int8_t  vy;           /* 横移速度 (×100使用) */
    int8_t  wz;           /* 旋转速度 (×100使用) */
    int16_t offset_angle; /* 对齐偏移角度 */
    uint8_t chassis_mode; /* 底盘模式 */
    uint8_t reserved;     /* 保留 */
} BoardComm_GimbalToChassis_cmd_t;

typedef struct
{
    uint8_t type;         /* 消息类型 = 0x02*/
    int16_t yaw;          /* 云台偏航角 */
    int16_t pitch;        /* 云台俯仰角 */
    uint8_t gimbal_mode;  /* 云台模式 */
    uint8_t chassis_mode; /* 底盘模式 */
    uint8_t shoot_mode;   /* 射击模式 */
} BoardComm_GimbalToChassis_ui_t;

typedef struct
{
    uint16_t current_hp;                /* 当前血量 */
    uint16_t chassis_power_limit;       /* 底盘功率限制 */
    uint16_t projectile_allowance_17mm; /* 剩余17mm弹量 */
    uint16_t game_progress    : 4;      /* 低4bit: 比赛阶段 */
    uint16_t robot_color      : 1;      /* bit4: 机器人颜色 (0=红,1=蓝) */
    uint16_t shooter_heat_pct : 7;      /* 7bit: 枪管热量% (heat/limit * 100) */
    uint16_t reserved         : 4;      /* 高4bit: 保留 */
} BoardComm_ChassisToGimbal_referee_t;

#pragma pack()
/**
 * @brief 初始化板间通信模块
 */
void Module_BoardComm_Init(void);
/**
 * @brief 发送板间通信数据
 * @param cmd 云台命令
 * @param ui 云台UI
 * @param referee 底盘状态
 * @note 需要发送哪个，填哪个，其他参数填NULL即可
 */
void Module_BoardComm_Send(BoardComm_GimbalToChassis_cmd_t *cmd, BoardComm_GimbalToChassis_ui_t *ui, BoardComm_ChassisToGimbal_referee_t *referee);
/**
 * @brief 获取板间通信数据
 * @param cmd 云台命令
 * @param ui 云台UI
 * @param referee 底盘状态
 * @note 需要获取哪个，填哪个，其他参数填NULL即可
 */
void Module_BoardComm_Get(BoardComm_GimbalToChassis_cmd_t *cmd, BoardComm_GimbalToChassis_ui_t *ui, BoardComm_ChassisToGimbal_referee_t *referee);

#endif // _MODULE_BOARDCOMM_H_
