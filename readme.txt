Installation:
1. Install Python 3 and add it to the PATH.  Ensure pip is added to path as well.

2. Open a command prompt and cd into this directory.

3. "pip install -r requirements.txt" to install Python libraries (colorama and pyserial)

Usage:
https://maximsupport.microsoftcrmportals.com/en-us/knowledgebase/article/KA-13834

Same procedure as the article above, but flash the MAX32630FTHR with "SH_Bootloader_v012.bin" instead of "MRD220_MAX32630_Host_FW_x.x.x_ASCII.bin".

The command in step 4 will now be a Python command instead:
"python flash.py -f [msbl filename] -p [your COM port]

The delay factor (-d) is no longer needed.