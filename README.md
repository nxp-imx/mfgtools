# uuu (Universal Update Utility), mfgtools 3.0

[![Build status](https://ci.appveyor.com/api/projects/status/github/NXPmicro/mfgtools?svg=true)](https://ci.appveyor.com/project/nxpfrankli/mfgtools-kvqcg)
[![Build Status](https://travis-ci.com/NXPmicro/mfgtools.svg?branch=master)](https://travis-ci.com/NXPmicro/mfgtools)

![GitHub](https://img.shields.io/github/license/NXPmicro/mfgtools.svg)

Freescale/NXP I.MX Chip image deploy tools.
**original linux version uses "linux" branch, windows version uses "windows" branch**

    uuu (universal update utility) for nxp imx chips -- libuuu-1.0.1-gffd9837

    Succeded:0       Failed:3               Wait for Known USB Devices to Appear...

    1:11     5/5 [                                        ] SDP: jump -f u-boot-dtb.imx -ivtinitramf....
    2:1      1/5 [===>                                    ] SDP: boot -f u-boot-imx7dsabresd_sd.imx ....

# Key features 
 - The real cross platform. Linux, Windows, MacOS(not test yet)
 - Multi devices program support
 - Daemon mode support
 - Few depedencies (only libusb, zlibc, libbz2)
 - Firmware (uboot/kernel) uses WCID to auto load the winusb driver on the Windows side. Windows7 users need to install the winusb driver from https://zadig.akeo.ie/  Windows10 will install the driver automatically.

# Examples:
```
  uuu u-boot.imx            Download u-boot.imx via HID device
  
  uuu list.uu               Run all the commands in list.uu
  
  uuu -s                    Enter shell mode. Input command. 

  uuu -v u-boot.imx         verbose mode
 
  uuu -d u-boot.imx         Once it detects the attachement of a known device, download boot.imx. 
                            
                            u-boot.imx can be replaced, new file will be download once board reset.
                            
                            Do not unplug the SD card, write to the SD card, nor plug in a SD card when debugging uboot.
                            
  uuu -b emmc u-boot.imx    write u-boot.imx to emmc boot partition. u-boot.imx need enable fastboot
  
  uuu -b emmc_all u-boot.imx sdcard.bz2\*
                            decompress sdcard.bz2 file and download the whole image into emmc
```

# Prebuilt Image and pdf document

The prebuilt image and document are here:
  - https://github.com/NXPmicro/mfgtools/releases
  - UUU.pdf is snapshot of [wiki](https://github.com/NXPmicro/mfgtools/wiki)
 
# How to Build:

## Windows
- git clone https://github.com/NXPmicro/mfgtools.git
- cd mfgtools
- git submodule init
- git submodule update
- open msvs/uuu.sln with Visual Studio 2017

Visual Studio

Note that, since uuu is an OSI compliant Open Source project, you are entitled to download and use the freely available Visual Studio Community Edition to build, run or develop for uuu. As per the Visual Studio Community Edition license this applies regardless of whether you are an individual or a corporate user.

## Linux
- git clone https://github.com/NXPmicro/mfgtools.git
- cd mfgtools
- sudo apt-get install libusb-1.0-0-dev libzip-dev libbz2-dev pkg-config cmake libssl-dev
- cmake .
- make

# Run environment
 - Windows 10 64 bit
 - Linux (Ubuntu) 64 bit
 - 32 bit systems will have problems with big files.

# License
uuu is licensed under the BSD license. See LICENSE.
The BSD licensed prebuilt Windows binary version of uuu is statically linked with the LGPL libusb library, which remains LGPL.

 - bzip2 (BSD license) is from https://github.com/enthought/bzip2-1.0.6
 - zlib  (zlib license) is from https://github.com/madler/zlib.git
 - libusb (LGPL-2.1) is from  https://github.com/libusb/libusb.git
