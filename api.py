import serial
from dataclasses import dataclass

@dataclass
class response:
    cmd: str
    ret: str
    err: int
    msg: str
    
class bootloader_api():
    def __init__(self, port: str, BAUD=9600, timeout=3 ):
        self.s = serial.Serial(port=port, baudrate=BAUD, timeout=timeout)

    def send_cmd(self, cmd: str, suppress=False) -> response:  
        self.s.write ( bytes( f"{cmd}\n", "ASCII" ) )
        resp = self._parse_response( self.s.readline() )

        if resp is False:
            # Unexpected response/I2C communication failure
            err = f"Failed to parse expected response from command {cmd}!"
            if not suppress: raise( Exception( err ) )
            else:
                print(err)
                return False

        elif resp.err != 0:
            # Error code received from Sensor Hub
            err = f"Error code {resp.err} received from sensor hub during command {cmd} with message: {resp.msg}"
            if not suppress: raise( Exception( err ) )
            else:
                print(err)
                return False

        else:
            # Everything's ok, pass any return values received
            return resp.ret

    def flash_page(self, page: bytes):
        self.s.write(b"flash\n")
        self.s.write( bytes( page ) )
        resp = self._parse_response( self.s.readline() )

        if resp is False:
            # Unexpected response/I2C communication failure
            raise( Exception( "Failed to parse expected response when flashing page data!") )

        elif resp.err != 0:
            # Error code received from Sensor Hub
            raise( Exception (f"Received error code {resp.err} from sensor hub while flashing page data with message: {resp.msg}") )

        else:
            # Page flashed OK
            return True


    def _parse_response(self, resp: bytes):
        # Response will be a string with the format:  cmd=[some command]$ret=[return value]$err=[error code]$msg=[error msg]
        # This function splits the string up into tokens and returns the response as a decoded dictionary
        copy = resp # Save a copy for error msg
        resp = resp.decode("ASCII").split('$')

        d = dict()
        for field in resp:
            key_pair = field.split('=')
            if len(key_pair) == 2: 
                d[key_pair[0]] = key_pair[1] # Construct a dictionary entry from the key-value pair.  Key will be first

        # Convert to integer values where we can for convenience
        for key in d.keys():
            try:
                d[key] = int(d[key])
            except:
                pass

        # Validate response
        if "cmd" not in d.keys() or "ret" not in d.keys() or "err" not in d.keys() or "msg" not in d.keys():
            return False
            #raise( Exception( f"Unexpected response from device - failed to parse fields.  Received:\n{copy}\n... and parsed into:\n{ret}" ) )

        return response( d["cmd"], d["ret"], d["err"], d["msg"] )
