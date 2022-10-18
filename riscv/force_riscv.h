#ifndef _FORCE_RISCV_H
#define _FORCE_RISCV_H
#ifdef ENABLE_FORCE_RISCV
#include <cstdint>
//!< MmuEvent - struct used to record memory events from simulator...
typedef enum _Memtype { Strong,Device,Normal } Memtype;
typedef unsigned int CacheType;
typedef unsigned int CacheAttrs;
struct MmuEvent
{
  MmuEvent(uint64_t _va, uint64_t _pa, Memtype _type, bool _has_stage_two, CacheType _outer_type, CacheAttrs _outer_attrs, CacheType _inner_type, CacheAttrs _inner_attrs)
    : va(_va), pa(_pa), type(_type), has_stage_two(_has_stage_two), outer_type(_outer_type), outer_attrs(_outer_attrs), inner_type(_inner_type), inner_attrs(_inner_attrs)
  {
  }

  uint64_t va;
  uint64_t pa;
  Memtype type;
  bool has_stage_two;
  CacheType outer_type;
  CacheAttrs outer_attrs;
  CacheType inner_type;
  CacheAttrs inner_attrs;
};

struct SimException {
  SimException() : mExceptionID(0), mExceptionAttributes(0), mpComments(""), mEPC(0) {}
  SimException(uint32_t exceptionID, uint32_t exceptionAttributes, const char* comments, uint64_t epc) :
    mExceptionID(exceptionID), mExceptionAttributes(exceptionAttributes), mpComments(comments), mEPC(epc) {}
  uint32_t mExceptionID; //!< 0x4E: eret. Other values are scause or mcause codes.
  uint32_t  mExceptionAttributes;  //!< copied from tval.
  const char* mpComments; //!<  exception comments, identifies enter, exit and m or s modes.
  uint64_t mEPC; //!< exception program counter.
};

extern "C" {
  // memory r/w callback
  void update_generator_memory(uint32_t cpuid, uint64_t virtualAddress, uint32_t memBank, uint64_t physicalAddress, uint32_t size, const char *pBytes, const char *pAccessType);

  // mmu update callback
  void update_mmu_event(MmuEvent *event);

  //exception handling callback
  void update_exception_event(const SimException* exception);

  // update_generator_register function: for the given cpuid, this callback function is called by the simulator to notify the user that a register has been accessed.
  //
  //  inputs:
  //      uint32_t cpuid -- refers to the processor ID
  //      const char* registerName -- the name of the reigster (programmer's name)
  //      uint64_t value -- the data stored in the register after update
  //      uint64_t mask -- 1's indicate relevant bits
  //      const char* accessType -- indicates if the access was a read or write.
  //
  void update_generator_register(uint32_t cpuid, const char* pRegisterName, uint64_t value, uint64_t mask, const char* pAccessType);  //!< update generator register information when step an instruction

  // update_vector_element function: for the given cpuid, this callback function is called by the simulator to notify the user that a vector register element has been read or written
  //
  //  inputs:
  //      uint32_t cpuid -- refers to the processor ID
  //      const char* pRegName -- the base name of the vector register does NOT include a suffix for physical register since this is a FORCE / hardware specific notion.
  //      uint32_t vecRegIndex -- the numerical index that goes with the vector register base name
  //      uint32_t eltIndex -- the numerical index of the element that is updated
  //      uint32_t eltByteWidth -- the number of bytes per element at the time of the update, used in FORCE with the eltIndex to dynamically associate physical registers for aggregated updates
  //      const uint8_t* value -- the contents of the ENTIRE vector register if this update is a "read" or *nothing* if this is a "write".
  //      uint32_t byteLength -- should match the size of the ENTIRE vector register.
  //      const char* pAccessType -- should be "read" or "write".
  //
  void update_vector_element(uint32_t cpuid, const char *pRegName, uint32_t vecRegIndex, uint32_t eltIndex, uint32_t eltByteWidth, const uint8_t* pValue, uint32_t  byteLength, const char* pAccessType);
}
#endif // ENABLE_FORCE_RISCV
#endif //_FORCE_RISCV_H
