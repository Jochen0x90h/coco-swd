#include <coco/convert.hpp>
#include <coco/debug.hpp>
#include <SwdDeviceTest.hpp>


using namespace coco;

namespace swd {

// Arm Cortex M0+
constexpr uint32_t ID_M0P = 0x0bc11477;

}

Coroutine write(Loop &loop, SwdDevice &swd, Buffer &buffer) {
    while (buffer.ready()) {
        //debug::toggleGreen();

        swd.reset();

        buffer.header<SwdDevice::Request>() = SwdDevice::Request::DEBUG_PORT | SwdDevice::Request::ADDRESS_0;
        co_await buffer.read();

        if (buffer.result() != Buffer::Result::SUCCESS) {
            debug::out << "parity error\n";
        } else if (buffer.value<uint32_t>() != swd::ID_M0P) {
            debug::out << "error: unknown id\n";
        } else {
            // SELECT: Bank 0
            buffer.header<SwdDevice::Request>() = SwdDevice::Request::DEBUG_PORT | SwdDevice::Request::ADDRESS_8;
            co_await buffer.writeValue<uint32_t>(0);

            // CTRL/STAT: Power-Up-Request
            buffer.header<SwdDevice::Request>() = SwdDevice::Request::DEBUG_PORT | SwdDevice::Request::ADDRESS_4;
            co_await buffer.writeValue<uint32_t>(0x50000000);
            debug::sleep(10us);
            co_await buffer.read();
            //debug::out << hex(buffer.value<uint32_t>()) << '\n';
            if ((buffer.value<uint32_t>() & 0xf0000000) == 0xf0000000) {
                debug::out << "success\n";
            } else {
                debug::out << "error: power up failed\n";
            }
        }

        co_await loop.sleep(1s);
    }
}


int main() {
    debug::out << "SwdDeviceTest\n";

    write(drivers.loop, drivers.swd, drivers.buffer1);

    drivers.loop.run();
    return 0;
}
