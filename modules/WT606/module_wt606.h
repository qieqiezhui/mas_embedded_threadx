/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2026-01-27 19:55:26
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2026-05-14 16:51:58
 * @FilePath: /mas_embedded_threadx/modules/WT606/module_wt606.h
 * @Description:
 */
#ifndef _MODULE_WT606_H_
#define _MODULE_WT606_H_

#include "bsp_uart.h"
#include "module_offline.h"
#include <stdbool.h>
#include <stdint.h>

// 这里根据需要自定义修改
#ifndef WT606_UART
#define WT606_UART huart1
#endif

#ifndef WT606_TASK_STACK_SIZE
#define WT606_TASK_STACK_SIZE 1024 /* 检测任务栈 (字节) */
#endif
#ifndef WT606_TASK_PRIORITY
#define WT606_TASK_PRIORITY 8 /* 检测任务优先级 */
#endif

typedef struct
{
    uint8_t         initialized;
    float           acc[3];
    float           gyro_dps[3];
    float           gyro_rps[3];
    float           Euler_degree[3]; // roll pitch yaw
    float           Euler_rad[3];    // roll pitch yaw
    float           YawTotalAngle_degree;
    float           YawTotalAngle_rad;
    int32_t         YawRoundCount;
    float           last_yaw_degree;
    UART_Device    *uart_dev;
    Offline_Device *offline_dev;
} Module_WT606_Device_t;

void Module_WT606_Init();

const Module_WT606_Device_t *Module_WT606_Get();

uint8_t Module_WT606_Get_offline_state(void);

#endif // _MODULE_WT606_H_
