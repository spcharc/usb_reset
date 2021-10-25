# usb_reset
Linux commandline tool to reset a USB device

# Usage
It resets a USB device based on its product_id:vendor_id pair.
```
# ./usb_reset 413c:2003
Product: Dell USB Keyboard (length: 17)
Manufacturer: Dell (length: 4)
Resetting this device ...
Finished.
#
```
You can also provide the product name and manufacturer name. In this case, this program will only reset the device when both of the strings does not match what the USB device returns.
```
# ./usb_reset 413c:2003 "Dell USB Keyboard" Dell
#
```
(No output since it did nothing. However when the USB device does not work properly, usually it cannot return the correct strings, in this case it will be resetted.)

# Auto reset a USB device when it stops working
I personally use it in a loop to reset a USB device that often stops working and requires me to unplug and plug it back.
```
#!/bin/sh

a="0"
while [ "$a" -eq "0" ]
do
    ./usb_reset [YOUR PRODUCT_ID:VENDOR_ID PAIR] [YOUR PRODUCT_NAME] [YOUR MANUFACTURER_NAME]
    a="$?"
    sleep 5
done

echo Last returned "$a"
```

# Compile
First of all, install `libusb` header files. On Debian based systems the command can be:

`apt list libusb-1.0-0-dev`

Compile:

`g++ usb_reset.cpp -o usb_reset -lusb-1.0`

# License
GNU GPL v3.0
