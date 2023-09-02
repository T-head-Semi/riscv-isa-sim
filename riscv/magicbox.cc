#include "processor.h"
#include "devices.h"
#include "simif.h"
#include "sim.h"
#include "dts.h"
#include <cstdlib>
#include "dts.h"

magicbox_t::magicbox_t(const simif_t* sim, bus_t *bus)
  : bus(bus), dma_src(0), dma_dest(0), dma_len(0)
{
    for (const auto& [hart_id, hart] : sim->get_harts()) {
        contexts[hart_id] = new magic_context_t(hart);
    }
}

#define LOG_REG	          0x0
#define INT_TRIGGER       0x4
#define INT_TRIGGER_INFO  0x8
#define INT_CLEAR_INFO    0xC
#define DMA_SADDR	0x10
#define DMA_DADDR	0x18
#define DMA_LEN	    0x20
#define DMA_START	0x24

bool magicbox_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
    switch (addr) {
    case DMA_SADDR:
        memcpy(bytes, &dma_src, len);
        break;
    case DMA_DADDR:
        memcpy(bytes, &dma_dest, len);
        break;
    case DMA_LEN:
        memcpy(bytes, &dma_len, len);
        break;
    default:
        return false;
    }
    return true;
}

size_t magicbox_t::type2MIPBits(uint32_t type) {
    switch(type) {
    case 0:
        return MIP_MSIP;
    case 1:
        return MIP_MTIP;
    case 2:
        return MIP_MEIP;
    case 3:
        return MIP_SSIP;
    case 4:
        return MIP_STIP;
    case 5:
        return MIP_SEIP;
    default:
        return 0;
    }
}

void magicbox_t::tick(reg_t rtc_ticks) {
    for (auto& [id, context]: contexts) {
        if (int_trigger & (1 << id)) {
            for (auto int_info = context->int_info.begin(); int_info < context->int_info.end();) {
                if(int_info->sche <= rtc_ticks) {
                    context->proc->get_state()->mip->backdoor_write_with_mask(int_info->type,
                                                                              int_info->type);
                    int_info = context->int_info.erase(int_info);
                } else {
                    int_info->sche -= rtc_ticks;
                    int_info++;
                }
            }
        } else {
            context->int_info.clear();
        }
    }
}

bool magicbox_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
    switch (addr) {
    case LOG_REG:
        std::cout << bytes[0];
        break;
    case INT_TRIGGER:
        int_trigger = *(uint32_t *)bytes;
        tick(0);
        break;
    case INT_TRIGGER_INFO: {
            uint32_t val = *(uint32_t *)bytes;
            uint32_t type = type2MIPBits((val >> 12) & 0x7);
            uint32_t sche = val & 0x0FFF;
            size_t core = (val >> 15) & 0x01F;
            if (contexts[core] != NULL && type != 0) {
                contexts[core]->int_info.push_back(int_trigger_t(type, sche));
            }
            tick(0);
        }
        break;
    case INT_CLEAR_INFO: {
            uint32_t val = *(uint32_t *)bytes;
            uint32_t type = val & 0x7;
            size_t core = (val >> 3) & 0x01F;
            if (contexts[core] != NULL) {
                contexts[core]->proc->get_state()->mip->backdoor_write_with_mask(type2MIPBits(type), 0);
            }
        }
        break;
    case DMA_SADDR:
        memcpy(&dma_src, bytes, len);
        break;
    case DMA_DADDR:
        memcpy(&dma_dest, bytes, len);
        break;
    case DMA_LEN:
        memcpy(&dma_len, bytes, len);
        break;
    case DMA_START: {
            uint8_t* dma_bytes = (uint8_t*)malloc(dma_len);
            bus->load(dma_src, dma_len, dma_bytes);
            bus->store(dma_dest, dma_len, dma_bytes);
        }
        break;
    default:
        return false;
    }
    return true;
}

std::string magicbox_generate_dts(const sim_t* sim)
{
  std::stringstream s;
  s << std::hex
    << "    MAGICBOX0: magicbox@" << MAGICBOX_BASE << " {\n"
       "      compatible = \"magicboxa\";\n"
       "      clock-frequency = <" << std::dec << (sim->CPU_HZ/sim->INSNS_PER_RTC_TICK) << ">;\n"
       "      interrupt-parent = <&PLIC>;\n"
       "      interrupts = <" << std::dec << MAGICBOX_INTERRUPT_ID;
  reg_t magicboxbs = MAGICBOX_BASE;
  reg_t magicboxsz = MAGICBOX_SIZE;
  s << std::hex << ">;\n"
       "      reg = <0x" << (magicboxbs >> 32) << " 0x" << (magicboxbs & (uint32_t)-1) <<
                   " 0x" << (magicboxsz >> 32) << " 0x" << (magicboxsz & (uint32_t)-1) << ">;\n"
       "    };\n";
  return s.str();
}

magicbox_t* magicbox_parse_from_fdt(const void* fdt, const sim_t* sim, reg_t* base)
{
  uint32_t magicbox_shift, magicbox_io_width;
  if (fdt_parse_magicbox(fdt, base,
                        "magicboxa") == 0) {
    bus_t *bus=sim->get_bus();
    return new magicbox_t(sim, bus);
  } else {
    return nullptr;
  }
}

REGISTER_DEVICE(magicbox, magicbox_parse_from_fdt, magicbox_generate_dts)
