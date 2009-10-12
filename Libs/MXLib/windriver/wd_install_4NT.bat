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


@echo + Note: To install USB Windows Drivers, you must have the administrative privileges!

@echo + Registering windrvr6.inf ...
wdreg_gui -inf .\windrvr6.inf install
if not ERRORLEVEL 0 (
    @echo + Can't register windrvr6.inf, :( 
    goto Exit_2
    )

for /f "eol= tokens=1 delims=,- " %%i in (.\inf.lst) do (
    @echo + Registering %%i
    wdreg_gui -inf %%i install
    if not ERRORLEVEL 0 (
        @echo + Can't register %%i, :(
        goto Exit_2
    )
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
@echo ^| Installation is OK!                      ^|  
@echo ^|                                          ^|  
@echo ^| Copyright: FSL, Inc                      ^|  
@echo ^| mailto: r65388@freescale.com             ^|  
@echo ^|                        Oooo              ^|  
@echo +---------------- oooO-- (   )-------------+  
@echo                      (   )) /                 
@echo                    \ (   (_/                  
@echo                     \_)                       

pause

