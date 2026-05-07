#ifndef VL53_X_H
#define VL53_X_H

#include <CPS4042/Hardwares/Board.h>
#include <CPS4042/Units/BaudRate.h>
#include <CPS4042/Wires/Pin.h>
#include <boost/pfr.hpp>
#include <iostream>
#include <queue>

namespace Sensors
{
using Vl530xVoltage = VoltageLevel3_3v;

template <std::uint64_t bar, std::uint64_t btr, typename WorkingVoltageTp>
requires std::is_base_of_v<AbstractVoltageLevel, WorkingVoltageTp>
struct Vl530xGpio
{
public:
    Pins::Vdd<WorkingVoltageTp> vdd {bar, btr, "Vl530x::vdd"};    // Pin 0
    Pins::Gnd<WorkingVoltageTp> gnd {bar, btr, "Vl530x::gnd"};    // Pin 1
    Pins::Sda<WorkingVoltageTp> sda {bar, btr, "Vl530x::sda"};    // Pin 2
    Pins::Scl<WorkingVoltageTp> scl {bar, btr, "Vl530x::scl"};    // Pin 3
};

class Vl530x : public Board<BaudRates::NotSpecified,
                            BitRates::same(BaudRates::NotSpecified),
                            Frequency::Drived, Vl530xVoltage, Vl530xGpio>
{
public:
    inline static constexpr Byte address = 0x29;

    explicit Vl530x() :
        Parent {"Vl530x::processor"}
    {
        m_processor->installProtocol(&i2c);

        std::cout << "one instance of Vl530x" << " created." << std::endl;
    }

    class I2C : public Protocols::AbstractI2C<Vl530x, Gpio>
    {

    public:
        explicit I2C(Vl530x* b) :
            Protocols::AbstractI2C<Vl530x, Gpio> {b}
        {}

        void
        init(Byte address) override
        {
            (void)address;
            this->m_started = false;
        }

        void
        write(Byte byte) override
        {
            m_outbound.push(byte);
        }

        Byte
        read() override
        {
            if(this->m_buffer.empty()) return 0;

            auto byte = this->m_buffer.front();
            this->m_buffer.pop();
            return byte;
        }

        void
        run(Gpio& gpio) override
        {
            if(!this->m_started)
            {
                if(gpio.sda.hasByteToRead())
                {
                    auto address = gpio.sda.read();
                    if(address == static_cast<Byte>(static_cast<UByte>(Vl530x::address) << 1))
                    {
                        gpio.sda.write(Bit::One);
                        this->m_started = true;
                    }
                }

                return;
            }

            while(gpio.sda.hasByteToRead())
            {
                this->m_buffer.push(gpio.sda.read());
            }

            while(!m_outbound.empty())
            {
                gpio.sda.write(m_outbound.front());
                m_outbound.pop();
            }
        }

    private:
        std::queue<Byte> m_outbound;

    } mutable i2c {this};

protected:
    inline void
    startModule() override
    {
        m_gpio.scl.onNextEdge([this](Vl530xVoltage level) {
            auto bit = Voltage::toBit(level);

            if(bit == Bit::One)    // positive edge
            {
                m_processor->nextCycle(m_gpio);
            }
        });
    }
};

}    // namespace Sensors

#endif    // VL53_X_H
