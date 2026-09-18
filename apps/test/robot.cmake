# test 兵种 — 新模块接入测试骨架 / 单板整机调试
# 用途: 新模块开发时在此单独验证 (见 robot_control.c 顶部工作流说明)
# 注意: single_board/ 下的 gimbal / chassis / shoot / robot 实现依赖下列模块,
#       少一个就会因缺头文件与实现而编不过

include(${CMAKE_CURRENT_LIST_DIR}/../../modules/module_config.cmake)

# 模块开关(按板型覆盖默认值)
set(MODULES_SINGLE   OFFLINE REMOTE BMI088 INS MOTOR)

# OFFLINE 参数
set(OFFLINE_BEEP_ENABLE     0)    # 测试兵种不蜂鸣
