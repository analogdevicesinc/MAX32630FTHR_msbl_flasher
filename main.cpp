/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
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