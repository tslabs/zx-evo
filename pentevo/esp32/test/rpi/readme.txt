
--- 1. Enable SPI
sudo nano /boot/firmware/config.txt

Add or uncomment:
dtparam=spi=on

sudo reboot

ls -l /dev/spidev*


--- 2. Connect ESP-SPI

For ESP32-S3:

MOSI    GPIO13
MISO    GPIO11
SCLK    GPIO12
CS      GPIO10

For ESP32-P4:
MOSI    GPIO20
MISO    GPIO22
SCLK    GPIO21
CS      GPIO23

RPi:
MOSI = GPIO10 (pin 19)
MISO = GPIO9  (pin 21)
SCLK = GPIO11 (pin 23)
CS0  = GPIO8  (pin 24)
CS1  = GPIO7  (pin 26)
