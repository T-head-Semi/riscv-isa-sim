cmake_minimum_required(VERSION 3.0.0)
project(fesvr VERSION 1.1.0)

set (CMAKE_CXX_STANDARD 17)

aux_source_directory(${PROJECT_SOURCE_DIR} FESVR_SRCS)

list(REMOVE_ITEM FESVR_SRCS
${PROJECT_SOURCE_DIR}/elf2hex.cc
)

# target
add_library(fesvr_org INTERFACE)
target_sources(fesvr_org INTERFACE ${FESVR_SRCS})
target_link_libraries(fesvr_org INTERFACE ${CMAKE_THREAD_LIBS_INIT})

message("fesvr=${FESVR_SRCS}")

# pthread
find_package(Threads REQUIRED)

# target
add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${FESVR_SRCS})
target_compile_definitions(${PROJECT_NAME} INTERFACE ENABLE_FORCE_RISCV)
target_link_libraries(${PROJECT_NAME} INTERFACE ${CMAKE_THREAD_LIBS_INIT})
