cmake_minimum_required(VERSION 3.13)
project(can_display_tests C)

set(CMAKE_C_STANDARD 11)
file(GLOB_RECURSE APP_SOURCES ${CMAKE_CURRENT_LIST_DIR}/../src/app/*.c)
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.c)
add_executable(tests ${TEST_SOURCES} ${APP_SOURCES})
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src/app)
# Undefined behaviour fails the run, not a line in its output.
target_compile_options(tests PRIVATE -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=undefined -g)
target_link_options(tests PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=undefined)
target_link_libraries(tests m)
