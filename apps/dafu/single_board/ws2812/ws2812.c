/*
 * @Description: WS2812(GRB, 800kHz) 灯带驱动 — F103 实现
 *
 * 时序(按灯珠规格, 已留余量):
 *   0 码: 高 400ns(<0.47µs) 低 900ns   → ~1.3µs/bit
 *   1 码: 高 800ns(0.58~1µs) 低 400ns   → ~1.2µs/bit
 *   RESET: 低电平 ≥80µs (本驱动留 100µs)
 *
 * Strip0(255) — SPI:
 *   紧凑位打包: 每个 WS bit 用 3 个 SPI bit 表达
 *     '1' = 0b110 (高 888ns + 低 444ns)   '0' = 0b100 (高 444ns + 低 888ns)
 *   目标 SPI 时钟 ≈2.25MHz(444ns/bit): SPI1 用 APB2=72MHz/32, SPI2 用 APB1=36MHz/16
 *   (CubeMX 里 SPI2 当前默认 /2=18MHz 太快, 必须设 Prescaler=/16!)
 *   帧缓冲 = 9 字节/LED → 255 带仅 2295B
 *   (经典 1 byte/bit 需 6.1KB, F103C8 放不下, 故用紧凑打包)
 *
 * Strip1/Strip2(86/92) — GPIO 位带:
 *   翻转期间关中断; 86 带约 2.6ms / 92 带约 2.8ms 的关中断窗口(仅状态切换时)
 */

#include "ws2812.h"

#include <string.h>

#define LOG_LVL LOG_LVL_WARNING
#define LOG_TAG "ws2812"
#include "ulog_def.h"

#if defined(STM32F103xB)

#include "gpio.h"
#include "bsp_dwt.h"

/* ================================================================
 * 硬件配置: 按实际 PCB 修改这里 (引脚也需在 CubeMX .ioc 配好)
 * ================================================================ */
/* Strip0(255): 1 = SPI, 0 = GPIO 位带。
 * ⚠ 本芯片 0 码高阈值很紧(<0.47µs): SPI@2.25MHz 的 0 码高=1 SPI bit=444ns 太贴边,
 *   实测会偶发错色(时而绿/红/个别灯珠不对)。GPIO 位带可到 ~320ns 已验证稳定,
 *   故默认 0。此时 PB15 需在 CubeMX 配成 GPIO_Output(High speed), 并停用 SPI2。 */
#define DAFU_STRIP0_USE_SPI 0

#if DAFU_STRIP0_USE_SPI
#include "spi.h"
/* SPI 实例: SPI2(MOSI=PB15)/SPI1(PA7). CubeMX 需把时钟配到 ~2.25MHz(APB1/16) */
#define WS2812_SPI_HANDLE  hspi2
#define WS2812_SPI_USE_DMA 0 /* 1=DMA(DMA1_Channel5) 0=阻塞 */
#define WS2812_SPI_TIMEOUT 200
/* 帧缓冲 = 9 字节/LED (24bit*3 SPIbit/8); 另需 32B 全0 复位尾巴 */
#define WS2812_FRAME_BYTES(_led) ((_led) * 9u)
#endif

/* 各带引脚 (GPIO 位带时用) */
#define DAFU_STRIP0_PORT GPIOB
#define DAFU_STRIP0_PIN  GPIO_PIN_15
#define DAFU_STRIP1_PORT GPIOB
#define DAFU_STRIP1_PIN  GPIO_PIN_8
#define DAFU_STRIP2_PORT GPIOB
#define DAFU_STRIP2_PIN  GPIO_PIN_9
/* ================================================================ */

/* GPIO 位带时序拍数 (72MHz, 1 拍≈13.9ns)。
 * ⚠ 实际高电平 = 标称 + 置高/读DWT/拉低开销(可能 +8~15 拍), 0 码高真实值必须 <0.47µs。
 *   这里再压低 T0H 加大余量: 标称 14 拍≈195ns(真实约 320~380ns), 远离 0.47µs 判读线;
 *   T1H 收中段 50 拍≈695ns(真实~800ns, 0.58~1µs 内)。若仍偶发白/彩 → 查示波器/走线/共地。 */
#define WS_CYC_T0H 14 /* 0 码高 ~195ns */
#define WS_CYC_T0L 72 /* 0 码低 ~1000ns → 0 周期 ~1.4µs */
#define WS_CYC_T1H 50 /* 1 码高 ~695ns */
#define WS_CYC_T1L 30 /* 1 码低 ~420ns → 1 周期 ~1.3µs */
#define WS_RESET_S 0.0001f /* RESET ≥80µs */

/* 中间螺旋带十字几何: 共 N 颗, 索引0 = 最外圈 1 号(正右), 由外向里绕 DAFU_SPIRAL_TURNS 圈。
 * 若画出的十字偏转/稀疏, 调这两个宏(TURNS=总圈数, TOL=方向容差)即可。 */
#ifndef DAFU_SPIRAL_TURNS
#define DAFU_SPIRAL_TURNS 9
#endif
#ifndef DAFU_CROSS_TOL
#define DAFU_CROSS_TOL 1500 /* 单位: 0~90°*N 之内, 越大十字越粗(可能密成实心) */
#endif

static uint8_t s_grb0[DAFU_STRIP0_LED_NUM * 3];
static uint8_t s_grb1[DAFU_STRIP1_LED_NUM * 3];
static uint8_t s_grb2[DAFU_STRIP2_LED_NUM * 3];
#if DAFU_STRIP0_USE_SPI
static uint8_t s_spi_frame[WS2812_FRAME_BYTES(DAFU_STRIP0_LED_NUM)]; /* 255带 2295B */
static uint8_t s_spi_reset[32]; /* 复位尾巴: 全 0 → MOSI 全程低 ≥80µs */
#endif

static uint8_t *strip_grb(uint8_t strip)
{
    switch (strip)
    {
        case 0: return s_grb0;
        case 1: return s_grb1;
        default: return s_grb2;
    }
}

uint16_t Ws2812_LedNum(uint8_t strip)
{
    switch (strip)
    {
        case 0: return DAFU_STRIP0_LED_NUM;
        case 1: return DAFU_STRIP1_LED_NUM;
        default: return DAFU_STRIP2_LED_NUM;
    }
}

void Ws2812_Init(void)
{
    memset(s_grb0, 0, sizeof(s_grb0));
    memset(s_grb1, 0, sizeof(s_grb1));
    memset(s_grb2, 0, sizeof(s_grb2));
}

static void argb_to_grb(uint32_t argb, uint8_t *g, uint8_t *r, uint8_t *b)
{
    *r = (uint8_t)(argb >> 16);
    *g = (uint8_t)(argb >> 8);
    *b = (uint8_t)(argb);
}

void Ws2812_Fill(uint8_t strip, uint32_t argb)
{
    uint8_t  g, r, b;
    uint8_t *p  = strip_grb(strip);
    uint16_t n  = Ws2812_LedNum(strip);

    argb_to_grb(argb, &g, &r, &b);
    for (uint16_t i = 0; i < n; i++)
    {
        p[i * 3 + 0] = g; /* GRB 序: 先 G */
        p[i * 3 + 1] = r;
        p[i * 3 + 2] = b;
    }
}

void Ws2812_SetLed(uint8_t strip, uint16_t idx, uint32_t argb)
{
    uint8_t  g, r, b;
    uint8_t *p = strip_grb(strip);

    if (idx >= Ws2812_LedNum(strip))
    {
        return;
    }
    argb_to_grb(argb, &g, &r, &b);
    p[idx * 3 + 0] = g;
    p[idx * 3 + 1] = r;
    p[idx * 3 + 2] = b;
}

/* 螺旋十字: 落在 0/90/180/270° 上的灯点亮(整数运算, 无浮点)。
 * 判定: | (i*TURNS*360) mod (90*N) | 距 0 足够近 即视为落在该方向半径上。 */
void Ws2812_DrawCross(uint8_t strip, uint32_t argb)
{
    uint8_t  g, r, b;
    uint8_t *p  = strip_grb(strip);
    uint16_t n  = Ws2812_LedNum(strip);
    uint32_t q  = 90UL * n; /* 一个方向(90°)对应的量纲 */

    argb_to_grb(argb, &g, &r, &b);

    /* 先全灭 */
    memset(p, 0, (size_t)n * 3u);

    if (strip == 0) /* 仅螺旋带(中间255)有该几何 */
    {
        for (uint16_t i = 0; i < n; i++)
        {
            uint32_t a    = (uint32_t)i * DAFU_SPIRAL_TURNS * 360UL;
            uint32_t rem  = a % q;
            uint32_t dist = (rem < (q - rem)) ? rem : (q - rem);
            if (dist <= (uint32_t)DAFU_CROSS_TOL)
            {
                p[i * 3 + 0] = g;
                p[i * 3 + 1] = r;
                p[i * 3 + 2] = b;
            }
        }
        /* 靶心(最后一颗, 若存在)也点亮 */
        if (n > 0)
        {
            p[(n - 1) * 3 + 0] = g;
            p[(n - 1) * 3 + 1] = r;
            p[(n - 1) * 3 + 2] = b;
        }
    }
}

#if DAFU_STRIP0_USE_SPI
/* ---------- 紧凑 SPI 位打包: 每 WS bit 写 3 SPI bit ---------- */
static uint8_t *s_out;
static uint32_t s_acc;
static uint8_t  s_nb;

static void bit_put(uint8_t val, uint8_t n)
{
    s_acc = (s_acc << n) | (uint32_t)val;
    s_nb += n;
    while (s_nb >= 8u)
    {
        *s_out++ = (uint8_t)(s_acc >> (s_nb - 8u));
        s_nb -= 8u;
    }
}

static void build_spi_frame(uint8_t *dst, const uint8_t *grb, uint16_t nled)
{
    s_out = dst;
    s_acc = 0;
    s_nb  = 0;

    for (uint16_t i = 0; i < (uint16_t)(nled * 3u); i++)
    {
        uint8_t byte = grb[i];
        for (int8_t b = 7; b >= 0; b--)
        {
            if (byte & (1u << b))
            {
                bit_put(0b110, 3); /* 1 码: 高 888ns + 低 444ns */
            }
            else
            {
                bit_put(0b100, 3); /* 0 码: 高 444ns + 低 888ns */
            }
        }
    }
    if (s_nb > 0u)
    {
        *s_out++ = (uint8_t)(s_acc << (8u - s_nb));
        s_nb     = 0u;
    }
}

static void spi_commit_strip0(void)
{
    uint16_t len = WS2812_FRAME_BYTES(DAFU_STRIP0_LED_NUM);

    build_spi_frame(s_spi_frame, s_grb0, DAFU_STRIP0_LED_NUM);

#if WS2812_SPI_USE_DMA
    if (HAL_SPI_Transmit_DMA(&WS2812_SPI_HANDLE, s_spi_frame, len) == HAL_OK)
    {
        while (HAL_SPI_GetState(&WS2812_SPI_HANDLE) != HAL_SPI_STATE_READY)
        {
        }
    }
#else
    HAL_SPI_Transmit(&WS2812_SPI_HANDLE, s_spi_frame, len, WS2812_SPI_TIMEOUT);
#endif
    /* RESET ≥80µs: MOSI 空闲不一定停在低, 补发全 0 字节(全程低)再保持低 */
    HAL_SPI_Transmit(&WS2812_SPI_HANDLE, s_spi_reset, (uint16_t)sizeof(s_spi_reset), WS2812_SPI_TIMEOUT);
    BSP_DWT_Delay(WS_RESET_S);
}
#endif /* DAFU_STRIP0_USE_SPI */

/* ---------- GPIO 位带 ---------- */
static inline void dwt_delay_cycles(uint32_t cyc)
{
    uint32_t t0 = DWT->CYCCNT;
    while ((uint32_t)(DWT->CYCCNT - t0) < cyc)
    {
    }
}

static void bitbang_strip(const uint8_t *grb, uint16_t nled, GPIO_TypeDef *port, uint16_t pin)
{
    __disable_irq();

    for (uint16_t i = 0; i < (uint16_t)(nled * 3u); i++)
    {
        uint8_t byte = grb[i];
        for (int8_t b = 7; b >= 0; b--)
        {
            if (byte & (1u << b))
            {
                port->BSRR = pin;             /* 高 */
                dwt_delay_cycles(WS_CYC_T1H); /* ~800ns */
                port->BRR = pin;              /* 低 */
                dwt_delay_cycles(WS_CYC_T1L); /* ~400ns */
            }
            else
            {
                port->BSRR = pin;             /* 高 */
                dwt_delay_cycles(WS_CYC_T0H); /* ~400ns */
                port->BRR = pin;              /* 低 */
                dwt_delay_cycles(WS_CYC_T0L); /* ~900ns */
            }
        }
    }

    /* 帧尾保持低 ≥80µs 作 RESET(仍关中断, 保证时序不被切碎) */
    BSP_DWT_Delay(WS_RESET_S);
    __enable_irq();
}

void Ws2812_Commit(uint8_t strip)
{
    switch (strip)
    {
        case 0:
#if DAFU_STRIP0_USE_SPI
            spi_commit_strip0();
#else
            bitbang_strip(s_grb0, DAFU_STRIP0_LED_NUM, DAFU_STRIP0_PORT, DAFU_STRIP0_PIN);
#endif
            break;
        case 1:
            bitbang_strip(s_grb1, DAFU_STRIP1_LED_NUM, DAFU_STRIP1_PORT, DAFU_STRIP1_PIN);
            break;
        case 2:
            bitbang_strip(s_grb2, DAFU_STRIP2_LED_NUM, DAFU_STRIP2_PORT, DAFU_STRIP2_PIN);
            break;
        default:
            break;
    }
}

#else /* 非 F103: 占位, 保证在其它板上也能编译 */

void Ws2812_Init(void) {}

uint16_t Ws2812_LedNum(uint8_t strip)
{
    (void)strip;
    return 0;
}

void Ws2812_Fill(uint8_t strip, uint32_t argb)
{
    (void)strip;
    (void)argb;
}

void Ws2812_SetLed(uint8_t strip, uint16_t idx, uint32_t argb)
{
    (void)strip;
    (void)idx;
    (void)argb;
}

void Ws2812_Commit(uint8_t strip)
{
    (void)strip;
}

void Ws2812_DrawCross(uint8_t strip, uint32_t argb)
{
    (void)strip;
    (void)argb;
}

#endif /* STM32F103xB */
