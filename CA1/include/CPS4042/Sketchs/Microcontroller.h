#ifndef MICROCONTROLLER_H
#define MICROCONTROLLER_H

#include <CPS4042/Hardwares/Boards/Esp8266.h>
#include <CPS4042/Sketchs/AbstractSketch.h>
#include <CPS4042/Utils/ByteStream.h>
#include <deque>

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
        static UByte            nextChannel {0};
        static bool             waitingForResponse {false};
        static UByte            requestedChannel {0};
        static std::deque<Byte> frameBuffer;

        if(!waitingForResponse)
        {
            requestedChannel = nextChannel % 2;
            node()->usart.write(static_cast<Byte>(requestedChannel));
            std::cout << "[MUX] request channel: " << static_cast<int>(requestedChannel)
                      << std::endl;

            waitingForResponse = true;
        }

        while(node()->usart.isDataAvailable())
        {
            frameBuffer.push_back(node()->usart.read());
        }

        if(waitingForResponse && frameBuffer.size() >= 3)
        {
            auto msb      = frameBuffer[0];
            auto lsb      = frameBuffer[1];
            auto checksum = frameBuffer[2];

            auto msbUnsigned = static_cast<UByte>(msb);
            auto lsbUnsigned = static_cast<UByte>(lsb);
            auto expectedChecksum =
              static_cast<Byte>(msbUnsigned > lsbUnsigned
                                  ? (msbUnsigned - lsbUnsigned)
                                  : (lsbUnsigned - msbUnsigned));

            if(checksum == expectedChecksum)
            {
                ByteStream<std::uint16_t> stream;
                stream << msb;
                stream << lsb;
                auto value = stream.take();

                std::cout << "[MUX] channel " << static_cast<int>(requestedChannel)
                          << " value: " << value << std::endl;
            }
            else
            {
                std::cout << "[MUX] channel " << static_cast<int>(requestedChannel)
                          << " invalid checksum." << std::endl;
            }

            frameBuffer.pop_front();
            frameBuffer.pop_front();
            frameBuffer.pop_front();

            waitingForResponse = false;
            nextChannel++;
        }

        delay(10);
        return 0;
    }
};


#endif    // MICROCONTROLLER_H
