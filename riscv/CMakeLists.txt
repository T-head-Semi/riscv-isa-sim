cmake_minimum_required(VERSION 3.8.0)
project(riscv VERSION 1.1.0)

set (CMAKE_CXX_STANDARD 17)

aux_source_directory(${PROJECT_SOURCE_DIR} RISCV_SRCS)

list(REMOVE_ITEM RISCV_SRCS
${PROJECT_SOURCE_DIR}/insn_template.cc
${PROJECT_SOURCE_DIR}/check-opcode-overlap.t.cc
)

add_library(riscv_org INTERFACE)
target_sources(riscv_org INTERFACE ${RISCV_SRCS})
target_include_directories(riscv_org INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

message("RISCV=${RISCV_SRCS}")
#--------------------------------------
add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${RISCV_SRCS})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${PROJECT_NAME} INTERFACE ENABLE_FORCE_RISCV)