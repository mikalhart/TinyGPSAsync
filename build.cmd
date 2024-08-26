:top
"C:\Users\Mikal\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\4.5.1/esptool.exe" --chip esp32c3 --port "COM27" --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 "c:\Users\Mikal\Documents\Arduino\libraries\TinyGPSAsync\build/DateTime.ino.bootloader.bin" 0x8000 "c:\Users\Mikal\Documents\Arduino\libraries\TinyGPSAsync\build/DateTime.ino.partitions.bin" 0xe000 "C:\Users\Mikal\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.8/tools/partitions/boot_app0.bin" 0x10000 "c:\Users\Mikal\Documents\Arduino\libraries\TinyGPSAsync\build/DateTime.ino.bin"
timeout /t 1 /nobreak > nul
goto top
