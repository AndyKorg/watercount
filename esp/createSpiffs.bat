оепедеюкрэ!
D:\esp8266\esp8266_rtos_sdk\tools\spiffsgen.py --page-size 256 --block-size 4096 --obj-name-len 32 --meta-len 4 --use-magic True --use-magic-len True --big-endian False 49152 D:\esp8266\watercounter\html D:\esp8266\watercounter\WebBin\webpage.bin
python D:/esp8266/esp8266_rtos_sdk/components/esptool_py/esptool/esptool.py --chip esp8266 --port "COM10" --baud 230400 --before "default_reset" --after "hard_reset" write_flash -z 0x70000 D:\esp8266\watercounter\WebBin\webpage.bin
