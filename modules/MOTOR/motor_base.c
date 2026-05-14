#include "motor_base.h"

#define LOG_TAG "MotorBase"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

static list_head_t g_motor_list;

static int update_callback(Motor_Base *motor, void *arg)
{
    (void)arg;

    if (Module_Offline_get_device_status(motor->offline_dev) == STATE_OFFLINE || motor->setting.enableflag == 0)
    {
        motor->controller.output           = 0;
        motor->controller.speed_PID.Output = 0;
        motor->controller.speed_PID.Iout   = 0;
        motor->controller.angle_PID.Output = 0;
        motor->controller.angle_PID.Iout   = 0;
        return 0;
    }

    if (motor->ControlAndSend) motor->ControlAndSend(motor);
    return 0;
}

void Motor_BaseInit(void)
{
    list_init(&g_motor_list);
    LOG_I("Motor base initialized");
}

void Motor_Register(Motor_Base *motor)
{
    if (motor == NULL) return;
    list_add(&g_motor_list, &motor->node);
}

void Motor_UpdateAll(void) { list_foreach_entry(&g_motor_list, Motor_Base, node, update_callback, NULL); }
