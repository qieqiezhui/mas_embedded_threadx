/*
 * @Description: dafu 机器人定义（纯 LED + 按键 轻量单板应用）
 *
 * 使用说明:
 *   1. 把按键/灯珠的硬件引脚映射改成你实际板子的 CubeMX 宏
 *   2. 如需增加模块，去 robot.cmake 打开对应 MODULES_*
 */

#ifndef __DAFU_DEF_H__
#define __DAFU_DEF_H__

#include <stdint.h>

/* ================================================================
 * WS2812 灯带配置 (GRB, 800kHz, 锁存保持, 状态切换才写一帧)
 *   Strip0 = 中间靶环带 271 颗, 1~9 环实测分段:
 *             1环 1-48 / 2环 49-90 / 3环 91-134 / 4环 135-174
 *             5环 175-210 / 6环 211-230 / 7环 231-250
 *             8环 251-262 / 9环 263-271
 *   Strip1 = 上灯带    86 颗  (GPIO 位带)
 *   Strip2 = 下灯带    92 颗  (GPIO 位带)
 * 注意: 具体引脚/SPI 实例统一收敛在 ws2812.c 顶部配置, PCB 定了改那即可
 * ================================================================ */
#define DAFU_STRIP_NUM        3
#define DAFU_STRIP0_LED_NUM   271
#define DAFU_STRIP1_LED_NUM   86
#define DAFU_STRIP2_LED_NUM   92

/* 颜色 (AARRGGBB, 与 BSP LED 习惯一致; 驱动内部会转成 GRB) */
#define DAFU_LED_COLOR_OFF    0xFF000000
#define DAFU_LED_COLOR_RED    0xFFFF0000
#define DAFU_LED_COLOR_GREEN  0xFF00FF00
#define DAFU_LED_COLOR_BLUE   0xFF0000FF
#define DAFU_LED_COLOR_YELLOW 0xFFFFFF00
#define DAFU_LED_COLOR_WHITE  0xFFFFFFFF

/* ================================================================
 * 按键配置
 * ================================================================ */
#define DAFU_KEY_NUM          9     /* 按键数量: idx0~7 = PA0~PA7, idx8 = PB0 */
#define DAFU_KEY_SCAN_MS      10    /* 扫描周期 (ms), 与主任务周期保持一致 */
#define DAFU_KEY_DEBOUNCE_MS  20    /* 软件消抖时间 (ms) */
#define DAFU_KEY_LONG_MS      800   /* 长按判定时间 (ms) */
#define DAFU_KEY_ACTIVE_LEVEL 0     /* 低有效: 按下接地 (配 CubeMX 输入上拉) */

/* 引脚占用提醒: 9 键占 PA0~PA7 + PB0; 灯带/SPI 等别选这些引脚 */

#endif /* __DAFU_DEF_H__ */
