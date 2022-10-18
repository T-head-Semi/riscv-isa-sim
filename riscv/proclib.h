// See LICENSE for license details.
#ifndef _RISCV_PROCLIB_H
#define _RISCV_PROCLIB_H
#include "processor.h"
#include "mmulib.h"

#ifdef ENABLE_FORCE_RISCV
// this class represents one processor in a RISC-V machine.
class proclib_t : public processor_t {
public:
  proclib_t(const isa_parser_t *isa, const cfg_t* cfg,
            simif_t* sim, uint32_t id, bool halt_on_reset,
            FILE *log_file, std::ostream& sout_): processor_t(isa, cfg, sim, id, halt_on_reset, log_file, sout_) {
    mmu = new mmulib_t(sim, cfg->endianness, this);
  }
  ~proclib_t() {}

  reg_t set_vl_api(reg_t reqVL, reg_t newType) override;
  void set_privilege(reg_t prv, bool virt) override;
  void set_privilege_api(reg_t prv) override;
  void set_csr_api(int which, reg_t val) override;
  bool set_pc_api(const std::string& name, const uint8_t* bytes, size_t len) override; //len advertises the size of the buffer
  bool retrieve_pc_api(uint8_t* bytes, const std::string& name, size_t len) override; //len advertises the size of the buffer
  void retrieve_privilege_api(reg_t* val) override;
  reg_t get_csr_api(int which) override;
};
#endif /* ENABLE_FORCE_RISCV */
#endif
