setlocal
set BUILD_FOLDER=c:\tmp\_ArduinoOutput\d1mini\weatherstation
rem grep "#define VERSION.*[0-9]" ..\Version.h | sed -n "s/.*VERSION.*\([0-9]\.[0-9]\+.*\)""/\1/p"
set VERSION=0.54
pushd %BUILD_FOLDER%
copy /y WeatherStation.ino.bin ws-firmware-%VERSION%.bin
md5 ws-firmware-%VERSION%.bin >ws-firmware-%VERSION%.md5
popd
