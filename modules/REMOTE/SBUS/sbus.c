#include "sbus.h"
#include "bsp_def.h"
#include "bsp_uart.h"
#include "usart.h"
#include "module_offline.h"
#include "module_remote.h"
#include <stdint.h>
#include <stdlib.h>

#define LOG_TAG "remote_sbus"
#define LOG_LVL LOG_LVL_DBG
#include "ulog_def.h"

#if defined(REMOTE_SOURCE) && (REMOTE_SOURCE == 1)

/* 内部变量 */
BUFFER_SECTION static uint8_t sbus_rx_buf[64];  /* 接收缓冲区 */
static UART_Device           *sbus_uart;        /* UART 设备 */
static bool                   sbus_initialized; /* 初始化标志 */

int8_t remote_sbus_init(Offline_Device **out_offline)
{
    REMOTE_UART.Init.BaudRate = 100000;
    if (HAL_UART_Init(&REMOTE_UART) != HAL_OK)
    {
        LOG_E("uart init error");
        return -1;
    }

    Offline_Init_config_t offline_cfg = {
        .name       = "sbus",
        .timeout_ms = 50,
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
        .huart           = &REMOTE_UART,
        .expected_rx_len = 25,
        .rx_buf          = sbus_rx_buf,
        .rx_buf_size     = 64,
        .rx_mode         = UART_MODE_DMA,
        .tx_mode         = UART_MODE_BLOCKING,
    };
    sbus_uart = BSP_UART_Device_Init(&uart_cfg);
    if (sbus_uart == NULL)
    {
        LOG_E("uart device init error");
        return -1;
    }

    sbus_initialized = true;
    LOG_I("sbus init success");
    return 0;
}

void remote_sbus_decode(Remote_Data_t *data, Offline_Device *offline)
{
    if (!sbus_initialized || !data) return;

    static uint8_t buf[32];
    uint32_t       rx_len;
    int            ret = BSP_UART_Read(sbus_uart, buf, &rx_len, TX_WAIT_FOREVER);
    if (ret <= 0) return;

    /* 校验帧头 / 帧尾 / 长度 */
    if (buf[0] != 0x0F || buf[24] != 0x00 || rx_len != 25) return;

    /* 通道解码 (ch1-4 做零偏 + 死区) */
    int16_t ch1 = ((int16_t)buf[1] >> 0 | ((int16_t)buf[2] << 8)) & 0x07FF;
    int16_t ch2 = ((int16_t)buf[2] >> 3 | ((int16_t)buf[3] << 5)) & 0x07FF;
    int16_t ch3 = ((int16_t)buf[3] >> 6 | ((int16_t)buf[4] << 2) | (int16_t)buf[5] << 10) & 0x07FF;
    int16_t ch4 = ((int16_t)buf[5] >> 1 | ((int16_t)buf[6] << 7)) & 0x07FF;

    if (ch1 < SBUS_CHX_UP || ch1 > SBUS_CHX_DOWN) ch1 = SBUS_CHX_BIAS;
    if (ch2 < SBUS_CHX_UP || ch2 > SBUS_CHX_DOWN) ch2 = SBUS_CHX_BIAS;
    if (ch3 < SBUS_CHX_UP || ch3 > SBUS_CHX_DOWN) ch3 = SBUS_CHX_BIAS;
    if (ch4 < SBUS_CHX_UP || ch4 > SBUS_CHX_DOWN) ch4 = SBUS_CHX_BIAS;

    ch1 -= SBUS_CHX_BIAS;
    ch2 -= SBUS_CHX_BIAS;
    ch3 -= SBUS_CHX_BIAS;
    ch4 -= SBUS_CHX_BIAS;

    if (abs(ch1) <= REMOTE_DEAD_ZONE) ch1 = 0;
    if (abs(ch2) <= REMOTE_DEAD_ZONE) ch2 = 0;
    if (abs(ch3) <= REMOTE_DEAD_ZONE) ch3 = 0;
    if (abs(ch4) <= REMOTE_DEAD_ZONE) ch4 = 0;

    data->channels[0] = ch1;
    data->channels[1] = ch2;
    data->channels[2] = ch3;
    data->channels[3] = ch4;

    /* 扩展通道 (ch5-16, 原始值) */
    data->channels[4]  = (int16_t)(((int16_t)buf[6] >> 4 | ((int16_t)buf[7] << 4)) & 0x07FF);
    data->channels[5]  = (int16_t)(((int16_t)buf[7] >> 7 | ((int16_t)buf[8] << 1) | (int16_t)buf[9] << 9) & 0x07FF);
    data->channels[6]  = (int16_t)(((int16_t)buf[9] >> 2 | ((int16_t)buf[10] << 6)) & 0x07FF);
    data->channels[7]  = (int16_t)(((int16_t)buf[10] >> 5 | ((int16_t)buf[11] << 3)) & 0x07FF);
    data->channels[8]  = (int16_t)(((int16_t)buf[12] << 0 | ((int16_t)buf[13] << 8)) & 0x07FF);
    data->channels[9]  = (int16_t)(((int16_t)buf[13] >> 3 | ((int16_t)buf[14] << 5)) & 0x07FF);
    data->channels[10] = (int16_t)(((int16_t)buf[14] >> 6 | ((int16_t)buf[15] << 2) | (int16_t)buf[16] << 10) & 0x07FF);
    data->channels[11] = (int16_t)(((int16_t)buf[16] >> 1 | ((int16_t)buf[17] << 7)) & 0x07FF);
    data->channels[12] = (int16_t)(((int16_t)buf[17] >> 4 | ((int16_t)buf[18] << 4)) & 0x07FF);
    data->channels[13] = (int16_t)(((int16_t)buf[18] >> 7 | ((int16_t)buf[19] << 1) | (int16_t)buf[20] << 9) & 0x07FF);
    data->channels[14] = (int16_t)(((int16_t)buf[20] >> 2 | ((int16_t)buf[21] << 6)) & 0x07FF);
    data->channels[15] = (int16_t)(((int16_t)buf[21] >> 5 | ((int16_t)buf[22] << 3)) & 0x07FF);

    if (buf[23] == 0x00)
    {
        if (offline) Module_Offline_device_update(offline);
    }
}

#else

int8_t remote_sbus_init(Offline_Device **out_offline) { (void)out_offline; return -1; }
void   remote_sbus_decode(Remote_Data_t *data, Offline_Device *offline) { (void)data; (void)offline; }

#endif