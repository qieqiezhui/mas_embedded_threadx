/*
 * @file        robot_control.c
 * @brief       diansai 兵种 — 电赛单板应用骨架
 *
 * 当前只跑一个心跳任务（PC13 翻转 + RTT 日志），用于验证板子接入是否正常。
 * 业务逻辑写在 heartbeat_task_entry 的「业务逻辑区」。
 *
 * 注: 本兵种未 include 生成的 robot_def.h（自定义兵种不在其枚举内）
 */
#include "robot_control.h"
#include "tx_api.h"
#include "bsp_def.h"
#include "diansai_def.h"

#define LOG_TAG "diansai"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

static TX_THREAD                  g_heartbeat_thread;
APPS_STACK_SECTION static uint8_t g_heartbeat_stack[DIANSAI_HEARTBEAT_STACK];

static void heartbeat_task_entry(ULONG arg)
{
    (void)arg;

    while (1)
    {
        HAL_GPIO_TogglePin(DIANSAI_LED_PORT, DIANSAI_LED_PIN);

        /* ===== 业务逻辑区 ===== */

        tx_thread_sleep(DIANSAI_HEARTBEAT_MS);
    }
}

void robot_control_init(void)
{
    UINT status = tx_thread_create(&g_heartbeat_thread, "diansai_hb", heartbeat_task_entry, 0, g_heartbeat_stack, sizeof(g_heartbeat_stack),
                                   DIANSAI_HEARTBEAT_PRIORITY, DIANSAI_HEARTBEAT_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        LOG_E("heartbeat thread create failed (0x%02x)", status);
        return;
    }

    LOG_I("diansai robot init done");
}
