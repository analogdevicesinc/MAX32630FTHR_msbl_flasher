#include "BootloaderAPI.h"

const char* cmd_table[] {
    "bootldr",
    "exit",
    "reset",
    "page_size",
    "num_pages",
    "set_iv",
    "set_auth",
    "erase",
    "flash",
    "bootloader_version",
    "op_mode",
    "sh_version"
};

// Utility function for comparing the starting value of a string against another string.  Used for command decoding.
bool starts_with(const char* str1, const char* str2)
{
	while (*str1 && *str2) {
		if (*str1 != *str2)
			return false;
		str1++;
		str2++;
	}

	if (*str2)
		return false;

	return true;
}

void Bootloader_API::clear_serial_buffer() {
    for (int i = 0; i < SERIAL_BUFFER_SIZE; i++) {
        serial_buffer[i] = '\0';
    }

    buffer_index = 0;
}

Bootloader_API::Bootloader_API(Bootloader* bl, USBSerial* usb) {
    // Link USB and Bootloader pointers
    this->usb = usb;
    this->bl = bl;

    // Allocate and clear serial buffer
    this->serial_buffer = (char*) malloc(SERIAL_BUFFER_SIZE);
    clear_serial_buffer();
}

Bootloader_API::~Bootloader_API() {
    free(this->serial_buffer);
}

void Bootloader_API::receive() {
    // Receive incoming serial data into the global buffer.  When a newline or return character is received, parse the buffer into a command and signal ready.

    char c;

    while(usb->available()) {

        c = usb->getc();
        //usb->printf("%c", c); // Echo char back to terminal

        if (c == '\n' || c == '\r') { // Newline/return             
            // Terminate string, parse the command, then clear the serial buffer
            serial_buffer[ buffer_index++ ] = '\0';
            parse_command();
            clear_serial_buffer();
        }

        else if ( (c == 0x08 || c == 0x7F) && buffer_index > 0) { // Backspace
            // Remove a character from the serial buffer
            serial_buffer[ buffer_index-- ] = '\0';
        }

        else if (buffer_index < SERIAL_BUFFER_SIZE) { // Any other character
            // Add the character to the serial buffer
            serial_buffer[ buffer_index++ ] = c;
        } 

        else {
            // Overflow
        }

    }
}

void Bootloader_API::parse_command() {
    // Initialize variables
    bootloader_cmd_t recvd_cmd = cmd_none;
    sh_err_t status; // Stores the Sensor Hub status byte returned after any communication sequences
    char* msg = (char*) malloc(MAX_MSG_SIZE); // Used for relaying a message to the host application (PC)
    memset(msg, '\0', MAX_MSG_SIZE); // Initialize string buffer
    char* ret = (char*) malloc(MAX_MSG_SIZE); // Used for sending return values to the host application (PC)
    memset(ret, '\0', MAX_MSG_SIZE); // Initialize string buffer

    // Compare serial buffer against command table
    for (int i = 0; i < NUM_CMDS; i++) {
        if (starts_with( serial_buffer, cmd_table[i] )) {
            recvd_cmd = (bootloader_cmd_t)i;
            break;
        }
    }

    // Drive the bootloader class based on the received command.
    // This switch statement calls the correct Bootloader class method, saves the status byte that the sensor hub returned during the transaction, and forms a logging message based on what happened.  The status byte and logging message get returned to the host application after the switch statement
    switch(recvd_cmd) {
        case cmd_none:
            break;
        
        case cmd_enter_bootloader:
        {
            status = bl->enter_bootloader();

            if (status != SUCCESS) {
                strcpy(msg, "Failed to enter bootloader mode.");
            } else {
                strcpy(msg, "Entered bootloader mode.");
            }            
            break;
        }

        case cmd_exit_bootloader:
        {
            status = bl->exit_bootloader();

            if (status != SUCCESS) {
                strcpy(msg, "Failed to enter application mode.");
            } else {
                strcpy(msg, "Successfully entered application mode.");
            }
            break;
        }            

        case cmd_reset:
            bl->reset();
            strcpy(msg, "Reset pulse sent.");
            break;

        case cmd_page_size:
        {
            // Command for getting the page size supported by the bootloader
            int page_size;
            status = bl->get_page_size(&page_size);

            if (status != SUCCESS) {
                strcpy(msg, "Failed to retrieve page size.");
            } else {
                strcpy(msg, "Successfully retrieved page size.");
                snprintf(ret, MAX_MSG_SIZE, "%i", page_size);
            }

            break;
        }

        case cmd_num_pages:
        {
            int num_pages;
            int num_tok = sscanf(serial_buffer, "num_pages %d", &num_pages); // sscanf decodes the string in "serial_buffer" using a printf-like format.  Here, we're looking for the number of pages to set.

            if (num_tok != 1) {
                status = ERR_INPUT_VALUE;
                strcpy(msg, "Invalid parameter passed to set # of pages command.");
            } else {
                status = bl->set_num_pages((uint8_t)num_pages);

                if (status != SUCCESS) {
                    snprintf(msg, MAX_MSG_SIZE, "Failed to set number of pages to %i", num_pages);
                } else {
                    snprintf(msg, MAX_MSG_SIZE, "Successfully set number of pages to %i", num_pages);
                }
            }

            break;
        }            

        case cmd_set_iv:
        {
            // Set Initialization Vector bytes.  This is a special sequence of bytes from the msbl file.  See the MAX32664 UG for more details.
            char iv_bytes[AES_NONCE_SIZE];
            if ( !this->parse_iv( iv_bytes ) ) {
                status = ERR_INPUT_VALUE;
                strcpy(msg, "Failed to parse IV bytes - failed in the parse_iv function in the API.  Did the host application pass in the bytes correctly?");
            } else {
                status = bl->set_iv(iv_bytes);

                if (status != SUCCESS) {
                    strcpy(msg, "Failed to set IV bytes - Failed in the communications to the sensor hub.  See error code.");
                } else {
                    strcpy(msg, "Successfully set IV bytes");
                }
            }

            break;
        } 

        case cmd_set_auth:
        // Set authentication bytes.  This is another special sequence of bytes from the msbl file.  See the MAX32664 UG for more details.
            char auth_bytes[AES_AUTH_SIZE];
            if ( !this->parse_auth( auth_bytes ) ) {
                status = ERR_INPUT_VALUE;
                strcpy(msg, "Failed to parse auth bytes - failed in the parse_auth function in the API.  Did the host application pass in the bytes correctly?");
            } else {
                status = bl->set_auth( auth_bytes );

                if (status != SUCCESS) {
                    strcpy(msg, "Failed to set auth bytes - Failed in the communications to the sensor hub.  See error code.");
                } else {
                    strcpy(msg, "Sucessfully set auth bytes");
                }
            }

            break;

        case cmd_erase:
        {
            // Erase the existing application.  
            status = bl->erase();

            if (status != SUCCESS) {
                strcpy(msg, "Failed to erase existing application.");
            } else {
                strcpy(msg, "Successfully erased existing application");
            }

            break;
        }

        case cmd_flash:
        {
            /* 'flash' is a special two-part command.  After sending the flash command, the API then clears the serial buffer and waits for a full page's worth of data (including CRC bytes).
            
            The reason that this is a two-part command is that the API looks for '\n' or '\r' characters to signal the end of a command.  However, when sending msbl file data the data itself may contain the same byte value as a '\n' or '\r' character.  This will launch the command too early, multiple times, etc.  So this command became a two-part command so that another parser wouldn't have to be written.
            */

           // Flash command received.  Prep for msbl page data
            clear_serial_buffer();

            // Receive data into serial buffer
            int i = 0;
            char c;
            while(i < MAX_PAGE_SIZE + CHECKBYTES_SIZE) {
                if (usb->available()) {
                    c = usb->getc();
                    serial_buffer[ buffer_index++ ] = c;
                    i++;
                }
            }

            // Flash the page data
            status = bl->flash(serial_buffer);

            // Error check
            if (status != SUCCESS) {
                strcpy(msg, "Failed to flash page.");
            } else {
                strcpy(msg, "Successfully flashed page.");
            }

            break;
        }            

        case cmd_bootloader_version:
        {
            // Get the bootloader version (which is different than the Sensor Hub version)
            bl_version_t version;
            status = bl->get_bootloader_version(&version);

            if (status != SUCCESS) {
                strcpy(msg, "Failed to get bootloader version.  Is the device in bootloader mode?");
            } else {
                strcpy(msg, "Successfully retrieved bootloader version.");
                snprintf(ret, MAX_MSG_SIZE, "%u.%u.%u", version.major, version.minor, version.rev);
            }
            break;
        }

        case cmd_op_mode:
        {
            // Get the current operating mode
            sh_opmode_t op_mode;
            status = bl->get_operating_mode(&op_mode);
            if(status == SUCCESS) {
                if (op_mode == APPLICATION_MODE) { strcpy(ret, "Application"); }
                else if (op_mode == RESET) { strcpy(ret, "Reset"); }
                else if (op_mode == BOOTLOADER_MODE) { strcpy(ret, "Bootloader"); }
            } else {
                strcpy(msg, "Failed to get operating mode");
            }
            break;
        }

        case cmd_sh_version:
        {
            // Get the sensor hub version.
            sh_version_t sh_version;
            status = bl->get_sh_version(&sh_version);
            if (status == SUCCESS) {
                snprintf(ret, MAX_MSG_SIZE, "%u.%u.%u", sh_version.major, sh_version.minor, sh_version.rev);
            }

            break;
        }

        default:
            status = ERR_UNAVAIL_CMD;
            strcpy(msg, "Invalid command sent to bootloader API.");
    }

    // Send a response message to the host application
    // '$' is used as a special splitter character that the host program can use to break up the fields
    usb->printf("cmd=%s$ret=%s$err=%u$msg=%s\n", cmd_table[recvd_cmd], ret, status, msg);
}

// The two parsers below decode hex-encoded strings into byte sequences.  These are necessary so that the set_iv and set_auth commands can be done in one command string

bool Bootloader_API::parse_iv(char* out)
{
    char cmdStr[] = "set_iv ";
    int length = strlen(serial_buffer);
    int expected_length = strlen(cmdStr) + 2*AES_NONCE_SIZE;
    if (length != expected_length) {
        return false;
    }

    const char* ivPtr = serial_buffer + strlen(cmdStr);

    int num_found;
    int byteVal;
    for (int ividx = 0; ividx < AES_NONCE_SIZE; ividx++) {
        num_found = sscanf(ivPtr, "%2X", &byteVal);

        if (num_found != 1 || byteVal > 0xFF) {
            return false;
        }

        out[ividx] = (uint8_t)byteVal;
        ivPtr += 2;
    }

    return true;
}

bool Bootloader_API::parse_auth(char* out) {
    char cmdStr[] = "set_auth ";
    int length = strlen( this->serial_buffer);
    int expected_length = strlen(cmdStr) + 2*AES_AUTH_SIZE;
    if (length != expected_length) {
        return false;
    }

    const char* macPtr = this->serial_buffer + strlen(cmdStr);

    int num_found;
    int byteVal;
    for (int aidx = 0; aidx < AES_AUTH_SIZE; aidx++) {
        num_found = sscanf(macPtr, "%2X", &byteVal);

        if (num_found != 1 || byteVal > 0xFF) {
            return false;
        }

        out[aidx] = (uint8_t)byteVal;
        macPtr += 2;
    }

    return true;
}