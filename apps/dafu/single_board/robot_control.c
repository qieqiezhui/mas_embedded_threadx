/*
 * @Description: dafu 单板主任务
 *
 *  1) 9 键检测(PA0~PA7+PB0), 消抖后锁存切换灯态(按下→中灭+外蓝, 再按回红)
 *  2) 灯带两态显示(空闲红 / 按下态), 见 strip_blink
 *  3) 打靶环数 CAN 上报(ringcomm): 条件编译区分 主控(收)/5×从板(发)
 */

#include "robot_control.h"
#include "dafu_def.h"
#include "led_ctrl.h"
#include "onboard_blink.h"
#include "strip_blink.h"
#include "key_detect.h" /* 9 键独立检测 */
#include "ringcomm.h"   /* 打靶环数 CAN 上报: 条件编译 主控收/从板发 */

#include "tx_api.h"
#include "bsp_def.h" /* APPS_STACK_SECTION */

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "dafu"
#include "ulog_def.h"

#ifndef APP_TASK_STACK_SIZE
#define APP_TASK_STACK_SIZE 512
#endif
#ifndef APP_TASK_PRIORITY
#define APP_TASK_PRIORITY 15
#endif
#ifndef APP_TASK_PERIOD_MS
#define APP_TASK_PERIOD_MS 10
#endif

/* 从板: 按键→CAN 上报(占位触发): 按下沿发 ring=键号+1, 用于验证 5→1 通路.
 * 真实打靶命中信号接入后, 在命中处改调 RingComm_SendRing(实际环数) 即可. */
#if (RINGCOMM_BOARD_ID != 0) && !defined(RINGCOMM_KEY_SEND)
#define RINGCOMM_KEY_SEND 1
#endif

/* 灯珠号↔环数分档边界标定: 1 = 中间 271 螺带单颗循环点亮(占用整条中间带, 该模式下
 * 不参与按键/灯联动/CAN 上报). 平时保持 0. */
#ifndef DAFU_LED_MAP_MODE
#define DAFU_LED_MAP_MODE 0 /* 当前=0: 标定已完成, 正常固件 */
#endif

static TX_THREAD                  robot_control_thread;
APPS_STACK_SECTION static uint8_t robot_control_thread_stack[APP_TASK_STACK_SIZE];

static void robot_control_task(ULONG thread_input)
{
    (void)thread_input;

    uint32_t last_beat = tx_time_get();
#if !DAFU_LED_MAP_MODE
    uint8_t prev_press = 0;  /* 上一次是否有键按下(消抖后) */
    uint8_t light_latch = 0; /* 灯显示锁存态: 0=空闲红, 1=按下态(中灭+外蓝) */
#endif

    while (1)
    {
#if DAFU_LED_MAP_MODE
        /* 标定模式: 单颗循环点亮扫描(内部用 DWT 真实时间定步进), 不参与按键/灯联动/CAN */
        StripBlink_MapTick();
        tx_thread_sleep(APP_TASK_PERIOD_MS);
        continue;
#else
        uint32_t now_ms = tx_time_get();

        /* ── 9 键扫描(消抖) ── */
        Key_Scan(now_ms);
        for (uint8_t i = 0; i < DAFU_KEY_NUM; i++)
        {
            Key_GetEvent(i); /* 清事件(暂不区分短/长按) */
        }

        /* ── 打靶交互(锁存切换) ──
         * 空闲: 中间带(十字/全亮红) + 外圈红
         * 按一下 → 打中态: 中间只亮被命中那环(idx+1 环), 其余灭; 外圈变蓝; 保持(不随松手回)
         * 再按一下 → 回空闲
         */
        uint8_t any_press = 0;
        uint8_t hit_idx   = DAFU_KEY_NUM; /* 首个被按下的键号(无效=DAFU_KEY_NUM) */
        for (uint8_t i = 0; i < DAFU_KEY_NUM; i++)
        {
            if (Key_IsPressed(i) != 0u)
            {
                if (any_press == 0u)
                {
                    hit_idx = i;
                }
                any_press = 1;
            }
        }
        if (any_press && !prev_press)
        {
            light_latch ^= 1u;
            if (light_latch)
            {
                /* 打中态: 点亮 idx+1 环(键号占位=环号, 1..9 与实测环段对应) */
                uint8_t ring = (uint8_t)(hit_idx + 1u);
                StripBlink_SetHit(ring);

#if (RINGCOMM_BOARD_ID != 0) && RINGCOMM_KEY_SEND
                RingComm_SendRing(ring); /* 从板: 上报环数 */
                LOG_I("SEND key=%u ring=%u", (unsigned)hit_idx, (unsigned)ring);
#endif
            }
            else
            {
                StripBlink_SetKeyHold(0); /* 回空闲 */
            }
        }
        prev_press = any_press;

#if (RINGCOMM_BOARD_ID != 0)
        /* CAN 发送健康检查 (~1s 一次, 排障用): state=7(BUS_OFF)/err 含 ACK → 总线无节点应答 */
        {
            static uint32_t t_health = 0;
            if ((now_ms - t_health) >= 1000U)
            {
                t_health = now_ms;
                RingComm_TxHealthLog();
            }
        }
#endif
#endif /* !DAFU_LED_MAP_MODE */

        tx_thread_sleep(APP_TASK_PERIOD_MS);

        /* 心跳日志 (每 5s 一条, 表明主任务活着) */
        if ((tx_time_get() - last_beat) >= 5000U)
        {
            last_beat = tx_time_get();
            LOG_I("dafu app alive");
        }
    }
}

void robot_control_init(void)
{
    UINT status;

    Led_Init();          /* 内含 Ws2812_Init, 全灭 */

    OnboardBlink_Init(); /* 板载 PC13 闪灯(标定/正常模式都开, 作运行指示) */

#if DAFU_LED_MAP_MODE
    StripBlink_MapReset(); /* 边界标定: 清空中间 271 带, 准备单颗循环点亮 */
#else
    Key_Init();          /* 9 键检测初始化 (状态清零) */

    StripBlink_Init();   /* 灯带: 空闲显示(全亮/十字红) */

    RingComm_Init();     /* CAN 上报: 主控=收5路 / 从板=建发送设备(RINGCOMM_BOARD_ID 编译期决定) */
#endif

    status = tx_thread_create(&robot_control_thread, "dafu_robot_control", robot_control_task, 0,
                              robot_control_thread_stack, APP_TASK_STACK_SIZE,
                              APP_TASK_PRIORITY, APP_TASK_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        LOG_E("robot_control_thread create failed!");
        return;
    }

    LOG_I("dafu robot_control init success!");
}
