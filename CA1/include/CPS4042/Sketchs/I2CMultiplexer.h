#ifndef I2C_MULTIPLEXER_SKETCH_H
#define I2C_MULTIPLEXER_SKETCH_H

#include <CPS4042/Hardwares/Comm/I2CMux.h>
#include <CPS4042/Hardwares/Sensors/VL530X.h>
#include <CPS4042/Sketchs/AbstractSketch.h>
#include <deque>

class I2CMultiplexer : public AbstractSketch<Sensors::I2CMux>
{
public:
    explicit I2CMultiplexer(Sensors::I2CMux* node) :
        AbstractSketch<Sensors::I2CMux> {node}
    {}

    std::int32_t
    setup(Sensors::I2CMux::Gpio& gpio) override
    {
        (void)gpio;
        std::cout << "I2CMultiplexer setup completed." << std::endl;
        return 0;
    }

    std::int32_t
    loop(Sensors::I2CMux::Gpio& gpio) override
    {
        (void)gpio;
        static UByte activeChannel {0};
        static bool  waitingForPacket {false};
        static std::deque<Byte> packetBuffer;

        while(node()->usart.isDataAvailable())
        {
            auto request  = static_cast<UByte>(node()->usart.read());
            activeChannel = request % 2;
            waitingForPacket = true;
            packetBuffer.clear();

            if(activeChannel == 0)
                node()->i2c0.init(Sensors::Vl530x::address);
            else
                node()->i2c1.init(Sensors::Vl530x::address);
        }

        if(waitingForPacket)
        {
            if(activeChannel == 0)
            {
                while(node()->i2c0.isDataAvailable())
                {
                    packetBuffer.push_back(node()->i2c0.read());
                }
            }
            else
            {
                while(node()->i2c1.isDataAvailable())
                {
                    packetBuffer.push_back(node()->i2c1.read());
                }
            }

            while(packetBuffer.size() >= 3 && waitingForPacket)
            {
                auto msb = packetBuffer[0];
                auto lsb = packetBuffer[1];
                auto checksum = packetBuffer[2];

                auto msbUnsigned = static_cast<UByte>(msb);
                auto lsbUnsigned = static_cast<UByte>(lsb);
                auto expectedChecksum =
                  static_cast<Byte>(msbUnsigned > lsbUnsigned
                                      ? (msbUnsigned - lsbUnsigned)
                                      : (lsbUnsigned - msbUnsigned));

                if(checksum == expectedChecksum)
                {
                    node()->usart.write(msb);
                    node()->usart.write(lsb);
                    node()->usart.write(checksum);
                    waitingForPacket = false;
                    packetBuffer.clear();
                }
                else
                {
                    packetBuffer.pop_front();
                }
            }
        }

        while(node()->i2c0.isDataAvailable() && activeChannel != 0)
        {
            (void)node()->i2c0.read();
        }

        while(node()->i2c1.isDataAvailable() && activeChannel != 1)
        {
            (void)node()->i2c1.read();
        }

        delay(1);
        return 0;
    }
};

#endif    // I2C_MULTIPLEXER_SKETCH_H
