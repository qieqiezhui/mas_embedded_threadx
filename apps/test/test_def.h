#ifndef __test_def_h__
#define __test_def_h__
#include <stdint.h>

// clang-format off
// 云台参数
#define PITCH_HORIZON_ANGLE 0.0f            // 云台处于水平位置时编码器值,若对云台有机械改动需要修改
#define PITCH_MAX_ANGLE     40.0f           // 云台竖直方向最大角度 (注意反馈如果是陀螺仪，则填写陀螺仪的角度)
#define PITCH_MIN_ANGLE     -20.0f          // 云台竖直方向最小角度 (注意反馈如果是陀螺仪，则填写陀螺仪的角度)
#define YAW_CHASSIS_ALIGN_ECD         3878            // 云台和底盘对齐指向相同方向时的电机编码器值,若对云台有机械改动需要修改
// 底盘参数
#define CHASSIS_MAX_SPEED_MPS         3.0f            // 底盘最大线速度 (m/s), 遥控满杆映射到此速度
// 发射参数
#define REDUCTION_RATIO_LOADER         36.0f          // 2006拨盘电机的减速比
#define ONE_BULLET_DELTA_ANGLE         60.0f          // 发射一发弹丸拨盘转动的距离,由机械设计图纸给出
#define NUM_PER_CIRCLE                 6              // 拨盘一圈的装载量

// clang-format on

//#pragma pack(1)

#define YAW_CHASSIS_ALIGN_ECD         3878            // 云台和底盘对齐指向相同方向时的电机编码器值,若对云台有机械改动需要修改
#define DEGREE_2_RAD        0.01745329252f // pi/180


typedef enum
{
    gimbal_genius_mode =0,
    gimbal_sb_mode,
    
}gimbal_mode_e;

typedef struct
{
    float         yaw;
    float         pitch;
    gimbal_mode_e gimbal_mode;
} Gimbal_Ctrl_Cmd_t;

typedef enum
{
    chassis_genius_mode =0,
    chassis_sb_mode,
    chassis_rotate_reverse,
    chassis_follow_gimbal_yaw,
    chassis_rotate
}chassis_mode_e;



typedef struct
{
    float         vx;
    float         vy;
    float          wz;
    float         offset_angle;
    chassis_mode_e chassis_mode;
} Chassis_Ctrl_Cmd_t;

typedef struct
{
    uint8_t       shoot_mode;
    uint8_t       load_mode;
    
    
} Shoot_Ctrl_Cmd_t;





#endif