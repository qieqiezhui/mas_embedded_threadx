/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2026-04-20 07:56:26
 * @LastEditors: laladuduqq 17503181697@163.com
 * @LastEditTime: 2026-04-21 13:14:04
 * @FilePath: \mas_threadx\board\bsp\bsp_init.c
 * @Description:
 */
#include "bsp_init.h"

#include "bsp_dwt.h"

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "Robot_Init"
#include "ulog_def.h"

void BSP_Init(void)
{
#if defined(STM32H723xx)
    BSP_DWT_Init(480);
#elif defined(STM32F407xx)
    BSP_DWT_Init(168);
#endif

    LOG_I("BSP Init finish");
}
