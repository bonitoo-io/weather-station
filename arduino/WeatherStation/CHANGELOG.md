# Changelog
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