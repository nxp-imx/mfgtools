# uuu (Universal Update Utility), mfgtools 3.0

![Build status](https://ci.appveyor.com/api/projects/status/github/NXPmicro/mfgtools?svg=true)

**original linux version using "linux" branch, windows version use "windows" branch**

Freescale/NXP I.MX Chip image deploy tools.

    uuu (universal update utility) for nxp imx chips -- libuuu-1.0.1-gffd9837

    Succues:0       Failure:3               Wait for Known USB Device Appear...

    1:11     5/5 [                                        ] SDP: jump -f u-boot-dtb.imx -ivtinitramf....
    2:1      1/5 [===>                                    ] SDP: boot -f u-boot-imx7dsabresd_sd.imx ....

# Key feature. 
 - The real cross platform. linux, windows, MacOS(not test yet)
 - Multi devices program support
 - Daemon mode support.
 - Small depedence (only libusb, zlibc, libbz2)
 - Firmware (uboot/kernel) use WCID to auto load winusb drvier in windows side. win7 user need install winusb driver. https://zadig.akeo.ie/  win10 will install driver automatically.

# Example:
```
  uuu u-boot.imx            Download u-boot.imx by HID device
  
  uuu list.uu               Run all commands in list.uu
  
  uuu -s                    Enter shell mode. input command. 

  uuu -v u-boot.imx         verbose mode
 
  uuu -d u-boot.imx         Once detect known device attach, download boot.imx. 
                            
                            u-boot.imx can be replaced, new file will be download once board reset.
                            
                            Avoid unplug sd, write sd, plug sd card when debug uboot.
                            
  uuu -b emmc u-boot.imx    write u-boot.imx to emmc boot partition. u-boot.imx need enable fastboot
  
  uuu -b emmc_all u-boot.imx sdcard.bz2\*
                            decompress sdcard.bz2 file and download whole image into emmc
```

# Prebuild Image and pdf document

  - https://github.com/NXPmicro/mfgtools/releases
  - UUU.pdf is snapshot of [wiki](https://github.com/NXPmicro/mfgtools/wiki)
 
# Build:

## Windows
- git clone https://github.com/NXPmicro/mfgtools.git
- cd mfgtools
- git submodule init
- git submodule update
- open msvs/uuu.sln by vs2017

## Linux
- git clone https://github.com/NXPmicro/mfgtools.git
- cd mfgtools
- sudo apt-get install libusb-1.0.0-dev libzip-dev libbz2-dev
- cmake .
- make

# Running environment
 - win10 64bit
 - linux (ubuntu) 64bit
 - All 32 bit system will be problem when met big file
