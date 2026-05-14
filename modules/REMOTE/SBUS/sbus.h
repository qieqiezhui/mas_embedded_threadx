/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2026-05-10 17:32:15
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2026-05-10 18:20:52
 * @FilePath: /mas_embedded_threadx/modules/REMOTE/SBUS/sbus.h
 * @Description:
 */
#ifndef _SBUS_H_
#define _SBUS_H_

#include "module_remote.h"
#include <stdint.h>

#define SBUS_CHX_BIAS ((uint16_t)1024)
#define SBUS_CHX_UP   ((uint16_t)240)
#define SBUS_CHX_DOWN ((uint16_t)1807)

/**
 * @brief SBUS 初始化 (配置 UART + 注册 offline)
 * @return 0 成功, -1 失败
 */
int8_t remote_sbus_init(void);

/**
 * @brief SBUS 单帧解码, 直接写入统一结构体
 * @param data 统一遥控数据结构指针
 */
void remote_sbus_decode(Remote_Data_t *data);

#endif // _SBUS_H_
