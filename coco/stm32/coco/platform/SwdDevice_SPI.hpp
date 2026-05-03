#pragma once

#include <coco/SwdDevice.hpp>
#include <coco/align.hpp>
#include <coco/swd.hpp>
#include <coco/platform/Loop_Queue.hpp>
#include <coco/platform/gpio.hpp>
#include <coco/platform/spi.hpp>
#include <coco/platform/nvic.hpp>
#include <coco/InterruptQueue.hpp>


namespace coco {

/// @brief Implementation of SWD for STM32
/// https://qcentlabs.com/posts/swd_banger/
class SwdDevice_SPI : public SwdDevice {
public:
    /// @brief Constructor for the SWD device.
    /// @param loop Event loop
    /// @param sckPin Clock pin, port and alternate function (see data sheet), connect to SWCLK
    /// @param misoPin Data input pin, port and alternate function (see data sheet), connect to SWDIO
    /// @param mosiPin Data output pin, port and alternate function (see data sheet), connect to SWDIO
    /// @param spiInfo Info of SPI instance to use
    /// @param prescaler SPI prescaler
    SwdDevice_SPI(Loop_Queue &loop, gpio::Config sckPin, gpio::Config misoPin, gpio::Config mosiPin,
        const spi::Info &spiInfo, spi::Format prescaler);


    // internal buffer base class, derives from IntrusiveListNode for the list of buffers and Loop_Queue::Handler to be notified from the event loop
    class BufferBase : public coco::Buffer, public IntrusiveListNode, public Loop_Queue::Handler {
        friend class SwdDevice_SPI;
    public:
        /// @brief Constructor.
        /// @param data Data of the buffer
        /// @param capacity Capacity of the buffer
        /// @param device Device
        BufferBase(uint8_t *data, int capacity, SwdDevice_SPI &device);
        ~BufferBase() override;

        // Buffer methods
        bool start() override;
        bool cancel() override;

    protected:
        void handle() override;

        SwdDevice_SPI &device_;
    };

    /// @brief Buffer for transferring data to/from a SWD device.
    /// @tparam C capacity of buffer
    template <int C>
    class Buffer : public BufferBase {
    public:
        Buffer(SwdDevice_SPI &device) : BufferBase(data_, C, device) {}

    protected:
        // header of size 4
        alignas(4) uint8_t data_[4 + C];
    };


    // BufferDevice methods
    int getBufferCount();
    BufferBase &getBuffer(int index);

    // SwdDevice methods
    void reset() override;

    /// @brief Call from SPI interrupt handler.
    /// See startup_stm32XXX.c, e.g. SPI1_IRQHandler()
    void SPI_IRQHandler();

protected:
    // send reset sequence and switch to SWD
    void startReset();

    // transfer first 32 bit word
    void transferFirst(BufferBase &buffer) {
        bool write = write_ = (buffer.op_ & BufferBase::Op::WRITE) != 0;
        auto reg = swd::Register(buffer.header_[0]);

        index_ = (!write && (reg & swd::Register::PORT_MASK) == swd::Register::ACCESS_PORT) ? -1 : 0;
        transferNext(buffer);
    }

    // transfer next 32 bit words
    bool transferNext(BufferBase &buffer);

    Loop_Queue &loop_;
    gpio::Config mosiPin_;

    // spi
    spi::Instance spi_;
    int spiIrq_;

    bool resetPending_ = false;
    bool write_;
    uint32_t data_;
    int index_;

    // state machine
    enum class Phase {
        REQUEST,
        ACK,
        WRITE_DATA1,
        WRITE_DATA2,
        WRITE_DATA3,
        READ_DATA1,
        READ_DATA2,
        READ_DATA3,
        RESET
    };
    Phase phase_;

    // list of buffers
    IntrusiveList<BufferBase> buffers_;

    // list of active transfers
    InterruptQueue<BufferBase> transfers_;
};

} // namespace coco
