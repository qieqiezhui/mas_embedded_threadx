#ifndef _VT02_H_
#define _VT02_H_

#include "module_remote.h"
#include <stdint.h>

/**
 * @brief VT02 初始化
 * @return 0 成功, -1 失败
 */
int8_t remote_vt02_init(void);

/**
 * @brief VT02 单帧解码, 直接写入统一结构体
 * @param data 统一遥控数据结构指针
 */
void remote_vt02_decode(Remote_Data_t *data);

#endif // _VT02_H_
