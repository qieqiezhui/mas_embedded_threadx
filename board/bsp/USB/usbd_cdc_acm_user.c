#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "usbd_cdc_acm_user.h"
#include "cmsis_gcc.h"
#include "kfifo.h"
#include "tx_api.h"


/* 端点地址 */
#define CDC_IN_EP          0x81
#define CDC_OUT_EP         0x02
#define CDC_INT_EP         0x83

/* USB 设备标识 */
#define USBD_VID           0xFFFF
#define USBD_PID           0xFFFF
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/* 配置描述符总长度 */
#define USB_CONFIG_SIZE    (9 + CDC_ACM_DESCRIPTOR_LEN)

/* 批量传输最大包长 */
#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

/* RX kfifo 缓冲区大小（2 的幂，≥ CDC_MAX_MPS） */
#define CDC_RX_FIFO_SIZE 1024

/* USB 描述符 */

static const uint8_t device_descriptor[] = {USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01)};

static const uint8_t config_descriptor[] = {USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
                                            CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x02)};

static const uint8_t device_quality_descriptor[] = {
    /* 设备限定描述符 */
    0x0a, USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
};

static const char *string_descriptors[] = {
    (const char[]){0x09, 0x04}, /* 语言 ID */
    "mas_embedded",             /* 制造商 */
    "CDC ACM Device",           /* 产品名称 */
    "000000000001",             /* 序列号 */
};

/* 描述符回调 */

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(string_descriptors) / sizeof(char *)))
    {
        return NULL;
    }
    return string_descriptors[index];
}

static const struct usb_descriptor cdc_descriptor = {
    .device_descriptor_callback         = device_descriptor_callback,
    .config_descriptor_callback         = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback    = NULL,
    .string_descriptor_callback         = string_descriptor_callback,
    .msosv1_descriptor                  = NULL,
    .msosv2_descriptor                  = NULL,
    .webusb_url_descriptor              = NULL,
    .bos_descriptor                     = NULL,
};

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t cdc_read_buffer[CDC_RX_FIFO_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t cdc_write_buffer[CDC_MAX_MPS];

static struct kfifo cdc_rx_fifo;
static TX_SEMAPHORE tx_sem;

/* USB 设备事件回调 */

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;
    switch (event)
    {
    case USBD_EVENT_RESET:
        break;
    case USBD_EVENT_CONNECTED:
        break;
    case USBD_EVENT_DISCONNECTED:
        break;
    case USBD_EVENT_RESUME:
        break;
    case USBD_EVENT_SUSPEND:
        break;
    case USBD_EVENT_CONFIGURED:
        tx_semaphore_put(&tx_sem);
        /* 启动首个 OUT 端点读传输，目标地址指向 kfifo 当前写入位置 */
        usbd_ep_start_read(busid, CDC_OUT_EP, cdc_read_buffer + (cdc_rx_fifo.in & cdc_rx_fifo.mask), CDC_MAX_MPS);
        break;
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        break;
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        break;
    default:
        break;
    }
}

/* CherryUSB 端点回调 */

/**
 * @brief 批量 OUT 端点回调（接收完成），推进 kfifo 写入索引。
 */
void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)ep;

    /* 推进 kfifo 写入索引 */
    __DMB();
    cdc_rx_fifo.in += nbytes;

    /* 计算下一次 DMA 目标地址，确保 4 字节对齐（DWC2 硬件要求） */
    unsigned int pos = cdc_rx_fifo.in & cdc_rx_fifo.mask;

    /* 若 pos 非 4 字节对齐，跳过补齐字节 */
    unsigned int align_gap = (4 - (pos & 0x3)) & 0x3;
    if (align_gap)
    {
        cdc_rx_fifo.in += align_gap;
        pos = cdc_rx_fifo.in & cdc_rx_fifo.mask;
    }

    /* 对齐后再计算尾部剩余空间，判断是否需要回绕 */
    unsigned int tail = (cdc_rx_fifo.mask + 1) - pos;
    if (tail < CDC_MAX_MPS)
    {
        /* 尾部连续空间不足 → 回绕到缓冲区起始 */
        cdc_rx_fifo.in += tail;
        pos = 0;
    }

    usbd_ep_start_read(busid, CDC_OUT_EP, cdc_read_buffer + pos, CDC_MAX_MPS);
}

/**
 * @brief 批量 IN 端点回调（发送完成）
 */
void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;

    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes)
    {
        /* 发送 0 长度包（ZLP） */
        usbd_ep_start_write(busid, CDC_IN_EP, NULL, 0);
    }
    else
    {
        tx_semaphore_put(&tx_sem);
    }
}

/* 端点 / 接口 注册结构体 */

static struct usbd_endpoint cdc_out_ep = {.ep_addr = CDC_OUT_EP, .ep_cb = usbd_cdc_acm_bulk_out};

static struct usbd_endpoint cdc_in_ep = {.ep_addr = CDC_IN_EP, .ep_cb = usbd_cdc_acm_bulk_in};

static struct usbd_interface intf0;
static struct usbd_interface intf1;



void cdc_acm_init(uint8_t busid, uintptr_t reg_base)
{
    kfifo_init(&cdc_rx_fifo, cdc_read_buffer, CDC_RX_FIFO_SIZE, 1);
    tx_semaphore_create(&tx_sem, "cdc_tx", 1);

    usbd_desc_register(busid, &cdc_descriptor);

    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf0));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf1));
    usbd_add_endpoint(busid, &cdc_out_ep);
    usbd_add_endpoint(busid, &cdc_in_ep);

    usbd_initialize(busid, reg_base, usbd_event_handler);
}


bool cdc_acm_send(const uint8_t *data, uint32_t len, uint32_t timeout)
{
    if (tx_semaphore_get(&tx_sem, timeout) != TX_SUCCESS || len > CDC_MAX_MPS || data == NULL)
    {
        return false;
    }

    memcpy(cdc_write_buffer, data, len);
    usbd_ep_start_write(0, CDC_IN_EP, cdc_write_buffer, len);
    return true;
}

uint32_t cdc_acm_available(void) { return kfifo_len(&cdc_rx_fifo); }

uint32_t cdc_acm_recv(uint8_t *data, uint32_t len) { return kfifo_out(&cdc_rx_fifo, data, len); }
