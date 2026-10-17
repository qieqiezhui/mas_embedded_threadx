# test 兵种 — 新模块接入与本地控制验证
# 新模块测试时，在 MODULES_SINGLE 中临时加入对应模块。

include(${CMAKE_CURRENT_LIST_DIR}/../../modules/module_config.cmake)

# 模块开关（按板型覆盖默认值）
set(MODULES_SINGLE   BMI088 OFFLINE MOTOR INS CHASSIS_TYPE REMOTE)


# OFFLINE 参数
set(OFFLINE_BEEP_ENABLE     0)    # 测试兵种不蜂鸣

# REMOTE 参数
set(REMOTE_UART             huart3) # 串口
set(REMOTE_VT_UART          huart6) # 图传串口
set(REMOTE_SOURCE           1)      # 遥控器选择: 0=none, 1=sbus, 2=dt7
set(REMOTE_VT_SOURCE        0)      # 图传选择:   0=none, 1=vt02, 2=vt03

# REFEREE 参数
set(REFEREE_UART            huart6) # 串口选择
