cmake_minimum_required(VERSION 3.13)
project(can_display_sim C)

set(CMAKE_C_STANDARD 11)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

file(GLOB APP_SOURCES ${CMAKE_CURRENT_LIST_DIR}/../src/app/*.c)
add_executable(simulator main.c ${APP_SOURCES})
target_include_directories(simulator PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src/app)
target_compile_options(simulator PRIVATE -Wall -Wextra -Werror)
target_link_libraries(simulator PkgConfig::SDL2 m)
