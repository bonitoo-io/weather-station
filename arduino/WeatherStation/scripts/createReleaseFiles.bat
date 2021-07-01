setlocal
set BUILD_FOLDER=c:\tmp\_ArduinoOutput\d1mini\weatherstation
pushd %BUILD_FOLDER%
copy /y WeatherStation.ino.bin ws-firmware-0.54.bin
md5 ws-firmware-0.54.bin >ws-firmware-0.54.md5
popd
