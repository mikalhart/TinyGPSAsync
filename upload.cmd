@echo off
setlocal

:: Check if a parameter is provided; if not, use default value
if "%~1"=="" (
    set "SketchName=TTF.ino"
) else (
    set "SketchName=%~1"
)

:: Construct file paths using the SketchName variable
set "bootloaderPath=c:\Users\Mikal\Documents\Arduino\libraries\TinyGPSAsync\build\%SketchName%.bootloader.bin"
set "partitionsPath=c:\Users\Mikal\Documents\Arduino\libraries\TinyGPSAsync\build\%SketchName%.partitions.bin"
set "firmwarePath=c:\Users\Mikal\Documents\Arduino\libraries\TinyGPSAsync\build\%SketchName%.bin"

:top
"C:\Users\Mikal\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\4.5.1\esptool.exe" --chip esp32c3 --port "COM72" --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 "%bootloaderPath%" 0x8000 "%partitionsPath%" 0xe000 "C:\Users\Mikal\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.8\tools\partitions\boot_app0.bin" 0x10000 "%firmwarePath%"

timeout /t 1 /nobreak > nul
goto top
