#ifndef SENSOR_RANGE_A_H
#define SENSOR_RANGE_A_H

#include <CPS4042/Hardwares/Sensors/VL530X.h>
#include <CPS4042/Sketchs/AbstractSketch.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

class SensorRangeA : public AbstractSketch<Sensors::Vl530x>
{
public:
    explicit SensorRangeA(Sensors::Vl530x* node) :
        AbstractSketch<Sensors::Vl530x> {node}
    {}

    std::int32_t
    setup(Sensors::Vl530x::Gpio& gpio) override
    {
        (void)gpio;
        std::cout << "SensorRangeA setup completed." << std::endl;
        return 0;
    }

    std::int32_t
    loop(Sensors::Vl530x::Gpio& gpio) override
    {
        (void)gpio;
        static boost::random::mt19937                   generator {11};
        static boost::random::uniform_int_distribution<> distribution {0, 20};

        auto value = static_cast<std::uint16_t>(distribution(generator));
        auto bytes = ByteVector<std::uint16_t> {value};
        auto msb   = getByte<1>(bytes);
        auto lsb   = getByte<0>(bytes);

        auto msbUnsigned = static_cast<UByte>(msb);
        auto lsbUnsigned = static_cast<UByte>(lsb);
        auto checksum    = static_cast<Byte>(
          msbUnsigned > lsbUnsigned ? (msbUnsigned - lsbUnsigned)
                                    : (lsbUnsigned - msbUnsigned));

        gpio.sda.write(msb);
        gpio.sda.write(lsb);
        gpio.sda.write(checksum);

        delay(20);
        return 0;
    }
};

#endif    // SENSOR_RANGE_A_H
