#ifndef USB_H
#define USB_H

#include <CPS4042/Hardwares/Board.h>
#include <CPS4042/Protocols/Protocol.h>
#include <CPS4042/Units/BaudRate.h>
#include <CPS4042/Wires/Pin.h>
#include <boost/pfr.hpp>

namespace Sensors
{

using UsbVoltage = VoltageLevel3_3v;

template <BaudRate BR, BitRate BTR, typename WorkingVoltageTp>
requires std::is_base_of_v<AbstractVoltageLevel, WorkingVoltageTp>
struct UsbGpio
{
public:
    Pins::Vdd<WorkingVoltageTp> vdd {BR, BTR, "Usb::vdd"};    // Pin 0
    Pins::Gnd<WorkingVoltageTp> gnd {BR, BTR, "Usb::gnd"};    // Pin 1
    Pins::Rx<WorkingVoltageTp>  rx {BR, BTR, "Usb::rx"};      // Pin 2
    Pins::Tx<WorkingVoltageTp>  tx {BR, BTR, "Usb::tx"};      // Pin 3
};

class Usb : public Board<BaudRates::NotSpecified,
                         BitRates::same(BaudRates::NotSpecified),
                         Frequency::F320khz, UsbVoltage, UsbGpio>
{
public:
    explicit Usb() :
        Parent {"Usb::processor"}
    {
        m_processor->installProtocol(&usart);
        std::cout << "one instance of Usb" << " created." << std::endl;
    }

    class USART : public Protocols::AbstractUsart<Usb, Gpio>
    {
    public:
        explicit USART(Usb* b) :
            Protocols::AbstractUsart<Usb, Gpio> {b}
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
            gpio.tx.setCanRead(false); //TODO: check tx if this is correct

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

protected:
    inline void
    startModule() override
    {}
};

}    // namespace Sensors

#endif    // USB_H
