# Copyright 2019-2021 T-Head Semiconductor Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.0.0)
project(handcar_cosim VERSION 1.1.0)

# # Fix behavior of CMAKE_CXX_STANDARD when targeting macOS.
# if (POLICY CMP0025)
#   cmake_policy(SET CMP0025 NEW)
# endif ()

include(CTest)
enable_testing()

# set c++ standard
set (CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

# cmake install path
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  message(STATUS "Setting default CMAKE_INSTALL_PREFIX path to ${PROJECT_SOURCE_DIR}")
  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE STRING "The path to use for make install" FORCE)
endif()

# depends
find_package(Boost 1.53 COMPONENTS Asio regex)
if (NOT Boost_FOUND)
  message("Not found Boost")
endif()

#--------------------------------------
# config file
set(DEFAULT_ISA "RV64IMAFDC" CACHE STRING "Default value for --isa switch" FORCE)
set(DEFAULT_PRIV "MSU" CACHE STRING "Default value for --priv switch" FORCE)
set(DEFAULT_VARCH "vlen:128,elen:64" CACHE STRING "Default value for --varch switch" FORCE)
set(TARGET_ARCH "riscv64-unknown-elf" CACHE STRING "Default value for --target switch" FORCE)
set(PREFIX " " CACHE STRING "Default value for --prefix" FORCE)

option(ENABLE_COMMITLOG "Enable commit log generation" OFF)
if (ENABLE_COMMITLOG)
    set(RISCV_ENABLE_COMMITLOG TRUE)
endif()
option(ENABLE_HISTOGRAM "Enable PC histogram generation" OFF)
if (ENABLE_HISTOGRAM)
    set(RISCV_ENABLE_HISTOGRAM TRUE)
endif()
option(ENABLE_DIRTY "Enable hardware management of PTE accessed and dirty bits" OFF)
if (ENABLE_DIRTY)
    set(RISCV_ENABLE_DIRTY TRUE)
endif()
option( "Enable hardware support for misaligned loads and stores" OFF)
if (ENABLE_MISALIGNED)
    set(RISCV_ENABLE_MISALIGNED TRUE)
endif()
option(ENABLE_DUAL_ENDIAN "Enable support for running target in either endianness" OFF)
if (ENABLE_MISALIGNED)
    set(RISCV_ENABLE_DUAL_ENDIAN TRUE)
endif()
if (DEFINED CMAKE_DL_LIBS)
    set(HAVE_DLOPEN TRUE CACHE BOOL "Dynamic library loading is supported" FORCE)
endif()
find_package(Threads REQUIRED)
if (Threads_FOUND)
    set(HAVE_LIBPTHREAD TRUE CACHE BOOL "libpthread is required" FORCE)
endif()

set(SOFTFLOAT_ENABLED TRUE CACHE BOOL "softflat is enabled" FORCE)
set(SPIKE_DASM_ENABLED TRUE CACHE BOOL "spike dasm is enabled" FORCE)
set(SPIKE_MAIN_ENABLED TRUE CACHE BOOL "spike main is enabled" FORCE)
set(RISCV_ENABLED TRUE CACHE BOOL "riscv is enabled" FORCE)
set(DTC "dtc" CACHE STRING "Default dtc" FORCE)

# header check
macro(CHECK_INCLUDE_FILE_ERROR INCLUDE_FILE HAVE_FILE)
    CHECK_INCLUDE_FILE(${INCLUDE_FILE} ${HAVE_FILE})
    IF(NOT ${HAVE_FILE})
        UNSET(HAVE_FILE CACHE)
        message( FATAL_ERROR "${INCLUDE_FILE} is not found" )
    ENDIF()
endmacro()
CHECK_INCLUDE_FILE_ERROR("strings.h" HAVE_STRINGS_H)
CHECK_INCLUDE_FILE_ERROR("string.h" HAVE_STRING_H)
CHECK_INCLUDE_FILE_ERROR("memory.h" HAVE_MEMORY_H)
CHECK_INCLUDE_FILE_ERROR("inttypes.h" HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE_ERROR("stdint.h" HAVE_STDINT_H)
CHECK_INCLUDE_FILE_ERROR("stdlib.h" HAVE_STDLIB_H)
CHECK_INCLUDE_FILE_ERROR("unistd.h" HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_ERROR("sys/stat.h" HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_ERROR("sys/types.h" HAVE_SYS_TYPES_H)
#TODO: fix following options
include(CheckStructHasMember)
CHECK_STRUCT_HAS_MEMBER("struct statx" st_ino sys/stat.h HAVE_STATX)
CHECK_STRUCT_HAS_MEMBER("struct statx" st_mnt_id sys/stat.h HAVE_STATX_MNT_ID)
#TODO: fix following options
set(STDC_HEADERS TRUE CACHE BOOL "stdc headers exists" FORCE)
# set(HAVE_STATX TRUE CACHE BOOL "Define to 1 if struct statx exists." FORCE)
# set(HAVE_STATX_MNT_ID TRUE CACHE BOOL "Define to 1 if struct statx has stx_mnt_id." FORCE)

configure_file(${CMAKE_SOURCE_DIR}/config.h.in.cmake ${CMAKE_BINARY_DIR}/config.h)

#------------------------
# other targets
add_subdirectory(./disasm)
add_subdirectory(./fdt)
add_subdirectory(./fesvr)
add_subdirectory(./force_mod)
#add_subdirectory(./customext)
add_subdirectory(./riscv)
add_subdirectory(./softfloat)


add_library(${PROJECT_NAME} MODULE ./spike_main/handcar_cosim_wrapper.cc ./spike_main/spike.cc)
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE force_mod riscv softfloat disasm fesvr fdt  "-undefined dynamic_lookup")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE force_mod riscv softfloat disasm fesvr fdt)
endif(APPLE)
target_include_directories(${PROJECT_NAME} PRIVATE
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/riscv
                    ${CMAKE_SOURCE_DIR}/riscv/insns
                    ${CMAKE_SOURCE_DIR}/fesvr
                    ${CMAKE_SOURCE_DIR}/softfloat
                    ${CMAKE_SOURCE_DIR}/force_mod
                    ${CMAKE_SOURCE_DIR}/customext
                    ${CMAKE_SOURCE_DIR}/disasm
                    ${CMAKE_SOURCE_DIR}/fdt
                    ${CMAKE_SOURCE_DIR}/spike_main
                    ${CMAKE_BINARY_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_FORCE_RISCV)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
# set_target_properties(${PROJECT_NAME} PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED YES
#     CXX_EXTENSIONS NO
# )

add_executable(spike ./spike_main/spike.cc)
target_link_libraries(spike riscv_org softfloat disasm_org fesvr_org fdt dl)
target_include_directories(spike PRIVATE
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/riscv
                    ${CMAKE_SOURCE_DIR}/riscv/insns
                    ${CMAKE_SOURCE_DIR}/fesvr
                    ${CMAKE_SOURCE_DIR}/softfloat
                    ${CMAKE_SOURCE_DIR}/force_mod
                    ${CMAKE_SOURCE_DIR}/customext
                    ${CMAKE_SOURCE_DIR}/disasm
                    ${CMAKE_SOURCE_DIR}/fdt
                    ${CMAKE_SOURCE_DIR}/spike_main
                    ${CMAKE_BINARY_DIR})
# set_target_properties(spike PROPERTIES CXX_STANDARD 17)

aux_source_directory(./spike_dasm DISASM_SRC)
add_executable(spike-dasm ${DISASM_SRC} ./riscv/extensions.cc ./disasm/isa_parser.cc)
target_link_libraries(spike-dasm softfloat disasm_org dl)
target_include_directories(spike-dasm PRIVATE
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/riscv
                    ${CMAKE_SOURCE_DIR}/riscv/insns
                    ${CMAKE_SOURCE_DIR}/fesvr
                    ${CMAKE_SOURCE_DIR}/softfloat
                    ${CMAKE_SOURCE_DIR}/force_mod
                    ${CMAKE_SOURCE_DIR}/customext
                    ${CMAKE_SOURCE_DIR}/disasm
                    ${CMAKE_SOURCE_DIR}/fdt
                    ${CMAKE_SOURCE_DIR}/spike_main
                    ${CMAKE_BINARY_DIR})
# set_target_properties(spike-dasm PROPERTIES CXX_STANDARD 17)

add_executable(spike-log-parser ./spike_main/spike-log-parser.cc ./riscv/extensions.cc ./disasm/isa_parser.cc)
target_include_directories(spike-log-parser PRIVATE
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/riscv
                    ${CMAKE_SOURCE_DIR}/riscv/insns
                    ${CMAKE_SOURCE_DIR}/fesvr
                    ${CMAKE_SOURCE_DIR}/softfloat
                    ${CMAKE_SOURCE_DIR}/force_mod
                    ${CMAKE_SOURCE_DIR}/customext
                    ${CMAKE_SOURCE_DIR}/disasm
                    ${CMAKE_SOURCE_DIR}/fdt
                    ${CMAKE_SOURCE_DIR}/spike_main
                    ${CMAKE_BINARY_DIR})
target_link_libraries(spike-log-parser riscv_org softfloat disasm_org fesvr_org fdt dl)

add_executable(xspike ./spike_main/xspike.cc)
target_include_directories(xspike PRIVATE
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/riscv
                    ${CMAKE_SOURCE_DIR}/riscv/insns
                    ${CMAKE_SOURCE_DIR}/fesvr
                    ${CMAKE_SOURCE_DIR}/softfloat
                    ${CMAKE_SOURCE_DIR}/force_mod
                    ${CMAKE_SOURCE_DIR}/customext
                    ${CMAKE_SOURCE_DIR}/disasm
                    ${CMAKE_SOURCE_DIR}/fdt
                    ${CMAKE_SOURCE_DIR}/spike_main
                    ${CMAKE_BINARY_DIR})
target_link_libraries(xspike dl)

add_executable(termios-xspike ./spike_main/xspike.cc)
target_include_directories(termios-xspike PRIVATE
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/riscv
                    ${CMAKE_SOURCE_DIR}/riscv/insns
                    ${CMAKE_SOURCE_DIR}/fesvr
                    ${CMAKE_SOURCE_DIR}/softfloat
                    ${CMAKE_SOURCE_DIR}/force_mod
                    ${CMAKE_SOURCE_DIR}/customext
                    ${CMAKE_SOURCE_DIR}/disasm
                    ${CMAKE_SOURCE_DIR}/fdt
                    ${CMAKE_SOURCE_DIR}/spike_main
                    ${CMAKE_BINARY_DIR})
target_link_libraries(termios-xspike dl)

add_library(handcar_dasm MODULE ./riscv/extensions.cc ./spike_dasm/handcar_dasm_wrapper.cc)
target_link_libraries(handcar_dasm softfloat disasm dl)
set_target_properties(handcar_dasm PROPERTIES PREFIX "")
target_include_directories(handcar_dasm PRIVATE
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/riscv
                    ${CMAKE_SOURCE_DIR}/riscv/insns
                    ${CMAKE_SOURCE_DIR}/fesvr
                    ${CMAKE_SOURCE_DIR}/softfloat
                    ${CMAKE_SOURCE_DIR}/force_mod
                    ${CMAKE_SOURCE_DIR}/customext
                    ${CMAKE_SOURCE_DIR}/disasm
                    ${CMAKE_SOURCE_DIR}/fdt
                    ${CMAKE_SOURCE_DIR}/spike_main
                    ${CMAKE_BINARY_DIR})
# Set the public header property to the one with the actual API.
set_target_properties(handcar_dasm PROPERTIES PUBLIC_HEADER ./spike_dasm/handcar_dasm_wrapper.h)

#--------------------------------------
# custom cmd: 
# . insns/*.h->insnc/*.cc
# . generate insn_list.h
include(CheckTypeSize)
check_type_size("__int128_t" SIZEOF_INT128)
if(${SIZEOF_INT128})
    set(HAVE_INT128 TRUE CACHE BOOL "have int128" FORCE)
else()
    message(FATAL_ERROR "__int128_t is not supported!!!")
endif()

# get all ins
file(GLOB INSN_SRCS "riscv/insns/*.h")
list(FILTER INSN_SRCS EXCLUDE REGEX "aes_common.h")
list(FILTER INSN_SRCS EXCLUDE REGEX "clmulhw.h")
list(FILTER INSN_SRCS EXCLUDE REGEX "clmulrw.h")
list(FILTER INSN_SRCS EXCLUDE REGEX "clmulw.h")
list(FILTER INSN_SRCS EXCLUDE REGEX "sm4_common.h")
list(FILTER INSN_SRCS EXCLUDE REGEX "vmvnfr_v.h")
list(FILTER INSN_SRCS EXCLUDE REGEX "clo8.h")
list(FILTER INSN_SRCS EXCLUDE REGEX "clo16.h")
list(FILTER INSN_SRCS EXCLUDE REGEX "clo32.h")

# insn_list.h
foreach(insn_tmp IN LISTS INSN_SRCS)
    get_filename_component(insn_name_tmp ${insn_tmp} NAME_WE)
    string(APPEND ins_macro "DEFINE_INSN(${insn_name_tmp})\n")
endforeach()
file(WRITE "${CMAKE_BINARY_DIR}/riscv/insn_list.h" ${ins_macro})
# {insn}.cc
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/riscv/insns)
foreach(insn_tmp IN LISTS INSN_SRCS)
    get_filename_component(insn_name_tmp ${insn_tmp} NAME_WE)
    set(insn_cc ${CMAKE_BINARY_DIR}/riscv/insns/${insn_name_tmp}.cc)
    add_custom_command(
        OUTPUT ${insn_cc}
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/gen_src.sh ${insn_name_tmp} ${CMAKE_BINARY_DIR}/riscv/insns
        DEPENDS ${CMAKE_SOURCE_DIR}/riscv/insns/${insn_name_tmp}.h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/riscv
        VERBATIM
    )

    target_sources(${PROJECT_NAME} PRIVATE ${insn_cc})
    target_sources(spike PRIVATE ${insn_cc})
    target_sources(spike-log-parser PRIVATE ${insn_cc})
endforeach()

#--------------------------------------
# install
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

install(TARGETS ${PROJECT_NAME} DESTINATION ../force-riscv/utils/handcar)
