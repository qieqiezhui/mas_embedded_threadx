#include "module_boardcomm.h"
#include "bsp_can.h"
#include "module_offline.h"
#include <string.h>

#define LOG_TAG "BoardComm"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

static BoardComm_GimbalToChassis_cmd_t     rx_cmd;
static BoardComm_GimbalToChassis_ui_t      rx_ui;
static BoardComm_ChassisToGimbal_referee_t rx_referee;

static Offline_Device *boardcomm_offline_dev = NULL;

/* CAN 接收回调 */
#if CHASSIS_BOARD
static void gimbal_rx_callback(Can_Device *dev, const uint8_t *data, uint8_t len)
{
    (void)dev;
    if (len != 8 || data == NULL) return;

    switch (data[0])
    {
    case BOARDCOMM_TYPE_CMD:
        memcpy(&rx_cmd, data, sizeof(rx_cmd));
        Module_Offline_device_update(boardcomm_offline_dev); // 只有收到命令时才更新离线状态
        break;
    case BOARDCOMM_TYPE_UI:
        memcpy(&rx_ui, data, sizeof(rx_ui));
        break;
    default:
        break;
    }
}
#endif

#if GIMBAL_BOARD
static void chassis_rx_callback(Can_Device *dev, const uint8_t *data, uint8_t len)
{
    (void)dev;
    if (len == sizeof(BoardComm_ChassisToGimbal_referee_t))
    {
        memcpy(&rx_referee, data, sizeof(rx_referee));
        Module_Offline_device_update(boardcomm_offline_dev);
    }
}
#endif

/* 对外函数 */

void Module_BoardComm_Init(void)
{
#if SINGLE_BOARD
    LOG_I("BoardComm skipped (single board)");
    return;
#endif

    /* 底盘 */
#if CHASSIS_BOARD
    Can_Device_Init_Config_s chassis_cfg = {
        .hcan        = BOARDCOMM_CAN,
        .tx_id       = BOARDCOMM_CHASSIS_ID,
        .rx_id       = BOARDCOMM_GIMBAL_ID,
        .rx_callback = gimbal_rx_callback,
    };
    if (!BSP_CAN_Device_Init(&chassis_cfg))
    {
        LOG_E("Failed to init can");
    }
#endif

    /* 云台 */
#if GIMBAL_BOARD
    Can_Device_Init_Config_s gimbal_cfg = {
        .hcan        = BOARDCOMM_CAN,
        .tx_id       = BOARDCOMM_GIMBAL_ID,
        .rx_id       = BOARDCOMM_CHASSIS_ID,
        .rx_callback = chassis_rx_callback,
    };
    if (!BSP_CAN_Device_Init(&gimbal_cfg))
    {
        LOG_E("Failed to init can");
    }
#endif

    Offline_Init_config_t offlineconfig = {.name = "boardcomm", .beep_times = 10, .enable = 1, .timeout_ms = 100};
    boardcomm_offline_dev               = Module_Offline_register(&offlineconfig);
    if (boardcomm_offline_dev == NULL)
    {
        LOG_E("offline device register error");
        return;
    }

    memset(&rx_cmd, 0, sizeof(rx_cmd));
    memset(&rx_ui, 0, sizeof(rx_ui));
    memset(&rx_referee, 0, sizeof(rx_referee));

    LOG_I("BoardComm initialized ");
}

void Module_BoardComm_Send(BoardComm_GimbalToChassis_cmd_t *cmd, BoardComm_GimbalToChassis_ui_t *ui, BoardComm_ChassisToGimbal_referee_t *referee)
{
    BSP_CanMsg_t msg;

    if (cmd)
    {
        cmd->type = BOARDCOMM_TYPE_CMD;
        msg       = (BSP_CanMsg_t){
                  .hcan = BOARDCOMM_CAN,
                  .id   = BOARDCOMM_GIMBAL_ID,
                  .len  = sizeof(*cmd),
        };
        memcpy(msg.data, cmd, msg.len);
        BSP_CAN_SendMessage(&msg);
    }

    if (ui)
    {
        ui->type = BOARDCOMM_TYPE_UI;
        msg      = (BSP_CanMsg_t){
                 .hcan = BOARDCOMM_CAN,
                 .id   = BOARDCOMM_GIMBAL_ID,
                 .len  = sizeof(*ui),
        };
        memcpy(msg.data, ui, msg.len);
        BSP_CAN_SendMessage(&msg);
    }

    if (referee)
    {
        msg = (BSP_CanMsg_t){
            .hcan = BOARDCOMM_CAN,
            .id   = BOARDCOMM_CHASSIS_ID,
            .len  = sizeof(*referee),
        };
        memcpy(msg.data, referee, msg.len);
        BSP_CAN_SendMessage(&msg);
    }
}

void Module_BoardComm_Get(BoardComm_GimbalToChassis_cmd_t *cmd, BoardComm_GimbalToChassis_ui_t *ui, BoardComm_ChassisToGimbal_referee_t *referee)
{
    if (cmd) memcpy(cmd, &rx_cmd, sizeof(*cmd));
    if (ui) memcpy(ui, &rx_ui, sizeof(*ui));
    if (referee) memcpy(referee, &rx_referee, sizeof(*referee));
}
