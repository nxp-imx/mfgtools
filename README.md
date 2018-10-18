# uuu (Universal Update Utility), mfgtools 3.0

**original linux version using "linux" branch, windows version use "windows" branch**

Freescale/NXP I.MX Chip image deploy tools.

    uuu (universal update utility) for nxp imx chips -- libuuu-1.0.1-gffd9837

    Succues:0       Failure:3               Wait for Known USB Device Appear...

    1:11     5/5 [                                        ] SDP: jump -f u-boot-dtb.imx -ivtinitramf....
    2:1      1/5 [===>                                    ] SDP: boot -f u-boot-imx7dsabresd_sd.imx ....


**under development**

# key feature. 
 - The real cross platform. linux, windows, MacOS(not test yet)
 - Multi devices program support
 - Daemon mode support.
 - Small depedence (only libusb and zlibc)
 - Firmware (uboot/kernel) use WCID to auto load winusb drvier in windows side. win7 user need install winusb driver. https://zadig.akeo.ie/  win10 will install driver automatically.

# example:
```
  uuu u-boot.imx            Download u-boot.imx by HID device
  
  uuu list.uu               Run all commands in list.uu
  
  uuu -s                    Enter shell mode. input command. 

  uuu -v u-boot.imx         verbose mode
 
  uuu -d u-boot.imx         Once detect known device attach, download boot.imx. 
                            
                            u-boot.imx can be replaced, new file will be download once board reset.
                            
                            Avoid unplug sd, write sd, plug sd card when debug uboot.
                            
  uuu -b emmc u-boot.imx    write u-boot.imx to emmc boot partition. u-boot.imx need enable fastboot
```

# Build:

## windows
- git clone https://github.com/NXPmicro/mfgtools.git
- cd mfgtools
- git submodule init
- git submodule update
- open msvs/uuu.sln by vs2017

## linux
- git clone https://github.com/NXPmicro/mfgtools.git
- cd mfgtools
- sudo apt-get install libusb-1.0.0-dev libzip-dev
- cmake .
- make
