cmake_minimum_required(VERSION 3.0.0)
project(softfloat VERSION 1.1.0)

set (CMAKE_CXX_STANDARD 17)

aux_source_directory(${PROJECT_SOURCE_DIR} SOFTFLOAT_SRCS)

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${SOFTFLOAT_SRCS})
target_include_directories(${PROJECT_NAME} INTERFACE ./)