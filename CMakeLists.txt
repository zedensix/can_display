cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include(pico_sdk_import.cmake)
project(can_display C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

file(GLOB APP_SOURCES src/app/*.c)
file(GLOB FIRMWARE_SOURCES src/firmware/*.c)
# Warnings are errors in this project's code only: the SDK's own sources
# are compiled into the same targets.
set_source_files_properties(${APP_SOURCES} ${FIRMWARE_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Werror")

function(add_firmware name)
    add_executable(${name} ${APP_SOURCES} ${FIRMWARE_SOURCES})
    target_include_directories(${name} PRIVATE src/app src/firmware)
    target_compile_definitions(${name} PRIVATE PICO_PANIC_FUNCTION=crash_panic CYW43_LWIP=0 ${ARGN})
    target_link_libraries(${name}
        pico_stdlib pico_flash
        hardware_adc hardware_dma hardware_flash hardware_pwm hardware_spi hardware_watchdog
        pico_cyw43_arch_poll pico_btstack_ble pico_btstack_cyw43)
    pico_enable_stdio_usb(${name} 1)
    pico_enable_stdio_uart(${name} 0)
    pico_add_extra_outputs(${name})
endfunction()

add_firmware(firmware)
add_firmware(firmware_sim SIM=1)
