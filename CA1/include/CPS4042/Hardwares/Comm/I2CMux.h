#ifndef I2C_MUX_H
#define I2C_MUX_H

#include <CPS4042/Hardwares/Board.h>
#include <CPS4042/Protocols/Protocol.h>
#include <CPS4042/Units/BaudRate.h>
#include <CPS4042/Wires/Pin.h>
#include <boost/pfr.hpp>
#include <queue>

namespace Sensors
{

using I2CMuxVoltage = VoltageLevel3_3v;

template <BaudRate BR, BitRate BTR, typename WorkingVoltageTp>
requires std::is_base_of_v<AbstractVoltageLevel, WorkingVoltageTp>
struct I2CMuxGpio
{
public:
    Pins::Vdd<WorkingVoltageTp> vdd {BR, BTR, "I2CMux::vdd"};    // Pin 0
    Pins::Gnd<WorkingVoltageTp> gnd {BR, BTR, "I2CMux::gnd"};    // Pin 1

    Pins::Rx<WorkingVoltageTp>  rx {BR, BTR, "I2CMux::rx"};    // Pin 2
    Pins::Tx<WorkingVoltageTp>  tx {BR, BTR, "I2CMux::tx"};    // Pin 3

    Pins::Sda<WorkingVoltageTp> sda0 {BR, BTR, "I2CMux::sda0"};    // Pin 4
    Pins::Scl<WorkingVoltageTp> scl0 {BR, BTR, "I2CMux::scl0"};    // Pin 5

    Pins::Sda<WorkingVoltageTp> sda1 {BR, BTR, "I2CMux::sda1"};    // Pin 6
    Pins::Scl<WorkingVoltageTp> scl1 {BR, BTR, "I2CMux::scl1"};    // Pin 7
};

class I2CMux : public Board<BaudRates::NotSpecified,
                            BitRates::same(BaudRates::NotSpecified),
                            Frequency::F320khz, I2CMuxVoltage, I2CMuxGpio>
{
public:
    explicit I2CMux() :
        Parent {"I2CMux::processor"}
    {
        m_processor->communicationClockChanged.connect(
          [this](Bit edge) { m_gpio.scl0.nextEdge(edge); });
        m_processor->communicationClockChanged.connect(
          [this](Bit edge) { m_gpio.scl1.nextEdge(edge); });

        m_processor->installProtocol(&usart);
        m_processor->installProtocol(&i2c0);
        m_processor->installProtocol(&i2c1);

        std::cout << "one instance of I2CMux" << " created." << std::endl;
    }

    class USART : public Protocols::AbstractUsart<I2CMux, Gpio>
    {
    public:
        explicit USART(I2CMux* b) :
            Protocols::AbstractUsart<I2CMux, Gpio> {b}
        {}

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
            gpio.tx.setCanRead(false);

            while(gpio.rx.hasByteToRead())
            {
                this->m_buffer.push(gpio.rx.read());
            }

            while(!m_outbound.empty())
            {
                gpio.tx.write(m_outbound.front());
                m_outbound.pop();
            }
        }

    private:
        std::queue<Byte> m_outbound;
    } mutable usart {this};

    class I2C0 : public Protocols::AbstractI2C<I2CMux, Gpio>
    {
    public:
        explicit I2C0(I2CMux* b) :
            Protocols::AbstractI2C<I2CMux, Gpio> {b}
        {}

        void
        init(Byte address) override
        {
            m_targetAddress = address;
            this->m_started = false;
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
                gpio.sda0.write(m_targetAddress);
                m_addressSent = true;
                this->m_started = true;
                return;
            }

            while(gpio.sda0.hasByteToRead())
            {
                this->m_buffer.push(gpio.sda0.read());
            }

            while(!m_outbound.empty())
            {
                gpio.sda0.write(m_outbound.front());
                m_outbound.pop();
            }
        }

    private:
        Byte             m_targetAddress {0};
        bool             m_addressSent {false};
        std::queue<Byte> m_outbound;
    } mutable i2c0 {this};

    class I2C1 : public Protocols::AbstractI2C<I2CMux, Gpio>
    {
    public:
        explicit I2C1(I2CMux* b) :
            Protocols::AbstractI2C<I2CMux, Gpio> {b}
        {}

        void
        init(Byte address) override
        {
            m_targetAddress = address;
            this->m_started = false;
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
                gpio.sda1.write(m_targetAddress);
                m_addressSent = true;
                this->m_started = true;
                return;
            }

            while(gpio.sda1.hasByteToRead())
            {
                this->m_buffer.push(gpio.sda1.read());
            }

            while(!m_outbound.empty())
            {
                gpio.sda1.write(m_outbound.front());
                m_outbound.pop();
            }
        }

    private:
        Byte             m_targetAddress {0};
        bool             m_addressSent {false};
        std::queue<Byte> m_outbound;
    } mutable i2c1 {this};

protected:
    inline void
    startModule() override
    {}
};

}    // namespace Sensors

#endif    // I2C_MUX_H
