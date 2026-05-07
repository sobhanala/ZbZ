 #include <CPS4042/Sketchs/HardDisk.h>
// #include <CPS4042/Sketchs/I2CMultiplexer.h>
#include <CPS4042/Sketchs/Microcontroller.h>
#include <CPS4042/Units/Bit.h>
#include <CPS4042/Units/Byte.h>
#include <CPS4042/Wires/Pin.h>
#include <CPS4042/main.h>

std::int32_t
main()
{
    Boards::Esp8266 esp8266;
    Sensors::Usb     usb;

    auto            linkRed    = std::make_shared<Link>();
    auto            linkBlack  = std::make_shared<Link>();
    auto            linkTxToRx = std::make_shared<Link>();
    auto            linkRxToTx = std::make_shared<Link>();

    CPS_SET_OBJECT_NAME(esp8266);
    CPS_SET_OBJECT_NAME(usb);

    CPS_SET_OBJECT_NAME_PTR(linkRed);
    CPS_SET_OBJECT_NAME_PTR(linkBlack);
    CPS_SET_OBJECT_NAME_PTR(linkTxToRx);
    CPS_SET_OBJECT_NAME_PTR(linkRxToTx);

    esp8266.gpio().vdd1.attachLink(linkRed);
    esp8266.gpio().gnd1.attachLink(linkBlack);
    esp8266.gpio().tx.attachLink(linkTxToRx);
    esp8266.gpio().rx.attachLink(linkRxToTx);

    usb.gpio().vdd.attachLink(linkRed);
    usb.gpio().gnd.attachLink(linkBlack);
    usb.gpio().rx.attachLink(linkTxToRx);
    usb.gpio().tx.attachLink(linkRxToTx);

    MicroController micro(&esp8266);
    HardDisk        hardDisk(&usb);

    micro.start();
    hardDisk.start();

    return Application::exec();
}
