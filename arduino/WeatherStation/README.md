# Weather Station

Weather station with OLED and WiFi (using ESP8266, SSD1306, and DHT11) with InfluxDB support

<img src="oled.gif" width="30%" height="30%">

## Arduino IDE

* Into "File/Preferences/Additional Boards Manager URLs:" add "http://arduino.esp8266.com/stable/package_esp8266com_index.json"
* Menu Tools/Board:/Boards Manager select esp8266 and Install it
* Select Tools/Board: NodeMCU 1.0 (ESP-12E Module)
* Connect weather station via USB
* Select Tools/Port: the serial port created by the weather station
* Open WeatherStation.ino file
* Update OPEN_WEATHER_MAP_API_KEY
* Click the secon icon in the toolbar "Upload"

* Create iot_center bucket inside InfluxDB

## Required Libraries

Libraries are accessible via Library manager or must installed manually, if source URL is provided.
Indentation means transitive dependency, but such a library must be installed as well.

* ESP8266 and ESP32 OLED driver for SSD1306 displays (by ThingPulse, Fabrice Weinberg)
* ESP8266 Weather Station (by ThingPulse) optimized version - from https://github.com/bonitoo-io/esp8266-weather-station branch [all-fixes](https://github.com/bonitoo-io/esp8266-weather-station/tree/all-fixes)
  * Json Streaming Parser (by Daniel Eichhorn)
* Support for Sensirion's humidity and temperature sensors - Supported sensors: SHTC1, SHTC3, SHTW1, SHTW2, SHT3x-DIS (I2C), SHT85, SHT3x-ARP, SHT4x
  * arduino-sht
* ESP8266 Influxdb (by Tobias Schürg, InfluxData)
* ESP Github Updater - from https://github.com/bonitoo-io/ESP-Github-Updater
* ESPAsyncWebServer - from https://github.com/bonitoo-io/ESPAsyncWebServer/ branch [feat/global_filter](https://github.com/bonitoo-io/ESPAsyncWebServer/tree/feat/global_filter)
  * ESPAsyncTCP - from https://github.com/bonitoo-io/ESPAsyncTCP
