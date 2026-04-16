cmake_minimum_required(VERSION 3.14)
project(boost_di_demos LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Boost.DI 只需要这一个头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 收集所有 demo 源文件
file(GLOB DEMO_SOURCES "demos/*.cpp")

foreach(DEMO_SOURCE ${DEMO_SOURCES})
    get_filename_component(DEMO_NAME ${DEMO_SOURCE} NAME_WE)
    add_executable(${DEMO_NAME} ${DEMO_SOURCE})
    target_compile_options(${DEMO_NAME} PRIVATE -Wall -Wextra -pedantic)
endforeach()

# 便捷目标: 构建所有 demo
add_custom_target(all_demos DEPENDS ${DEMO_NAMES})
