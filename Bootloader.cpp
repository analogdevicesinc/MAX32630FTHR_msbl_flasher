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

#include "Bootloader.h"

// "wait_ms" implementation for convenience.  mbed-os only gives us wait_us and wait_ns.
void wait_ms(int ms) {
    wait_us(ms * 1000);
}

Bootloader::Bootloader(I2C* i2c, PinName mfio, PinName rstn) {
    this->i2c = i2c;
    this->i2c->frequency(400000); // Set I2C Freq to 400kHz
    this->rstn = DigitalOut(rstn); // Load RSTN pin w/ state of 1
    this->rstn = 1;
    this->mfio = DigitalOut(mfio); // Load MFIO pin w/ state of 1
    this->mfio = 1;

    // Despite specifying output state of 1 and manually writing 1 to the pins, there is still about a 20us low 'glitch' on each of the pins as the initialize.
    wait_ms(10);
}

sh_err_t Bootloader::enter_bootloader() {
    // Hardware MFIO/RSTN sequence to enter bootloader mode
    rstn = 0;
    mfio = 0;
    wait_ms(10);
    rstn = 1;
    wait_ms(50);
    mfio = 1;

    // Device is now in bootloader mode and will enter application mode after ~780ms if the "enter bootloader" software command is not received.

    // Send the "Enter Bootloader" software command.  This must be sent!  Otherwise, the bootloader will exit prematurely.
    char send[3] = { 0x01, 0x00, 0x08 };
    char recv;

    i2c->write(this->sh_addr, send, 3);
    wait_ms(2);
    i2c->read(this->sh_addr, &recv, 1);

    return (sh_err_t)recv;
}

sh_err_t Bootloader::exit_bootloader() {
    // Hardware sequence for entering application mode.
    rstn = 0;
    mfio = 1;
    wait_ms(10);
    rstn = 1;
    wait_ms(50);

    // Device now in application mode, but initialization still has to complete for ~1.5s
    wait_ms(1500);

    return SUCCESS;
}

sh_err_t Bootloader::get_bootloader_version(bl_version_t* out) {
    // Get bootloader version
    char send[2] = { 0x81, 0x00 };
    char recv[4];

    i2c->write(this->sh_addr, send, 2);
    wait_ms(2);
    i2c->read(this->sh_addr, recv, 4);

    if (recv[0] != SUCCESS) {
        return (sh_err_t)recv[0];
    }

    out->major = (uint8_t)recv[1];
    out->minor = (uint8_t)recv[2];
    out->rev = (uint8_t)recv[3];

    return (sh_err_t)recv[0];
}

sh_err_t Bootloader::get_sh_version(sh_version_t* out) {
    // Get Sensor Hub firmware version.  This command is only available in application mode.
    char send[2] = { 0xFF, 0x03 };
    char recv[4];

    // 'C' variants will need MFIO pulse to wake up the Sensor Hub.  Only application mode commands need the MFIO pulse.
    mfio = 0;
    wait_us(300);
    i2c->write(this->sh_addr, send, 2);
    mfio = 1;

    wait_ms(2);

    mfio = 0;
    wait_us(300);
    i2c->read(this->sh_addr, recv, 4);
    mfio = 1;

    if (recv[0] != SUCCESS) {
        return (sh_err_t)recv[0];
    }

    out->major = (uint8_t)recv[1];
    out->minor = (uint8_t)recv[2];
    out->rev = (uint8_t)recv[3];

    return (sh_err_t)recv[0];
}

sh_err_t Bootloader::get_operating_mode(sh_opmode_t* out) {
    char send[2] = { 0x02, 0x00 };
    char recv[2];

    // 'C' variants will need MFIO pulse to wake up from sleep.
    mfio = 0;
    wait_us(300);
    i2c->write(this->sh_addr, send, 2);
    mfio = 1;

    wait_ms(2);

    mfio = 0;
    wait_us(300);
    i2c->read(this->sh_addr, recv, 2);
    mfio = 1;

    *out = (sh_opmode_t)recv[1];
    return (sh_err_t)recv[0];
}

void Bootloader::reset() {
    // Hardware reset
    rstn = 0;
    wait_ms(10);
    rstn = 1;
}

sh_err_t Bootloader::get_page_size(int* out) {
    char send[2] = { 0x81, 0x01 };
    char recv[3];

    i2c->write(this->sh_addr, send, 2);
    wait_ms(2);
    i2c->read(this->sh_addr, recv, 3);

    if (recv[0] != SUCCESS) {
        return (sh_err_t)recv[0];
    }

    *out = (int)( (recv[1] << 8) | (recv[0]) );
    return (sh_err_t)recv[0];
}

sh_err_t Bootloader::set_num_pages(int num_pages) {
    // Number of pages to flash is held in byte 44 of msbl file.  Bootloader expects to 2 bytes specifying number of pages to flash, MSB first.
    char send[4] = {0x80, 0x02, 0x00, (char)((num_pages & 0xFF))};
    char recv;

    i2c->write(this->sh_addr, send, 4);
    wait_ms(2);
    i2c->read(this->sh_addr, &recv, 1);

    return (sh_err_t)recv;
}

sh_err_t Bootloader::set_iv(char* iv_bytes) {
    char send[AES_NONCE_SIZE + 2];
    // Insert the family byte and index byte in front of the IV bytes before sending
    send[0] = 0x80;
    send[1] = 0x00;
    for (int i = 0; i < AES_NONCE_SIZE; i++) {
        send[i + 2] = iv_bytes[i];
    }

    char recv;

    i2c->write(this->sh_addr, send, AES_NONCE_SIZE + 2);
    wait_ms(2);
    i2c->read(this->sh_addr, &recv, 1);

    return (sh_err_t)recv;
}

sh_err_t Bootloader::set_auth(char* auth_bytes) {
    char send[AES_AUTH_SIZE + 2];
    // Insert the family byte and index byte in front of the auth bytes before sending
    send[0] = 0x80;
    send[1] = 0x01;
    for (int i = 0; i < AES_AUTH_SIZE; i++) {
        send[i + 2] = auth_bytes[i];
    }

    char recv;

    i2c->write(this->sh_addr, send, AES_AUTH_SIZE + 2);
    wait_ms(2);
    i2c->read(this->sh_addr, &recv, 1);

    return (sh_err_t)recv;
}

sh_err_t Bootloader::erase() {
    char send[2] = { 0x80, 0x03 };
    char recv;

    i2c->write(this->sh_addr, send, 2);
    wait_ms(3000);  // This command has a 1400ms delay listed.  However, for larger applications this can take longer.  
                    // The 3s window here is used to account for the largest page size (29 pages).  A "safe" number was found to be 100ms/page.
    i2c->read(this->sh_addr, &recv, 1);

    return (sh_err_t)recv;
}

sh_err_t Bootloader::flash(char* page) {
    char* send = (char*) malloc(MAX_PAGE_SIZE + CHECKBYTES_SIZE + 2);
    send[0] = 0x80;
    send[1] = 0x04;
    memcpy(&send[2], page, MAX_PAGE_SIZE + CHECKBYTES_SIZE);

    char recv;

    i2c->write(this->sh_addr, send, MAX_PAGE_SIZE + CHECKBYTES_SIZE + 2);
    wait_ms(680);
    i2c->read(this->sh_addr, &recv, 1);

    free(send);

    return (sh_err_t)recv;
}
