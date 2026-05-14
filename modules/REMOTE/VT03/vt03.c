/*
 * @Author: laladuduqq 2807523947@qq.com
 * @Date: 2025-09-15 09:30:21
 * @LastEditors: laladuduqq 2807523947@qq.com
 * @LastEditTime: 2026-05-10 18:32:14
 * @FilePath: /mas_embedded_threadx/modules/REMOTE/VT03/vt03.c
 * @Description:
 */
#include "vt03.h"
#include "bsp_def.h"
#include "bsp_uart.h"
#include "crc_rm.h"
#include "module_offline.h"
#include "module_remote.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "usart.h"

#define LOG_TAG "remote_vt03"
#define LOG_LVL LOG_LVL_DBG
#include "ulog_def.h"

#if defined(REMOTE_VT_SOURCE) && (REMOTE_VT_SOURCE == 2)

/* 模块内部变量 */
BUFFER_SECTION static uint8_t vt03_rx_buf[64];  /* 接收缓冲区 */
static UART_Device           *vt03_uart;        /* UART 设备 */
static bool                   vt03_initialized; /* 初始化标志 */

int8_t remote_vt03_init(Offline_Device **out_offline)
{
    REMOTE_VT_UART.Init.BaudRate = 921600;
    if (HAL_UART_Init(&REMOTE_VT_UART) != HAL_OK)
    {
        LOG_E("uart init error");
        return -1;
    }

    Offline_Init_config_t offline_cfg = {
        .name       = "vt03",
        .timeout_ms = 100,
        .beep_times = 0,
        .enable     = 1,
    };
    Offline_Device *offline = Module_Offline_register(&offline_cfg);
    if (offline == NULL)
    {
        LOG_E("offline register error");
        return -1;
    }
    if (out_offline) *out_offline = offline;

    UART_Device_init_config uart_cfg = {
        .huart           = &REMOTE_VT_UART,
        .expected_rx_len = 21,
        .rx_buf          = vt03_rx_buf,
        .rx_buf_size     = 64,
        .rx_mode         = UART_MODE_DMA,
        .tx_mode         = UART_MODE_BLOCKING,
    };
    vt03_uart = BSP_UART_Device_Init(&uart_cfg);
    if (vt03_uart == NULL)
    {
        LOG_E("uart device init error");
        return -1;
    }

    vt03_initialized = true;
    LOG_I("vt03 init success");
    return TX_SUCCESS;
}

void remote_vt03_decode(Remote_Data_t *data, Offline_Device *offline)
{
    if (!vt03_initialized || !data) return;

    static uint8_t buf[32];
    uint32_t       rx_len;
    int            ret = BSP_UART_Read(vt03_uart, buf, &rx_len, TX_WAIT_FOREVER);
    if (ret <= 0 || rx_len != 21) return;

    /* 帧头校验 */
    if (buf[0] != 0xA9 || buf[1] != 0x53) return;

    /* CRC16 校验 */
    if (Verify_CRC16_Check_Sum(buf, 21) != RM_TRUE) return;

    /* 摇杆通道解码 (零偏 + 死区)  */
    int16_t ch1 = (buf[2] | (buf[3] << 8)) & 0x07FF;
    int16_t ch2 = ((buf[3] >> 3) | (buf[4] << 5)) & 0x07FF;
    int16_t ch3 = ((buf[4] >> 6) | (buf[5] << 2) | (buf[6] << 10)) & 0x07FF;
    int16_t ch4 = ((buf[6] >> 1) | (buf[7] << 7)) & 0x07FF;

    if (ch1 < VT03_CH_VALUE_MIN || ch1 > VT03_CH_VALUE_MAX || ch2 < VT03_CH_VALUE_MIN || ch2 > VT03_CH_VALUE_MAX || ch3 < VT03_CH_VALUE_MIN ||
        ch3 > VT03_CH_VALUE_MAX || ch4 < VT03_CH_VALUE_MIN || ch4 > VT03_CH_VALUE_MAX)
    {
        return;
    }

    data->channels[0] = ch1 - VT03_CH_VALUE_OFFSET;
    data->channels[1] = ch2 - VT03_CH_VALUE_OFFSET;
    data->channels[2] = ch3 - VT03_CH_VALUE_OFFSET;
    data->channels[3] = ch4 - VT03_CH_VALUE_OFFSET;

    if (abs(data->channels[0]) <= REMOTE_DEAD_ZONE) data->channels[0] = 0;
    if (abs(data->channels[1]) <= REMOTE_DEAD_ZONE) data->channels[1] = 0;
    if (abs(data->channels[2]) <= REMOTE_DEAD_ZONE) data->channels[2] = 0;
    if (abs(data->channels[3]) <= REMOTE_DEAD_ZONE) data->channels[3] = 0;

    /* 滚轮 */
    int16_t wheel = ((buf[8] >> 1) | (buf[9] << 7)) & 0x07FF;
    wheel -= VT03_CH_VALUE_OFFSET;
    data->vt03.wheel = wheel;

    /* 按键 */
    data->vt03.switch_pos   = (buf[7] >> 4) & 0x03;
    data->vt03.pause        = (buf[7] >> 6) & 0x01;
    data->vt03.custom_left  = (buf[7] >> 7) & 0x01;
    data->vt03.custom_right = (buf[8] >> 0) & 0x01;
    data->vt03.trigger      = (buf[9] >> 4) & 0x01;

    /* 鼠标 */
    data->mouse.mouse_x = (int16_t)(buf[10] | (buf[11] << 8));
    data->mouse.mouse_y = (int16_t)(buf[12] | (buf[13] << 8));
    data->mouse.mouse_z = (int16_t)(buf[14] | (buf[15] << 8));
    data->mouse.mouse_l = buf[16] & 0x03;
    data->mouse.mouse_r = (buf[16] >> 2) & 0x03;
    data->mouse.mouse_m = (buf[16] >> 4) & 0x03;

    if (data->mouse.mouse_x < -32768 || data->mouse.mouse_x > 32767) data->mouse.mouse_x = 0;
    if (data->mouse.mouse_y < -32768 || data->mouse.mouse_y > 32767) data->mouse.mouse_y = 0;
    if (data->mouse.mouse_z < -32768 || data->mouse.mouse_z > 32767) data->mouse.mouse_z = 0;

    /* 键盘 */
    data->keyboard.key_code = buf[17] | (buf[18] << 8);

    if (offline) Module_Offline_device_update(offline);
}

#else

int8_t remote_vt03_init(Offline_Device **out_offline)
{
    (void)out_offline;
    return -1;
}
void remote_vt03_decode(Remote_Data_t *data, Offline_Device *offline)
{
    (void)data;
    (void)offline;
}

#endif
