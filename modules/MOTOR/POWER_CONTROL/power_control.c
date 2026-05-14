#include "power_control.h"
#include "user_lib.h"
#include <math.h>
#include <string.h>

#define LOG_TAG "PowerCtrl"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

#define STEER_RATIO      0.8f
#define ERROR_THRESHOLD  20.0f
#define PROP_THRESHOLD   15.0f
#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define ABS(x)           ((x) >= 0.0f ? (x) : -(x))

/* 注册表 */
typedef struct
{
    Motor_Base       *motor;
    PowerCtrl_Role_e  role;
    PowerCtrl_Param_t param;
    /* 中间结果 */
    float raw_power;
    float assigned_power;
    float output;
} PC_Node_t;

static PC_Node_t s_nodes[POWER_CTRL_MAX_MOTORS];
static uint8_t   s_count;

static float       s_power_limit   = 45.0f;
static float       s_buffer_energy = 60.0f;
static uint8_t     s_use_buffer    = 1;
static PIDInstance s_buffer_pid;

/* 辅助: output → 扭矩 → 电流 → output */
static float motor_output_to_torque(Motor_Base *m)
{
    /* output 是电流整数值, 反解为扭矩 */
    float Kt_gear = m->info.torque_constant * m->info.gear_ratio;
    if (Kt_gear < 1e-6f) return 0;

    switch (m->info.motor_type)
    {
    case M3508:
        return IntegerToCurrent(-20.0f, 20.0f, -16384, 16384, (int16_t)m->controller.output) * Kt_gear;
    case M2006:
        return IntegerToCurrent(-10.0f, 10.0f, -10000, 10000, (int16_t)m->controller.output) * Kt_gear;
    case GM6020_CURRENT:
        return IntegerToCurrent(-3.0f, 3.0f, -16384, 16384, (int16_t)m->controller.output) * Kt_gear;
    /* 非 DJI 电机: output 直接就是扭矩 */
    case DM4310:
    case DM6220:
    case ZDT_STEEP_MOTOR:
        return m->controller.output;
    default:
        return 0;
    }
}

static void motor_set_torque(Motor_Base *m, float torque_nm)
{
    float Kt_gear = m->info.torque_constant * m->info.gear_ratio;
    if (Kt_gear < 1e-6f)
    {
        m->controller.output = torque_nm;
        return;
    }

    switch (m->info.motor_type)
    {
    case M3508:
        m->controller.output = currentToInteger(-20.0f, 20.0f, -16384, 16384, torque_nm / Kt_gear);
        break;
    case M2006:
        m->controller.output = currentToInteger(-10.0f, 10.0f, -10000, 10000, torque_nm / Kt_gear);
        break;
    case GM6020_CURRENT:
        m->controller.output = currentToInteger(-3.0f, 3.0f, -16384, 16384, torque_nm / Kt_gear);
        break;
    default:
        /* DM / UART : output 直接是扭矩 */
        m->controller.output = torque_nm;
        break;
    }
}

/* 分组限幅 */
static void limit_group(PC_Node_t **nodes, uint8_t N, float max_power, float *cmd_sum_out)
{
    if (N == 0)
    {
        if (cmd_sum_out) *cmd_sum_out = 0;
        return;
    }

    float omega[POWER_CTRL_MAX_MOTORS], tau[POWER_CTRL_MAX_MOTORS];
    float cmdPow[POWER_CTRL_MAX_MOTORS], errRad[POWER_CTRL_MAX_MOTORS];
    float sum_cmd = 0, allocatable = 0, sum_pos = 0, sum_err = 0;

    for (uint8_t i = 0; i < N; i++)
    {
        Motor_Base *m  = nodes[i]->motor;
        float       k1 = nodes[i]->param.k1, k2 = nodes[i]->param.k2, k3 = nodes[i]->param.k3;

        omega[i]  = m->measure.speed_rad / m->info.gear_ratio;
        tau[i]    = motor_output_to_torque(m);
        cmdPow[i] = tau[i] * omega[i] + k1 * ABS(omega[i]) + k2 * tau[i] * tau[i] + k3;

        nodes[i]->raw_power = cmdPow[i];
        sum_cmd += cmdPow[i];
        errRad[i] = ABS(m->controller.ref - m->measure.speed_rad);

        if (cmdPow[i] <= 0)
            allocatable += (-cmdPow[i]);
        else
        {
            sum_pos += cmdPow[i];
            sum_err += errRad[i];
        }
    }

    if (cmd_sum_out) *cmd_sum_out = sum_cmd;
    if (sum_cmd <= max_power)
    {
        for (uint8_t i = 0; i < N; i++)
        {
            nodes[i]->assigned_power = cmdPow[i];
            nodes[i]->output         = nodes[i]->motor->controller.output;
        }
        return;
    }

    allocatable += max_power;
    float confidence;
    if (sum_err >= ERROR_THRESHOLD)
        confidence = 1.0f;
    else if (sum_err > PROP_THRESHOLD)
        confidence = (sum_err - PROP_THRESHOLD) / (ERROR_THRESHOLD - PROP_THRESHOLD);
    else
        confidence = 0.0f;

    for (uint8_t i = 0; i < N; i++)
    {
        Motor_Base *m = nodes[i]->motor;
        if (cmdPow[i] <= 0)
        {
            nodes[i]->assigned_power = cmdPow[i];
            continue;
        }

        float w_err   = (sum_err > 1e-5f) ? (errRad[i] / sum_err) : (1.0f / N);
        float w_prop  = (sum_pos > 1e-5f) ? (cmdPow[i] / sum_pos) : (1.0f / N);
        float w       = confidence * w_err + (1.0f - confidence) * w_prop;
        float p_alloc = w * allocatable;

        if (errRad[i] < 10.47f && cmdPow[i] < p_alloc) p_alloc = cmdPow[i];
        nodes[i]->assigned_power = p_alloc;

        float Omega = omega[i];
        float a = nodes[i]->param.k2, b = Omega;
        float c = nodes[i]->param.k1 * ABS(Omega) + nodes[i]->param.k3 - p_alloc;
        float tau_new;

        if (a < 1e-8f)
            tau_new = (ABS(Omega) > 1e-5f) ? (p_alloc - nodes[i]->param.k1 * ABS(Omega) - nodes[i]->param.k3) / Omega : 0;
        else
        {
            float delta = b * b - 4 * a * c;
            if (delta < 0)
            {
                tau_new = -b / (2 * a);
            }
            else
            {
                float sd = sqrtf(delta);
                float r1 = (-b + sd) / (2 * a), r2 = (-b - sd) / (2 * a);
                tau_new = (m->controller.output >= 0) ? r1 : r2;
            }
        }
        tau_new = CLAMP(tau_new, -m->info.max_torque, m->info.max_torque);
        motor_set_torque(m, tau_new);
        nodes[i]->output = m->controller.output;
    }
}

/* 对外函数 */

void PowerControl_Register(Motor_Base *motor, PowerCtrl_Role_e role, PowerCtrl_Param_t param)
{
    if (!motor || s_count >= POWER_CTRL_MAX_MOTORS) return;

    /* 已注册则更新 */
    for (uint8_t i = 0; i < s_count; i++)
    {
        if (s_nodes[i].motor == motor)
        {
            s_nodes[i].role  = role;
            s_nodes[i].param = param;
            return;
        }
    }

    s_nodes[s_count].motor = motor;
    s_nodes[s_count].role  = role;
    s_nodes[s_count].param = param;
    s_count++;
}

void PowerControl_SetLimit(float power_limit_w, float buffer_energy_j, uint8_t use_buffer)
{
    s_power_limit   = power_limit_w;
    s_buffer_energy = buffer_energy_j;
    s_use_buffer    = use_buffer;
}

void PowerControl_Update(void)
{
    if (s_count == 0) return;

    /* 收集 drive/steer */
    PC_Node_t *drives[POWER_CTRL_MAX_MOTORS], *steers[POWER_CTRL_MAX_MOTORS];
    uint8_t    n_drive = 0, n_steer = 0;
    for (uint8_t i = 0; i < s_count; i++)
    {
        if (s_nodes[i].role == PC_ROLE_DRIVE)
            drives[n_drive++] = &s_nodes[i];
        else if (s_nodes[i].role == PC_ROLE_STEER)
            steers[n_steer++] = &s_nodes[i];
    }

    /* 可用功率 */
    float effective = s_power_limit;
    if (s_use_buffer)
    {
        if (s_buffer_pid.Kp == 0) /* lazy init */
        {
            PID_Init_Config_s cfg = {.MaxOut = 50, .DeadBand = 5, .Kp = 5, .Improve = 0x01};
            PIDInit(&s_buffer_pid, &cfg);
        }
        PIDCalculate(&s_buffer_pid, s_buffer_energy, 30.0f);
        effective -= s_buffer_pid.Output;
    }

    if (n_steer == 0)
    {
        limit_group(drives, n_drive, effective, NULL);
    }
    else
    {
        float steer_sum = 0;
        limit_group(steers, n_steer, effective * STEER_RATIO, &steer_sum);
        float steer_actual = (steer_sum < effective * STEER_RATIO) ? steer_sum : effective * STEER_RATIO;
        float drive_limit  = effective - steer_actual;
        if (drive_limit < 0) drive_limit = 0;
        limit_group(drives, n_drive, drive_limit, NULL);
    }
}
