/*
 * @file        robot_control.c
 * @brief       test 兵种 — 新模块接入与本地控制验证
 *
 * 新模块开发时，可在主循环中临时验证模块 API；测试完成后整理用法并移除临时代码。
 */
#include "robot_control.h"
#include "tx_api.h"
#include "bsp_def.h"
#include "robot_fun.h"
#include "chassis.h"
#include "gimbal.h"
#include "test_def.h"
#include "module_ins.h"
#include "shoot.h"

#define LOG_TAG "test_robot"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

static const Ins_t *ins = NULL;
static TX_THREAD                  robot_control_thread;
APPS_STACK_SECTION static uint8_t robot_control_thread_stack[1024];
static uint16_t yaw_ecd;
static Gimbal_Ctrl_Cmd_t          gimbal_cmd;
static Shoot_Ctrl_Cmd_t           shoot_cmd;
static Chassis_Ctrl_Cmd_t         chassis_cmd;

static void robot_control_task(ULONG thread_input)
{
    (void)thread_input;
    while (1)
    {
        /* 遥控器控制输入 */
        remote_control(&chassis_cmd, &shoot_cmd, &gimbal_cmd);
        chassis_cmd.offset_angle = CalcOffsetAngle(yaw_ecd) * 360.0f / 8191.0f;
        chassis_cmd.vx           = chassis_cmd.vx * CHASSIS_MAX_SPEED_MPS;
        chassis_cmd.vy           = chassis_cmd.vy * CHASSIS_MAX_SPEED_MPS;
        chassis_cmd.wz           = chassis_cmd.wz * CHASSIS_MAX_SPEED_MPS;
        chassis_task(&chassis_cmd);
        gimbal_task(&gimbal_cmd, &yaw_ecd);
        shoot_task(&shoot_cmd);
        tx_thread_sleep(2);
    }
}

int robot_control_init(void)
{
    ins = Module_INS_get();
    if (ins == NULL)
    {
        LOG_E("INS module is not initialized");
        return -1;
    }

    /* 底盘初始化 */
    chassis_init();
    gimbal_init();
    shoot_init();

    UINT status = tx_thread_create(&robot_control_thread, "robot_control_thread", robot_control_task, 0, robot_control_thread_stack,
                                   sizeof(robot_control_thread_stack), 30, 30, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        LOG_E("robot control thread create failed (0x%02x)", status);
        return -1;
    }

    LOG_I("test robot control init done");
    return 0;
}
