/*
 * @Description: 打靶环数 CAN 上报模块 (5 从板 → 1 主控)
 *
 * 角色(条件编译): 用 RINGCOMM_BOARD_ID 区分
 *   RINGCOMM_BOARD_ID = 0   → 主控(收): 注册 5 路接收, 按板(1..5)存最新环数
 *   RINGCOMM_BOARD_ID = 1~5 → 从板(发): TX ID = RINGCOMM_TX_ID_BASE + 板号
 *
 * 5 块从板的按键/灯逻辑完全一致, 仅靠编译宏改 TX ID 区分。
 * 数据帧固定 8 字节: board_id + ring + status + seq + reserved(4)。
 * 主控侧每个上报都同时带 板号(1..5) 与 环数(1..9):
 *   查询: RingComm_GetRing(board) / RingComm_GetBoardMask()
 *   实时: RingComm_SetRxCallback(cb) → cb(board, ring)
 */

#ifndef __RINGCOMM_H__
#define __RINGCOMM_H__

#include <stdint.h>
#include "bsp_can.h"

/* ============ 编译期配置: 按板子改 ============ */
/* 角色/板号: 0=主控(收); 1..5=从板(发) */
#ifndef RINGCOMM_BOARD_ID
#define RINGCOMM_BOARD_ID 1 /* 当前=1: 发送端 */
#endif
/* 从板 TX ID 基址: 实际 TX = RINGCOMM_TX_ID_BASE + BOARD_ID (1..5) */
#ifndef RINGCOMM_TX_ID_BASE
#define RINGCOMM_TX_ID_BASE 0x300U
#endif
/* 用哪路 CAN */
#ifndef RINGCOMM_CAN
#define RINGCOMM_CAN BSP_CAN_HANDLE1
#endif
/* 环数档位: 与实测一致 = 9 (1..9 环) */
#ifndef RINGCOMM_RING_CNT
#define RINGCOMM_RING_CNT 9
#endif
/* ============================================ */

#define RINGCOMM_MAX_BOARD 5
#define RINGCOMM_INVALID   0xFF /* 尚未收到/非法环数 */

/* 上报帧: 固定 8 字节 */
typedef struct __attribute__((packed))
{
    uint8_t board_id;  /* 板号 1..5 */
    uint8_t ring;      /* 环数 */
    uint8_t status;    /* 状态/备用 */
    uint8_t seq;       /* 序号(丢帧/去重) */
    uint8_t reserved[4];
} RingReport_t;
_Static_assert(sizeof(RingReport_t) == 8, "RingReport_t must be 8 bytes");

/* 初始化: 从板建发送设备; 主控注册 5 路接收 */
void RingComm_Init(void);

/* 灯珠号(1..271) → 环数(1..9): 按实测环段映射(见 ringmap) */
uint8_t RingComm_RingOfLed(uint16_t led);

/* 从板: 上报指定环数 */
void RingComm_SendRing(uint8_t ring);

/* 从板: 上报"命中某颗灯珠"对应环数(占位路径, 便于先联调) */
void RingComm_SendLedHit(uint16_t led);

/* 从板: 打印 CAN 控制器状态/错误码(排障用: 判断是否因无 ACK/总线错误而发不出) */
void RingComm_TxHealthLog(void);

/* 主控: 读取某板(1..5)最新环数; 没收到过返回 RINGCOMM_INVALID */
uint8_t RingComm_GetRing(uint8_t board);

/* 主控: 哪些板(1..5)已上报过: bit(b-1)=1 表示板 b 收到过 */
uint32_t RingComm_GetBoardMask(void);

/* 主控: 每收到一帧上报即回调 (board=1..5, ring=1..9).
 * 注意: 在 CAN RX 线程上下文执行, 勿放耗时/阻塞代码; NULL 关闭 */
typedef void (*RingComm_RxCallback_t)(uint8_t board, uint8_t ring);
void RingComm_SetRxCallback(RingComm_RxCallback_t cb);

#endif /* __RINGCOMM_H__ */
