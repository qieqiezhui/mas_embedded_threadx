# Utils library
add_library(utils OBJECT
    utils_init.c
    ulog/SEGGER_RTT.c
    ulog/ulog.c
    ulog/printf.c
    list/list.c
    kfifo/kfifo.c
)

target_compile_options(utils PRIVATE -O2)

target_include_directories(utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ulog
    ${CMAKE_CURRENT_SOURCE_DIR}/list
    ${CMAKE_CURRENT_SOURCE_DIR}/kfifo
)

target_link_libraries(utils PUBLIC stm32cubemx azrtos::threadx)
