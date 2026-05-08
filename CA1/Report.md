# I2C

## توضیح پیاده‌سازی

در این بخش پروتکل ساده‌شده‌ی I2C بین `ESP8266` و سنسور `VL530X` پیاده‌سازی شده است. ارتباط روی `SDA` انجام می‌شود و `SCL` از کلاک میکروکنترلر تغذیه می‌شود.

---

## پیاده‌سازی I2C در `ESP8266`

مسیر فایل:

```text
include/CPS4042/Hardwares/Boards/Esp8266.h
```

در سازنده‌ی بورد، لبه‌های کلاک پردازنده به پایه‌ی `scl` وصل شده‌اند و پروتکل `i2c` روی پردازنده نصب شده است:

```cpp
m_processor->communicationClockChanged.connect(
  [this](Bit edge) { m_gpio.scl.nextEdge(edge); });

m_processor->installProtocol(&i2c);
```

### تابع `init`

```cpp
void init(Byte address) override
{
    m_targetAddress = address;
    m_started       = false;
    m_addressSent   = false;
}
```

آدرس سنسور مقصد ذخیره می‌شود و وضعیت ارتباط به حالت قبل از شروع برمی‌گردد. `m_addressSent` هم `false` می‌شود تا در اجرای بعدی `run()` آدرس دوباره روی `SDA` فرستاده شود.

### تابع `write`

```cpp
void write(Byte byte) override
{
    m_outbound.push(byte);
}
```

داده‌ی خروجی در صف `m_outbound` قرار می‌گیرد. ارسال واقعی در `run()` انجام می‌شود تا Sketch مستقیماً درگیر زمان‌بندی پین‌ها نباشد.

### تابع `read`

```cpp
Byte read() override
{
    if(this->m_buffer.empty()) return 0;

    auto byte = this->m_buffer.front();
    this->m_buffer.pop();
    return byte;
}
```

این تابع اولین بایت آماده در بافر ورودی را برمی‌گرداند. در Sketch قبل از خواندن، `isDataAvailable()` بررسی می‌شود، بنابراین مقدار `0` فقط حالت محافظتی برای بافر خالی است.

### تابع `run`

ابتدای ارتباط با ارسال آدرس انجام می‌شود:

```cpp
if(!m_addressSent)
{
    gpio.sda.write(m_targetAddress);
    m_addressSent = true;
    return;
}
```

تا وقتی ACK دریافت نشده باشد، Master منتظر بیت پاسخ سنسور می‌ماند:

```cpp
if(!this->m_started)
{
    if(gpio.sda.hasBitToRead())
    {
        auto ack = gpio.sda.readBit();
        if(ack == Bit::One)
        {
            this->m_started = true;
        }
        else
        {
            m_addressSent = false;
        }
    }

    return;
}
```

اگر ACK برابر `1` باشد، ارتباط فعال می‌شود. اگر مقدار دیگری خوانده شود، `m_addressSent` دوباره `false` می‌شود تا آدرس در سیکل بعدی مجدداً ارسال شود.

بعد از شروع ارتباط، بایت‌های دریافتی از `SDA` وارد بافر پروتکل می‌شوند:

```cpp
while(gpio.sda.hasByteToRead())
{
    auto incoming = gpio.sda.read();
    this->m_buffer.push(incoming);
}
```

و اگر داده‌ای در صف خروجی باشد، روی `SDA` نوشته می‌شود:

```cpp
while(!m_outbound.empty())
{
    gpio.sda.write(m_outbound.front());
    m_outbound.pop();
}
```

---

## پیاده‌سازی I2C در `VL530X`

مسیر فایل:

```text
include/CPS4042/Hardwares/Sensors/VL530X.h
```

آدرس ثابت سنسور در خود کلاس تعریف شده است:

```cpp
inline static constexpr Byte address = 0x29;
```

از آن‌جا که فرکانس سنسور `Frequency::Drived` است، سنسور خودش loop مستقل ندارد و با لبه‌ی مثبت `SCL` جلو می‌رود:

```cpp
m_gpio.scl.onNextEdge([this](Vl530xVoltage level) {
    auto bit = Voltage::toBit(level);

    if(bit == Bit::One)
    {
        m_processor->nextCycle(m_gpio);
    }
});
```

### تابع `init`

```cpp
void init(Byte address) override
{
    (void)address;
    this->m_started = false;
}
```

آدرس ورودی استفاده نمی‌شود، چون آدرس سنسور ثابت است. فقط وضعیت ارتباط ریست می‌شود.

### دریافت آدرس و ارسال ACK

```cpp
if(!this->m_started)
{
    if(gpio.sda.hasByteToRead())
    {
        auto address = gpio.sda.read();
        if(address == static_cast<Byte>(static_cast<UByte>(Vl530x::address) << 1))
        {
            gpio.sda.write(Bit::One);
            this->m_started = true;
        }
    }

    return;
}
```

سنسور تا قبل از شروع ارتباط منتظر دریافت آدرس از `SDA` می‌ماند. در این پیاده‌سازی، مقدار دریافتی با آدرس shift شده‌ی `VL530X` مقایسه شده است و در صورت تطبیق، ACK برابر `1` روی `SDA` نوشته می‌شود.

بعد از شروع ارتباط، داده‌های ورودی در بافر ذخیره می‌شوند و داده‌های صف خروجی روی `SDA` نوشته می‌شوند:

```cpp
while(gpio.sda.hasByteToRead())
{
    this->m_buffer.push(gpio.sda.read());
}

while(!m_outbound.empty())
{
    gpio.sda.write(m_outbound.front());
    m_outbound.pop();
}
```

---

## Sketch میکروکنترلر

مسیر فایل:

```text
include/CPS4042/Sketchs/Microcontroller.h
```

در `setup()` آدرس سنسور برای پروتکل ثبت می‌شود:

```cpp
node()->i2c.init(Sensors::Vl530x::address);
delay(50);
```

در `loop()` بایت‌های دریافتی از I2C در `frameBuffer` جمع می‌شوند:

```cpp
while(node()->i2c.isDataAvailable())
{
    frameBuffer.push_back(node()->i2c.read());
}
```

فریم مورد انتظار سه بایتی است:

```text
MSB, LSB, checksum
```

برای بررسی checksum، اختلاف قدرمطلق دو بایت داده محاسبه می‌شود:

```cpp
auto expectedChecksum =
  static_cast<Byte>(msbUnsigned > lsbUnsigned
                      ? (msbUnsigned - lsbUnsigned)
                      : (lsbUnsigned - msbUnsigned));
```

اگر checksum معتبر باشد، مقدار اصلی با `ByteStream<std::uint16_t>` ساخته و چاپ می‌شود:

```cpp
ByteStream<std::uint16_t> stream;
stream << msb;
stream << lsb;
auto distance = stream.take();
```

بعد از پردازش هر فریم، سه بایت از ابتدای بافر حذف می‌شود تا فریم بعدی بررسی شود.

---

## Sketch سنسور

مسیر فایل:

```text
include/CPS4042/Sketchs/Sensor.h
```

سنسور در هر اجرای `loop()` یک مقدار فاصله در بازه‌ی `0` تا `4000` تولید می‌کند:

```cpp
static boost::random::uniform_int_distribution<> distribution {0, 4000};
auto distance = static_cast<std::uint16_t>(distribution(generator));
```

مقدار `uint16_t` با `ByteVector` به دو بایت شکسته می‌شود:

```cpp
auto bytes = ByteVector<std::uint16_t> {distance};

auto msb = getByte<1>(bytes);
auto lsb = getByte<0>(bytes);
```

سپس checksum ساخته می‌شود:

```cpp
auto checksum = static_cast<Byte>(
  msbUnsigned > lsbUnsigned ? (msbUnsigned - lsbUnsigned)
                            : (lsbUnsigned - msbUnsigned));
```

و هر سه بایت از طریق I2C ارسال می‌شوند:

```cpp
node()->i2c.write(msb);
node()->i2c.write(lsb);
node()->i2c.write(checksum);
```

---

## اتصال‌ها در `main.cpp`

مسیر فایل:

```text
src/main.cpp
```

در `main()` یک `ESP8266` و یک `VL530X` ساخته شده‌اند:

```cpp
Boards::Esp8266 esp8266;
Sensors::Vl530x vl530x;
```

اتصال‌های تغذیه و زمین:

```cpp
esp8266.gpio().vdd1.attachLink(linkRed);
esp8266.gpio().gnd1.attachLink(linkBlack);

vl530x.gpio().vdd.attachLink(linkRed);
vl530x.gpio().gnd.attachLink(linkBlack);
```

اتصال‌های I2C:

```cpp
esp8266.gpio().scl.attachLink(linkGreen);
esp8266.gpio().sda.attachLink(linkYellow);

vl530x.gpio().scl.attachLink(linkGreen);
vl530x.gpio().sda.attachLink(linkYellow);
```

در پایان Sketchها ساخته و اجرا می‌شوند:

```cpp
MicroController micro(&esp8266);
Sensor          disSen(&vl530x);

micro.start();
disSen.start();
```

---

## سوالات

**سؤال ۱: با توجه به اینکه `I2C` (نسخه ی اصلی که در درس با آن آشنا شدهاید) یک پروتکل باس است در صورت اتصال بیشتر از یک سنسور با آدرسهای مشابه چه اشکالی پیش میآید؟**

در I2C همه‌ی دستگاه‌ها روی باس مشترک قرار دارند و Slaveها بر اساس آدرس تصمیم می‌گیرند که باید پاسخ بدهند یا نه. اگر دو سنسور با آدرس یکسان روی همان باس باشند، هر دو آدرس ارسال‌شده از طرف Master را متعلق به خودشان می‌دانند و هم‌زمان ACK می‌دهند.

مشکل اصلی بعد از ACK ایجاد می‌شود. Master دیگر راهی ندارد بفهمد پاسخ مربوط به کدام سنسور است. اگر هر دو سنسور داده ارسال کنند، هر دو روی همان خط `SDA` اثر می‌گذارند و داده‌ی خوانده‌شده می‌تواند مخلوط یا عملاً غیرقابل اعتماد شود. حتی اگر ساختار open-drain در I2C واقعی باعث سوختن سخت‌افزار نشود، نتیجه‌ی منطقی باس دیگر نماینده‌ی یک سنسور مشخص نیست. برای همین در یک باس I2C، آدرس Slaveها باید یکتا باشد یا باید از راه‌حل‌هایی مثل I2C Multiplexer، تغییر آدرس سخت‌افزاری، یا باس جداگانه استفاده شود.

**سؤال ۲: چهار مورد از اشکالات کلی این پروتکل سادهسازی شده نسبت به آنچه از `I2C` در درس آموختهاید بیان کنید.**

چهار اشکال اصلی این نسخه‌ی ساده‌شده نسبت به I2C کامل:

1. شرط‌های استاندارد `START`، `STOP` و `Repeated START` به شکل واقعی پیاده‌سازی نشده‌اند. ارتباط عملاً با ارسال آدرس و تغییر وضعیت داخلی `started` جلو می‌رود.
2. ACK/NACK فقط به صورت ساده برای شروع ارتباط استفاده شده است. در I2C واقعی بعد از هر بایت، ACK یا NACK معنی دارد و Master می‌تواند پایان خواندن یا خطا را با NACK مشخص کند.
3. ساختار آدرس‌دهی کامل I2C مدل نشده است. در I2C واقعی آدرس ۷ یا ۱۰ بیتی همراه با بیت `R/W` و ترتیب مشخص روی باس ارسال می‌شود، ولی این نسخه فقط یک مدل ساده از آدرس را استفاده می‌کند.
4. رفتار واقعی باس و شرایط رقابتی پوشش داده نشده است. مواردی مثل arbitration در حالت چند Master، clock stretching، bus busy، timeout و بازیابی باس در این پیاده‌سازی وجود ندارند.

**سؤال ۳: کارکرد کلاسهای `ByteVector`, `ByteStream` و توابع `getByte` را توضیح دهید.**

`ByteVector` یک مقدار عددی صحیح را نگه می‌دارد و امکان دسترسی بایت‌به‌بایت به حافظه‌ی همان مقدار را می‌دهد:

```cpp
ByteVector<std::uint16_t> bytes {distance};
```

در این پروژه از آن برای شکستن مقدار `uint16_t` سنسور به دو بایت استفاده شده است. `data()` اشاره‌گر بایتی به مقدار داخلی برمی‌گرداند و `operator[]` هم بایت مورد نظر را با اندیس می‌دهد.

`getByte` یک تابع کمکی برای برداشتن یک بایت مشخص است. یک نسخه از آن از `ByteVector` بایت می‌گیرد:

```cpp
auto msb = getByte<1>(bytes);
auto lsb = getByte<0>(bytes);
```

نسخه‌های دیگر می‌توانند مستقیماً از خود متغیر عددی بایت بردارند. در همه‌ی نسخه‌ها، اندیس با `static_assert` کنترل می‌شود تا خارج از اندازه‌ی نوع نباشد.

`ByteStream` مسیر برعکس را انجام می‌دهد. بایت‌های دریافتی را به ترتیب وارد یک مقدار unsigned می‌کند:

```cpp
ByteStream<std::uint16_t> stream;
stream << msb;
stream << lsb;
auto distance = stream.take();
```

هر بار که بایتی وارد می‌شود، مقدار قبلی ۸ بیت به چپ شیفت داده می‌شود و بایت جدید به انتهای آن اضافه می‌شود. وقتی تعداد بایت‌ها به اندازه‌ی نوع مقصد رسید، `take()` مقدار ساخته‌شده را برمی‌گرداند و stream را پاک می‌کند.
