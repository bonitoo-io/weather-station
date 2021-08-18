# Weather Station
## Description
TODO what it does 
- time,forecast, temp, wind, clouds, internal sensor, moon phse, video
- language

## QuickStart Guide

### WiFi Connection
Weather station requires WiFi network (for weather forecast, to store temperature/humidity history, ...)
Because it does not have any keyboard to configure the WiFi, the Weather Station requires your mobile phone or computer to set it up. Note that only 2.4GHz WiFi networks are supported.

WiFi Configuration Steps:
1) Plug USB cable into the Weather Station
2) The Weather Station shows a name of WiFi network for configuration that is created by the Weather Station itself
3) Connect your phone or laptop to this WiFi 
4) If phone or laptop shows that the WiFi does not offer Internet connection, please confirm the connection - it was created just for configuration of the Weather Station
5) A configuration page should be displayed automatically. If not, open your browser and type http://1.1.1.1. In both cases you should see Weather Station web page.
6) Select your home/company WiFi network from the list that Weather Station should connect. The list is sorted from the strongest one. If your WiFi network is hidden, please enter the name (SSID) manually
7) Enter your WiFi network password, if exists
8) Click to Save button
9) The Weather Station should reboot and sync all the data via the newly configured WiFi
10) If you see again the same message on the OLED display as in the second step, the network name or password has been entered incorrectly. Please repeat the process.


### InfluxDB Configuration
If you want to record your temperature and humidity measured by the Weather Station sensor, you need to setup InfluxDB account.
You can also use a free [InfluxDB Cloud](https://cloud2.influxdata.com) account, or you can deploy your own InfluxDB instance.
The following steps require a Weather Station with properly connected WiFi network (see the previous paragraph).

InfluxDB Configuration Steps:
1) Wait for the last page on the OLED display
2) Entre into browser in your phone or computer the address from the last page
3) Your browser should show Weather Station Web page. Your phone or computer must be connected to the same WiFi network as the Weather station otherwise you cannot see the web page.
4) Select InfluxDB Settings from the menu
5) Fill all required fields (specific bucket must be created in the InfluxDB as well as toke with read/write access)
6) Click to Save button
7) If you see an exclamation mark icon in the top left corner, the fields are not entered correctly, or Weather Station lost the connection to the InfluxDB
 
## Technical part

### Hardware

Developed for Weather Station DIY kit that consists of:
- `IoT Platform` - [ESP8266](https://docs.ai-thinker.com/_media/esp8266/docs/esp-12f_product_specification_en.pdf)
- `Display` - OLED 0,96" 128x64 [SSD1306](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- `Temperature and Humidity sensor` - [DHT11](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf)

### Source Code
This is IoT Weather Station project. It consist of three sub-projects:
1) [Full Arduino Implementation](arduino/WeatherStation#weather-station)
2) [Basic Micropython Implementation](micropython)
3) [3D Printer Case](case)
