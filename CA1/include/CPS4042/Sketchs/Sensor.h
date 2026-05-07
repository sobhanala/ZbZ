#ifndef SENSOR_H
#define SENSOR_H

#include <CPS4042/Hardwares/Sensors/VL530X.h>
#include <CPS4042/Sketchs/AbstractSketch.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

class Sensor : public AbstractSketch<Sensors::Vl530x>
{
public:
    explicit Sensor(Sensors::Vl530x* node) :
        AbstractSketch<Sensors::Vl530x> {node}
    {}

    std::int32_t
    setup(Sensors::Vl530x::Gpio& gpio) override
    {
        std::cout << "vl530x setup completed." << std::endl;
        return 0;
    }

    std::int32_t
    loop(Sensors::Vl530x::Gpio& gpio) override
    {
        gpio;

        static boost::random::mt19937                   generator {42};
        static boost::random::uniform_int_distribution<> distribution {0, 4000};

        auto distance = static_cast<std::uint16_t>(distribution(generator));
        auto bytes    = ByteVector<std::uint16_t> {distance};

        auto msb = getByte<1>(bytes);
        auto lsb = getByte<0>(bytes);

        auto msbUnsigned = static_cast<UByte>(msb);
        auto lsbUnsigned = static_cast<UByte>(lsb);
        auto checksum    = static_cast<Byte>(
          msbUnsigned > lsbUnsigned ? (msbUnsigned - lsbUnsigned)
                                    : (lsbUnsigned - msbUnsigned));

        node()->i2c.write(msb);
        node()->i2c.write(lsb);
        node()->i2c.write(checksum);

        delay(25);
        return 0;
    }
};

#endif // SENSOR_H
