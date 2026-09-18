# LabDAQ-Control: STM32 High-Speed Data Acquisition & Cyclic Test Control System

[![Platform](https://img.shields.io/badge/Platform-STM32F407VG%20%7C%20Cortex--M4F%20168MHz-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f407vg.html)
[![IDE](https://img.shields.io/badge/IDE-STM32CubeIDE%20%2F%20Makefile-brightgreen.svg)]()
[![Channels](https://img.shields.io/badge/Channels-16%20Analog%20Multiplexed-orange.svg)]()
[![Sampling](https://img.shields.io/badge/Target%20Rate-1000%20SPS%2Fch-red.svg)]()
[![Protocols](https://img.shields.io/badge/Protocols-SCPI%20%7C%20Modbus%20RTU%20%7C%20TCP%20Stream-purple.svg)]()

> **مستندات و فریم‌ور کنترلر داده‌برداری پرسرعت ۱۶ کاناله و آزمون خستگی سیکلیک بر پایه میکروکنترلر STM32F407**  
> Complete STM32CubeIDE firmware, hardware schematics, wiring pinouts, real-time digital filtering, and multi-protocol control engine.

---

## 📋 فهرست مطالب / Table of Contents
- [معرفی سیستم / Overview](#معرفی-سیستم--overview)
- [مشخصات سخت‌افزاری / Hardware Specifications](#مشخصات-سخت‌افزاری--hardware-specifications)
- [سیم‌بندی و پین‌اوت / Wiring & Pinout](#سیم‌بندی-و-پین‌اوت--wiring--pinout)
- [ساختار مخزن / Repository Structure](#ساختار-مخزن--repository-structure)
- [پروتکل‌های ارتباطی / Communication Protocols](#پروتکل‌های-ارتباطی--communication-protocols)
  - [دستورات SCPI / SCPI Commands](#دستورات-scpi--scpi-commands)
  - [استریم خودکار اترنت UDP چندپخشی / UDP Multicast 1000Hz Telemetry](#استریم-خودکار-اترنت-udp-چندپخشی--udp-multicast-1000hz-telemetry)
  - [موتور متقارن پردازش دستورات / Unified Command Processor](#موتور-متقارن-پردازش-دستورات--unified-command-processor)
  - [وب‌سرور داخلی و چارت زنده / Embedded HTML Web Server](#وب‌سرور-داخلی-و-چارت-زنده--embedded-html-web-server)
  - [نگاشت رجیسترهای مدباس / Modbus RTU Register Map](#نگاشت-رجیسترهای-مدباس--modbus-rtu-register-map)
  - [بسته باینری استریم پرسرعت / Binary TCP Streaming Frame](#بسته-باینری-استریم-پرسرعت--binary-tcp-streaming-frame)
- [فیلترهای دیجیتال / Digital Signal Conditioning](#فیلترهای-دیجیتال--digital-signal-conditioning)
- [راهنمای راه‌اندازی و کامپایل / Build & Flash Guide](#راهنمای-راه‌اندازی-و-کامپایل--build--flash-guide)

---


## 2026 Control Console Expansion

The project now includes a structured control layer for the complete laboratory workflow, not only raw acquisition. The embedded management console is organized into **Data Logger**, **Triaxial Tests**, **PID / EP Motors**, **Channels & Calibration**, **Time**, **Logs**, and **Settings** tabs. Two EP motor channels have independent calibration and PID state, while all 16 analog inputs have editable engineering names, units, gain and offset. See [docs/CONTROL_SYSTEM.md](docs/CONTROL_SYSTEM.md), [docs/WEB_API.md](docs/WEB_API.md) and [ROADMAP.md](ROADMAP.md).

> Safety: closed-loop hydraulic/EP output is intentionally gated until the actual PWM→0–10 V timer pins, polarity, feedback mapping and emergency-stop chain are commissioned on the real machine.

## معرفی سیستم / Overview

سیستم **LabDAQ-Control** یک پلتفرم جامع اندازه‌گیری آزمایشگاهی و کنترل چرخه‌ای (Cyclic Test) است که برای ثبت بلادرنگ سیگنال‌های حسگرهای آنالوگ (لودسل، سنسورهای جابه‌جایی LVDT، پتانسیومترها، فشار و دما) و همزمان اعمال پالس‌های تحریک مکانیکی یا الکتریکی طراحی شده است.

### ویژگی‌های کلیدی / Key Features
1. **۱۶ کانال ورودی آنالوگ** با مالتی‌پلکسر آنالوگ `CD74HC4067` و مبدل سطح ولتاژ دوراهه `TXS0108E` (تبدیل ۳.۳ ولت میکرو به ۵.۰ ولت سنسورها/مالتی‌پلکسر).
2. **نرخ نمونه‌برداری بالا با DMA**: استفاده از ADC1 با رزولوشن ۱۲ بیت و انتقال داده‌ها با وقفه Half/Full به بافر چرخشی Ping-Pong بدون اشغال CPU.
3. **موتور آزمون چرخه‌ای (Cyclic Engine)**: شمارنده خودکار سیکل‌ها، تنظیم فرکانس اعمال بار (۰.۱ الی ۵۰ هرتز)، ایجاد پالس همگام‌ساز (SYNC Out) و فرمان اکچویتور (Actuator/Valve).
4. **فیلترهای دیجیتال Real-time**: میانگین متحرک (MAV)، فیلتر با پاسخ ضربه بی‌نهایت (IIR Butterworth Low-Pass)، فیلتر نمایی (EMA) و حذف اسپایک (Median Filter).
5. **پشتیبانی چند پروتکلی**:
   - استاندارد ابزاردقیق **SCPI** روی پورت سریال/USB
   - پروتکل صنعتی **Modbus RTU** روی بستر RS485 (تراشه SP3485)
   - استریم پرسرعت باینری پکت‌های داده با CRC16 روی اترنت **LAN8720A**

---

## مشخصات سخت‌افزاری / Hardware Specifications

| مؤلفه | مدل / مشخصه | توضیحات |
| :--- | :--- | :--- |
| **میکروکنترلر اصلی** | STM32F407VGT6 (ARM Cortex-M4F) | فرکانس ۱۶۸ مگاهرتز، ۱ مگابایت فلش، ۱۹۲ کیلوبایت رم، واحد FPU سخت‌افزاری |
| **برد توسعه** | EWB-STM32F407V-LAN-V3.0 | مجهز به پورت اترنت RMII LAN8720A، اسلات microSD، پورت USB، کریستال ۲۵ مگاهرتز |
| **مبدل سطح منطقی** | TXS0108E (8-Channel Bi-directional) | تبدیل منطق 3.3V پورت E به 5.0V برای مالتی‌پلکسر و خروجی‌های ایزوله |
| **مالتی‌پلکسر آنالوگ** | CD74HC4067 (16-to-1 Analog Switch) | زمان تاخیر سوئیچینگ ۵ میکروثانیه، مقاومت هدایت کم (Ron ~ 70Ω) |
| **درایور RS-485** | SP3485 / MAX485 | نیمه‌دوطرفه (Half-Duplex) با پایه جهت جریان داده DIR (PD7) |
| **ورودی ADC اصلی** | PA4 (ADC1_IN4) | دریافت سیگنال مشترک آنالوگ (SIG) از خروجی مالتی‌پلکسر |
| **تغذیه سیستم** | 5.0V DC ورودی اصلی | رگولاتورهای آنبرد ۳.۳ ولت AMS1117-3.3V برای هسته دیجیتال |

---

## سیم‌بندی و پین‌اوت / Wiring & Pinout

```
   [ STM32F407V Board ]                     [ TXS0108E Level Shifter ]                  [ CD74HC4067 MUX & Actuator ]
+-------------------------+                 +--------------------------+                 +----------------------------+
| PE8  (MUX Address S0)   | --------------> | A1 (3.3V) ---> B1 (5.0V) | --------------> | S0 (Pin 10)                |
| PE9  (MUX Address S1)   | --------------> | A2 (3.3V) ---> B2 (5.0V) | --------------> | S1 (Pin 11)                |
| PE10 (MUX Address S2)   | --------------> | A3 (3.3V) ---> B3 (5.0V) | --------------> | S2 (Pin 14)                |
| PE11 (MUX Address S3)   | --------------> | A4 (3.3V) ---> B4 (5.0V) | --------------> | S3 (Pin 13)                |
| PE12 (MUX /EN Enable)   | --------------> | A5 (3.3V) ---> B5 (5.0V) | --------------> | /EN (Pin 15) Active Low    |
| PE13 (Cyclic SYNC Out)  | --------------> | A6 (3.3V) ---> B6 (5.0V) | --------------> | External Trigger / Scope   |
| PE14 (Actuator Out)     | --------------> | A7 (3.3V) ---> B7 (5.0V) | --------------> | Relay / Valve Solenoid     |
| PE15 (Status Flag Out)  | --------------> | A8 (3.3V) ---> B8 (5.0V) | --------------> | Front Panel Test Run LED   |
|                         |                 |                          |                 |                            |
| PA4 (ADC1_IN4)          | <----------------------------------------------------------- | SIG (Pin 1 Common Analog)  |
|                         |                 | VCCA = +3.3V             |                 | VCC = +5.0V                |
| GND                     | --------------- | GND                      | --------------- | GND (Pin 12)               |
+-------------------------+                 +--------------------------+                 +----------------------------+
```

برای مشاهده جدول کامل کلیه پین‌ها، به فایل [`docs/WIRING.md`](docs/WIRING.md) مراجعه فرمایید.

---

## ساختار مخزن / Repository Structure

```
LabDAQ-Control/
├── AGENTS.md                  # دستورالعمل و راهنمای کار برای ایجنت‌های هوش مصنوعی
├── README.md                  # راهنمای جامع پروژه و مستندات اصلی
├── MEMORY.md                  # لاگ حافظه، تصمیمات معماری، ساختار حافظه و پارامترها
├── docs/                      # مستندات تکمیلی، محاسبات و استانداردهای ارتباطی
│   ├── ARCHITECTURE.md        # لایه‌بندی معماری سخت‌افزار، فریم‌ور و نرم‌افزار
│   ├── WIRING.md              # مستندات دقیق سیم‌بندی، سطوح ولتاژ و جدول اتصال پین‌ها
│   ├── PROTOCOLS.md           # جزییات کامل پروتکل‌های SCPI، Modbus RTU و TCP Packet
│   ├── FILTERS.md             # تئوری و معادلات فیلترهای دیجیتال (MAV, IIR Butterworth)
│   ├── HARDWARE.md            # تشریح ماژول‌های الکترونیکی و ملاحظات نویز
│   └── images/                # تصاویر مدار، بورد و سیم‌بندی
├── hardware/                  # فایل‌ها و نقشه‌های سخت‌افزاری
│   ├── board/                 # مستندات برد و پین‌اوت هدرهای J1 و J2
│   └── docs/                  # مستندات شماتیک و نقشه‌های پیاده‌سازی
└── firmware/                  # کدهای سورس پروژه STM32CubeIDE
    ├── .project               # فایل پروژه اکلیپس / STM32CubeIDE
    ├── .cproject              # تنظیمات بیلد و ابزارهای GCC ARM
    ├── Makefile               # اسکریپت کامپایل مستقیم با arm-none-eabi-gcc
    ├── STM32F407VGTx_FLASH.ld # اسکریپت لینکر فلش و مموری مپ
    ├── Config/
    │   └── labdaq_config.h    # تعاریف سخت‌افزاری، تنظیمات کانال‌ها و نرخ نمونه‌برداری
    ├── Core/
    │   ├── Inc/               # هدرهای اصلی (main.h, stm32f4xx_it.h, ...)
    │   └── Src/               # حلقه‌های اجرایی، وقفه و پیکربندی کلاک ۱۶۸ مگاهرتز
    └── Drivers/
        ├── labdaq_mux16.c/.h  # درایور کنترل مالتی‌پلکسر ۱۶ کاناله
        ├── labdaq_adc.c/.h    # درایور ADC با DMA و تایمر تریگر
        ├── labdaq_buffer.c/.h # بافر چرخشی پینگ‌پنگ و دابل بافر
        ├── labdaq_filter.c/.h # موتور پردازش سیگنال بلادرنگ (Butterworth, MAV, EMA)
        ├── labdaq_sampler.c/.h# موتور زمان‌بندی نمونه‌برداری و آزمون چرخه‌ای
        └── labdaq_comm.c/.h   # پارسر SCPI، مدباس RTU و سازنده پکت باینری
```

---

## پروتکل‌های ارتباطی / Communication Protocols

### دستورات SCPI / SCPI Commands
ارتباط روی پورت سریال (USART1 با بادریت 115200) با فرمت اسکی استاندارد IEEE-488.2:

- `*IDN?`: شناسایی دستگاه
- `:MEAS:VOLT:ALL?`: خواندن ولتاژ کالیبره تمام ۱۶ کانال بر حسب میلی‌ولت
- `:MEAS:VOLT:CHAN? <0..15>`: خواندن ولتاژ یک کانال مشخص
- `:MEAS:RAW:ALL?`: خواندن مقادیر خام ۱۲ بیتی (۰ الی ۴۰۹۵)
- `:RATE <Hz>`: تنظیم فرکانس نمونه‌برداری به ازای هر کانال (۱۰ الی ۱۰۰۰۰ هرتز)
- `:FILTER <BYPASS|MAV|EMA|IIR|MEDIAN> [param]`: تنظیم پارامتر فیلتر فعال
- `:CYCLIC:START <freq>,<target_cycles>`: شروع تست چرخه‌ای مکانیکی
- `:CYCLIC:STOP`: توقف اضطراری و قطع اکچویتور
- `:CYCLIC:STATUS?`: گزارش وضعیت تست، تعداد سیکل‌های طی شده و سیکل هدف

---

### استریم خودکار اترنت UDP چندپخشی / UDP Multicast 1000Hz Telemetry
برای کاهش تاخیر، حذف سربار پروتکل Handshake و بی‌نیازی کلاینت از دانستن IP دستگاه:
- **نرخ ارسال:** دقیقا ۱۰۰۰ بسته در ثانیه (۱ میلی‌ثانیه بین هر ارسال).
- **آدرس چندپخشی (Multicast):** `239.255.0.100:5001` (قابلیت سوئیچ به Unicast/Broadcast).
- **تگ زمان سخت‌افزاری (Hardware Timestamp):** دقت میکروثانیه (µs) استخراج شده از رجیستر کلاک `DWT->CYCCNT` هسته کورتکس ۱۶۸ مگاهرتز (۵.۹ نانوثانیه وضوح داخلی).
- **فرمت بسته:** هدر `0xAA55`، شناسه بسته، شماره ترتیبی فریم ۳۲ بیتی، تگ زمان ۶۴ بیتی، ولتاژهای ۱۶ کانال، وضعیت تست و چک‌سام نهایی CRC16.

---

### موتور متقارن پردازش دستورات / Unified Command Processor
محیط یکپارچه برای پردازش فرامین دریافتی:
- هر دستوری که از خط سریال (`USART1`) بیاید یا روی سوکت شبکه (`UDP Port 5001`) دریافت شود، توسط تابع مشترک `LABDAQ_Unified_ExecuteCommand` تحلیل و اجرا می‌گردد.
- نیازی به دانستن IP دستگاه نیست؛ کلاینت می‌تواند دستور را به آدرس مولتی‌کست یا پورت ۵۰۰۱ بفرستد و پاسخ تاییدیه را بلافاصله دریافت کند.
- پاسخ در همان سوکت یا درگاه صادرکننده فرمان مستقیما بازگردانده می‌شود.

---

### وب‌سرور داخلی و چارت زنده / Embedded HTML Web Server
روی پورت استاندارد ۸۰ وب با استفاده از پشته شبکه LwIP:
- **آدرس دسترسی:** `http://192.168.1.150/` (یا IP اختصاص داده شده از DHCP).
- **تنظیم نرخ نمونه‌برداری:** فرم تعاملی با قابلیت انتخاب نرخ‌های ۱۰۰، ۵۰۰، ۱۰۰۰، ۲۰۰۰، ۵۰۰۰ و ۱۰۰۰۰ هرتز.
- **انتخاب کانال جهت نمایش چارت:** منوی آبشاری برای سوئیچ چارت بین ۱۶ کانال آنالوگ.
- **اسیلوسکوپ HTML5 Canvas:** رسم بلادرنگ شکل‌موج سیگنال ورودی آنالوگ در مرورگر کاربر بدون نیاز به نصب هرگونه نرم‌افزار اضافی.

---

### نگاشت رجیسترهای مدباس / Modbus RTU Register Map
روی بستر RS-485 (USART2 با کنترل پایه جهت PD7):

- `0x0000` (RO): کد شناسه دستگاه (`0x4C44` - "LD")
- `0x0001` (RO): نسخه فریم‌ور
- `0x0002` (RO): وضعیت آزمون (0=آماده، 2=در حال اجرا، 3=مکث، 4=پایان‌یافته)
- `0x0003 - 0x0004` (RO): شمارنده ۳۲ بیتی سیکل کنونی
- `0x0010 - 0x001F` (RO): ولتاژ خوانده‌شده ۱۶ کانال ورودی آنالوگ (میلی‌ولت)
- `0x0020` (RW): فرمان شروع/توقف آزمون
- `0x0021` (RW): نرخ نمونه‌برداری (هرتز)
- `0x0024` (RW): فرکانس تحریک آزمون چرخه‌ای ضرب‌در ۱۰۰

---

## راهنمای راه‌اندازی و کامپایل / Build & Flash Guide

### روش اول: استفاده از STM32CubeIDE
1. نرم‌افزار **STM32CubeIDE** را باز کنید.
2. از منو گزینه `File -> Open Projects from File System...` را انتخاب کنید.
3. پوشه `firmware/` را به عنوان مسیر دایرکتوری انتخاب کرده و دکمه `Finish` را بزنید.
4. روی پروژه کلیک‌راست کرده و گزینه `Build Project` را بزنید.
5. بورد را با پروگرمر **ST-LINK V2** (پین‌های SWDIO, SWCLK, GND, 3.3V) وصل کرده و دکمه `Run / Debug` را بزنید.

### روش دوم: استفاده از Makefile و کامپایلر خط فرمان
```bash
cd firmware
make -j4
```
فایل‌های خروجی در مسیر `firmware/build/`:
- `LabDAQ-Control.elf`
- `LabDAQ-Control.bin`
- `LabDAQ-Control.hex`

برای پروگرم کردن با `st-flash`:
```bash
st-flash write build/LabDAQ-Control.bin 0x8000000
```
