#ifndef MICROCONTROLLER_H
#define MICROCONTROLLER_H

#include <CPS4042/Hardwares/Boards/Esp8266.h>
#include <CPS4042/Hardwares/Sensors/VL530X.h>
#include <CPS4042/Sketchs/AbstractSketch.h>

class MicroController : public AbstractSketch<Boards::Esp8266>
{
public:
    explicit MicroController(Boards::Esp8266* node) :
        AbstractSketch<Boards::Esp8266> {node}
    {}

    std::int32_t
    setup(Boards::Esp8266::Gpio& gpio) override
    {
        (void)gpio;
        std::cout << "esp8266 setup completed." << std::endl;
        return 0;
    }

    std::int32_t
    loop(Boards::Esp8266::Gpio& gpio) override
    {
        (void)gpio;
        static UByte nextAddress {0};
        static bool  waitingForResponse {false};
        static UByte lastRequest {0};

        if(!waitingForResponse)
        {
            auto request = static_cast<Byte>(nextAddress);
            node()->usart.write(request);
            lastRequest = nextAddress;
            std::cout << "[USART] request address: 0x" << std::hex
                      << static_cast<int>(lastRequest) << std::dec << std::endl;

            waitingForResponse = true;
        }

        while(node()->usart.isDataAvailable())
        {
            auto data = static_cast<UByte>(node()->usart.read());
            std::cout << "[USART] response address 0x" << std::hex
                      << static_cast<int>(lastRequest) << " -> data 0x"
                      << static_cast<int>(data) << std::dec << std::endl;

            nextAddress++;    waitingForResponse = false;
        
        }

        delay(10);
        return 0;
    }
};


#endif    // MICROCONTROLLER_H
