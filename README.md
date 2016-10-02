# PiLEDlights
For the Raspberry Pi - Make LEDs blink on network and/or SD card/USB storage activity.

hddledPi blinks a LED on any mass storage access. Not only on SD card access, but also on USB thumbdrive and hard drive activity.  
netledPi blinks a LED when there is activity on any network interface. Not only the built-in ethernet interface, but also on any other USB ethernet or WiFi interface.  
actledPi blinks the Pi's ACT led on all mass storage I/O, i.e. not only the SD card.

netledPi and hddledPi make use of Gordon Henderson's wiringPi library - wiringpi.com - so you have to have that installed in order to build the programs. actledPi is a standalone, no special libraries are needed.

Building the programs is easy:  
gcc -Wall -O3 -o netledPi netledPi.c -lwiringPi  
gcc -Wall -O3 -o hddledPi hddledPi.c -lwiringPi  
gcc -Wall -O3 -o actledPi actledPi.c

hddledPi uses wiringPi pin 10 by default. It is BCM_GPIO 8, physical pin 24 on the Pi's P1 header.  
netledPi uses wiringPi pin 11 by default. It is BCM_GPIO 7, physical pin 25 on the Pi's P1 header.  
Note: These pins are also used for the SPI interface. If you have SPI add-ons connected, you'll have to use the -p option to change to another, unused pin.

Options for netledPi and hddledPi:  
 -d, --detach               Detach from terminal (become a daemon),  
 -p, --pin=VALUE            GPIO pin (using wiringPi numbering scheme) where LED is connected  
 -r, --refresh=VALUE        Refresh interval (default: 20 ms)  
 
 Options for actledPi:  
 -d, --detach               Detach from terminal (become a daemon)  
 -r, --refresh=VALUE        Refresh interval (default: 20 ms)  
 
 netledPi and hddledPi need super-user privileges, so you have to start them with "sudo", e.g.  
 sudo netledPi -d -p 29
 
 I have only tested the programs on Raspbian.
