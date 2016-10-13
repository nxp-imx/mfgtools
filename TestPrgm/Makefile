CC ?= g++
CMAKE ?= cmake
CFLAGS ?= -L . -Wl,-rpath=. -l MfgToolLib -I../MfgToolLib -I/usr/include/libusb-1.0 -l usb-1.0 -g -l stdc++ -std=c++11 -l pthread  -fpermissive -Wno-write-strings

default:
	cd ../MfgToolLib && make
	cp ../MfgToolLib/libMfgToolLib.so ./
	$(CC) mfgtoolCLI.cpp $(CFLAGS) -o mfgtoolcli
cmake:
	cd ../MfgToolLib && $(CMAKE) .
clean:
	rm ./mfgtoolcli
	cd ../MfgToolLib && make clean
