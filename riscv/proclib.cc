#include "arith.h"
#include "proclib.h"
#include "extension.h"
#include "common.h"
#include "config.h"
#include "simif.h"
#include "mmulib.h"
#include "disasm.h"
#include "platform.h"
#include <cinttypes>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <assert.h>
#include <limits.h>
#include <stdexcept>
#include <string>
#include <algorithm>
#include "decode_macros.h"

#ifdef ENABLE_FORCE_RISCV
reg_t proclib_t::set_vl_api(reg_t reqVL, reg_t newType)
{
  state.force = true;
  VU.vlmax = -1;
  reg_t vl = VU.set_vl(1, 1, reqVL, newType);
  VU.setvl_count--;
  state.force = false;
  return vl;
}

void proclib_t::set_privilege(reg_t prv, bool virt)
{
  processor_t::set_privilege(prv, virt);
  update_generator_register(this->id, "privilege", prv, 0x3ull, "write");
}

void proclib_t::set_privilege_api(reg_t prv)
{
  processor_t::set_privilege(prv, state.v);
}

void proclib_t::set_csr_api(int which, reg_t val)
{
  val = zext_xlen(val);
  switch (which)
  {
  case CSR_VL:
    set_vl_api(val, VU.vtype->read());
    break;
  case CSR_VTYPE:
    set_vl_api(VU.vl->read(), val);
    break;
  default:
    {
      auto search = state.csrmap.find(which);
      if (search != state.csrmap.end()) {
        state.force = true;
        search->second->unlogged_write(val);
        state.force = false;
      }
      break;
    }
  }
  return;
}

bool proclib_t::set_pc_api(const std::string& name, const uint8_t* bytes, size_t len) //len advertises the size of the buffer
{
    if (bytes == nullptr) {
        return false;
    }

    if (name == std::string("PC") || name == std::string("pc")) {
        if (len != sizeof(state.pc)) {
            return false;
        } else {
            memcpy(&(state.pc), bytes, len);
            return true;
        }
    } else {
        return false;
    }
}

bool proclib_t::retrieve_pc_api(uint8_t* bytes, const std::string& name, size_t len) //len advertises the size of the buffer
{
    if (bytes == nullptr) {
        return false;
    }

    if (name == std::string("PC") || name == std::string("pc")) {
        if (len != sizeof(state.pc)) {
            return false;
        } else {
            memcpy(bytes, &(state.pc), len);
            return true;
        }
    } else {
        return false;
    }
}

void proclib_t::retrieve_privilege_api(reg_t* val)
{
  *val = state.prv;
}

reg_t proclib_t::get_csr_api(int which)
{
  auto search = state.csrmap.find(which);
  if (search != state.csrmap.end()) {
    return search->second->read();
  }
  return 0xDEADBEEFDEADBEEF;
}
#endif /* ENABLE_FORCE_RISCV */
