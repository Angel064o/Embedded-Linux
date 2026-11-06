cmake_minimum_required(VERSION 3.18)
project(brazo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_library(PIGPIO_LIB pigpio REQUIRED)
find_package(Threads REQUIRED)

add_executable(brazo src/main.c src/PWM.c)
target_include_directories(brazo PRIVATE include)
target_compile_options(brazo PRIVATE -Wall -Wextra)
target_link_libraries(brazo PRIVATE ${PIGPIO_LIB} rt Threads::Threads)