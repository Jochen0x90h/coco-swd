#include <coco/convert.hpp>
#include <coco/debug.hpp>
#include <SwdDeviceTest.hpp>
#include <swd.hpp>


using namespace coco;

template <int N>
struct FixedArray {
    uint32_t data[N];
};

Coroutine write(Loop &loop, SwdDevice &swd, Buffer &buffer) {
    while (buffer.ready()) {
        //debug::toggleGreen();
        co_await loop.sleep(1s);

        debug::out << "reset\n";
        swd.reset();

        //continue;

        // read IDCODE
        //debug::out << "read IDCODE\n";
        buffer.header<swd::Register>() = swd::Register::IDCODE;
        co_await buffer.read(4);
        if (buffer.error()) {
            // print error message
            debug::out << buffer.error().message() << '\n';
            continue;
        }
        if (buffer.cast<uint32_t>() != swd::ID_M0P) {
            debug::out << "error: unknown id " << hex(buffer.cast<uint32_t>()) << '\n';
            continue;
        }
        //debug::out << "id " << hex(buffer.cast<uint32_t>()) << '\n';

        // read IDCODE again
        buffer.header<swd::Register>() = swd::Register::IDCODE;
        co_await buffer.read(4);
        if (buffer.cast<uint32_t>() != swd::ID_M0P) {
            debug::out << "error: unknown id2 " << hex(buffer.cast<uint32_t>()) << '\n';
            continue;
        }
        //debug::out << "id2 " << hex(buffer.cast<uint32_t>()) << '\n';

        // read ctrl/stat (always allowed)
        buffer.header<swd::Register>() = swd::Register::CTRL_STAT;
        co_await buffer.read(4);
        debug::out << "ctrl/stat " << hex(buffer.cast<uint32_t>()) << '\n';

        // select bank 0
        buffer.header<swd::Register>() = swd::Register::SELECT;
        buffer.cast<uint32_t &>() = 0;
        co_await buffer.write(4);
        if (buffer.error()) {
            // print error message
            debug::out << buffer.error().message() << " while bank select\n";
            continue;
        }

        // power-up request
        buffer.header<swd::Register>() = swd::Register::CTRL_STAT;
        buffer.cast<uint32_t &>() = 0x50000000;
        co_await buffer.write(4);
        debug::sleep(10us);
        co_await buffer.read(4);
        if ((buffer.cast<uint32_t &>() & 0xf0000000) != 0xf0000000) {
            debug::out << "error: power up failed\n";
            continue;
        }

    //debug::out << "power up " << hex(buffer.value<uint32_t>()) << '\n';

        // select 0xf0
        //buffer.header<swd::Register>() = swd::Register::SELECT;
        //co_await buffer.writeValue<uint32_t>(0xf0);



        // select bank 0
        buffer.header<swd::Register>() = swd::Register::SELECT;
        buffer.cast<uint32_t &>() = 0;
        co_await buffer.write(4);

        //debug::sleep(100us);

        // read csw
        //buffer.header<swd::Register>() = swd::Register::CSW;
        //co_await buffer.read(4);
        //debug::out << "csw " << hex(buffer.cast<uint32_t>()) << '\n';

        // configure
        buffer.header<swd::Register>() = swd::Register::CSW;
        //buffer.cast<uint32_t &>() = 0x23000012; // st-link
        buffer.cast<uint32_t &>() = 0x23000002; // st-link no auto-increment
        co_await buffer.write(4);
        //co_await buffer.writeValue<swd::ControlStatusWord>(swd::ControlStatusWord::AHB_DEFAULT
        //    | swd::ControlStatusWord::DEVICE_ENABLE | swd::ControlStatusWord::INCREMENT_OFF
        //    | swd::ControlStatusWord::SIZE_32);

        // read csw
        buffer.header<swd::Register>() = swd::Register::CSW;
        co_await buffer.read(4);
        debug::out << "csw " << hex(buffer.cast<uint32_t>()) << '\n';


        /*// get device id
        buffer.header<swd::Register>() = swd::Register::TAR;
        buffer.cast<uint32_t &>() = 0x1FFF7590UL;
        co_await buffer.write(4);
        buffer.header<swd::Register>() = swd::Register::DRW;
        co_await buffer.read(4);
        debug::out << "device " << hex(buffer.value<uint32_t>()) << '\n';
        */

        // read flash
        /*buffer.header<swd::Register>() = swd::Register::TAR;
        buffer.cast<uint32_t &>() = 0x08000000;
        co_await buffer.write(4);

        for (int i = 0; i < 16; ++i) {
            buffer.header<swd::Register>() = swd::Register::DRW;
            co_await buffer.read(4);
            debug::out << hex(buffer.cast<uint32_t>()) << ' ';
        }
*/

        // check lock (read FLASH->CR)
        buffer.header<swd::Register>() = swd::Register::TAR;
        buffer.cast<uint32_t &>() = 0x40022014;
        co_await buffer.write(4);
        buffer.header<swd::Register>() = swd::Register::DRW;
        co_await buffer.read(4);
        if (buffer.error()) {
            // print error message
            debug::out << buffer.error().message() << " while reading FLASH->CR\n";
            continue;
        }
        uint32_t flashCR = buffer.cast<uint32_t>();
        debug::out << "pre FLASH->CR " << hex(flashCR) << '\n';
        if (flashCR & 0x80000000) {

            // read ctrl/stat (always allowed)
            buffer.header<swd::Register>() = swd::Register::CTRL_STAT;
            co_await buffer.read(4);
            debug::out << "ctrl/stat2 " << hex(buffer.cast<uint32_t>()) << '\n';

            // unlock (write keys to FLASH->KEYR)
            buffer.header<swd::Register>() = swd::Register::TAR;
            buffer.cast<uint32_t &>() = 0x40022008;
            co_await buffer.write(4);
            buffer.header<swd::Register>() = swd::Register::DRW;
            buffer.cast<FixedArray<2> &>() = {0x45670123, 0xCDEF89AB};
            co_await buffer.write(8);
            if (buffer.error()) {
                // print error message
                debug::out << buffer.error().message() << " while unlocking\n";
                continue;
            }
            debug::out << "unlocked\n";
            debug::sleep(10us);

            // check lock (read FLASH->CR)
            buffer.header<swd::Register>() = swd::Register::TAR;
            buffer.cast<uint32_t &>() = 0x40022014;
            co_await buffer.write(4);
            buffer.header<swd::Register>() = swd::Register::DRW;
            co_await buffer.read(4);
            if (buffer.error()) {
                // print error message
                debug::out << buffer.error().message() << " while reading FLASH->CR\n";
                continue;
            }
            debug::out << "FLASH->CR " << hex(buffer.cast<uint32_t>()) << '\n';
        }
    }
}


int main() {
    debug::out << "SwdDeviceTest\n";

    write(drivers.loop, drivers.swd, drivers.buffer1);

    drivers.loop.run();
    return 0;
}
