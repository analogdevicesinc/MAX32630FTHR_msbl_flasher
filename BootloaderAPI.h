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