#include "module_vision.h"
#include "usbd_cdc_acm_user.h"
#include <string.h>
#include "tx_api.h"
#include "module_offline.h"

#define LOG_TAG "module_vision"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

#define SEND_HEADER 0xAA
#define SEND_TAIL   0x55
#define RECV_HEADER 0xBB
#define RECV_TAIL   0x55

static ReceivePacket   rx_packet;
static bool            rx_valid;
static Offline_Device *offline_dev = NULL;

/* 接收状态机 */
typedef enum
{
    STATE_WAIT_HEADER,
    STATE_RECV_DATA,
} RecvState_t;

static RecvState_t recv_state = STATE_WAIT_HEADER;
static uint8_t     recv_buf[sizeof(ReceivePacket)];
static uint8_t     recv_idx;

/* 对外函数 */

void Module_Vision_Init(void)
{
    recv_state = STATE_WAIT_HEADER;
    recv_idx   = 0;
    rx_valid   = false;

    Offline_Init_config_t offlineconfig = {.name = "minipc", .beep_times = 10, .enable = 1, .timeout_ms = 100};
    offline_dev                         = Module_Offline_register(&offlineconfig);
    if (offline_dev == NULL)
    {
        LOG_E("offline device register error");
        return;
    }

    LOG_I("Vision module initialized");
}

void Module_Vision_Send(SendPacket *packet)
{
    packet->header = SEND_HEADER;
    packet->tail   = SEND_TAIL;
    cdc_acm_send((uint8_t *)packet, sizeof(SendPacket), TX_WAIT_FOREVER);
}

ReceivePacket *Module_Vision_Receive(void)
{
    uint8_t byte;

    /* 读取所有可用字节 */
    while (cdc_acm_available())
    {
        cdc_acm_recv(&byte, 1);

        switch (recv_state)
        {
        case STATE_WAIT_HEADER:
            if (byte == RECV_HEADER)
            {
                recv_buf[0] = byte;
                recv_idx    = 1;
                recv_state  = STATE_RECV_DATA;
            }
            break;

        case STATE_RECV_DATA:
            recv_buf[recv_idx++] = byte;
            if (recv_idx >= sizeof(ReceivePacket))
            {
                /* 尾字节校验 */
                if (recv_buf[sizeof(ReceivePacket) - 1] == RECV_TAIL)
                {
                    memcpy(&rx_packet, recv_buf, sizeof(ReceivePacket));
                    rx_valid = true;
                }
                recv_state = STATE_WAIT_HEADER;
            }
            break;
        }
    }

    if (rx_valid)
    {
        rx_valid = false;
        return &rx_packet;
    }
    return NULL;
}
