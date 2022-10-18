#include "mmulib.h"
#include "arith.h"
#include "simif.h"
#include "processor.h"

//DEBUG
#include <iostream>

#ifdef ENABLE_FORCE_RISCV
 #define RISCV_XLATE_VIRT      (1U << 0)
 #define RISCV_XLATE_VIRT_HLVX (1U << 1)
 #define RISCV_XLATE_LR        (1U << 2)

int mmulib_t::translate_api(reg_t addr, reg_t* paddr, uint64_t* pmp_info, reg_t len, access_type type, uint32_t xlate_flags = 0)
{
  int status = 0;
  if (!proc){
    status = 1;
    return status;
  }

  bool virt = proc->get_state()->v;
  bool hlvx = xlate_flags & RISCV_XLATE_VIRT_HLVX;
  reg_t mode = proc->get_state()->prv;
  if (type != FETCH) {
    if (!proc->get_state()->debug_mode && get_field(proc->get_state()->mstatus->read(), MSTATUS_MPRV)) {
      mode = get_field(proc->get_state()->mstatus->read(), MSTATUS_MPP);
      if (get_field(proc->get_state()->mstatus->read(), MSTATUS_MPV) && mode != PRV_M)
        virt = true;
    }
    if (xlate_flags & RISCV_XLATE_VIRT) {
      virt = true;
      mode = get_field(proc->get_state()->hstatus->read(), HSTATUS_SPVP);
    }
  }

  reg_t temp_paddr = 0ull;
  status = walk_api(addr, &temp_paddr, type, mode, virt, hlvx);
  temp_paddr |= (addr & (PGSIZE-1));

  reg_t temp_pmpaddr = 0ull;
  uint8_t temp_pmpcfg = 0;
  if (status == 0 && !pmp_ok_api(temp_paddr, &temp_pmpaddr, &temp_pmpcfg, len, type, mode))
  {
    status = 1; // Failed pmp check, either there was no match or there was only a partial match of the PMP requriements for that physical address.
  }

  if (pmp_info != nullptr)
  {
    *pmp_info = (temp_pmpaddr << 6) | (uint64_t)temp_pmpcfg; // This implies a 56 bit address
  }

  *paddr = temp_paddr;
  return status;
}

bool mmulib_t::pmp_ok_api(reg_t addr, reg_t* pmpaddr_ptr, uint8_t* pmpcfg_ptr, reg_t len, access_type type, reg_t mode)
{
  if (!proc || proc->n_pmp == 0)
    return true;

  for (size_t i = 0; i < proc->n_pmp; i++) {
    // Check each 4-byte sector of the access
    bool any_match = false;
    bool all_match = true;
    for (reg_t offset = 0; offset < len; offset += 1 << PMP_SHIFT) {
      reg_t cur_addr = addr + offset;
      bool match = proc->get_state()->pmpaddr[i]->match4(cur_addr);
      any_match |= match;
      all_match &= match;
      if (pmpaddr_ptr != nullptr && pmpcfg_ptr != nullptr)
      {
        *pmpaddr_ptr = match = proc->get_state()->pmpaddr[i]->tor_paddr();
        *pmpcfg_ptr = proc->get_state()->pmpaddr[i]->cfg;
      }
    }

    if (any_match) {
      // If the PMP matches only a strict subset of the access, fail it
      if (!all_match)
        return false;

      return proc->get_state()->pmpaddr[i]->access_ok(type, mode);
    }
  }

  // in case matching region is not found
  const bool mseccfg_mml = proc->get_state()->mseccfg->get_mml();
  const bool mseccfg_mmwp = proc->get_state()->mseccfg->get_mmwp();
  return ((mode == PRV_M) && !mseccfg_mmwp
          && (!mseccfg_mml || ((type == LOAD) || (type == STORE))));
}

int mmulib_t::walk_api(reg_t addr, reg_t* paddr_ptr, access_type type, reg_t mode, bool virt, bool hlvx)
{
  try {
    reg_t paddr = walk((mem_access_info_t){addr, mode, virt, {false, hlvx, false}, type}, false);
    if (paddr_ptr != nullptr) {
      *paddr_ptr = paddr;
      return 0;
    } else {
      return 7;
    }
  } catch (trap_load_access_fault& t) {
    return 2;
  } catch (trap_store_access_fault& t) {
    return 2;
  } catch (trap_instruction_access_fault& t) {
    return 2;
  }   catch (trap_instruction_page_fault& t) {
    return 3;
  } catch (trap_load_page_fault& t) {
    return 4;
  } catch (trap_store_page_fault& t) {
    return 5;
  } catch (trap_t &t) {
    return 6;
  }
}
#endif /* ENABLE_FORCE_RISCV */
