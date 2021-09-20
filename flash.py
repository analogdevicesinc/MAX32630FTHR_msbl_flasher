"""
/*******************************************************************************
* Copyright (C) Maxim Integrated Products, Inc., All rights Reserved.
* 
* This software is protected by copyright laws of the United States and
* of foreign countries. This material may also be protected by patent laws
* and technology transfer regulations of the United States and of foreign
* countries. This software is furnished under a license agreement and/or a
* nondisclosure agreement and may only be used or reproduced in accordance
* with the terms of those agreements. Dissemination of this information to
* any party or parties not specified in the license agreement and/or
* nondisclosure agreement is expressly prohibited.
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
"""

import argparse
import colorama
import serial
from colorama import *
import time
from msbl import MsblFile

def parse_response(resp: bytes):
	# Response will be a string with the format:  cmd=[some command]$ret=[return value]$err=[error code]$msg=[error msg]
	# This function splits the string up into tokens and returns the response as a decoded dictionary
	copy = resp # Save a copy for error msg
	resp = resp.decode("ASCII").split('$')

	ret = dict()
	for field in resp:
		key_pair = field.split('=')
		if len(key_pair) == 2: 
			ret[key_pair[0]] = key_pair[1] # Construct a dictionary entry from the key-value pair.  Key will be first

	# Convert to integer values where we can for convenience
	for key in ret.keys():
		try:
			ret[key] = int(ret[key])
		except:
			pass

	# Validate response
	if "cmd" not in ret.keys() or "ret" not in ret.keys() or "err" not in ret.keys() or "msg" not in ret.keys():
		raise( Exception( f"Unexpected response from device - failed to parse fields.  Received:\n{copy}\n... and parsed into:\n{ret}" ) )

	return ret

if __name__ == "__main__":
	# Parse command line arguments
	parser = argparse.ArgumentParser()
	parser.add_argument("-f", "--msblfile", required=True, type=str, help="msbl file as input")
	parser.add_argument("-p", "--port", required=True, type=str, help=("Serial port name in Windows and device file path in Linux."))
	args = parser.parse_args()

	colorama.init(convert=True)

	print("Reading msbl file " + Fore.YELLOW + args.msblfile + Fore.WHITE + "...", end="")
	msbl = MsblFile(args.msblfile)
	print(Fore.GREEN + "Success!" + Fore.WHITE)
	msbl.print_info()

	# Open serial port to MAX32630FTHR
	print(f"Opening serial connection to MAX32630FTHR on {Fore.YELLOW}{args.port}{Fore.WHITE}...", end="")
	try:
		s = serial.Serial(args.port)
		s.timeout = 5
	except Exception as e:
		print(f"{Fore.RED}Failed.")
		print(f"{Fore.CYAN}Is the MAX32630FTHR connected?  Is the right port specified?{Fore.WHITE}")
		raise(e)
	
	print(f"{Fore.GREEN}Success!{Fore.WHITE}")

	# Let Sensor Hub initialize
	print("Sleeping for 1.5s to let Sensor Hub initialize...")
	time.sleep(1.5)

	# Check for any existing firmware
	print("Checking for existing firmware...")
	print("\tAttempting to retrieve version number...", end="")
	s.write(b"sh_version\n")
	resp = parse_response(s.readline())

	if (resp["err"] != 0):
		print(f"\t{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		print(f"{Fore.CYAN}Failed to retrieve current version #, but there may not be anything flashed.  Attempting to proceed anyways...{Fore.WHITE}")

	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")
		print(f"\t{Fore.YELLOW}Current firmware is v{resp['ret']}{Fore.WHITE}")

	# Enter bootloader mode
	print(f"Entering bootloader mode...", end="")
	s.write(b"bootldr\n")
	resp = parse_response(s.readline())

	# Error check
	if resp["err"] != 0: 
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		raise( Exception( "Failed to enter bootloader mode!  Failed on sending the software command." ) )
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")

	# Verify operating mode
	print(f"Retrieving operating mode...", end="")
	s.write(b"op_mode\n")
	resp = parse_response(s.readline())

	# Error check
	if resp["err"] != 0: 
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		raise( Exception( "Failed to retrieve current operating mode!" ) )
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")
		if resp["ret"] != "Bootloader":
			raise( Exception (f"Expected to be in bootloader mode, but in {resp['ret']} mode instead!") )
		else:
			print(f"{Fore.GREEN}Verified bootloader mode.{Fore.WHITE}")

	# Get bootloader version
	print(f"Getting bootloader version...", end="")
	s.write(b"bootloader_version\n")
	resp = parse_response(s.readline())

	# Error check
	if resp["err"] != 0:
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		raise( Exception( "Failed to get bootloader version!" ) )
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")
		print(f"\t{Fore.YELLOW}Bootloader version: {resp['ret']}{Fore.WHITE}")

	# Get page size
	print(f"Getting page size...", end="")
	s.write(b"page_size\n")
	resp = parse_response(s.readline())

	# Error check
	if resp["err"] != 0:
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		raise( Exception( "Failed to get supported page size!" ) )
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")
		print(f"\tPage size: {resp['ret']}...", end="")
		if resp['ret'] == msbl.header.pageSize:
			print(f"{Fore.GREEN}Page size in bootloader matches page size of msbl file.{Fore.WHITE}")
		else:
			print(f"{Fore.RED}Page size in bootloader does not match page size of msbl file ({msbl.header.pageSize})")

	# Set number of pages to flash
	print(f"Setting number of pages to flash from msbl file ({msbl.header.numPages})...", end="")
	s.write(f"num_pages {msbl.header.numPages}\n".encode("ASCII"))
	resp = parse_response(s.readline())

	# Error check
	if resp["err"] != 0:
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		raise( Exception( "Failed to set number of pages to flash!" ) )
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")

	# Set IV bytes ('nonce' from the header)
	print(f"Setting Initialization Vector (IV) bytes...", end="")
	nonce_hex = "".join("{:02X}".format(c) for c in msbl.header.nonce) # Firmware expects an ASCII representation of the hex values for the IV bytes concatenated into one long string
	s.write(f"set_iv {nonce_hex}\n".encode("ASCII"))

	# Error check
	resp = parse_response(s.readline())
	if resp["err"] != 0:
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		raise( Exception( "Failed to set IV bytes!" ) )
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")

	# Set authentication bytes
	print(f"Setting Authentication bytes...", end="")
	auth_hex = "".join("{:02X}".format(c) for c in msbl.header.auth)
	s.write(f"set_auth {auth_hex}\n".encode("ASCII"))

	# Error check
	resp = parse_response(s.readline())
	if resp["err"] != 0:
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		raise( Exception( "Failed to set authentication bytes!" ) )
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")

	# Erase existing application flash memory
	print(f"Erasing existing application flash memory...", end="")
	s.write(b"erase\n")

	resp = parse_response(s.readline())
	if resp["err"] != 0:
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		raise( Exception( "Failed to erase existing application!" ) )
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")

	# Flash the msbl file page by page
	print("Flashing msbl file...")
	i = 1
	for page in msbl.pages:
		print(f"\tFlashing page {i}/{int(msbl.header.numPages)}...", end="")
		s.write(b"flash\n") # Send flash command
		s.write(bytes(page)) # Send page data
	
		# Error check
		resp = parse_response(s.readline())

		if resp["err"] != 0:
			print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
			raise( Exception( f"Failed to flash page {i}/{int(msbl.header.numPages)}!" ) )
		else:
			print(f"{Fore.GREEN}Success!{Fore.WHITE}")

		i += 1

	# Exit bootloader mode, enter application mode
	print("Entering application mode...", end="")
	s.write(b"exit\n")
	resp = parse_response(s.readline())

	if resp["err"] != 0:
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")

	# Verify application mode
	print(f"Retrieving operating mode...", end="")
	s.write(b"op_mode\n")
	resp = parse_response(s.readline())

	# Error check
	if resp["err"] != 0: 
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
		raise( Exception( "Failed to retrieve current operating mode!" ) )
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")
		if resp["ret"] != "Application":
			raise( Exception (f"Expected to be in application mode, but in {resp['ret']} mode instead!") )
		else:
			print(f"{Fore.GREEN}Verified application mode.{Fore.WHITE}")

	# Verify new firmware version
	print("Retrieving new firmware version...", end="")
	s.write(b"sh_version\n")
	resp = parse_response(s.readline())

	if resp["err"] != 0:
		print(f"{Fore.RED}Failed.\n{resp}{Fore.WHITE}")
	else:
		print(f"{Fore.GREEN}Success!{Fore.WHITE}")

	print(f"New firmware version is {Fore.YELLOW}v{resp['ret']}{Fore.WHITE}")
