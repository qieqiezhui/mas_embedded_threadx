# diansai 兵种 — 电赛专用应用
# 当前未接入任何 modules；需要时把模块加入 MODULES_SINGLE，
# 并在 board/*/CMakeLists.txt 打开对应的 BSP 外设 + 在 CubeMX 补齐外设源。

include(${CMAKE_CURRENT_SOURCE_DIR}/../../modules/module_config.cmake)

# 模块开关(按板型覆盖默认值)
set(MODULES_SINGLE)
