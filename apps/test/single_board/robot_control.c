#include "robot_control.h"
#include "bsp_def.h"
#include "robot_fun.h"
#include "chassis.h"
#include "gimbal.h"
#include "test_def.h"
#include "module_ins.h"

const static Ins_t *ins = NULL;
static TX_THREAD                  robot_control_thread;
APPS_STACK_SECTION static uint8_t robot_control_thread_stack[1024];
static uint16_t yaw_ecd;
 static Gimbal_Ctrl_Cmd_t  gimbal_cmd;
 static Shoot_Ctrl_Cmd_t   shoot_cmd;
 static Chassis_Ctrl_Cmd_t chassis_cmd;

static void robot_control_task(ULONG thread_input)
{

    while(1)
    {
        /* 遥控器控制输入 */
    remote_control(&chassis_cmd, &shoot_cmd, &gimbal_cmd);
        

    //chassis_cmd.offset_angle = CalcOffsetAngle(yaw_ecd) * 360.0f / 8191.0f;
        chassis_cmd.offset_angle = CalcOffsetAngle(yaw_ecd) * 360.0f / 8191.0f;
        chassis_cmd.vx           = chassis_cmd.vx * CHASSIS_MAX_SPEED_MPS;
        chassis_cmd.vy           = chassis_cmd.vy * CHASSIS_MAX_SPEED_MPS;
        chassis_cmd.wz           = chassis_cmd.wz * CHASSIS_MAX_SPEED_MPS;
        chassis_task(&chassis_cmd);
        gimbal_task(&gimbal_cmd,&yaw_ecd);
        tx_thread_sleep(2);
    }
}

void robot_control_init(void)
{

    ins = Module_INS_get();
    if (ins == NULL)
    {
        return;
    }
    UINT status;

    /* 底盘初始化 */
    chassis_init();
    gimbal_init();
    status = tx_thread_create(&robot_control_thread, "robot_control_thread", robot_control_task, 0, robot_control_thread_stack, 1024, 30, 30,
                              TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        return;
    }

}

