#include "vt02.h"
#include "bsp_def.h"
#include "bsp_uart.h"
#include "crc_rm.h"
#include "module_offline.h"
#include "module_remote.h"
#include <string.h>
#include "usart.h"

#define LOG_TAG "remote_vt02"
#define LOG_LVL LOG_LVL_DBG
#include "ulog_def.h"

#if defined(REMOTE_VT_SOURCE) && (REMOTE_VT_SOURCE == 1)

/* 模块内部变量 */
BUFFER_SECTION static uint8_t vt02_rx_buf[128]; /* 接收缓冲区 */
static UART_Device           *vt02_uart;        /* UART 设备 */
static Offline_Device        *vt02_offline;     /* 离线设备 */
static bool                   vt02_initialized; /* 初始化标志 */

int8_t remote_vt02_init(void)
{
    REMOTE_VT_UART.Init.BaudRate = 115200;
    if (HAL_UART_Init(&REMOTE_VT_UART) != HAL_OK)
    {
        LOG_E("uart init error");
        return TX_DELETED;
    }

    Offline_Init_config_t offline_cfg = {
        .name       = "vt02",
        .timeout_ms = 100,
        .beep_times = 0,
        .enable     = 2,
    };
    vt02_offline = Module_Offline_register(&offline_cfg);
    if (vt02_offline == NULL)
    {
        LOG_E("offline register error");
        return -1;
    }

    UART_Device_init_config uart_cfg = {
        .huart           = &REMOTE_VT_UART,
        .expected_rx_len = 0,
        .rx_buf          = vt02_rx_buf,
        .rx_buf_size     = 128,
        .rx_mode         = UART_MODE_DMA,
        .tx_mode         = UART_MODE_BLOCKING,
    };
    vt02_uart = BSP_UART_Device_Init(&uart_cfg);
    if (vt02_uart == NULL)
    {
        LOG_E("uart device init error");
        return -1;
    }

    vt02_initialized = true;
    LOG_I("vt02 init success");
    return 0;
}

void remote_vt02_decode(Remote_Data_t *data)
{
    if (!vt02_initialized || !data) return;

    static uint8_t buf[64];
    uint32_t       rx_len;
    int            ret = BSP_UART_Read(vt02_uart, buf, &rx_len, TX_WAIT_FOREVER);
    if (ret <= 0) return;

    /* 验证帧头 CRC8 */
    if (!Verify_CRC8_Check_Sum(buf, 5)) return;

    /* 判断是否为键鼠遥控数据 (0x0304) */
    uint16_t cmd_id = (buf[6] << 8) | buf[5];
    if (cmd_id != 0x0304) return;

    /* 解析键鼠数据 (偏移量 7) */
    uint8_t *d = &buf[7];

    data->mouse.mouse_x = (int16_t)((d[1] << 8) | d[0]);
    data->mouse.mouse_y = (int16_t)((d[3] << 8) | d[2]);
    data->mouse.mouse_z = (int16_t)((d[5] << 8) | d[4]);
    data->mouse.mouse_l = d[6];
    data->mouse.mouse_r = d[7];

    data->keyboard.key_code = (d[9] << 8) | d[8];

    Module_Offline_device_update(vt02_offline);
}

#else

int8_t remote_vt02_init(void) { return -1; }
void   remote_vt02_decode(Remote_Data_t *data) { (void)data; }

#endif
