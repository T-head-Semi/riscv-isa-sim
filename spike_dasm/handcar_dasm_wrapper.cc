// See LICENSE for license details.

// This little program finds occurrences of strings like
//  DASM(ffabc013)
// in its input, then replaces them with the disassembly
// enclosed hexadecimal number, interpreted as a RISC-V
// instruction.

#include "disasm.h"
#include "extension.h"
#include <iostream>
#include <string>
#include <cstdint>
#include <cstdlib>
#include "handcar_dasm_wrapper.h"

int get_disassembly_alone(const char* isa, const char* extension, unsigned opcode, char* pDisassembly)
{
  const char* final_isa = DEFAULT_ISA;
  if (isa) {
    final_isa = isa;
  }

  isa_parser_t isa_parser(isa, DEFAULT_PRIV);
  disassembler_t* disassembler = new disassembler_t(&isa_parser);
  if (extension) {
    std::function<extension_t*()> ext = find_extension(extension);
    for (auto disasm_insn : ext()->get_disasms()) {
      disassembler->add_insn(disasm_insn);
    }
  }

  std::string disassembly_temp = disassembler->disassemble(opcode);

  if (pDisassembly == nullptr) {
    return 2;
  }

  disassembly_temp.copy(pDisassembly, disassembly_temp.size(), 0);
  pDisassembly[disassembly_temp.size()]='\0';
  return 0;
}
