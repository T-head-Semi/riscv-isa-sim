// See LICENSE for license details.
#include "simlib.h"
#include "mmulib.h"
#include "proclib.h"
#include "byteorder.h"
#include <fstream>
#include <map>
#include <sstream>
#include <climits>
#include <cstdlib>
#include <cassert>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "disasm.h"
#include "dts.h"

// for elf loading
#include "elf.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

// for address translation
#include "memtracer.h"

#ifdef ENABLE_FORCE_RISCV
simlib_t::simlib_t(const cfg_t *cfg, bool halted,
      std::vector<std::pair<reg_t, abstract_mem_t*>> mems,
      std::vector<const device_factory_t*> plugin_device_factories,
      const std::vector<std::string>& args,
      const debug_module_config_t &dm_config, const char *log_path,
      bool dtb_enabled, const char *dtb_file,
      bool socket_enabled,
      FILE *cmd_file)
  : sim_t(cfg, halted, mems, plugin_device_factories, args, dm_config, log_path, dtb_enabled, dtb_file,
          socket_enabled, cmd_file),
    start_pc(cfg->start_pc.value_or(reg_t(-1)))
{
}

simlib_t::~simlib_t()
{
}

int simlib_t::step_simulator(int target_id, int num_steps, int stx_failed)
{
  processor_t* proc_ptr = get_core_by_id(target_id);
  if (proc_ptr == nullptr)
    return 1;
  proc_ptr->step(num_steps);
  const char pc_name[] = "PC\0";
  const uint64_t pc_mask = 0xFFFFFFFFFFFFFFFF;
  uint64_t pc_value = 0x0;
  const char pc_update_access_type[] = "write\0";
  get_pc_api(target_id, reinterpret_cast<uint8_t*>(&pc_value), pc_name, sizeof(uint64_t));
  update_generator_register(target_id, pc_name, pc_value, pc_mask, pc_update_access_type);

  return 0;
}

int simlib_t::get_disassembly(int target_id, const uint64_t* pc, char** opcode, char** disassembly)
{
  std::string opcode_temp;
  std::string disassembly_temp;
  uint64_t opcode_num = 0ull;

  if (procs.size() > 0)
  {
    processor_t* proc_ptr = get_core_by_id(target_id);
    if (proc_ptr == nullptr)
      return 1;
    try {
      const disassembler_t* disassembler = proc_ptr->get_disassembler(); // should be OK to get processor's disassembler, using it should be idempotent

      // currently this fails a check in the Spike code if the pc isn't found so the code does not have the opportunity to return 1.
      insn_fetch_t fetched = proc_ptr->get_mmu()->load_insn(*pc);
      opcode_num = fetched.insn.bits() & ((1ull << (8 * insn_length(fetched.insn.bits()))) - 1); //This is the conversion the processor_t::disasm(...) uses to format the opcode for output.
      disassembly_temp = disassembler->disassemble(fetched.insn);
      // format the string interpretation of the opcode as a hex number.
      std::stringstream stream;
      stream << "0x" << std::hex << opcode_num;
      opcode_temp = stream.str();
    } catch(...) {
      strcpy(*opcode,"00000000");
      strcpy(*disassembly,"?");
      return 1; // there may not be an instruction at the PC (example: page fault on branch)
    }
  }
  else
  {
    return 2; // No processors, therefore no configured disassembler for us to use
  }

  // At this point disassembly proceeded correctly. Now check the output buffers
  if (opcode == nullptr || disassembly == nullptr)
  {
    return 2;
  }
  size_t opcode_buffer_length = opcode != nullptr ? strlen(*opcode) : 0;
  size_t disassembly_buffer_length = disassembly != nullptr ? strlen(*disassembly) : 0;
  if ((opcode_buffer_length < (opcode_temp.size()+1)) || (disassembly_buffer_length < (disassembly_temp.size()+1))) // check for enough size to place the null termination character
  {
    return 2; // The user didn't give us room to put the answers
  }

  // Warning: the 'insn't' null and string length tests above can fail to uncover a corner case of bad input, where a pointer to a string literal is aliased by a char* pointer.
  // The compiler will accept this, but it will fail when you try to copy as in below, basically be sure your string buffer was correctly allocated.
  opcode_temp.copy(*opcode, opcode_temp.size(), 0);
  (*opcode)[opcode_temp.size()]='\0'; // No idea how the user is initializing these strings, so just null terminate the relevant part.
  disassembly_temp.copy(*disassembly, disassembly_temp.size(), 0);
  (*disassembly)[disassembly_temp.size()]='\0';

  return 0;
}

// Not only loads the elf file in the specified path into the memory model but readies the simulator for stepping.
int simlib_t::load_program_now(const char* elfPath)
{
  std::map<std::string, uint64_t> symbols = load_payload(elfPath, &entry);

  start_pc = start_pc == reg_t(-1) ? get_entry_point() : start_pc;
  processor_t* usable_proc_ptr = nullptr;

  for (size_t i = 0; i < procs.size(); i++) {
    procs[i]->get_state()->pc = start_pc;
  }

  return 0;
}

bool simlib_t::doesCoreWithIdExist(size_t i)
{
  for (processor_t* proc_ptr: procs)
  {
    if (proc_ptr != nullptr && proc_ptr->get_state() != nullptr && proc_ptr->get_id() == i)
    {
      return true;
    }
  }

  return false;
}

void simlib_t::dump_sparse_memory(std::ostream & out)
{
  for (unsigned i = 0; i < mems.size(); i++) {
    mems[i].second->dump(out);
  }
}

void simlib_t::sparse_read(reg_t addr, size_t len, uint8_t* bytes)
{
  size_t align = chunk_align();
  if (len && (addr & (align-1)))
  {
    size_t this_len = std::min(len, align - size_t(addr & (align-1)));
    uint8_t chunk[align];

    read_chunk(addr & ~(align-1), align, chunk);
    memcpy(bytes, chunk + (addr & (align-1)), this_len);

    bytes += this_len;
    addr += this_len;
    len -= this_len;
  }

  if (len) {
    uint8_t chunk[align];

    read_chunk(addr, align, chunk);
    memcpy(bytes, chunk, len);
  }
}

void simlib_t::clear_chunk(reg_t taddr, size_t len)
{
  char zeros[chunk_max_size()];
  memset(zeros, 0, chunk_max_size());

  for (size_t pos = 0; pos < len; pos += chunk_max_size())
    write_chunk(taddr + pos, std::min(len - pos, chunk_max_size()), zeros);
}

void simlib_t::initialize_multiword(reg_t addr, size_t len, const void* bytes) // To support multiword initializations during elf loading
{
  size_t align = chunk_align();
  if (len && (addr & (align-1)))
  {
    size_t this_len = std::min(len, align - size_t(addr & (align-1)));
    uint8_t chunk[align];

    read_chunk(addr & ~(align-1), align, chunk);
    memcpy(chunk + (addr & (align-1)), bytes, this_len);
    write_chunk(addr & ~(align-1), align, chunk);

    bytes = (char*)bytes + this_len;
    addr += this_len;
    len -= this_len;
  }

  if (len & (align-1))
  {
    size_t this_len = len & (align-1);
    size_t start = len - this_len;
    uint8_t chunk[align];

    read_chunk(addr + start, align, chunk);
    memcpy(chunk, (char*)bytes + start, this_len);
    write_chunk(addr + start, align, chunk);

    len -= this_len;
  }

  // now we're aligned
  bool all_zero = len != 0;
  for (size_t i = 0; i < len; i++)
    all_zero &= ((const char*)bytes)[i] == 0;

  if (all_zero) {
    clear_chunk(addr, len);
  } else {
    size_t max_chunk = chunk_max_size();
    for (size_t pos = 0; pos < len; pos += max_chunk)
      write_chunk(addr + pos, std::min(max_chunk, len - pos), (char*)bytes + pos);
  }
}

uint64_t simlib_t::get_csr_number(const std::string& input_name)
{
  //Cant use a switch here unless we map the names to ints beforehand
  #define DECLARE_CSR(name, number) if (input_name == #name) return number;
  #include "encoding.h"              // generates if's for all csrs
  return 0xDEADBEEFDEADBEEF;         // else return a value outside the ISA established 4096 possible
  #undef DECLARE_CSR
}

uint64_t simlib_t::get_xpr_number(const std::string& input_name)
{
  return std::find(xpr_name, xpr_name + NXPR, input_name) - xpr_name;
}

uint64_t simlib_t::get_fpr_number(const std::string& input_name)
{
  return std::find(fpr_name, fpr_name + NFPR, input_name) - fpr_name;
}

uint64_t simlib_t::get_vecr_number(const std::string& input_name)
{
  return std::find(vr_name, vr_name + NVPR, input_name) - vr_name;
}

std::string simlib_t::get_csr_name(uint64_t index)
{
  if (index < NCSR)
    return csr_name(index);
  else
    return "unknown-csr";
}

std::string simlib_t::get_xpr_name(uint64_t index)
{
  if (index < NXPR)
    return xpr_name[index];
  else
    return "unknown-xpr";
}

std::string simlib_t::get_fpr_name(uint64_t index)
{
  if (index < NFPR)
    return fpr_name[index];
  else
    return "unknown-fpr";
}

std::string simlib_t::get_vecr_name(uint64_t index)
{
  if (index < NVPR)
    return vr_name[index];
  else
    return "unknown-vr";
}

int simlib_t::read_csr(uint32_t procid, uint64_t index, uint64_t* value, uint32_t* length)
{
  //Check if the pointers point to something
  if (value == nullptr || length == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the index is valid
  if (index >= NCSR)
    return 3;

  //Check if the index corresponds to a defined csr
  // WARNING: there are a number of CSRs that are defined in encoding.h but have no entry in the processor class get_csr method. In this case, for now, a value of 0xDEADBEEFDEADBEEF will be returned
  std::string temp_name = get_csr_name(index);
  size_t unknown = temp_name.find("unknown");
  if (unknown != std::string::npos)
    return 3;

  //All checks pass, so go ahead and load the value buffer and set the length to indicate the used bytes in the value buffer
  *value = get_core_by_id(procid)->get_csr_api(index);
  *length = get_core_by_id(procid)->get_xlen() / 8;

  return 0;
}

int simlib_t::read_csr(uint32_t procid, const std::string& input_name, uint64_t* value, uint32_t* length)
{
  //Check if the pointers point to something
  if (value == nullptr || length == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the name is valid
  uint64_t index = get_csr_number(input_name);
  if (index >= NCSR)
    return 3;

  //All checks pass, so go ahead and load the value buffer and set the length to indicate the used bytes in the value buffer
  *value = get_core_by_id(procid)->get_csr_api(index);
  *length = get_core_by_id(procid)->get_xlen() / 8;

  return 0;
}

int simlib_t::read_xpr(uint32_t procid, const std::string& input_name, uint64_t* value, uint32_t* length)
{
  //Check if the pointers point to something
  if (value == nullptr || length == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the name is valid
  uint64_t index = get_xpr_number(input_name);
  if (index >= NXPR)
    return 3;

  //All checks pass, so go ahead and load the value buffer and set the length to indicate the used bytes in the value buffer
  *value = get_core_by_id(procid)->get_state()->XPR[index];
  *length = get_core_by_id(procid)->get_xlen() / 8;

  return 0;
}

int simlib_t::read_xpr(uint32_t procid, uint64_t index, uint64_t* value, uint32_t* length)
{
  //Check if the pointers point to something
  if (value == nullptr || length == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //check if the index provided is valid
  if (index >= NXPR)
    return 3;

  //All checks pass, so go ahead and load the value buffer and set the length to indicate the used bytes in the value buffer
  //*value = get_core_by_id(procid)->get_state()->XPR[index];

  *value = get_core_by_id(procid)->get_state()->XPR[index];
  *length = get_core_by_id(procid)->get_xlen() / 8;

  return 0;
}

int simlib_t::read_fpr(uint32_t procid, const std::string& input_name, uint8_t* value, uint32_t* length)
{
  //Check if the pointers point to something
  if (value == nullptr || length == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //check if the name provided is valid
  uint64_t index = get_fpr_number(input_name);
  if (index >= NFPR)
    return 3;

  //Check that the advertised length of the provided buffer is sufficient
  if (*length < sizeof(freg_t))
    return 4;

  //All checks have passed, so go ahead and load the value buffer and set the length to indicate the used bytes in the value buffer
  freg_t temp_fpr_val = get_core_by_id(procid)->get_state()->FPR[index];
  memcpy(value, &temp_fpr_val, sizeof(freg_t));
  *length = sizeof(freg_t);

  return 0;
}

int simlib_t::read_fpr(uint32_t procid, uint64_t index, uint8_t* value, uint32_t* length)
{
  //Check if the pointers point to something
  if (value == nullptr || length == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the index provided is valid
  if (index >= NFPR)
    return 3;

  if (*length < sizeof(freg_t))
    return 4;

  //All checks have passed
  freg_t temp_fpr_val = get_core_by_id(procid)->get_state()->FPR[index];
  memcpy(value, &temp_fpr_val, sizeof(freg_t));
  *length = sizeof(freg_t);

  return 0;
}

int simlib_t::read_vecr(uint32_t procid, const std::string& input_name, uint8_t* value, uint32_t* length)
{
  //Check that the pointers point to something
  if (value == nullptr || length == nullptr)
    return 1;

  //Check that the procid AKA hart is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the name provided is valid
  uint64_t index = get_vecr_number(input_name);
  if (index >= NVPR)
    return 3;

  //Check that the advertised length of the buffer in bytes can hold the requested data
  size_t vlen = get_core_by_id(procid)->VU.get_vlen() / 8;
  if (vlen > *length){
    return 4;
  }

  //All checks passed
  size_t elen = get_core_by_id(procid)->VU.get_elen() / 8;
  size_t num_elem = vlen/elen;

  //Write the elements into the value buffer
  for (size_t element = 0; element < num_elem; ++element)
  {
    uint64_t val = 0ull;
    switch (elen)
    {
      case 8:
      val = get_core_by_id(procid)->VU.elt<uint64_t>(index, element);
      break;
      case 4:
      val = get_core_by_id(procid)->VU.elt<uint32_t>(index, element);
      break;
      case 2:
      val = get_core_by_id(procid)->VU.elt<uint16_t>(index, element);
      break;
      case 1:
      val = get_core_by_id(procid)->VU.elt<uint8_t>(index, element);
      break;
    }

    memcpy(value + element * elen, &val, elen);
  }

  //Set the length value to the number of bytes that was written
  *length = vlen;

  return 0;
}

int simlib_t::read_vecr(uint32_t procid, uint64_t index, uint8_t* value, uint32_t* length)
{
  //Check that the pointers point to something
  if (value == nullptr || length == nullptr)
    return 1;

  //Check that the procid AKA hart is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the index provided is valid
  if (index >= NVPR)
    return 3;

  //Check that the advertised length of the buffer in bytes can hold the requested data
  size_t vlen = get_core_by_id(procid)->VU.get_vlen() / 8;
  if (vlen > *length){
    return 4;
  }

  //All checks passed
  size_t elen = get_core_by_id(procid)->VU.get_elen() / 8;
  size_t num_elem = vlen/elen;

  //Write the elements into the value buffer
  for (size_t element = 0; element < num_elem; ++element)
  {
    uint64_t val = 0ull;
    switch (elen)
    {
      case 8:
      val = get_core_by_id(procid)->VU.elt<uint64_t>(index, element);
      break;
      case 4:
      val = get_core_by_id(procid)->VU.elt<uint32_t>(index, element);
      break;
      case 2:
      val = get_core_by_id(procid)->VU.elt<uint16_t>(index, element);
      break;
      case 1:
      val = get_core_by_id(procid)->VU.elt<uint8_t>(index, element);
      break;
    }

    memcpy(value + element * elen, &val, elen);
  }

  //Set the length value to the number of bytes that was written
  *length = vlen;

  return 0;
}

int simlib_t::partial_read_vecr(uint32_t procid, uint64_t index, uint8_t* pValue, uint32_t length, uint32_t offset)
{
  //Check that the pointers point to something
  if (pValue == nullptr)
    return 1;

  //Check that the procid AKA hart is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the index provided is valid
  if (index >= NVPR)
    return 3;

  //Check that the advertised length of the buffer in bytes can hold the requested data
  size_t vlen = get_core_by_id(procid)->VU.get_vlen() / 8;
  if (vlen < (offset + length)){
    return 4;
  }

  //All checks passed; write the elements into the value buffer
  // make this a memcpy
  for (size_t element = offset; element < (offset + length); ++element)
  {
    pValue[element - offset] = get_core_by_id(procid)->VU.elt<uint8_t>(index, element);
  }

  return 0;
}

int simlib_t::partial_write_vecr(uint32_t procid, uint64_t index, const uint8_t* pValue, uint32_t length, uint32_t offset)
{
  //Check that the pointers point to something
  if (pValue == nullptr)
    return 1;

  //Check that the procid AKA hart is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the index provided is valid
  if (index >= NVPR)
    return 3;

  //Check that the advertised length of the buffer in bytes can hold the requested data
  size_t vlen = get_core_by_id(procid)->VU.get_vlen() / 8;
  if (vlen < (offset + length)){
    return 4;
  }

  //All checks passed; write the elements into the value buffer
  // make this a memcpy
  for (size_t element = offset; element < (offset + length); ++element)
  {
    get_core_by_id(procid)->VU.elt<uint8_t>(index, element) = pValue[element - offset];
  }

  return 0;
}

int simlib_t::write_csr(uint32_t procid, uint64_t index, const uint64_t* value, uint32_t length)
{
  //Check if the pointers point to something
  if (value == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the index is valid
  if (index >= NCSR)
    return 3;

  //Is the indended write length matching the xlen value?
  if (length != (get_core_by_id(procid)->get_xlen() / 8))
    return 4;

  //All checks pass, so go ahead and and write to the csr
  get_core_by_id(procid)->set_csr_api(index, *value);

  return 0;
}

int simlib_t::write_csr(uint32_t procid, const std::string& input_name, const uint64_t* value, uint32_t length)
{
  //Check if the pointers point to something
  if (value == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the name is valid
  uint64_t index = get_csr_number(input_name);
  if (index >= NCSR)
    return 3;

  //Is the indended write length matching the xlen value?
  if (length != (get_core_by_id(procid)->get_xlen() / 8))
    return 4;

  //All checks pass, so go ahead and and write to the csr
  get_core_by_id(procid)->set_csr_api(index, *value);

  return 0;
}

int simlib_t::write_xpr(uint32_t procid, const std::string& input_name, const uint64_t* value, uint32_t length)
{
  //Check if the pointers point to something
  if (value == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the name is valid
  uint64_t index = get_xpr_number(input_name);
  if (index >= NXPR)
    return 3;

  //Is the indended write length matching the xlen value?
  //if (length != (get_core_by_id(procid)->get_xlen() / 8))
  //  return 4;

  //All checks pass, so go ahead and load the value buffer and set the length to indicate the used bytes in the value buffer
  get_core_by_id(procid)->get_state()->XPR.write(index, *value);

  return 0;
}

int simlib_t::write_xpr(uint32_t procid, uint64_t index, const uint64_t* value, uint32_t length)
{
  //Check if the pointers point to something
  if (value == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //check if the index provided is valid
  if (index >= NXPR)
    return 3;

  //Is the indended write length matching the xlen value?
  //if (length != (get_core_by_id(procid)->get_xlen() / 8))
  //  return 4;

  //All checks pass so we're go to write
  get_core_by_id(procid)->get_state()->XPR.write(index, *value);

  return 0;
}

int simlib_t::write_fpr(uint32_t procid, const std::string& input_name, const uint8_t* value, uint32_t length)
{
  //Check if the pointers point to something
  if (value == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //check if the name provided is valid
  uint64_t index = get_fpr_number(input_name);
  if (index >= NFPR)
    return 3;

  //Is the indended write length matching the flen value?
  if (length > sizeof(freg_t))
    return 4;

  //All checks have passed perform the write via a memory operation into the floating point register model
  freg_t temp_fpr_val;
  temp_fpr_val.v[0] = 0xffffffffffffffffull;
  temp_fpr_val.v[1] = 0xffffffffffffffffull;
  memcpy(&temp_fpr_val, value, length);
  get_core_by_id(procid)->get_state()->FPR.write(index, temp_fpr_val);

  return 0;
}

int simlib_t::write_fpr(uint32_t procid, uint64_t index, const uint8_t* value, uint32_t length)
{
  //Check if the pointers point to something
  if (value == nullptr)
    return 1;

  //Check if the procid AKA hart id is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the index provided is valid
  if (index >= NFPR)
    return 3;

  //Is the indended write length matching the flen value?
  if (length > sizeof(freg_t))
    return 4;

  //All checks have passed perform the write via a memory operation into the floating point register model
  freg_t temp_fpr_val;
  temp_fpr_val.v[0] = 0xffffffffffffffffull;
  temp_fpr_val.v[1] = 0xffffffffffffffffull;
  memcpy(&temp_fpr_val, value, length);
  get_core_by_id(procid)->get_state()->FPR.write(index, temp_fpr_val);

  return 0;
}

//will probably have to cast differently here and other places to the get the pointer types the same and compatible with the function.
int simlib_t::write_vecr(uint32_t procid, const std::string& input_name, const uint8_t* value, uint32_t length)
{
  //Check that the pointers point to something
  if (value == nullptr)
    return 1;

  //Check that the procid AKA hart is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the name provided is valid
  uint64_t index = get_vecr_number(input_name);
  if (index >= NVPR)
    return 3;

  //Check that the advertised length of the buffer in bytes equals the size of the vector register
  size_t vlen = get_core_by_id(procid)->VU.get_vlen() / 8;
  if (vlen != length)
    return 4;

  //All checks passed
  size_t elen = get_core_by_id(procid)->VU.get_elen() / 8;
  size_t num_elem = vlen/elen;

  //Write the elements into the value buffer
  for (size_t element = 0; element < num_elem; ++element)
  {
    switch (elen)
    {
      case 8:
      {
        uint64_t* reg = &(get_core_by_id(procid)->VU.elt<uint64_t>(index, element));
        memcpy(reg, value + element * elen, elen);
        break;
      }
      case 4:
      {
        uint32_t* reg = &(get_core_by_id(procid)->VU.elt<uint32_t>(index, element));
        memcpy(reg, value + element * elen, elen);
        break;
      }
      case 2:
      {
        uint16_t* reg = &(get_core_by_id(procid)->VU.elt<uint16_t>(index, element));
        memcpy(reg, value + element * elen, elen);
        break;
      }
      case 1:
      {
        uint8_t* reg = &(get_core_by_id(procid)->VU.elt<uint8_t>(index, element));
        memcpy(reg, value + element * elen, elen);
        break;
      }
    }
  }

  return 0;
}

int simlib_t::write_vecr(uint32_t procid, uint64_t index, const uint8_t* value, uint32_t length)
{
  //Check that the pointers point to something
  if (value == nullptr)
    return 1;

  //Check that the procid AKA hart is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  //Check if the index provided is valid
  if (index >= NVPR)
    return 3;

  //Check that the advertised length of the buffer in bytes can hold the requested data
  size_t vlen = get_core_by_id(procid)->VU.get_vlen() / 8;
  if (vlen != length)
    return 4;

  //All checks passed
  size_t elen = get_core_by_id(procid)->VU.get_elen() / 8;
  size_t num_elem = vlen/elen;

  //Write the elements into the value buffer
  for (size_t element = 0; element < num_elem; ++element)
  {
    switch (elen)
    {
      case 8:
      {
        uint64_t* reg = &(get_core_by_id(procid)->VU.elt<uint64_t>(index, element));
        memcpy(reg, value + element * elen, elen);
        break;
      }
      case 4:
      {
        uint32_t* reg = &(get_core_by_id(procid)->VU.elt<uint32_t>(index, element));
        memcpy(reg, value + element * elen, elen);
        break;
      }
      case 2:
      {
        uint16_t* reg = &(get_core_by_id(procid)->VU.elt<uint16_t>(index, element));
        memcpy(reg, value + element * elen, elen);
        break;
      }
      case 1:
      {
        uint8_t* reg = &(get_core_by_id(procid)->VU.elt<uint8_t>(index, element));
        memcpy(reg, value + element * elen, elen);
        break;
      }
    }
  }

  return 0;
}

int simlib_t::translate_virtual_address_api(int procid, const uint64_t* vaddr, int intent, uint64_t* paddr, uint64_t* memattrs)
{
  // check the pointers do point to something
  if (vaddr == nullptr || paddr == nullptr || memattrs == nullptr)
    return 1;

  // Check that the procid AKA hart is valid
  if (not doesCoreWithIdExist(procid))
    return 2;

  // get the mmu for the specified processor and call the translation api function
  access_type type;
  switch (intent)
  {
    case(0):
      type = access_type::LOAD;
      break;
    case(1):
      type = access_type::STORE;
      break;
    case(2):
      type = access_type::FETCH;
      break;
  }
  int status = get_core_by_id(procid)->get_mmu()->translate_api(*vaddr, paddr, memattrs, reg_t(1) /* length */, static_cast<access_type>(intent), 0);
  // length is set to 1 byte here because the api requirements were for individual bytes only.

  if (status == 0)
  {
    return 0;
  }

  return status + 2;
}

bool simlib_t::set_pc_api(int procid, const std::string& name, const uint8_t* bytes, size_t len)
{
  if (bytes == nullptr)
  {
    return false;
  }
  else if (not doesCoreWithIdExist(procid))
  {
    return false;
  }

  processor_t* proc_ptr = get_core_by_id(procid);
  if (proc_ptr == nullptr)
  {
    return false;
  }

  return get_core_by_id(procid)->set_pc_api(name, bytes, len);
}

bool simlib_t::get_pc_api(int procid, uint8_t* bytes, const std::string& name, size_t len)
{
  if (bytes == nullptr)
  {
    return false;
  }
  else if (not doesCoreWithIdExist(procid))
  {
    return false;
  }

  processor_t* proc_ptr = get_core_by_id(procid);
  if (proc_ptr == nullptr)
  {
    return false;
  }

  return get_core_by_id(procid)->retrieve_pc_api(bytes, name, len); // The method called in processor_t will validate the name and length.
}

bool simlib_t::set_privilege_api(int procid, const uint64_t* val)
{
  if (nullptr == val) return false;
  if (!doesCoreWithIdExist(procid)) return false;

  processor_t* proc_ptr = get_core_by_id(procid);
  if (nullptr == proc_ptr) return false;

  //assert in legalize_privilege should catch invalid values
  //register field is only 2 bits so shouldn't be able to pass invalid case from force
  proc_ptr->set_privilege_api(*val);
  return true;
}

bool simlib_t::get_privilege_api(int procid, uint64_t* val)
{
  if (nullptr == val) return false;
  if (!doesCoreWithIdExist(procid)) return false;

  processor_t* proc_ptr = get_core_by_id(procid);
  if (nullptr == proc_ptr) return false;

  proc_ptr->retrieve_privilege_api(val);
  return true;
}

bool simlib_t::inject_simulator_events(int procid, uint32_t events)
{
  if (!doesCoreWithIdExist(procid)) return false;

  processor_t* proc_ptr = get_core_by_id(procid);

  if (nullptr == proc_ptr) return false;

  proc_ptr->get_state()->mip->backdoor_write_with_mask(1 << events, 1 << events);
  return true;
}
#endif /* ENABLE_FORCE_RISCV */
