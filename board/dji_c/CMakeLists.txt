cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME base)

# ThreadX configuration for Cortex-M4 / GCC
set(THREADX_ARCH cortex_m4)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Add ThreadX RTOS
add_subdirectory(../../threadx threadx)

# Add shared project libraries
add_subdirectory(../../utils ${CMAKE_CURRENT_BINARY_DIR}/utils)
add_subdirectory(../../board/bsp ${CMAKE_CURRENT_BINARY_DIR}/bsp)
add_subdirectory(../../robot ${CMAKE_CURRENT_BINARY_DIR}/robot)
# CMSIS-DSP settings for Cortex-M4
set(LOOPUNROLL ON CACHE BOOL "Loop unrolling for max performance" FORCE)
set(DISABLEFLOAT16 ON CACHE BOOL "Disable float16 kernels (not needed for M4)" FORCE)

add_subdirectory(../../CMSIS-DSP ${CMAKE_CURRENT_BINARY_DIR}/cmsisdsp)

# Apply max performance compiler flags to CMSIS-DSP
target_compile_options(CMSISDSP PRIVATE -O3 -ffast-math -fno-math-errno)
# Provide CMSIS-Core headers (cmsis_compiler.h) to CMSIS-DSP
target_include_directories(CMSISDSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    azrtos::threadx
    utils
    bsp
    robot
    CMSISDSP

    # Add user defined libraries
)
