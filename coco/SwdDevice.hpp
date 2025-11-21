#pragma once

#include <coco/BufferDevice.hpp>
#include <coco/enum.hpp>


namespace coco {

/// @brief Interface for ARM Serial Wire Debug (SWD) protocol
/// https://developer.arm.com/documentation/ddi0413/c/debug-access-port/sw-dp/protocol-description
class SwdDevice : public BufferDevice {
public:

    enum class Request : uint32_t {
        DEBUG_PORT = 0,
        ACCESS_PORT = 1 << 1,
        PORT_MASK = 1 << 1,

        ADDRESS_0 = 0,
        ADDRESS_4 = 1 << 3,
        ADDRESS_8 = 2 << 3,
        ADDRESS_12 = 3 << 3,
        ADDRESS_MASK = 3 << 3,
    };

    SwdDevice(State state) : BufferDevice(state) {}

    virtual ~SwdDevice() {}

    /// @brief Emit reset sequence
    /// Emits 50 cycles of SWCLK with SWDIO high after all pending transfers
    virtual void reset() = 0;
};
COCO_ENUM(SwdDevice::Request)

} // namespace coco
