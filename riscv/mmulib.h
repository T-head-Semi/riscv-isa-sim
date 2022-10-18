#ifndef _RISCV_MMULIB_H
#define _RISCV_MMULIB_H

#ifdef ENABLE_FORCE_RISCV
#include "mmu.h"

class mmulib_t: public mmu_t {
public:
  mmulib_t(simif_t* sim, endianness_t endianness, processor_t* proc): mmu_t(sim, endianness, proc) {}
  ~mmulib_t() {}

  // Translate a VA to a PA by performing a page table walk but don't set any state bits
  // and instead of throwing exceptions, return codes are used.
  //
  // Does a pmp check on the recovered PA.
  //
  //    returns:
  //        0 - walk was successful
  //        1 - PMP problem with PA after address translation somehow
  //        2 - access exception while trying to check pmp status of page table entry PA
  //        3 - walk was unsuccessful and access type was FETCH
  //        4 - walk was unsuccessful and access type was LOAD
  //        5 - walk was unsuccessful and access type was STORE
  //        6 - walk was unsuccessful and access type was not any of the above
  //        7 - walk would have been successful had paddr_ptr not been a null pointer
  int translate_api(reg_t addr, reg_t *paddr, uint64_t* pmp_info, reg_t len, access_type type, uint32_t xlate_flags) override;

  bool pmp_ok_api(reg_t addr, reg_t* pmpaddr_ptr, uint8_t* pmpcfg_ptr, reg_t len, access_type type, reg_t mode) override;

  // perform a page table walk but don't set any state bits
  // and instead of throwing exceptions, return codes are used:
  //
  //    returns:
  //        0 - walk was successful
  //        2 - access exception while trying to check pmp status of page table entry PA
  //        3 - walk was unsuccessful and access type was FETCH
  //        4 - walk was unsuccessful and access type was LOAD
  //        5 - walk was unsuccessful and access type was STORE
  //        6 - walk was unsuccessful and access type was not any of the above
  //        7 - walk would have been successful had paddr_ptr not been a null pointer
  int walk_api(reg_t addr, reg_t* paddr_ptr, access_type type, reg_t prv, bool virt, bool hlvx) override;
};
#endif /* ENABLE_FORCE_RISCV */
#endif
