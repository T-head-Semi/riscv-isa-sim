//
// Copyright (C) [2020] Futurewei Technologies, Inc.
//
// FORCE-RISCV is licensed under the Apache License, Version 2.0 (the License);
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
// FIT FOR A PARTICULAR PURPOSE.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef __WRAPPER_H__
#define __WRAPPER_H__
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
#include <string>
extern "C" {
#endif

// get_disassembly_alone function: for the given opcode, populates the information to disassemby string buffers.
//
// warning:
//     the caller has responsibility over allocating and managing the memory for the strings.
//
// inputs:
//     char* isa, isa string for supported standard ISA
//     char* extension, isa string for supported custom ISA
//     unsigned opcode,  opcode for the searched instruction
//     char** disassembly, pointer to an allocated string buffer
//
//  outputs:
//      disassembly, copys the disassembly text to disassembly
//
//  returns:
//      0 success,
//      2 could not complete because no allocated disassembly string buffer is provided
//
int get_disassembly_alone(const char* isa, const char* extension, unsigned opcode, char* pDisassembly);

#ifdef __cplusplus
};
#endif

#endif

