cmake_minimum_required(VERSION 3.0.0)
project(disasm VERSION 1.1.0)

set (CMAKE_CXX_STANDARD 17)

aux_source_directory(${PROJECT_SOURCE_DIR} DISASM_SRCS)

# target
add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${DISASM_SRCS})
target_compile_definitions(${PROJECT_NAME} INTERFACE ENABLE_FORCE_RISCV)

add_library(disasm_org INTERFACE)
target_sources(disasm_org INTERFACE ${DISASM_SRCS})