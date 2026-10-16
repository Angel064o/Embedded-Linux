cmake_minimum_required(VERSION 3.16)

project(P6 C)

add_executable(P6
    src/main.c
    src/PWM.c
)

target_include_directories(P6
    PRIVATE
    include
)

target_compile_options(P6
    PRIVATE
    -Wall -Wextra
)