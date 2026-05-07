/*
 * @Author: laladuduqq 2807523947@qq.com
 * @FilePath: /mas_embedded_threadx/board/bsp/CAN/bsp_can_internal.h
 * @Description: CAN BSP 内部类型
 */
#ifndef _BSP_CAN_INTERNAL_H_
#define _BSP_CAN_INTERNAL_H_

#include "bsp_can.h"
#include "list.h"
#include "kfifo.h"
#include <stdbool.h>

/* 设备查找表条目 */
typedef struct
{
    uint32_t    rx_id; /* 0 = 空闲槽位 */
    Can_Device *dev;   /* 设备指针 */
} can_dev_slot_t;

struct Can_Device
{
    list_node_t          node;              /* 强制第一个 */
    uint32_t             tx_id;             /* 发送 ID */
    uint32_t             rx_id;             /* 接收 ID */
    uint8_t              filter_bank_index; /* 过滤器索引 */
    BSP_CAN_RxCallback_t rx_callback;       /* 接收回调函数 */
    void                *user_arg;          /* 用户参数 */
    void                *_bus;              /* 指向 CAN_Bus_Manager 的指针 */
};

typedef struct
{
#if defined(STM32H723xx)
    FDCAN_HandleTypeDef *hcan;
#elif defined(STM32F407xx)
    CAN_HandleTypeDef *hcan;
#endif
    can_dev_slot_t devices[BSP_CAN_MAX_DEV_PER_BUS];  /* 设备查找表 */
    list_head_t    device_list;                       /* 设备链表头 */
    struct kfifo   rx_fifo;                           /* 接收 FIFO */
    BSP_CanMsg_t   rx_fifo_buf[BSP_CAN_RX_FIFO_SIZE]; /* 接收 FIFO 缓冲区 */
    struct kfifo   tx_fifo;                           /* 发送 FIFO */
    BSP_CanMsg_t   tx_fifo_buf[BSP_CAN_TX_FIFO_SIZE]; /* 发送 FIFO 缓冲区 */
    bool           initialized;                       /* 是否初始化 */
} CAN_Bus_Manager;

extern CAN_Bus_Manager g_can_bus[];

/* 设备查找 */
static inline Can_Device *can_dev_find(CAN_Bus_Manager *bus, uint32_t rx_id)
{
    for (int i = 0; i < BSP_CAN_MAX_DEV_PER_BUS; i++)
    {
        if (bus->devices[i].rx_id == rx_id) return bus->devices[i].dev;
    }
    return NULL;
}

static inline int can_dev_find_empty(CAN_Bus_Manager *bus)
{
    for (int i = 0; i < BSP_CAN_MAX_DEV_PER_BUS; i++)
    {
        if (bus->devices[i].dev == NULL) return i;
    }
    return -1;
}

static inline void can_dev_insert(CAN_Bus_Manager *bus, int slot, uint32_t rx_id, Can_Device *dev)
{
    bus->devices[slot].rx_id = rx_id;
    bus->devices[slot].dev   = dev;
}

static inline void can_dev_remove(CAN_Bus_Manager *bus, uint32_t rx_id)
{
    for (int i = 0; i < BSP_CAN_MAX_DEV_PER_BUS; i++)
    {
        if (bus->devices[i].rx_id == rx_id)
        {
            bus->devices[i].rx_id = 0;
            bus->devices[i].dev   = NULL;
            return;
        }
    }
}

#endif
