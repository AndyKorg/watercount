�������� �� ���� ���������� �� �� ������������� ������ ������ SPIFFS ���������� �������
����� html ����������� � ��� � ������� createSpiffs.bat
��������� ��������:

* ������� ���������� ������ � �������� �������:
D:\esp8266\esp8266_rtos_sdk\tools\spiffsgen.py 49152 D:\esp8266\watercounter\html D:\esp8266\watercounter\WebBin\webpage.bin
49152 - ��� ������ ������� ����������� ��� spiffs ������� ������� ����� (�� ��������� ���� 4096)
� ���� ������� ������ ����� ����������� � �������� ������� �������� partitions.csv
������:
...
storage,  data, spiffs,  ,        50K,  - 50 Kb - ��������� ������� ����� 49152

* ������ � ���:
python D:/esp8266/esp8266_rtos_sdk/components/esptool_py/esptool/esptool.py --chip esp8266 --port "COM10" --baud 230400 --before "default_reset" --after "hard_reset" write_flash -z 0x70000 D:\esp8266\watercounter\WebBin\web.bin

0x70000  - ����� � �������� ���������� ������ SPIFFS, ���� ���� �� ������ � ������� ��������, �� ����� ��������� ����� � ���������:
...
I (53) boot: SPI Mode       : QIO
I (56) boot: SPI Flash Size : 1MB
I (59) boot: Partition Table:
I (62) boot: ## Label            Usage          Type ST Offset   Length
I (68) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (75) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (81) boot:  2 factory          factory app      00 00 00010000 00060000
I (88) boot:  3 storage          Unknown data     01 82 00070000 0000c800  <--- Offset ��� � ���� ����� ������ ������� spiffs


python F:/esp8266/esp8266_rtos_sdk/components/esptool_py/esptool/esptool.py --chip esp8266 --port "COM4" --baud 230400 --before "default_reset" --after "hard_reset" write_flash -z 0x110000 F:\esp8266\watercounter\WebBin\web.bin
