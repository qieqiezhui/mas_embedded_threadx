#ifndef USBD_CDC_ACM_USER_H
#define USBD_CDC_ACM_USER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief  初始化 CDC ACM 设备
 * @param busid    USB 总线 ID（0 起始）
 * @param reg_base USB OTG 外设寄存器基地址
 */
void cdc_acm_init(uint8_t busid, uintptr_t reg_base);

/**
 * @brief  通过 CDC ACM 批量 IN 端点发送数据
 * @param data  待发送数据指针
 * @param len   数据字节数（不超过 CDC_MAX_MPS）
 * @param timeout  超时时间
 * @return      true: 传输已启动  false: 上次传输未完成 / 参数无效
 */
bool cdc_acm_send(const uint8_t *data, uint32_t len, uint32_t timeout);

/**
 * @brief  获取接收缓冲区中可读的字节数
 * @return 可读字节数
 */
uint32_t cdc_acm_available(void);

/**
 * @brief  从接收缓冲区读取数据
 * @param data  存放读取数据的缓冲区
 * @param len   期望读取的最大字节数
 * @return      实际读取的字节数
 */
uint32_t cdc_acm_recv(uint8_t *data, uint32_t len);

#endif
