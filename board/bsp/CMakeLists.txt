# bsp library
add_library(bsp OBJECT
    bsp_init.c
    DWT/bsp_dwt.c
)

target_include_directories(bsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/DWT
)

target_link_libraries(bsp PUBLIC stm32cubemx azrtos::threadx utils)
