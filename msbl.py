from ctypes import *
from colorama import *
from copy import deepcopy

class MsblHeader(Structure):
	_fields_ = [('magic', 4 * c_char),
				('formatVersion', c_uint),
				('target', 16 * c_char),
				('enc_type', 16 * c_char),
				('nonce', 11 * c_ubyte),
				('resv0', c_ubyte),
				('auth', 16 * c_ubyte),
				('numPages', c_ushort),
				('pageSize', c_ushort),
				('crcSize', c_ubyte),
				('resv1', 3 * c_ubyte)]

class AppHeader(Structure):
	_fields_ = [('crc32', c_uint),
				('length', c_uint),
				('validMark', c_uint),
				('boot_mode', c_uint)]

class Page(Structure):
	_fields_ = [('data', (8192 + 16) * c_ubyte)]

class CRC32(Structure):
	_fields_ = [('val', c_uint)]

class MsblFile:
    header = MsblHeader()
    pages = []
    crc32 = CRC32()

    def __init__( self, filename ):
        with open( filename, 'rb' ) as msbl:
            # Read out header	
            if msbl.readinto( self.header ) != sizeof( self.header ):
                raise( Exception( f"{Fore.RED}Failed to load msbl file {filename}.  Invalid header{Fore.WHITE}" ) )

            # Read out page data
            tmp_page = Page()
            while msbl.readinto( tmp_page ) == sizeof( tmp_page ):
                self.pages.append( deepcopy( tmp_page.data ) ) # Deep copy is used so that a new copy of the page data is actually appended to the list.  See https://docs.python.org/3/library/copy.html 

            # Verify the correct number of pages were read.
            if (len(self.pages) != self.header.numPages):
               raise( Exception (f"{Fore.RED}Failed to load msbl file.  Expected to read {self.header.numPages} pages but read {len(self.pages)} instead.{Fore.WHITE}") )


            # Read out CRC32		
            msbl.seek(-4, 2)
            msbl.readinto(self.crc32)

        # msbl file data is now loaded into class attributes (header, pages, crc32)

    def print_info(self):
        # Print MSBL file information
        print(f"\tMagic: {self.header.magic.decode('ASCII')}")
        print(f"\tFormat Version: {self.header.formatVersion}")
        print(f"\tTarget: {self.header.target.decode('ASCII')}")
        print(f"\tEncoding Type: {self.header.enc_type.decode('ASCII')}")
        print(f"\tNum Pages: {self.header.numPages}")
        print(f"\tPage Size: {self.header.pageSize}")
        print(f"\tCRC Size: {self.header.crcSize}")
        print(f"\tSize of header: {sizeof(self.header)}")
        print(f"\tResv0: {self.header.resv0}")
        print(f"\tCRC32: {hex(self.crc32.val)}")