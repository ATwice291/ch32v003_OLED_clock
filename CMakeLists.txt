cmake_minimum_required(VERSION 3.13)
project(CH32V003_OLED_Clock C CXX ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(RISCV_GCC_PREFIX "c:/WCH/xpack-riscv-none-elf-gcc-14.2.0-3/bin/riscv-none-elf")

set(CMAKE_C_COMPILER ${RISCV_GCC_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${RISCV_GCC_PREFIX}-g++)
set(CMAKE_ASM_COMPILER ${RISCV_GCC_PREFIX}-gcc)
set(CMAKE_OBJCOPY ${RISCV_GCC_PREFIX}-objcopy)
set(CMAKE_SIZE ${RISCV_GCC_PREFIX}-size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker.ld)

# Указать флаги компилятора
set(COMMON_FLAGS "-march=rv32ec_zicsr -mabi=ilp32e -ffunction-sections -fdata-sections")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(COMMON_FLAGS "${COMMON_FLAGS} -O0 -g3")
    set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build/Debug")
endif()
if(CMAKE_BUILD_TYPE MATCHES Release)
    set(COMMON_FLAGS "${COMMON_FLAGS} -Os")
    set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build/Release")
endif()

set(CMAKE_C_FLAGS "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "${COMMON_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "-T ${LINKER_SCRIPT} -nostartfiles -Wl,--gc-sections")

include_directories(inc)
include_directories(Periph)
add_executable(${PROJECT_NAME}.elf
    src/main.cpp
    src/startup_ch32v00x.S
    inc/core_riscv.c
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/firmware.hex
    # COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/firmware.bin
    COMMENT "Generating HEX and BIN files"
    COMMAND ${CMAKE_SIZE} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
    COMMENT "Firmware size:"
)