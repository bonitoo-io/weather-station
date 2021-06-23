setlocal
set BUILD_FOLDER=c:\tmp\_ArduinoOutput\d1mini\weatherstation
pushd %BUILD_FOLDER%
copy /y WeatherStation.ino.bin ws-firmware-0.50.bin
md5 ws-firmware-0.50.bin >ws-firmware-0.50.md5
popd
