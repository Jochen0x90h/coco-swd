#include "SwdDevice_SPI.hpp"
#include <coco/debug.hpp>


namespace coco {

constexpr int SPI_CR2_DS_5BIT = SPI_CR2_DS_2 | SPI_CR2_FRXTH;
constexpr int SPI_CR2_DS_8BIT = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0 | SPI_CR2_FRXTH;
constexpr int SPI_CR2_DS_13BIT = SPI_CR2_DS_3 | SPI_CR2_DS_2;

// SwdDevice_SPI

SwdDevice_SPI::SwdDevice_SPI(Loop_Queue &loop, gpio::Config sckPin, gpio::Config misoPin, gpio::Config mosiPin,
    const spi::Info &spiInfo, spi::ClockConfig clockConfig)
    : SwdDevice(State::READY)
    , loop_(loop)
    , mosiPin_(mosiPin)
    , spiIrq_(spiInfo.irq)
{
    // configure pins (mosi is reconfigured to alternate function when writing)
    gpio::enableAlternate(sckPin);
    gpio::enableAlternate(misoPin);
    gpio::enableInput(mosiPin);

    spi_ = spiInfo.enableClock()
        .configureMaster(clockConfig,
            spi::Config::PHA0_POL0 | spi::Config::LSB_FIRST,
            spi::Format::DATA_8,
            spi::Interrupt::RX)
        .start();
}

int SwdDevice_SPI::getBufferCount() {
    return buffers_.count();
}

SwdDevice_SPI::BufferBase &SwdDevice_SPI::getBuffer(int index) {
    return buffers_.get(index);
}

void SwdDevice_SPI::reset() {
    nvic::Guard gurad(spiIrq_);
    if (!resetPending_) {
        resetPending_ = true;
        if (transfers_.empty())
            startReset();
    }
}

// called from interrupt
void SwdDevice_SPI::SPI_IRQHandler() {
    auto &RXDR8 = spi_.RXDR8();
    auto &RXDR16 = spi_.RXDR16();
    auto &TXDR8 = spi_.TXDR8();
    auto &TXDR16 = spi_.TXDR16();

    debug::toggleGreen();

    switch (phase_) {
    case Phase::REQUEST:
        {
            // read data register to clear RXNE flag
            uint8_t dummy = RXDR8;
            (void)dummy;

            // disable output
            gpio::enableInput(mosiPin_);

            // switch to ACK phase
            phase_ = Phase::ACK;

            // read 1 bit turnaround, 3 bit ACK, 1 bit turnaround or first read data bit
            spi_.setFormat(spi::Format::DATA_5);

            // dummy write to start read transfer
            TXDR8 = 0x0e;
        }
        break;
    case Phase::ACK:
        {
            // read data register (clears RXNE flag)
            uint32_t data = RXDR8;

            // extract ack field
            int ack = (data >> 1) & 7;

            if (!write_) {
                // read 33 bytes (31 data, 1 parity, 1 turnaround)

                // read is done in three phases (12 13 8)
                phase_ = Phase::READ_DATA1;

                // store first bit that was already read
                data_ = (data >> 4) & 1;

                // read first 12 bit
                spi_.setFormat(spi::Format::DATA_12);

                // start transfer
                TXDR16 = 0;
            } else {
                // write 34 bytes (32 data, 1 parity, 1 turnaround)

                // enable output
                gpio::enableAlternate(mosiPin_);

                // write is done in three phases (13 13 8 bit)
                phase_ = Phase::WRITE_DATA1;

                // write first 13 bit
                spi_.setFormat(spi::Format::DATA_13);

                // start transfer
                data = data_;
                TXDR16 = data;
                data_ = (data >> 13) | (parity(data) << 19);
            }
        }
        break;
    case Phase::READ_DATA1:
        {
            // get 12 bit
            data_ |= (RXDR16 & 0xfff) << 1;

            // next data phase
            phase_ = Phase::WRITE_DATA2;

            // read next 13 bit
            spi_.setFormat(spi::Format::DATA_13);

            // start transfer
            TXDR16 = 0;
        }
        break;
    case Phase::READ_DATA2:
        {
            // get 13 bit
            data_ |= (RXDR16 & 0x1fff) << 13;

            // next data phase
            phase_ = Phase::WRITE_DATA3;

            // read remaining 8 bit
            spi_.setFormat(spi::Format::DATA_8);

            // start transfer
            TXDR8 = 0;
        }
        break;
    case Phase::READ_DATA3:
        {
            // get remaining 8 bit
            data_ |= RXDR8 << 24;

            // return to request phase
            phase_ = Phase::REQUEST;

            // end of transfer
            int r = transfers_.pop(
                [this](BufferBase &buffer) {
                    // notify app that buffer has finished
                    loop_.push(buffer);
                    return true;
                },
                [](BufferBase &next) {
                    // start next buffer
                    next.start();
                }
            );
            if (r != 2 && resetPending_) {
                startReset();
            }
        }
        break;
    case Phase::WRITE_DATA1:
        {
            // read data register to clear RXNE flag
            int dummy = RXDR16;
            (void)dummy;

            // next data phase
            phase_ = Phase::WRITE_DATA2;

            // write next 13 bit
            // start transfer
            uint32_t data = data_;
            TXDR16 = data;
            data_ = (data >> 15);
        }
        break;
    case Phase::WRITE_DATA2:
        {
            // read data register to clear RXNE flag
            int dummy = RXDR16;
            (void)dummy;

            // next data phase
            phase_ = Phase::WRITE_DATA3;

            // write remaining 8 bit
            spi_.setFormat(spi::Format::DATA_8);

            // start transfer
            uint32_t data = data_;
            TXDR8 = data;
        }
        break;
    case Phase::WRITE_DATA3:
        {
            // read data register to clear RXNE flag
            int dummy = RXDR8;
            (void)dummy;

            // return to request phase
            phase_ = Phase::REQUEST;

            // end of transfer
            int r = transfers_.pop(
                [this](BufferBase &buffer) {
                    // notify app that buffer has finished
                    loop_.push(buffer);
                    return true;
                },
                [](BufferBase &next) {
                    // start next buffer
                    next.start();
                }
            );
            if (r != 2 && resetPending_) {
                startReset();
            }
        }
        break;
    case Phase::RESET:
        {
            // read data register to clear RXNE flag
            int dummy = RXDR16;
            (void)dummy;

            int data = data_;
            if (data > 0) {
                // send more clock cycles
                data_ = data - 1;
                TXDR16 = 0;
            } else {
                // reset finished
                resetPending_ = false;

                // reset format and phase
                phase_ = Phase::REQUEST;
                spi_.setFormat(spi::Format::DATA_8);

                transfers_.visitFirst([](BufferBase &next) {
                    // start next buffer
                    next.start();
                });
            }
        }
        break;
    }
}

void SwdDevice_SPI::startReset() {
    auto &TXDR16 = spi_.TXDR16();

    phase_ = Phase::RESET;

    // start 50 clock cycles
    spi_.setFormat(spi::Format::DATA_16);

    // use as counter
    data_ = 3;

    TXDR16 = 0xffff;
}



// SwdDevice_SPI::BufferBase

SwdDevice_SPI::BufferBase::BufferBase(uint8_t *data, int capacity, SwdDevice_SPI &device)
    : coco::Buffer(data, 4, 0, capacity, BufferBase::State::READY), device_(device)
{
    device.buffers_.add(*this);
}

SwdDevice_SPI::BufferBase::~BufferBase() {
}

bool SwdDevice_SPI::BufferBase::start(Op op) {
    if (st.state != State::READY) {
        assert(st.state != State::BUSY);
        return false;
    }

    // check if READ or WRITE flag is set
    assert((op & Op::READ_WRITE) != 0);

    op_ = op;
    auto &device = device_;

    // add to list of pending transfers and start immediately if list was empty
    if (device.transfers_.push(nvic::Guard(device.spiIrq_), *this)) {
        if (!device.resetPending_)
            start();
    }

    // set state
    setBusy();

    return true;
}

bool SwdDevice_SPI::BufferBase::cancel() {
    if (st.state != State::BUSY)
        return false;
    auto &device = device_;

    // remove from pending transfers if not yet started, otherwise complete normally
    if (device.transfers_.remove(nvic::Guard(device.spiIrq_), *this, false)) {
        // cancel succeeded: set buffer ready again
        // resume application code, therefore interrupt should be enabled at this point
        setReady(0);
    }

    return true;
}

void SwdDevice_SPI::BufferBase::start() {
    auto &device = device_;
    auto &TXDR8 = device.spi_.TXDR8();

    // enable output
    gpio::enableAlternate(device.mosiPin_);

    // create request byte
    bool write = device.write_ = (op_ & Op::WRITE) != 0;
    uint8_t request = 1 // start
        | (header_[0] & uint8_t(SwdDevice::Request::PORT_MASK | SwdDevice::Request::ADDRESS_MASK)) // port and address
        | (write ? 0 : 4) // read/write
        | 0x80; // park

    // parity
    request |= ((request << 4) ^ (request << 3) ^ (request << 2) ^ (request << 1)) & (1 << 5);

    // get data to write if it is a write request
    if (write) {
        device.data_ = *(uint32_t *)(data_);
    }

    // start write
    TXDR8 = request;

    // -> wait for SPI interrupt
    debug::out << "start\n";
}

void SwdDevice_SPI::BufferBase::handle() {
    setReady();
}

} // namespace coco
