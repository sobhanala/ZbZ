#ifndef MICROCONTROLLER_H
#define MICROCONTROLLER_H

#include <CPS4042/Hardwares/Boards/Esp8266.h>
#include <CPS4042/Hardwares/Sensors/VL530X.h>
#include <CPS4042/Sketchs/AbstractSketch.h>
#include <CPS4042/Utils/ByteStream.h>
#include <CPS4042/Utils/Wave.h>
#include <bitset>
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
        node()->i2c.init(Sensors::Vl530x::address);
        delay(50);
        return 0;
    }

    std::int32_t
    loop(Boards::Esp8266::Gpio& gpio) override
    {
        (void)gpio;
        static std::deque<Byte> frameBuffer;

        while(node()->i2c.isDataAvailable())
        {
            frameBuffer.push_back(node()->i2c.read());
        }

        while(frameBuffer.size() >= 3)
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
                auto distance = stream.take();

                std::cout << "[I2C] valid measurement: " << distance
                          << " (checksum: " << static_cast<int>(checksum)
                          << ")" << std::endl;
            }
            else
            {
                std::cout << "[I2C] invalid frame: checksum mismatch (got "
                          << static_cast<int>(checksum) << ", expected "
                          << static_cast<int>(expectedChecksum) << ")"
                          << std::endl;
            }

            frameBuffer.pop_front();
            frameBuffer.pop_front();
            frameBuffer.pop_front();
        }

        delay(10);
        return 0;
    }
};


#endif    // MICROCONTROLLER_H
