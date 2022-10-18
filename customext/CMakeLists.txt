cmake_minimum_required(VERSION 3.0.0)
project(customext VERSION 1.1.0)

set (CMAKE_CXX_STANDARD 17)

set(CUSTOMEXT_SRCS 
    ./dummy_rocc.cc
    ./cflush.cc)

add_library(${PROJECT_NAME} INTERFACE ${CUSTOMEXT_SRCS})
target_include_directories(${PROJECT_NAME} INTERFACE ./)