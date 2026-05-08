# I2C Multiplexer

## توضیح پیاده‌سازی

در این بخش `I2CMux` بین میکروکنترلر و دو سنسور `VL530X` قرار گرفته است. ارتباط میکروکنترلر با Mux از طریق `USART` انجام می‌شود و Mux برای هر سنسور یک کانال I2C جدا دارد.

---

## بورد `I2CMux`

مسیر فایل:

```text
include/CPS4042/Hardwares/Comm/I2CMux.h
```

برای Mux علاوه بر `VDD` و `GND`، یک زوج `rx/tx` برای ارتباط با میکروکنترلر و دو زوج `sda/scl` برای کانال‌های I2C تعریف شده است:

```cpp
Pins::Rx<WorkingVoltageTp>  rx {BR, BTR, "I2CMux::rx"};
Pins::Tx<WorkingVoltageTp>  tx {BR, BTR, "I2CMux::tx"};

Pins::Sda<WorkingVoltageTp> sda0 {BR, BTR, "I2CMux::sda0"};
Pins::Scl<WorkingVoltageTp> scl0 {BR, BTR, "I2CMux::scl0"};

Pins::Sda<WorkingVoltageTp> sda1 {BR, BTR, "I2CMux::sda1"};
Pins::Scl<WorkingVoltageTp> scl1 {BR, BTR, "I2CMux::scl1"};
```

در سازنده، کلاک پردازنده‌ی Mux به هر دو خط `scl0` و `scl1` وصل شده است:

```cpp
m_processor->communicationClockChanged.connect(
  [this](Bit edge) { m_gpio.scl0.nextEdge(edge); });
m_processor->communicationClockChanged.connect(
  [this](Bit edge) { m_gpio.scl1.nextEdge(edge); });
```

بعد از آن هر سه پروتکل روی پردازنده نصب می‌شوند:

```cpp
m_processor->installProtocol(&usart);
m_processor->installProtocol(&i2c0);
m_processor->installProtocol(&i2c1);
```

`usart` سمت میکروکنترلر را پوشش می‌دهد و `i2c0` و `i2c1` دو کانال جدا برای سنسورها هستند.

### USART داخل Mux

کلاس `USART` همان نقش بخش قبل را دارد. داده‌ی ورودی از `rx` وارد `m_buffer` می‌شود و داده‌ی خروجی از صف `m_outbound` روی `tx` نوشته می‌شود:

```cpp
gpio.tx.setCanRead(false);

while(gpio.rx.hasByteToRead())
{
    this->m_buffer.push(gpio.rx.read());
}

while(!m_outbound.empty())
{
    gpio.tx.write(m_outbound.front());
    m_outbound.pop();
}
```

`setCanRead(false)` برای `tx` لازم است تا Mux داده‌ای را که خودش ارسال کرده، دوباره به عنوان ورودی نخواند.

### کانال‌های `I2C0` و `I2C1`

هر کانال آدرس مقصد، وضعیت ارسال آدرس و صف خروجی خودش را دارد:

```cpp
Byte             m_targetAddress {0};
bool             m_addressSent {false};
std::queue<Byte> m_outbound;
```

در `init()` آدرس سنسور مقصد ثبت و وضعیت کانال ریست می‌شود:

```cpp
m_targetAddress = address;
this->m_started = false;
m_addressSent   = false;
```

در `run()` ابتدا آدرس روی `sda` همان کانال نوشته می‌شود:

```cpp
if(!m_addressSent)
{
    gpio.sda0.write(m_targetAddress);
    m_addressSent = true;
    this->m_started = true;
    return;
}
```

برای کانال دوم همین منطق با `sda1` تکرار شده است. بعد از فعال شدن کانال، بایت‌های رسیده از سنسور خوانده می‌شوند و بایت‌های صف خروجی روی همان کانال ارسال می‌شوند:

```cpp
while(gpio.sda0.hasByteToRead())
{
    this->m_buffer.push(gpio.sda0.read());
}

while(!m_outbound.empty())
{
    gpio.sda0.write(m_outbound.front());
    m_outbound.pop();
}
```

در نتیجه داده‌ی کانال‌ها با هم قاطی نمی‌شود؛ هر کانال با `sda/scl` جدا و بافر پروتکل جدا کار می‌کند.

---

## Sketch مربوط به Mux

مسیر فایل:

```text
include/CPS4042/Sketchs/I2CMultiplexer.h
```

در `loop()` سه وضعیت اصلی نگه داشته شده است:

```cpp
static UByte activeChannel {0};
static bool  waitingForPacket {false};
static std::deque<Byte> packetBuffer;
```

`activeChannel` کانالی است که میکروکنترلر درخواست کرده، `waitingForPacket` یعنی Mux هنوز منتظر فریم معتبر از سنسور است، و `packetBuffer` بایت‌های خوانده‌شده از کانال I2C را نگه می‌دارد.

درخواست کانال از USART خوانده می‌شود:

```cpp
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
```

`request % 2` باعث می‌شود فقط کانال‌های موجود، یعنی `0` و `1`، انتخاب شوند. بعد از انتخاب کانال، همان I2C با آدرس سنسور `VL530X` مقداردهی می‌شود.

وقتی Mux منتظر پاسخ سنسور است، فقط از کانال فعال داده می‌خواند:

```cpp
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
```

فریم سنسورها سه بایتی است: `MSB`، `LSB` و `checksum`. چک‌سام مثل بخش‌های قبل از اختلاف قدرمطلق دو بایت داده محاسبه شده است:

```cpp
auto expectedChecksum =
  static_cast<Byte>(msbUnsigned > lsbUnsigned
                      ? (msbUnsigned - lsbUnsigned)
                      : (lsbUnsigned - msbUnsigned));
```

اگر چک‌سام درست باشد، همان سه بایت بدون تغییر از طریق USART به میکروکنترلر فرستاده می‌شود:

```cpp
node()->usart.write(msb);
node()->usart.write(lsb);
node()->usart.write(checksum);
waitingForPacket = false;
packetBuffer.clear();
```

اگر فریم معتبر نباشد، فقط یک بایت از ابتدای بافر حذف می‌شود:

```cpp
packetBuffer.pop_front();
```

با این کار اگر ابتدای بافر روی مرز درست فریم نباشد، Mux می‌تواند از بایت بعدی دوباره فریم سه‌بایتی بسازد.

در انتهای `loop()` داده‌های کانال غیرفعال دور ریخته می‌شوند:

```cpp
while(node()->i2c0.isDataAvailable() && activeChannel != 0)
{
    (void)node()->i2c0.read();
}

while(node()->i2c1.isDataAvailable() && activeChannel != 1)
{
    (void)node()->i2c1.read();
}
```

این قسمت مانع می‌شود داده‌های قدیمی یک کانال در درخواست بعدی به اشتباه استفاده شوند.

---

## Sketch میکروکنترلر

مسیر فایل:

```text
include/CPS4042/Sketchs/Microcontroller.h
```

میکروکنترلر کانال‌ها را یکی‌یکی درخواست می‌کند:

```cpp
static UByte nextChannel {0};
static bool  waitingForResponse {false};
static UByte requestedChannel {0};
static std::deque<Byte> frameBuffer;
```

تا وقتی پاسخ درخواست قبلی نیامده باشد، درخواست جدید ارسال نمی‌شود. درخواست کانال به این شکل روی USART نوشته می‌شود:

```cpp
requestedChannel = nextChannel % 2;
node()->usart.write(static_cast<Byte>(requestedChannel));
```

پاسخ Mux در `frameBuffer` جمع می‌شود:

```cpp
while(node()->usart.isDataAvailable())
{
    frameBuffer.push_back(node()->usart.read());
}
```

بعد از رسیدن سه بایت، میکروکنترلر چک‌سام را دوباره حساب می‌کند. اگر فریم معتبر باشد، `MSB` و `LSB` با `ByteStream<std::uint16_t>` به مقدار اصلی تبدیل می‌شوند:

```cpp
ByteStream<std::uint16_t> stream;
stream << msb;
stream << lsb;
auto value = stream.take();
```

بعد از پردازش هر فریم، سه بایت از بافر حذف می‌شود و کانال بعدی درخواست خواهد شد:

```cpp
frameBuffer.pop_front();
frameBuffer.pop_front();
frameBuffer.pop_front();

waitingForResponse = false;
nextChannel++;
```

---

## Sketch سنسورها

مسیر فایل‌ها:

```text
include/CPS4042/Sketchs/SensorRangeA.h
include/CPS4042/Sketchs/SensorRangeB.h
```

هر دو سنسور از همان سخت‌افزار `VL530X` استفاده می‌کنند، ولی داده‌ی قابل تفکیک تولید می‌کنند. سنسور اول مقدار بین `0` تا `20` می‌سازد:

```cpp
static boost::random::uniform_int_distribution<> distribution {0, 20};
```

سنسور دوم مقدار بین `50` تا `100` می‌سازد:

```cpp
static boost::random::uniform_int_distribution<> distribution {50, 100};
```

در هر دو Sketch مقدار `uint16_t` به دو بایت شکسته می‌شود:

```cpp
auto bytes = ByteVector<std::uint16_t> {value};
auto msb   = getByte<1>(bytes);
auto lsb   = getByte<0>(bytes);
```

سپس چک‌سام از اختلاف دو بایت ساخته و فریم سه‌بایتی روی `sda` نوشته می‌شود:

```cpp
gpio.sda.write(msb);
gpio.sda.write(lsb);
gpio.sda.write(checksum);
```

این دو بازه‌ی جدا باعث می‌شود در خروجی میکروکنترلر مشخص باشد داده از کدام کانال آمده است.

---

## اتصال‌ها در `main.cpp`

مسیر فایل:

```text
src/main.cpp
```

در `main()` یک میکروکنترلر، یک Mux و دو سنسور ساخته شده‌اند:

```cpp
Boards::Esp8266 esp8266;
Sensors::I2CMux mux;
Sensors::Vl530x sensorA;
Sensors::Vl530x sensorB;
```

ارتباط USART بین میکروکنترلر و Mux به صورت ضربدری وصل شده است:

```cpp
esp8266.gpio().tx.attachLink(linkTxToRx);
esp8266.gpio().rx.attachLink(linkRxToTx);

mux.gpio().rx.attachLink(linkTxToRx);
mux.gpio().tx.attachLink(linkRxToTx);
```

کانال صفر Mux به سنسور اول وصل شده است:

```cpp
mux.gpio().sda0.attachLink(linkSdaCh0);
mux.gpio().scl0.attachLink(linkSclCh0);

sensorA.gpio().sda.attachLink(linkSdaCh0);
sensorA.gpio().scl.attachLink(linkSclCh0);
```

کانال یک هم به سنسور دوم وصل شده است:

```cpp
mux.gpio().sda1.attachLink(linkSdaCh1);
mux.gpio().scl1.attachLink(linkSclCh1);

sensorB.gpio().sda.attachLink(linkSdaCh1);
sensorB.gpio().scl.attachLink(linkSclCh1);
```

در پایان Sketch هر بورد ساخته و اجرا می‌شود:

```cpp
MicroController micro(&esp8266);
I2CMultiplexer  muxSketch(&mux);
SensorRangeA    sensorSketchA(&sensorA);
SensorRangeB    sensorSketchB(&sensorB);
```

---

## سوالات

**سؤال ۱: تابع `reverse` در هدر `Byte.h` و مکانهای استفاده از آن را مطالعه کنید، کارکرد آن را توضیح دهید و بگویید آیا از همچین عملیاتی در بوردهای واقعی استفاده میشود؟ راه حل دنیای واقعی آن چیست؟**

تابع `reverse` ترتیب بیت‌های یک بایت را برعکس می‌کند. در حلقه از بیت کم‌ارزش‌تر شروع می‌کند، هر بیت را با `takeNthBit(b, i)` برمی‌دارد، خروجی را یک بیت به چپ شیفت می‌دهد و بیت خوانده‌شده را به انتهای آن اضافه می‌کند:

```cpp
for(std::uint8_t i = 0; i < bitWidth(Byte {}); i++)
{
    auto bit = takeNthBit(b, i);
    res <<= 1;
    res += (std::uint8_t)bit;
}
```

محل استفاده‌ی اصلی آن در `Transmitter::read()` است:

```cpp
auto first = m_receivingBuffer.takeRest();
first = reverse(first);
```

علت این کار به نحوه‌ی ساخت بایت در `BitQueue` برمی‌گردد. هنگام `push(Byte)`، بیت‌ها از LSB به MSB وارد صف می‌شوند، اما `takeMany()` موقع ساختن بایت، هر بیت جدید را بعد از شیفت چپ اضافه می‌کند. در نتیجه ترتیب بیتی که از صف ساخته می‌شود برعکس مقدار اصلی است و `reverse` آن را اصلاح می‌کند.

در سخت‌افزار واقعی معمولاً این اصلاح به شکل یک تابع عمومی روی هر بایت انجام نمی‌شود. ماژول‌های UART/SPI/I2C ترتیب ارسال و دریافت بیت‌ها را در منطق داخلی خودشان مدیریت می‌کنند؛ مثلاً UART می‌داند داده را LSB-first شیفت دهد و رجیستر دریافت را درست پر کند. در بعضی پردازنده‌ها هم اگر واقعاً نیاز به bit-reversal باشد، دستورالعمل یا واحد سخت‌افزاری مخصوص مثل `RBIT` وجود دارد. بنابراین در این پروژه `reverse` بیشتر برای جبران مدل صف بیت و شبیه‌سازی نرم‌افزاری لازم شده است.

**سؤال ۲: توابع `takeNthBit` در هدر `Bit.h` را توضیح دهید. تفاوت این دو در چیست؟**

هر دو نسخه‌ی `takeNthBit` یک بیت مشخص از یک مقدار عددی را با mask کردن استخراج می‌کنند. نسخه‌ی اول مقدار و شماره‌ی بیت را در زمان اجرا می‌گیرد:

```cpp
takeNthBit(T value, std::uint8_t n)
```

اگر `n` خارج از محدوده‌ی عرض نوع باشد، پیام خطا چاپ می‌کند و `Bit::X` برمی‌گرداند. بعد یک mask با `1 << n` می‌سازد و با `value & mask` مشخص می‌کند بیت مورد نظر صفر است یا یک.

نسخه‌ی دوم پارامترها را در زمان کامپایل می‌گیرد:

```cpp
takeNthBit<auto byte, std::uint8_t n>()
```

در این نسخه بررسی محدوده با `static_assert` انجام می‌شود؛ یعنی اگر شماره‌ی بیت نامعتبر باشد، خطا در زمان کامپایل رخ می‌دهد، نه زمان اجرا.

پس تفاوت اصلی این دو نسخه در زمان مشخص بودن ورودی‌ها و نوع کنترل خطاست. نسخه‌ی run-time برای مقادیری مناسب است که هنگام اجرای برنامه مشخص می‌شوند. نسخه‌ی compile-time برای مقدارهای ثابت مناسب‌تر است و هزینه‌ی بررسی زمان اجرا ندارد.

**سؤال ۳: تابع `attachPinToCommunicationClock` از کلاس `Board` توضیح دهید. (از جهت کد و کاربرد)**

این تابع یک پین دیجیتال از GPIO بورد را به سیگنال کلاک ارتباطی پردازنده وصل می‌کند. شماره‌ی پین به صورت template parameter داده می‌شود:

```cpp
template <std::uint64_t pinIndex>
inline constexpr void attachPinToCommunicationClock()
```

اول بررسی می‌کند که بورد از نوع `Drived` نباشد:

```cpp
static_assert(frequency() != Frequency::Drived, ...);
```

یعنی این تابع برای بوردی است که خودش کلاک تولید می‌کند، نه بوردی که با کلاک خارجی جلو می‌رود. بعد معتبر بودن شماره‌ی پین را با تعداد فیلدهای GPIO چک می‌کند:

```cpp
static_assert(GpioHelper::field_count_v<Gpio> > pinIndex, ...);
```

سپس پین مورد نظر با `boost::pfr::get<pinIndex>(m_gpio)` برداشته می‌شود و نوع آن بررسی می‌شود که حتماً `Pins::Digital` باشد:

```cpp
static_assert(std::is_same_v<Pins::Digital<WorkingVoltageTp>,
                             std::decay_t<decltype(pin)>>, ...);
```

در نهایت، پین به سیگنال `communicationClockChanged` پردازنده وصل می‌شود:

```cpp
m_processor->communicationClockChanged.connect(
  [this, &pin](Bit edge) { pin.write(edge); });
```

کاربردش این است که هر بار پردازنده لبه‌ی کلاک ارتباطی را تولید کرد، همان مقدار روی پین انتخاب‌شده نوشته شود. در این پروژه همین ایده برای رساندن کلاک به خطوط `SCL` استفاده شده است؛ با این تفاوت که در بعضی بوردها مثل `Esp8266` و `I2CMux` اتصال کلاک مستقیم در سازنده با `nextEdge` انجام شده است. در سخت‌افزار واقعی، این کار معمولاً داخل peripheral انجام می‌شود و برنامه لازم نیست در هر لبه، پین کلاک را دستی تغییر دهد.
