#ifndef __GIMBAL_H__
#define __GIMBAL_H__


#include "test_def.h"

void gimbal_init(void);
void gimbal_task(Gimbal_Ctrl_Cmd_t *gimbal_cmd,uint16_t *yaw_ecd);

#endif