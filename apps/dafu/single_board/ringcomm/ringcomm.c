/*
 * @Description: 打靶环数 CAN 上报实现 (条件编译: 主控收 / 从板发)
 */

#include "ringcomm.h"

#include <string.h>

#include "ringmap.h"

#define LOG_TAG "ringcomm"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

#if RINGCOMM_BOARD_ID == 0
/* ============ 主控(收) ============ */
static Can_Device *s_rx_dev[RINGCOMM_MAX_BOARD];
static uint8_t     s_ring[RINGCOMM_MAX_BOARD]; /* index = board-1 */
static uint32_t    s_mask = 0;                 /* bit(b-1): 板 b 已上报过 */
static RingComm_RxCallback_t s_rx_cb = 0;

/* BSP CAN 收到帧回调: 由 dev->rx_id 反推板号, 同时提供 板号+环数 */
static void ringcomm_rx(Can_Device *dev, const uint8_t *data, uint8_t len)
{
    if (dev == NULL || data == NULL || len < sizeof(RingReport_t))
    {
        return;
    }
    uint8_t board = (uint8_t)(dev->rx_id - RINGCOMM_TX_ID_BASE);
    if ((board < 1u) || (board > RINGCOMM_MAX_BOARD))
    {
        return;
    }
    RingReport_t *r = (RingReport_t *)(void *)data;
    s_ring[board - 1u] = r->ring;
    s_mask |= (uint32_t)(1u << (board - 1u));
    if (s_rx_cb != 0)
    {
        s_rx_cb(board, r->ring); /* 实时把 板号+环数 交给上层 */
    }
    LOG_I("RX board=%u ring=%u seq=%u", (unsigned)board, (unsigned)r->ring, (unsigned)r->seq);
}

void RingComm_Init(void)
{
    for (uint8_t b = 1; b <= RINGCOMM_MAX_BOARD; b++)
    {
        Can_Device_Init_Config_s cfg = {
            .hcan        = RINGCOMM_CAN,
            .tx_id       = RINGCOMM_TX_ID_BASE + b,
            .rx_id       = RINGCOMM_TX_ID_BASE + b,
            .rx_callback = ringcomm_rx,
        };
        s_rx_dev[b - 1u] = BSP_CAN_Device_Init(&cfg);
        s_ring[b - 1u]   = RINGCOMM_INVALID;
        if (s_rx_dev[b - 1u] == NULL)
        {
            LOG_E("board %u rx init failed", (unsigned)b);
        }
    }
    LOG_I("ringcomm master: listen 0x%lX..0x%lX", (unsigned long)(RINGCOMM_TX_ID_BASE + 1u),
          (unsigned long)(RINGCOMM_TX_ID_BASE + RINGCOMM_MAX_BOARD));
}

uint8_t RingComm_GetRing(uint8_t board)
{
    if ((board < 1u) || (board > RINGCOMM_MAX_BOARD))
    {
        return RINGCOMM_INVALID;
    }
    return s_ring[board - 1u];
}

uint32_t RingComm_GetBoardMask(void)
{
    return s_mask;
}

void RingComm_SetRxCallback(RingComm_RxCallback_t cb)
{
    s_rx_cb = cb;
}

#else /* 从板(发): RINGCOMM_BOARD_ID 1..5 */
static Can_Device *s_tx_dev;
static uint8_t     s_seq = 0; /* 发送序号(仅从板用) */

void RingComm_Init(void)
{
    /* 注册一个设备以初始化 CAN 总线并建立发送通道 (rx 用本板 id, 不回显) */
    Can_Device_Init_Config_s cfg = {
        .hcan        = RINGCOMM_CAN,
        .tx_id       = RINGCOMM_TX_ID_BASE + RINGCOMM_BOARD_ID,
        .rx_id       = RINGCOMM_TX_ID_BASE + RINGCOMM_BOARD_ID,
        .rx_callback = NULL,
    };
    s_tx_dev = BSP_CAN_Device_Init(&cfg);
    if (s_tx_dev == NULL)
    {
        LOG_E("board %u can init failed", (unsigned)RINGCOMM_BOARD_ID);
        return;
    }
    LOG_I("ringcomm slave #%u: TX id 0x%lX", (unsigned)RINGCOMM_BOARD_ID,
          (unsigned long)(RINGCOMM_TX_ID_BASE + RINGCOMM_BOARD_ID));
}

static void ringcomm_send(const RingReport_t *r)
{
    if (s_tx_dev == NULL)
    {
        return;
    }
    BSP_CAN_Send(s_tx_dev, (const uint8_t *)r, sizeof(RingReport_t));
}

void RingComm_SendRing(uint8_t ring)
{
    RingReport_t r;
    memset(&r, 0, sizeof(r));
    r.board_id = RINGCOMM_BOARD_ID;
    r.ring     = ring;
    r.status   = 0;
    r.seq      = s_seq++;
    ringcomm_send(&r);
}

void RingComm_SendLedHit(uint16_t led)
{
    RingComm_SendRing(RingComm_RingOfLed(led));
}

void RingComm_TxHealthLog(void)
{
    if (s_tx_dev == NULL)
    {
        return;
    }
    CAN_HandleTypeDef *h = RINGCOMM_CAN;
    if (h == NULL)
    {
        return;
    }
    /* 状态: 4=ERROR_ACTIVE 5=ERROR_WARNING 6=ERROR_PASSIVE 7=BUS_OFF (正常=4) */
    HAL_CAN_StateTypeDef st = HAL_CAN_GetState(h);
    uint32_t             err = HAL_CAN_GetError(h); /* 含 HAL_CAN_ERROR_ACK 等 */
    LOG_I("CAN health: state=%u err=0x%lX", (unsigned)st, (unsigned long)err);
}

#endif /* RINGCOMM_BOARD_ID */

/* 从板侧: 以上主控 API 提供空实现, 保证任意角色都能链接 */
#if RINGCOMM_BOARD_ID != 0
uint32_t RingComm_GetBoardMask(void)
{
    return 0u;
}

void RingComm_SetRxCallback(RingComm_RxCallback_t cb)
{
    (void)cb;
}
#endif

/* 灯珠号(1..271) → 环数(1..9): 实测环段映射 */
uint8_t RingComm_RingOfLed(uint16_t led)
{
    return RingMap_RingOfLed(led);
}
