# Changelog
## 1.13
## Fixes
 - Improved Web server response
 - Improved stability
 - Improved memory footprint
 - Optimized settings validations
 - Faster switch from AP mode
 - Added geo location fallback when Weather Forecast is not available for city

## 1.12
## Fixes
- Fixed temp offset calculation in calibration

## 1.11
## Features
- Support for all Sensirion SHT* sensors
  - Automatic detection
- Improved OLED config screen
## Fixes
- WebUI
  - More robust data requests to server
  - Occasional WiFi scanning error
  - Skip validations when device is in AP mode

## 1.10
## Features
 - Sensor Calibration Dialog

## 1.05
## Features
 - Display Night Mode
## Fixes
 - Faster initial WiFi reconnection
 - Improved web server response caching

## 1.01
## Fixes
  - Fixed limit on list of scanned WiFi networks
  - Updated Github certificate for firmware download
  - Firmware update does not verify certificate by default
  - Reliable web server start after regional settings update
## 1.00
## Features
 - Restructured settings UI
  - Introduced Display settings
  - Update setting moved to Advanced settings
## Fixes
 - Non-latin languages forced to English

## 0.59
## Features
 - Improved OLED debug screen
 - Added repeated resets check
## Fixes
 - Trying update weather info more times

## 0.58
 - Fixed update version comparing

## 0.58-rc9
## Fixes
- Fixed update version comparing

## 0.58-rc8
## Fixes
 - Improved Web UI validation messages
 - Improved DB write error handling
 - Fixed occasionally missing tags
 - Fixed update version comparing

## 0.58 rc7
## Features
 - Sensirion SHTC3 temperature and humidity sensor support
 - Autocalibration of DHT11
## Fixes
 - Improved initial WiFi settings behavior
 - Fixed HTTP server response on cached URL
 - Improved Web UI validation messages

## 0.58 rc6
## Features
 - Dual color OLED support
 - Humidity offset autocalibration (when offset is 0)
 - Auto-reboot when InfluxDB write fails for the whole hour
## Fixes
 - Memory optimization
 - IP loc detection reloads country specific settings when location is unchanged

## 0.58 rc5
## Features
 - InfluxDB client with streaming write
 - Web Server improvements:
   - All request logging
   - No cache for api responses
   - Concurrent writing and api request check
   - All UI requests to server handle 503 and 427
  - Added Covid-19 OLED screen

## 0.58 rc4
## Fixes
 - InfluxDB Client with fixed disabled retrying
 - Decreased batch size
 - Increased HTTP timeout

## 0.58 rc3
## Fixes
 - Reverting InfluxDB Client to the latest release

## 0.58 rc2
## Features
 - New application icon
 - Reporting WiFi reconnects count
## Fixes
 - Detecting UTC offset change always
 - Improved stability

## 0.58 rc1
## Fixes
 - Improved initial web page loading
 - Using memory optimized InfluxDB Client

## 0.57 [2021-10-12]
## Features
 - Split Status page to About and Status
   - Added automatic refresh to Status page
 - Added monitoring of synchronization services
 - Added Advanced Settings
   - Including ability to configure display screens

## Fixes
 - Fixed setting configure language after reset
 - Improved web server response stability

## 0.56 [2021-09-30]
 - Improved WiFi setting wizard
   - Improved on-screen instruction
   - Initial landing page when WiFi is not working is WiFi Scanner
   - WiFi signal strength is shown in percents
 - Added readme with user guide
 - Smoother app icon
 - Added Updater Settings
 - Added WiFi network management
   - Each connected network is remembered
 - Added Regional Settings
   - Possibility to disable location detection
   - Manual setting of regional parameters
 - Allowing upload firmware via web interface

## 0.55
 - Added validation of WiFi params during initial configuration
 - Simplified on screen setup instructions
 - Removed passwords from debug output
 - Using [custom memory optimized esp8266-weather-station library fork](https://github.com/bonitoo-io/esp8266-weather-station)
 - Web server is shut down during updating data to save RAM

## 0.54
 - InfluxDB Settings token obfuscation
 - WifiScan page improved
 - Built with SDK 3.0.0

## 0.53
 - Display improvements
 - About temperature unit fix
 - Added writing of device status

## 0.52
 - Fixing on time update issue (#13)

## 0.51
 - About page
 - Display improvement

## 0.50
 - InfluxDB server settings web UI

## 0.49
 - Updating from github using template file name and MD5 checksum - version 1.0.0 of ESP-GitHub-Updater is required

## 0.48
 - OTA updates from github