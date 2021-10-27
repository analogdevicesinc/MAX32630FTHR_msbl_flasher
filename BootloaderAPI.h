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
#include "Bootloader.h"

#define SERIAL_BUFFER_SIZE 16384
#define MAX_MSG_SIZE 256

typedef enum {
    cmd_none = -1,
    cmd_enter_bootloader,
    cmd_exit_bootloader,
    cmd_reset,
    cmd_page_size,
    cmd_num_pages,
    cmd_set_iv,
    cmd_set_auth,
    cmd_erase,
    cmd_flash,
    cmd_bootloader_version,
    cmd_op_mode,
    cmd_sh_version,
    NUM_CMDS
} bootloader_cmd_t;

// This bootloader API class implements the serial command set and drives the correct method in the low-level Bootloader class for each command.  It handles parsing incoming data and relaying error messages back to the host application.

class Bootloader_API {
public:
    Bootloader_API(Bootloader* bl, USBSerial* usb);
    ~Bootloader_API();
    void receive();
protected:
    void parse_command();
    void clear_serial_buffer();
    bool parse_iv(char* out);
    bool parse_auth(char* out);
private:
    char* serial_buffer;
    int buffer_index = 0;
    USBSerial* usb;
    Bootloader* bl;
};