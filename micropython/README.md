# Micropython ESP8266 Weather Station

## Description

This is PoC Weather Station project written in Micropython language for ESP8266 device. 
It is designed to show possibilities and limits of current Micropython firmware 
and implement a simple InfluxDB2 Client with timestamp support. In order to have relatively accurate timestamps it also 
implements periodic RTC synchronisation with NTP server. OLED display 
(using included micropython's DHT11 and SSD1306 driver) shows DHT11 data with random screen coordinates
to avoid screen burn-in.

<img src="ws.gif" width="30%" height="30%">

It consists of:
  - `config.json` - JSON objects with configuration.
  - `webrepl_cfg.py` - WebREPL configuration. 
  - `boot.py` - Micropython boot file.
  - `main.py` - Micropython main code.
  - `wm.py` - Wi-Fi Manager to allow automatic wi-fi connections according to _known_networks_ entries in `config.json`. 
  In case of no _known_networks_ available it is also capable of running private access point with WebRepl.

## How to run the Micropython WeatherStation

1) Flash the firmware.
2) Install python shell.
3) Update `config.json`:
    - configure wi-fi connection details (With Internet connection for NTP routine)
    - add your influxdb2 connection details
    - adjust intervals
4) Set password for WebREPL connection in `webrepl_cfg.py`  
4) Adjust Point object parameters to reflect your infrastructure:
   ```python
   def publish(data, client):
    point = Point("dht11") \
        .field("temperature", data["t"]) \
        .field("humidity", data["rh"]) \
        .tag("location", "office") \
        .time(time.time(), WritePrecision.S)
   ...
   ```
   - Available functions for getting time:
     https://docs.micropython.org/en/latest/library/utime.html#module-utime
     
5) Connect to the device.
6) Put all files into device.
7) Restart WeatherStation device.

## Firmware

Before execution of any micropython code each ESP device must be erased and flashed with latest micropython firmware.

How-To Guide: https://micropython-docs-esp32.readthedocs.io/en/esp32_doc/esp8266/tutorial/intro.html#intro

Download page: https://micropython.org/download/esp8266/

## Remote shells

Used to manipulate with files and run python code over serial connection.

Top 3 shells to choose from:

Ampy: https://github.com/scientifichackers/ampy

Mpfshell: https://github.com/wendlers/mpfshell

Rshell: https://github.com/dhylands/rshell


## Caveats

### SSL error (OSError -40):

With Micropython's ESP8266 port version (=< 1.15) the https post to cloud2.influxdata.com fails with OSError -40. 
The issue seems to be that certain certificate types are not supported in axtls of underlining ESP-SDK.
Only solution seems to be wait until micropython will be supporting newer ESP-SDK with mbedtls, 
or compile your own micropython firmware with a hack described in:  
https://github.com/micropython/micropython-lib/issues/400

More details: https://docs.micropython.org/en/latest/esp8266/general.html#ssl-tls-limitations

### NTP time in UTC only:

There's currently no timezone support in MicroPython (1.15) for ESP8266 port, and the RTC is set in UTC time.

### Internal Real-time Clock limits:

RTC in ESP8266 has very bad accuracy, drift may be seconds per minute. 
As a workaround, to measure short enough intervals you can use utime.time(), etc. 
and/or synchronise time via ntp regularly. 
Due to limitations of the ESP8266 chip the internal real-time clock (RTC) will overflow every 7:45h. 
If a long-term working RTC time is required then time() or localtime() must be called at least once within 7 hours. 
MicroPython will then handle the overflow.

