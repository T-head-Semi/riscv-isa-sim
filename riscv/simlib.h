// See LICENSE for license details.

#ifndef _RISCV_SIMLIB_H
#define _RISCV_SIMLIB_H

#include "sim.h"

#ifdef ENABLE_FORCE_RISCV
#include "Force_Enums.h"
#include "elfloader.h"

// this class encapsulates the processors and memory in a RISC-V machine.
class simlib_t: public sim_t {
public:
  simlib_t(const cfg_t *cfg, bool halted,
      std::vector<std::pair<reg_t, abstract_mem_t*>> mems,
      std::vector<const device_factory_t*> plugin_device_factories,
      const std::vector<std::string>& args,
      const debug_module_config_t &dm_config, const char *log_path,
      bool dtb_enabled, const char *dtb_file,
      bool socket_enabled,
      FILE *cmd_file); // needed for command line option --cmd

  ~simlib_t();

  // load the elf file and reset
  int load_program_now(const char* elfPath);

  // run the simulation incrementally
  int step_simulator(int target_id, int num_steps, int stx_failed);

  // fetch the instruction at the given pc using the debug_mmu and return the opcode and disassembly
  int get_disassembly(int target_id, const uint64_t* pc, char** opcode, char** disassembly);

  processor_t* get_core_by_id(size_t id) {
    for(processor_t* proc_ptr : procs)
    {
      if(proc_ptr != nullptr && proc_ptr->get_state() != nullptr && proc_ptr->get_id() == id)
        return proc_ptr;
    }

    return nullptr;
  }

  bool doesCoreWithIdExist(size_t i);

  // for debugging the sparse memory model
  void dump_sparse_memory(std::ostream & out);

  uint64_t get_csr_number(const std::string& input_name);
  uint64_t get_xpr_number(const std::string& input_name);
  uint64_t get_fpr_number(const std::string& input_name);
  uint64_t get_vecr_number(const std::string& input_name);

  std::string get_csr_name(uint64_t index);
  std::string get_xpr_name(uint64_t index);
  std::string get_fpr_name(uint64_t index);
  std::string get_vecr_name(uint64_t index);

  int read_csr(uint32_t procid, const std::string& input_name, uint64_t* value, uint32_t* length);
  int read_csr(uint32_t procid, uint64_t index, uint64_t* value, uint32_t* length);

  int read_xpr(uint32_t procid, const std::string& input_name, uint64_t* value, uint32_t* length);
  int read_xpr(uint32_t procid, uint64_t index, uint64_t* value, uint32_t* length);

  int read_fpr(uint32_t procid, const std::string& input_name, uint8_t* value, uint32_t* length);
  int read_fpr(uint32_t procid, uint64_t index, uint8_t* value, uint32_t* length);

  int read_vecr(uint32_t procid, const std::string& input_name, uint8_t* value, uint32_t* length);
  int read_vecr(uint32_t procid, uint64_t index, uint8_t* value, uint32_t* length);
  int partial_read_vecr(uint32_t procid, uint64_t index, uint8_t* pValue, uint32_t length, uint32_t offset);

  int write_csr(uint32_t procid, const std::string& input_name, const uint64_t* value, uint32_t length);
  int write_csr(uint32_t procid, uint64_t index, const uint64_t* value, uint32_t length);

  int write_xpr(uint32_t procid, const std::string& input_name, const uint64_t* value, uint32_t length);
  int write_xpr(uint32_t procid, uint64_t index, const uint64_t* value, uint32_t length);

  int write_fpr(uint32_t procid, const std::string& input_name, const uint8_t* value, uint32_t length);
  int write_fpr(uint32_t procid, uint64_t index, const uint8_t* value, uint32_t length);

  int write_vecr(uint32_t procid, const std::string& input_name, const uint8_t* value, uint32_t length);
  int write_vecr(uint32_t procid, uint64_t index, const uint8_t* value, uint32_t length);
  int partial_write_vecr(uint32_t procid, uint64_t index, const uint8_t* pValue, uint32_t length, uint32_t offset);

  bool set_pc_api(int procid, const std::string& name, const uint8_t* bytes, size_t len);
  bool get_pc_api(int procid, uint8_t* bytes, const std::string& name, size_t len);

  bool set_privilege_api(int procid, const uint64_t* val);
  bool get_privilege_api(int procid, uint64_t* val);

  // translate_virtual_address_api function: attempts to translate a virtual address into a physical address, returns any error information and also gathers the relevant pmp address and pmp configuration.
  //
  //  meaning of 'intent':
  //    0 - indicates a 'LOAD' access
  //    1 - indicates a 'STORE' access
  //    2 - indicates a 'FETCH' access
  //
  //  returns:
  //    0 - success
  //    1 - some pointer arguments were null
  //    2 - invalid procid
  //    3 - PMP problem with PA after address translation somehow
  //    4 - access exception while trying to check pmp status of page table entry PA
  //    5 - walk was unsuccessful and access type was FETCH
  //    6 - walk was unsuccessful and access type was LOAD
  //    7 - walk was unsuccessful and access type was STORE
  //    8 - walk was unsuccessful and access type was not any of the above
  //
  int translate_virtual_address_api(int procid, const uint64_t* vaddr, int intent, uint64_t* paddr, uint64_t* memattrs);

  bool inject_simulator_events(int procid, uint32_t events);

  void sparse_read(reg_t paddr, size_t len, uint8_t* bytes);
  void initialize_multiword(reg_t taddr, size_t len, const void* src); // To support multiword initializations during elf loading

private:

  friend class processor_t;
  friend class mmu_t;
  friend class debug_module_t;

  //void idle();
  void read_chunk_partially_initialized(reg_t taddr, size_t len, void* dst);
  void clear_chunk(reg_t taddr, size_t len);

  reg_t start_pc;
  reg_t set_reservations[16];
};

extern volatile bool ctrlc_pressed;
#endif  /* ENABLE_FORCE_RISCV */
#endif
