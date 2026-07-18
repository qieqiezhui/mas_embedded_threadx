#include "module_remote.h"
#include <stdint.h>
//#include <string.h>
#include "user_lib.h"
#include "robot_fun.h"


static int16_t ch8=0;
static int16_t ch5=0;
float temp[4] = {0};
int16_t CalcOffsetAngle(float getyawangle)
{
    float offset_ecd;

    const float ECD_MAX  = 8191.0f;
    const float ECD_HALF = 4095.5f;

    offset_ecd = getyawangle - YAW_CHASSIS_ALIGN_ECD;

    // 归一化到最小旋转角度对应的编码器差值 ([-ECD_HALF, ECD_HALF])
    while (offset_ecd > ECD_HALF) offset_ecd -= ECD_MAX;
    while (offset_ecd < -ECD_HALF) offset_ecd += ECD_MAX;

    return (int16_t)offset_ecd;
}

void remote_control(Chassis_Ctrl_Cmd_t *Chassis_Ctrl, Shoot_Ctrl_Cmd_t *Shoot_Ctrl, Gimbal_Ctrl_Cmd_t *Gimbal_Ctrl)
{

    uint8_t state = Module_Remote_get_offline_status();

    if(state&0x01)
    {
        if(Chassis_Ctrl)
        {
         ch8 = Module_Remote_get_channel(8);
        if (ch8 == SBUS_CHX_UP)
            Chassis_Ctrl->chassis_mode = chassis_follow_gimbal_yaw;
        else if (ch8 == SBUS_CHX_BIAS)
            Chassis_Ctrl->chassis_mode = chassis_genius_mode;
        else if (ch8 == SBUS_CHX_DOWN)
        {
            Chassis_Ctrl->chassis_mode = chassis_rotate_reverse;
            
        }

        Chassis_Ctrl->vx = (float)Module_Remote_get_channel(2) / (float)(SBUS_CHX_DOWN - SBUS_CHX_BIAS);
            Chassis_Ctrl->vy = (float)Module_Remote_get_channel(1) / (float)(SBUS_CHX_DOWN - SBUS_CHX_BIAS);
            temp[0] = (float)Module_Remote_get_channel(2);
            temp[1] = (float)Module_Remote_get_channel(1);
            

        }
        if(Shoot_Ctrl)
        {


        
        }
        if(Gimbal_Ctrl)
        {
             ch5 = Module_Remote_get_channel(5);


            if(ch5 == SBUS_CHX_UP)
            {
                Gimbal_Ctrl->gimbal_mode = gimbal_genius_mode;
                Gimbal_Ctrl->yaw -= 0.001f * (float)Module_Remote_get_channel(4);
                Gimbal_Ctrl->pitch += 0.001f * (float)Module_Remote_get_channel(3);
                VAL_LIMIT(Gimbal_Ctrl->pitch, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE);
                temp[2] = (float)Module_Remote_get_channel(3);
                temp[3] = (float)Module_Remote_get_channel(4);
            }
            else if(ch5 == SBUS_CHX_BIAS)
            {
                Gimbal_Ctrl->gimbal_mode = gimbal_sb_mode;
            }
            else if(ch5 == SBUS_CHX_DOWN)
            {
                //Gimbal_Ctrl->gimbal_mode = gimbal_zero_force;
            }
            
        }
    }
    

}