/*******************************************************************************
* Copyright (C) Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*******************************************************************************
*/

#include "mbed.h"
#include "USBSerial.h"
#include "BootloaderAPI.h"

////////////////////////////////
// COMPILER DEFINITIONS
////////////////////////////////
#define LED_ON 0
#define LED_OFF 1

////////////////////////////////
// FLAGS
////////////////////////////////

int main()
{
    // LED pin
    DigitalOut led(LED1);
    led = LED_ON;

    // Load sensor interface pins
    I2C i2c(P3_4, P3_5);
    PinName mfio(P5_4);
    PinName rstn(P5_6);

    // Main bootloader class.  This drives the sensor interface pins
    Bootloader bl(&i2c, mfio, rstn);

    // Set up USB.  These default HID/PID values set up a generic virtual COM port, and the 
    // class will block here until a USB connection is made
    USBSerial usb(true, 0x1f00, 0x2012, 0x0001);    

    // API class to drive the bootloader class via USB serial commands
    Bootloader_API api(&bl, &usb);

    // Main loop
    while (true) {

        // Service incoming serial data
        if (usb.readable()) {
            api.receive();
            led = !led;
        }

        led = LED_ON;
        
    }
}