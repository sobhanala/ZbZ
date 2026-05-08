#include <CPS4042/Sketchs/I2CMultiplexer.h>
#include <CPS4042/Sketchs/Microcontroller.h>
#include <CPS4042/Sketchs/SensorRangeA.h>
#include <CPS4042/Sketchs/SensorRangeB.h>
#include <CPS4042/Units/Bit.h>
#include <CPS4042/Units/Byte.h>
#include <CPS4042/Wires/Pin.h>
#include <CPS4042/main.h>

std::int32_t
main()
{
    Boards::Esp8266 esp8266;
    Sensors::I2CMux mux;
    Sensors::Vl530x sensorA;
    Sensors::Vl530x sensorB;

    auto            linkRed      = std::make_shared<Bus>();
    auto            linkBlack    = std::make_shared<Bus>();
    auto            linkTxToRx   = std::make_shared<Link>();
    auto            linkRxToTx   = std::make_shared<Link>();
    auto            linkSdaCh0   = std::make_shared<Link>();
    auto            linkSclCh0   = std::make_shared<Link>();
    auto            linkSdaCh1   = std::make_shared<Link>();
    auto            linkSclCh1   = std::make_shared<Link>();

    CPS_SET_OBJECT_NAME(esp8266);
    CPS_SET_OBJECT_NAME(mux);
    CPS_SET_OBJECT_NAME(sensorA);
    CPS_SET_OBJECT_NAME(sensorB);

    CPS_SET_OBJECT_NAME_PTR(linkRed);
    CPS_SET_OBJECT_NAME_PTR(linkBlack);
    CPS_SET_OBJECT_NAME_PTR(linkTxToRx);
    CPS_SET_OBJECT_NAME_PTR(linkRxToTx);
    CPS_SET_OBJECT_NAME_PTR(linkSdaCh0);
    CPS_SET_OBJECT_NAME_PTR(linkSclCh0);
    CPS_SET_OBJECT_NAME_PTR(linkSdaCh1);
    CPS_SET_OBJECT_NAME_PTR(linkSclCh1);

    esp8266.gpio().vdd1.attachLink(linkRed);
    esp8266.gpio().gnd1.attachLink(linkBlack);
    esp8266.gpio().tx.attachLink(linkTxToRx);
    esp8266.gpio().rx.attachLink(linkRxToTx);

    mux.gpio().vdd.attachLink(linkRed);
    mux.gpio().gnd.attachLink(linkBlack);
    mux.gpio().rx.attachLink(linkTxToRx);
    mux.gpio().tx.attachLink(linkRxToTx);
    mux.gpio().sda0.attachLink(linkSdaCh0);
    mux.gpio().scl0.attachLink(linkSclCh0);
    mux.gpio().sda1.attachLink(linkSdaCh1);
    mux.gpio().scl1.attachLink(linkSclCh1);

    sensorA.gpio().vdd.attachLink(linkRed);
    sensorA.gpio().gnd.attachLink(linkBlack);
    sensorA.gpio().sda.attachLink(linkSdaCh0);
    sensorA.gpio().scl.attachLink(linkSclCh0);

    sensorB.gpio().vdd.attachLink(linkRed);
    sensorB.gpio().gnd.attachLink(linkBlack);
    sensorB.gpio().sda.attachLink(linkSdaCh1);
    sensorB.gpio().scl.attachLink(linkSclCh1);

    MicroController micro(&esp8266);
    I2CMultiplexer  muxSketch(&mux);
    SensorRangeA    sensorSketchA(&sensorA);
    SensorRangeB    sensorSketchB(&sensorB);

    micro.start();
    muxSketch.start();
    sensorSketchA.start();
    sensorSketchB.start();

    return Application::exec();
}
