#pragma once

#include <coco/BufferDevice.hpp>
#include <coco/enum.hpp>


namespace coco {

/// @brief Interface for ARM Serial Wire Debug (SWD) protocol
/// https://developer.arm.com/documentation/ddi0413/c/debug-access-port/sw-dp/protocol-description
class SwdDevice : public BufferDevice {
public:
    /// @brief Buffer header type
    ///
    enum class HeaderType {
        // header contains a register designated by swd::Register
        REGISTER = 0,

        // header contains an address for Register::TAR, data gets read/writte to Register::DRW assuming bank 0
        ADDRESS = 1,
    };

    SwdDevice(State state) : BufferDevice(state) {}

    virtual ~SwdDevice() {}

    /// @brief Emit reset sequence after all pending transfers.
    /// Reset sequence is 50+ cycles of SWCLK with SWDIO high, switch from JTAG to SWD, 50+ cycles with SWDIO high, 2 clock cycles with SWDIO low
    virtual void reset() = 0;
};

} // namespace coco
