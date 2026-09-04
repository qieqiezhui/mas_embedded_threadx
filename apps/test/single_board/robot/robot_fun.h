#ifndef __ROBOT_FUN_H__
#define __ROBOT_FUN_H__
#include "test_def.h"


void remote_control(Chassis_Ctrl_Cmd_t *Chassis_Ctrl, Shoot_Ctrl_Cmd_t *Shoot_Ctrl, Gimbal_Ctrl_Cmd_t *Gimbal_Ctrl);
int16_t CalcOffsetAngle(float getyawangle);


#endif