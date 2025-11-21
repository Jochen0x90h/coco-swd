#include <coco/debug.hpp>
#include <SwdDeviceTest.hpp>


using namespace coco;

const uint32_t swdWriteData[] = {0x5500ffaa};

Coroutine write(Loop &loop, SwdDevice &swd, Buffer &buffer) {
    while (buffer.ready()) {
        //debug::toggleGreen();
        buffer.header<SwdDevice::Request>() = SwdDevice::Request::DEBUG_PORT | SwdDevice::Request::ADDRESS_0;
        swd.reset();
        co_await buffer.writeArray(swdWriteData);

        co_await loop.sleep(1s);
    }
}


int main() {
    debug::out << "SwdDeviceTest\n";

    write(drivers.loop, drivers.swd, drivers.buffer1);

    drivers.loop.run();
    return 0;
}
