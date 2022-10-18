cmake_minimum_required(VERSION 3.0.0)
project(fdt VERSION 1.1.0)

set (CMAKE_CXX_STANDARD 17)

aux_source_directory(${PROJECT_SOURCE_DIR} FDT_SRCS)

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${FDT_SRCS})