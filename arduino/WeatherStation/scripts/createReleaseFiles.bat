setlocal
set BUILD_FOLDER=c:\tmp\_ArduinoOutput\d1mini\weatherstation
rem grep "#define VERSION.*[0-9]" ..\Version.h | sed -n "s/.*VERSION.*\([0-9]\.[0-9]\+.*\)""/\1/p"
set VERSION=1.15
pushd %BUILD_FOLDER%
copy /y WeatherStation.ino.bin ws-firmware-%VERSION%.bin
copy /y WeatherStation.ino.elf ws-firmware-%VERSION%.elf
copy /y WeatherStation.ino.map ws-firmware-%VERSION%.map
md5 ws-firmware-%VERSION%.bin >ws-firmware-%VERSION%.md5
popd
