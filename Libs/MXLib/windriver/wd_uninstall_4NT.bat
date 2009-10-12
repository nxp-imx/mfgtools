@rem **********************************************************************
@rem  Copyright 2005-2008 by Freescale Semiconductor, Inc.
@rem  All modifications are confidential and proprietary information
@rem  of Freescale Semiconductor, Inc. ALL RIGHTS RESERVED.
@rem ***********************************************************************
@rem ============================================================
@rem                    wd_install_4NT.sh
@rem  
@rem  Description: Install USB windows driver.
@rem  Author:   Terry Lu
@rem  Data:     26-05-2008
@rem  History:
@rem 
@rem    26-05-2008      1.0     Initial create.
@rem ============================================================

@echo off

setlocal enabledelayedexpansion

if not exist .\inf.lst (
    @echo + Can't find inf_set.txt, :( 
    goto Exit_1
)

if not exist .\wdreg_gui.exe (
    @echo + Can't find wdreg_gui.exe, :( 
    goto Exit_1
)

if not exist .\windrvr6.inf (
    @echo + Can't find windrvr6.inf, :( 
    goto Exit_1
)

for /f "eol= tokens=1 delims=,- " %%i in (.\inf.lst) do (
    if not exist %%i (
        @echo + Can't find %%i, :(
        goto Exit_1
    )
)

@echo + Note: To uninstall USB Windows Drivers, you must have the administrative privileges!

@rem Uninstall all Plug-and-Play devices (USB/PCI/PCMCIA) that have been registered with WinDriver via an INF file

@echo + Deregistering windrvr6.inf ...
wdreg_gui -inf .\windrvr6.inf uninstall
if not ERRORLEVEL 0 (
    @echo + Can't deregister windrvr6.inf, :(
    )

for /f "eol= tokens=1 delims=,- " %%i in (.\inf.lst) do (
    @echo + Deregistering %%i ...
    wdreg_gui -inf %%i uninstall
    if not ERRORLEVEL 0 (
        @echo + Can't deregister %%i, :(
    )
)

@rem Search for files as follows: for i.MX32 boards, search for VID_15A2&PID_002; for i.MX31 boards, search for VID_0425&PID_21FF, for i.MX37, search for VID_15A2&PID_002C

@echo + Trying to Delete inf files ...
for /r %windir%\inf\ %%i in (oem*.inf) do (
    for /f "eol= tokens=2 delims=,- " %%j in (.\inf.lst) do (
        find "%%j" %%i
        if "!ERRORLEVEL!"=="0" (
            @echo + Deleting %%i ... 
            del /F %%i
        )
    )
)

@rem If windrvr6.sys was successfully unloaded, erase the following files (if they exist)

if exist %windir%\system32\drivers\windrvr6.sys (
    echo + Deleting windrvr6.sys ...
    del %windir%\system32\drivers\windrvr6.sys
)

if exist %windir%\inf\windrvr6.inf (
    echo + Deleting windrvr6.inf ...
    del %windir%\inf\windrvr6.inf
)

if exist %windir%\system32\wdapi900.dll (
    echo + Deleting wdapi900.dll ...
    del %windir%\system32\wdapi900.dll
)

goto Exit_0

:Exit_1
@echo + Exit 1
pause
exit 1

:Exit_2
@echo + Exit 2
pause
exit 2

:Exit_0

@echo                     \\\^|///                   
@echo                   \\  - -  //                
@echo                    (  @ @  )                 
@echo +----------------oOOo-(_)-oOOo-------------+ 
@echo ^|                                          ^| 
@echo ^| Uninstallation is OK!                    ^| 
@echo ^|                                          ^| 
@echo ^| Copyright: FSL, Inc                      ^| 
@echo ^| mailto: r65388@freescale.com             ^| 
@echo ^|                        Oooo              ^| 
@echo +---------------- oooO-- (   )-------------+ 
@echo                      (   )) /                
@echo                    \ (   (_/                 
@echo                     \_)                      

pause
