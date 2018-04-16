# ESP Server
a simple webserver for the esp8266 boards.

files are served off the root of a sdcard and are restricted to the 8:3 filename format.
it supports uart over wifi on port 24.

the board can be updated by placing the update bin file in the root of the sdcard with the filename "fwupdate.bin" and rebooting board.
make sure the compiled bin/update file is for your board type.

