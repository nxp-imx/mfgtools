# uuu(universial update utiles). Old name is mfgtools 
Freescale/NXP I.MX Chip image deploy tools.

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
