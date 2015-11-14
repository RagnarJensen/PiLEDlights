# PiLEDlights
For the Raspberry Pi - Make LEDs blink on network and/or SD card/USB storage activity.

hddledPi blinks a LED on any mass storage access. Not only on SD card access, but also on USB thumbdrive and hard drive activity. netledPi blinks a LED when there is activity on any network interface. Not only the built-in ethernet interface, but also on any USB ethernet or WiFi interface.

netledPi and hddledPi make use of Gordon Henderson's wiringPi library - wiringpi.com - so you have to have that installed in order to build the programs.

Building the programs is easy:  
gcc -Wall -O3 -o netledPi netledPi.c -lwiringPi  
gcc -Wall -O3 -o hddledPi hddledPi.c -lwiringPi  

hddledPi uses wiringPi pin 10 by default. It is BCM_GPIO 8, physical pin 24 on the Pi's P1 header.  
netledPi uses wiringPi pin 11 by default. It is BCM_GPIO 7, physical pin 25 on the Pi's P1 header.  
Note: These pins are also used for the SPI interface. If you have SPI add-ons connected, you'll have to use the -p option to change to another, unused pin.

Options:  
 -d, --detach               Detach from terminal (become a daemon),  
 -p, --pin=VALUE            GPIO pin (using wiringPi numbering scheme) where LED is connected  
 -r, --refresh=VALUE        Refresh interval (default: 20 ms)  
 
 The programs need super-user privileges, so you have to start them with "sudo", e.g.  
 sudo netledPi -d -p 29
 
