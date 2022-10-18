#include "processor.h"
#include "devices.h"
#include "sim.h"
#include "dts.h"
#include <cstdlib>

magicbox_t::magicbox_t(bus_t *bus, abstract_interrupt_controller_t *intctrl)
  : bus(bus), intctrl(intctrl),
  dma_src(0), dma_dest(0), dma_len(0)
{
}

#define LOG_REG	    0x0
#define DMA_SADDR	0x8
#define DMA_DADDR	0x10
#define DMA_LEN	    0x18
#define DMA_START	0x20
#define TRIGGER_EVENT   0x32

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

bool magicbox_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
    switch (addr) {
    case LOG_REG:
        std::cout << bytes[0];
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
    case TRIGGER_EVENT:
        intctrl->set_interrupt_level(bytes[0], 1);
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
    abstract_interrupt_controller_t* intctrl = sim->get_intctrl();
    bus_t *bus=sim->get_bus();
    return new magicbox_t(bus, intctrl);
  } else {
    return nullptr;
  }
}

REGISTER_DEVICE(magicbox, magicbox_parse_from_fdt, magicbox_generate_dts)
