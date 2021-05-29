# Micropython ESP8266 Weather Station

## Description

TBD

## How to run the Micropython WeatherStation

1) Flash the firmware
2) Install python shell
3) update `config.json`:
    - add your wifi AP details (with Internet connection)
    - add your influxdb2 instance details
    - adjust intervals
3) Connect to the device
4) put `boot.py`, `main.py`, `influxdb2.py`, `ntp.py`, `wm.py`, `config.json` into device
5) restart WeatherStation device

## Firmware

Before execution of any micropython code each ESP device must be erased and flashed with latest micropython firmware.

Guide: https://micropython-docs-esp32.readthedocs.io/en/esp32_doc/esp8266/tutorial/intro.html#intro
Download page: https://micropython.org/download/esp8266/

## Remote shells

Used to manipulate with files and run python code over serial connection.

Top 3 shells to choose from:

Ampy: https://github.com/scientifichackers/ampy

Mpfshell: https://github.com/wendlers/mpfshell

Rshell: https://github.com/dhylands/rshell

## Notice about ssl error (OSError -40):

With current Micropython version 1.15 the https post to cloud2.influxdata.com fails with OSError -40. 
The issue seems to be that certain certificate types are not supported in axtls of underlining ESP-SDK.
Only solution seems to be wait until micropython will be supporting newer ESP-SDK with mbedtls, 
or compile your own micropython firmware with a hack described in: https://github.com/micropython/micropython-lib/issues/400


