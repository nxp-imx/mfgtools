# uuu(universial update utiles). Old name is mfgtools 
** original linux version using linux branch, windows version use windows branch **

Freescale/NXP I.MX Chip image deploy tools.

    uuu (universal update utitle) for nxp imx chips -- libuuu-1.0.1-gffd9837

    Succues:0       Failure:3               Wait for Known USB Device Appear...

    1:11     5/5 [                                        ] SDP: jump -f u-boot-dtb.imx -ivtinitramf....
    2:1      1/5 [===>                                    ] SDP: boot -f u-boot-imx7dsabresd_sd.imx ....


**under developing**

# key feature. 
 - The real cross platform. linux, windows, MacOS(not test yet)
 - Multi devices program support
 - Daemon mode support.
 - Small depedence (only libusb)
 - Firmware (uboot/kernel) use WCID to auto load winusb drvier in windows side.

# example:
  uuu u-boot.imx            Download u-boot.imx by HID device
  
  uuu list.uu               Run all commands in list.uu
  
  uuu -d u-boot.imx         Once detect known device attach, download boot.imx. 
                            
                            u-boot.imx can be replaced, new file will be download once board reset.
                            
                            Avoid unplug sd, write sd, plug sd card when debug uboot.

# Build:

## windows
- git clone git@github.com:codeauroraforum/mfgtools.git -b uuu
- git submodule init
- git submodule update
- open msvs/uuu.sln by vs2017

## linux
- git clone git@github.com:codeauroraforum/mfgtools.git -b uuu
- sudo do apt-get install libusb-1.0.0-dev
- cmake .
- make
