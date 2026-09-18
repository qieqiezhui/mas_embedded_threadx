cmake_minimum_required(VERSION 3.22)
project(base)

# MCU flags / 链接脚本见本目录 cmake/gcc-arm-none-eabi.cmake

# 本板差异
# ThreadX configuration for Cortex-M4 / GCC
set(THREADX_ARCH cortex_m4)

# 当前 .ioc 只配置了 PC13 GPIO，BSP 外设按需开启：
# 开启某项前先在 CubeMX 补齐对应外设源（tim.c / spi.c / usart.c / can.c / i2c.c / usb_otg.c）
set(BSP_USB_ENABLE  OFF) # 未配 USB OTG，DWC2 端口自动联动关闭
set(BSP_UART_ENABLE OFF)
set(BSP_SPI_ENABLE  OFF)
set(BSP_I2C_ENABLE  OFF)
set(BSP_PWM_ENABLE  OFF)
set(BSP_CAN_ENABLE  OFF)
set(BSP_LED_ENABLE  OFF) # RGB 灯依赖 TIM5 CH1/2/3
set(BSP_BEEP_ENABLE OFF) # 蜂鸣器依赖 TIM4 CH3

# 公共构建逻辑
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/board_common.cmake)

# 板级专属 sources / includes / defines / libraries 在此追加
# target_sources(${CMAKE_PROJECT_NAME} PRIVATE ...)
