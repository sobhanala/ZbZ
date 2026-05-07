#ifndef ESP8266_H
#define ESP8266_H

#include <CPS4042/Hardwares/Board.h>
#include <CPS4042/Protocols/Protocol.h>
#include <CPS4042/Units/BaudRate.h>
#include <CPS4042/Wires/Pin.h>
#include <boost/pfr.hpp>
#include <queue>

namespace Boards
{

using Esp8266Voltage = VoltageLevel3_3v;

template <BaudRate BR, BitRate BTR, typename WorkingVoltageTp>
requires std::is_base_of_v<AbstractVoltageLevel, WorkingVoltageTp>
struct Esp8266Gpio
{
public:
    Pins::Vdd<WorkingVoltageTp>     vdd1 {BR, BTR, "Esp8266::vdd1"};    // Pin 0
    Pins::Gnd<WorkingVoltageTp>     gnd1 {BR, BTR, "Esp8266::gnd1"};    // Pin 1

    Pins::Vdd<WorkingVoltageTp>     vdd2 {BR, BTR, "Esp8266::vdd2"};    // Pin 2
    Pins::Gnd<WorkingVoltageTp>     gnd2 {BR, BTR, "Esp8266::gnd2"};    // Pin 3

    Pins::Vdd<WorkingVoltageTp>     vdd3 {BR, BTR, "Esp8266::vdd3"};    // Pin 4
    Pins::Gnd<WorkingVoltageTp>     gnd3 {BR, BTR, "Esp8266::gnd3"};    // Pin 5

    Pins::Rx<WorkingVoltageTp>      rx {BR, BTR, "Esp8266::rx"};        // Pin 6
    Pins::Tx<WorkingVoltageTp>      tx {BR, BTR, "Esp8266::tx"};        // Pin 7

    Pins::Sda<WorkingVoltageTp>     sda {BR, BTR, "Esp8266::sda"};      // Pin 8
    Pins::Scl<WorkingVoltageTp>     scl {BR, BTR, "Esp8266::scl"};      // Pin 9

    Pins::Digital<WorkingVoltageTp> d0 {BR, BTR, "Esp8266::d0"};    // Pin 10
    Pins::Digital<WorkingVoltageTp> d1 {BR, BTR, "Esp8266::d1"};    // Pin 11
    Pins::Digital<WorkingVoltageTp> d2 {BR, BTR, "Esp8266::d2"};    // Pin 12
    Pins::Digital<WorkingVoltageTp> d3 {BR, BTR, "Esp8266::d3"};    // Pin 13
    Pins::Digital<WorkingVoltageTp> d4 {BR, BTR, "Esp8266::d4"};    // Pin 14
    Pins::Digital<WorkingVoltageTp> d5 {BR, BTR, "Esp8266::d5"};    // Pin 15
    Pins::Analog<WorkingVoltageTp>  a0 {BR, BTR, "Esp8266::a1"};    // Pin 16
};

class Esp8266
    : public Board<BaudRates::B115200, BitRates::same(BaudRates::B115200),
                   Frequency::F320khz, Esp8266Voltage, Esp8266Gpio>
{
public:
    explicit Esp8266() :
        Parent {"Esp8266::Processor"}
    {
        m_processor->communicationClockChanged.connect(
          [this](Bit edge) { m_gpio.scl.nextEdge(edge); });

        m_processor->installProtocol(&i2c);
        m_processor->installProtocol(&usart);

        std::cout << "one instance of Esp8266" << " created." << std::endl;
    };

    class I2C : public Protocols::AbstractI2C<Esp8266, Gpio>
    {
    public:
        explicit I2C(Esp8266* b) :
            Protocols::AbstractI2C<Esp8266, Gpio> {b}
        {}

        void
        init(Byte address) override
        {
            m_targetAddress = address;
            m_started       = false;
            m_addressSent   = false;
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
            if(!m_addressSent)
            {
                gpio.sda.write(m_targetAddress);
                m_addressSent = true;
                return;
            }

            if(!this->m_started)
            {
                if(gpio.sda.hasBitToRead())
                {
                    auto ack = gpio.sda.readBit();
                    if(ack == Bit::One)
                    {
                        this->m_started = true;
                    }
                    else
                    {
                        m_addressSent = false;
                    }
                }

                return;
            }

            while(gpio.sda.hasByteToRead())
            {
                auto incoming = gpio.sda.read();
                this->m_buffer.push(incoming);
            }

            while(!m_outbound.empty())
            {
                gpio.sda.write(m_outbound.front());
                m_outbound.pop();
            }
        }

    private:
        Byte             m_targetAddress {0};
        bool             m_addressSent {false};
        std::queue<Byte> m_outbound;

    } mutable i2c {this};

    class USART : public Protocols::AbstractUsart<Esp8266, Gpio>
    {
    public:
        explicit USART(Esp8266* b) :
            Protocols::AbstractUsart<Esp8266, Gpio> {b}
        {}

        void
        write(Byte byte) override
        {}

        Byte
        read() override
        {
            return 0;
        }

        void
        run(Gpio& gpio) override
        {}

    } mutable usart {this};

protected:
    inline void
    startModule() override
    {}
};
}    // namespace Boards

#endif    // ESP8266_H
