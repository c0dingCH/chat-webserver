cmake_minimum_required(VERSION 3.10)
project(Chat)

set(CMAKE_CXX_COMPILER "g++")

# 公共编译选项
set(CMAKE_CXX_FLAGS "-g -Wall -Werror -std=c++17")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")

find_package(Threads REQUIRED)
set(CMAKE_MODULE_PATH /usr/local/share/cmake ${CMAKE_MODULE_PATH})
find_package(redis++ REQUIRED PATHS /usr/local/share/cmake/redis++ NO_DEFAULT_PATH)

# 包含目录
include_directories(
    base
    tcp
    http
    https
    timer
    log
    business
    database
    /usr/local/include/sw
)


# 自动收集所有子目录里的源文件
file(GLOB SRC_BASE base/*.cpp)
file(GLOB SRC_TCP tcp/*.cpp)
file(GLOB SRC_HTTP http/*.cpp)
file(GLOB SRC_TIMER timer/*.cpp)
file(GLOB SRC_LOG log/*.cpp)
file(GLOB SRC_BUSINESS business/*.cpp)
file(GLOB SRC_DATABASE database/*.cpp)
set(SRC_ALL ${SRC_BASE} ${SRC_TCP} ${SRC_HTTP} ${SRC_TIMER} ${SRC_LOG} ${SRC_BUSINESS} ${SRC_DATABASE})

#添加动态库
add_library(chat SHARED  ${SRC_ALL})
target_link_libraries(chat ${CMAKE_THREAD_LIBS_INIT} mysqlclient nghttp2 redis++::redis++)



# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/test/)

# 自动收集 test 目录下所有 cpp 文件
file(GLOB TESTS test/*.cpp)

# 遍历每个测试文件，生成对应的可执行文件
foreach(test_file ${TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)  # 去掉路径和扩展名
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} chat)    
endforeach()

