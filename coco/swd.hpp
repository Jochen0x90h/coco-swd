#pragma once

#include <coco/enum.hpp>


namespace coco {
namespace swd {

enum class Register : uint32_t {
    DEBUG_PORT = 0,
    ACCESS_PORT = 1 << 1,
    PORT_MASK = 1 << 1,

    ADDRESS_0 = 0,
    ADDRESS_4 = 1 << 3,
    ADDRESS_8 = 2 << 3,
    ADDRESS_C = 3 << 3,
    ADDRESS_MASK = 3 << 3,


    // identification code register, read only
    IDCODE = DEBUG_PORT | ADDRESS_0,

    // control/status register, read/write (6.2.3)
    CTRL_STAT = DEBUG_PORT | ADDRESS_4,

    // access port select register, read only
    SELECT = DEBUG_PORT | ADDRESS_8,

    // read buffer, read only
    RDBUFF = DEBUG_PORT | ADDRESS_C,


    // access port bank 0 registers

    // control/status word register
    CSW = ACCESS_PORT | ADDRESS_0,

    // transfer address register
    TAR = ACCESS_PORT | ADDRESS_4,

    // data read/write register
    DRW = ACCESS_PORT | ADDRESS_C,
};
COCO_ENUM(Register)


/// @brief Fields of the Control/Status Word register (CSW)
/// ARM Debug Interface v5: 11.2.1 Control/Status Word (CSW) Register
/// https://developer.arm.com/documentation/ddi0316/d/functional-description/apb-ap/apb-ap-control-status-word-register--csw--0x00?lang=en
enum class ControlStatusWord : uint32_t {
  //  STM_SPECIFIC = 0x23000000,

    // https://github.com/openocd-org/openocd/blob/461af9b3abd1aeb94f047f990074b7e5d954fbea/src/target/arm_adi_v5.h#L178
    SPIDEN = 1 << 23,
    DBGSWENABLE = 1u << 31,

    // AHB: Privileged
    AHB_HPROT1 = 1 << 25,

    // AHB: set HMASTER signals to AHB-AP ID
    AHB_MASTER_DEBUG = 1 << 29,

    // AHB5: non-secure access via HNONSEC
    AHB_SPROT = 1 << 30,

    AHB_DEFAULT = AHB_HPROT1 | AHB_MASTER_DEBUG | DBGSWENABLE,


    // AXI: Privileged
    AXI_ARPROT0_PRIV = 1 << 28,

    // AXI: Non-secure
    AXI_ARPROT1_NONSEC = 1 << 29,

    AXI_DEFAULT = AXI_ARPROT0_PRIV | AXI_ARPROT1_NONSEC | DBGSWENABLE,


    APB_DEFAULT = DBGSWENABLE,


    DEVICE_ENABLE = 1 << 6,

    INCREMENT_OFF = 0,
    INCREMENT_SINGLE = 1 << 4,
    INCREMENT_PACKED = 2 << 4,

    SIZE_8 = 0,
    SIZE_16 = 1,
    SIZE_32 = 2,
};
COCO_ENUM(ControlStatusWord)


// list of ID codes
// https://github.com/stlink-org/stlink/issues/903
constexpr uint32_t ID_M0P = 0x0bc11477; // Arm Cortex M0+
constexpr uint32_t ID_M4F = 0x2ba01477; // Arm Cortex M4F

} // namespace swd
} // namespace coco
