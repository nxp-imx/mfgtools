CC ?= g++
CMAKE ?= cmake
CFLAGS ?= -L . -Wl,-rpath=. -l:libMfgToolLibStatic.a -I../MfgToolLib -I/usr/include/libusb-1.0 -l usb-1.0 -g -static-libgcc -static-libstdc++ -l:libstdc++.a  -std=c++11 -l pthread  -fpermissive -Wno-write-strings -l:libfdt.a
default:
	cd ../MfgToolLib && make
	cp ../MfgToolLib/libMfgToolLib.so ./
	cp ../MfgToolLib/libMfgToolLibStatic.a ./
	$(CC) mfgtoolCLI.cpp $(CFLAGS) -o mfgtoolcli
cmake:
	cd ../MfgToolLib && $(CMAKE) .
clean:
	rm ./mfgtoolcli
	cd ../MfgToolLib && make clean
