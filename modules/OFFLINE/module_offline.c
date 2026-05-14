#include "module_offline.h"

#include "list.h"
#include "tx_api.h"
#include "bsp_def.h"
#include "bsp_dwt.h"
#include "bsp_beep.h"
#include "bsp_led.h"

#include <string.h>
#if OFFLINE_WATCHDOG_ENABLE
#include "iwdg.h"
#endif

#define LOG_TAG "Offline"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

struct Offline_Device
{
    list_node_t node;       /* 链表节点 (首字段, 不可移动) */
    char        name[32];   /* 设备名称  */
    uint32_t    timeout_ms; /* 超时时间  */
    uint8_t     beep_times; /* 蜂鸣次数*/
    bool        is_offline; /* 当前离线状态*/
    uint32_t    last_time;  /* 上次心跳时间戳 */
    uint8_t     enable;     /* 检测开关 */
};

// 模块内部变量

static list_head_t                g_device_list;                         /* 设备链表头 (哨兵节点) */
static volatile bool              g_silent_error;                        /* 静音报警标志  */
static bool                       g_initialized;                         /* 模块初始化完成标志*/
static TX_THREAD                  g_task_thread;                         /* 检测任务句柄 */
APPS_STACK_SECTION static uint8_t g_task_stack[OFFLINE_TASK_STACK_SIZE]; /* 检测任务栈*/

/* 链表遍历回调上下文 */
typedef struct
{
    uint32_t now;                /* 当前 DWT 毫秒时间戳 */
    bool     has_severe_fault;   /* 是否存在 beep_times > 0 的离线设备 */
    bool     has_silent_fault;   /* 是否存在 beep_times = 0 的离线设备 */
    uint8_t  request_beep_times; /* 设备中最小的 beep_times (蜂鸣次数) */
} detect_ctx_t;

/**
 * @brief 单设备离线判定回调
 * @param dev 设备节点指针 (外层结构体, 非 list_node_t)
 * @param arg detect_ctx_t 上下文指针, 遍历结果累积于此
 * @return 0 继续遍历, 非 0 提前终止
 */
static int detect_callback(Offline_Device *dev, void *arg)
{
    detect_ctx_t *ctx = (detect_ctx_t *)arg;

    /* 未启用的设备跳过 */
    if (!dev->enable) return 0;

    /* 心跳超时判定 */
    if (ctx->now - dev->last_time > dev->timeout_ms)
    {
        dev->is_offline = true;

        if (dev->beep_times > 0)
        {
            /* 取最小 beep_times */
            ctx->has_severe_fault = true;
            if (ctx->request_beep_times == 0 || dev->beep_times < ctx->request_beep_times)
            {
                ctx->request_beep_times = dev->beep_times;
            }
        }
        else
        {
            /* beep_times = 0 */
            ctx->has_silent_fault = true;
        }
    }
    else
    {
        /* 恢复在线 */
        dev->is_offline = false;
    }

    return 0;
}

/* 检测任务 */
static void offline_detect_task_entry(ULONG arg)
{
    (void)arg;

    /* 蜂鸣/LED 状态机变量 */
    static uint32_t beep_period_start_time; /* 当前 2s 大周期起始时刻 */
    static uint32_t beep_cycle_start_time;  /* 当前闪烁子周期起始时刻 */
    static uint8_t  remaining_beep_cycles;  /* 当前大周期内剩余闪烁次数 */

    while (1)
    {
#if OFFLINE_WATCHDOG_ENABLE
#if defined(STM32H723xx)
        HAL_IWDG_Refresh(&hiwdg1);
#elif defined(STM32F407xx)
        HAL_IWDG_Refresh(&hiwdg);
#endif
#endif
        if (!g_initialized) break;

        /* 链表遍历检测 */
        detect_ctx_t ctx = {
            .now                = (uint32_t)BSP_DWT_GetTimeline_ms(),
            .has_severe_fault   = false,
            .has_silent_fault   = false,
            .request_beep_times = 0,
        };

        list_foreach_entry(&g_device_list, Offline_Device, node, detect_callback, &ctx);

        uint8_t current_beep_times; /* 本周期蜂鸣次数 */

        if (ctx.has_severe_fault)
        {
            /* 取最小 beep_times */
            g_silent_error     = false;
            current_beep_times = ctx.request_beep_times;
        }
        else if (ctx.has_silent_fault)
        {
            /* 不发声但置标志位 */
            g_silent_error     = true;
            current_beep_times = 0;
        }
        else
        {
            /* 全部在线 */
            g_silent_error     = false;
            current_beep_times = 0;
        }

        /* 蜂鸣/LED  */
        uint32_t now = ctx.now;

        if (g_silent_error)
        {
            /* 红灯常亮, 蜂鸣关闭, 重置状态机 */
            BSP_BEEP_Set(0, 0);
            BSP_LED_Show(LED_Red);
            remaining_beep_cycles  = 0;
            beep_period_start_time = now;
        }
        else
        {
            /* 检查是否进入新的大周期 */
            if (now - beep_period_start_time > 2000UL)
            {
                remaining_beep_cycles  = current_beep_times;
                beep_period_start_time = now;
                beep_cycle_start_time  = now;
            }

            if (remaining_beep_cycles > 0)
            {
                /* 子周期 0–100ms: 红灯亮 + 蜂鸣响 */
                if (now - beep_cycle_start_time < 100UL)
                {
#if OFFLINE_BEEP_ENABLE
                    BSP_BEEP_Set(OFFLINE_BEEP_TUNE_VALUE, OFFLINE_BEEP_CTRL_VALUE);
#else
                    BSP_BEEP_Set(0, 0);
#endif
                    BSP_LED_Show(LED_Red);
                }
                /* 子周期 100–200ms: 黑灯 + 蜂鸣停 */
                else if (now - beep_cycle_start_time < 200UL)
                {
                    BSP_BEEP_Set(0, 0);
                    BSP_LED_Show(LED_Black);
                }
                /* 子周期 >200ms: 完成一次闪烁, 递减计数, 进入下一次 */
                else
                {
                    remaining_beep_cycles--;
                    beep_cycle_start_time = now;
                }
            }
            else
            {
                /* 当前大周期内闪烁已完成 (或无故障) */
                BSP_BEEP_Set(0, 0);
                if (current_beep_times > 0)
                {
                    BSP_LED_Show(LED_Black);
                }
                else
                {
                    BSP_LED_Show(LED_Green);
                }
            }
        }

        tx_thread_sleep(10);
    }
}

void Module_Offline_init(void)
{
    if (g_initialized) return;

    list_init(&g_device_list);
    g_silent_error = false;

    BSP_BEEP_Start();
    BSP_LED_Show(LED_Green);

    if (tx_thread_create(&g_task_thread, "Offline Detect", offline_detect_task_entry, 0, g_task_stack, OFFLINE_TASK_STACK_SIZE, OFFLINE_TASK_PRIORITY,
                         OFFLINE_TASK_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        LOG_E("Failed to create offline detect task");
        return;
    }

#if OFFLINE_WATCHDOG_ENABLE
#if defined(STM32H723xx)
    MX_IWDG1_Init();
    __HAL_DBGMCU_FREEZE_IWDG1();
#elif defined(STM32F407xx)
    MX_IWDG_Init();
    __HAL_DBGMCU_FREEZE_IWDG();
#endif
#endif

    g_initialized = true;
    LOG_I("Offline module initialized");
}

Offline_Device *Module_Offline_register(const Offline_Init_config_t *init)
{
    if (init == NULL || !g_initialized)
    {
        LOG_E("Register failed: init=%p initialized=%d", (void *)init, g_initialized);
        return NULL;
    }

    Offline_Device *dev = NULL;
    BSP_MEM_ALLOC_WAIT(dev, sizeof(Offline_Device), TX_NO_WAIT);
    if (dev == NULL)
    {
        LOG_E("Register [%s] failed: out of memory", init->name ? init->name : "?");
        return NULL;
    }

    /* 初始化节点 */
    memset(dev, 0, sizeof(Offline_Device));
    if (init->name)
    {
        strncpy(dev->name, init->name, sizeof(dev->name) - 1);
        dev->name[sizeof(dev->name) - 1] = '\0';
    }
    dev->timeout_ms = init->timeout_ms;
    dev->beep_times = init->beep_times;
    dev->enable     = init->enable;
    dev->is_offline = false;
    dev->last_time  = (uint32_t)BSP_DWT_GetTimeline_ms(); /* 初始心跳 = 当前时间 */
    dev->node.size  = sizeof(Offline_Device);             /* 类型安全检查 */

    /* 入链 */
    list_add(&g_device_list, &dev->node);

    LOG_I("Device [%s] registered (timeout=%lu ms, beep=%u)", dev->name, (unsigned long)dev->timeout_ms, dev->beep_times);
    return dev;
}

void Module_Offline_device_update(Offline_Device *dev)
{
    if (dev == NULL) return;
    dev->last_time = (uint32_t)BSP_DWT_GetTimeline_ms();
}

void Module_Offline_device_enable(Offline_Device *dev)
{
    if (dev == NULL) return;
    dev->enable = 1;
}

void Module_Offline_device_disable(Offline_Device *dev)
{
    if (dev == NULL) return;
    dev->enable = 0;
}

uint8_t Module_Offline_get_device_status(Offline_Device *dev)
{
    if (dev == NULL) return STATE_OFFLINE;
    return dev->is_offline ? STATE_OFFLINE : STATE_ONLINE;
}
