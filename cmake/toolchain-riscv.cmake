set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

set(RISCV_GCC_PREFIX "c:/WCH/xpack-riscv-none-elf-gcc-14.2.0-3/bin/riscv-none-elf")

set(CMAKE_C_COMPILER ${RISCV_GCC_PREFIX}-gcc.exe)
set(CMAKE_CXX_COMPILER ${RISCV_GCC_PREFIX}-g++.exe)
set(CMAKE_ASM_COMPILER ${RISCV_GCC_PREFIX}-gcc.exe)
set(CMAKE_OBJCOPY ${RISCV_GCC_PREFIX}-objcopy.exe)
set(CMAKE_SIZE ${RISCV_GCC_PREFIX}-size.exe)


set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)