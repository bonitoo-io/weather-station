@echo off
SET COM_PORT=COM14
SET BIN_FILE=ws-firmware-1.01.bin
SET SDK_VERSION=3.0.2
SET SDK_VERSION_TOOLS=3.7.2-post1

%APPDATA%\..\Local\Arduino15\packages\esp8266\tools\python3\%SDK_VERSION_TOOLS%\python3 -I %APPDATA%\..\Local\Arduino15\packages\esp8266\hardware\esp8266\%SDK_VERSION%\tools\upload.py --chip esp8266 --port %COM_PORT% --baud 921600 --before default_reset --after hard_reset write_flash 0x0 %BIN_FILE%
@echo on