# dafu: 纯 LED + 按键 轻量单板应用
# 先加载默认模块配置，再覆盖差异

include(${CMAKE_CURRENT_LIST_DIR}/../../modules/module_config.cmake)

# 模块开关: 轻量应用不需要 IMU/遥控/裁判/电机等
# 仅保留 OFFLINE 看门狗/离线检测即可 (LED/按键在 bsp/app 层)
set(MODULES_SINGLE )
set(MODULES_GIMBAL  )
set(MODULES_CHASSIS )

# OFFLINE 默认参数
set(OFFLINE_WATCHDOG_ENABLE 0)    # 开启看门狗
set(OFFLINE_BEEP_ENABLE     0)    # 关闭蜂鸣器
set(OFFLINE_TASK_STACK_SIZE 512)  # 任务栈大小
set(OFFLINE_TASK_PRIORITY   6)    # 任务优先级
