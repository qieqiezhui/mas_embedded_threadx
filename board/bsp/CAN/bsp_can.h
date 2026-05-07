/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2026-05-05 22:15:45
 * @FilePath: /mas_embedded_threadx/board/bsp/CAN/bsp_can.h
 * @Description:
 */
#ifndef _BSP_CAN_H_
#define _BSP_CAN_H_

#if defined(STM32H723xx)
#include "fdcan.h"
#elif defined(STM32F407xx)
#include "can.h"
#endif

#include <stdint.h>

#if defined(STM32H723xx)
#define BSP_CAN_BUS_NUM 3
#elif defined(STM32F407xx)
#define BSP_CAN_BUS_NUM 2
#endif

#define BSP_CAN_RX_FIFO_SIZE    8
#define BSP_CAN_TX_FIFO_SIZE    8
#define BSP_CAN_MAX_DEV_PER_BUS 8
#define BSP_CAN_MAX_STD_ID      2048

typedef struct Can_Device Can_Device;

typedef void (*BSP_CAN_RxCallback_t)(Can_Device *dev, const uint8_t *data, uint8_t len);

typedef struct
{
#if defined(STM32H723xx)
    FDCAN_HandleTypeDef *hcan;
#elif defined(STM32F407xx)
    CAN_HandleTypeDef *hcan;
#endif
    uint32_t             tx_id;
    uint32_t             rx_id;
    BSP_CAN_RxCallback_t rx_callback;
    void                *user_arg;
} Can_Device_Init_Config_s;

typedef struct
{
#if defined(STM32H723xx)
    FDCAN_HandleTypeDef *hcan;
#elif defined(STM32F407xx)
    CAN_HandleTypeDef *hcan;
#endif
    uint32_t id;
    uint8_t  data[8];
    uint8_t  len;
} BSP_CanMsg_t;

/**
 * @brief 初始化 CAN 设备
 * @param config 设备初始化配置
 * @return 设备指针，失败返回 NULL
 */
Can_Device *BSP_CAN_Device_Init(Can_Device_Init_Config_s *config);
/**
 * @brief 发送 CAN 消息
 * @param device 设备指针
 * @param data 数据指针
 * @param len 数据长度
 * @return 成功返回 0，失败返回 -1
 */
int BSP_CAN_Send(Can_Device *device, const uint8_t *data, uint8_t len);
/**
 * @brief 发送 CAN 消息
 * @param msg 消息指针
 * @return 成功返回 0，失败返回 -1
 */
int BSP_CAN_SendMessage(BSP_CanMsg_t *msg);

#endif /* _BSP_CAN_H_ */
