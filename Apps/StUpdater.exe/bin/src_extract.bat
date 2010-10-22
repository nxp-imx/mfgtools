rem @echo off

@echo ***************************************************************************
@echo * Firmware Update Source Extraction
@echo ***************************************************************************

rem go to root folder

cd ..\..\..

if exist temp rmdir /S /Q temp
mkdir temp

@echo ****************** APPS ***************************************************
mkdir temp\Apps
if exist  Apps\StUpdater.exe\Release rmdir /S /Q Apps\StUpdater.exe\Release
if exist  Apps\StUpdater.exe\Debug   rmdir /S /Q Apps\StUpdater.exe\Debug
del /Q Apps\StUpdater.exe\*.aps
del /Q Apps\StUpdater.exe\*.bak
del /Q Apps\StUpdater.exe\*.user
del /Q Apps\StUpdater.exe\*.ncb
del /Q Apps\StUpdater.exe\*.suo	
xcopy /S /I Apps\StUpdater.exe temp\Apps\StUpdater.exe

mkdir temp\Apps\StBinder\bin
copy Apps\StBinder\Bin\StBinder.exe temp\Apps\StBinder\Bin\StBinder.exe

@echo ****************** COMMON *************************************************
mkdir temp\Common
copy Common\scsidefs.h         temp\Common\scsidefs.h
copy Common\wnaspi32.h         temp\Common\wnaspi32.h
copy Common\updater_res.h      temp\Common\updater_res.h
copy Common\updater_restypes.h temp\Common\updater_restypes.h

@echo ****************** Customization ******************************************
mkdir temp\Customization
xcopy /I Customization\*.*	temp\Customization

@echo ****************** Docs ******************************************
rem mkdir temp\Docs
rem xcopy /I Docs\*.*	temp\Docs

@echo ****************** DRIVERS ************************************************
mkdir temp\Drivers\StRcvryDriver\bin
xcopy /I Drivers\StRcvryDriver\bin\*.*	temp\Drivers\StRcvryDriver\bin

@echo ****************** LIBS ***************************************************
mkdir temp\Libs
if exist  Libs\mscdevlib\Release rmdir /S /Q Libs\mscdevlib\Release
if exist  Libs\mscdevlib\Debug   rmdir /S /Q Libs\mscdevlib\Debug
del /Q Libs\mscdevlib\*.user
xcopy /S /I Libs\mscdevlib	temp\Libs\mscdevlib

mkdir temp\Libs\WinSupport
copy Libs\WinSupport\stdafx.h           temp\Libs\WinSupport\stdafx.h
copy Libs\WinSupport\AutoPlayReject.cpp temp\Libs\WinSupport\AutoPlayReject.cpp
copy Libs\WinSupport\AutoPlayReject.h   temp\Libs\WinSupport\AutoPlayReject.h
copy Libs\WinSupport\ColorStaticST.cpp	temp\Libs\WinSupport\ColorStaticST.cpp
copy Libs\WinSupport\ColorStaticST.h	temp\Libs\WinSupport\ColorStaticST.h
copy Libs\WinSupport\StSplashWnd.cpp	temp\Libs\WinSupport\StSplashWnd.cpp
copy Libs\WinSupport\StSplashWnd.h	temp\Libs\WinSupport\StSplashWnd.h

mkdir temp\Libs\DevSupport
copy Libs\DevSupport\RecoveryDeviceGuid.h temp\Libs\DevSupport\RecoveryDeviceGuid.h

@echo ****************** RESOURCES **********************************************
mkdir temp\Resources
copy Resources\freescale_logo.bmp	temp\Resources\freescale_logo.bmp

@echo ****************** RELEASE ************************************************
mkdir temp\Release
mkdir temp\Release\Docs
mkdir temp\Release\Tools

@echo ****************** ROOT ***************************************************
copy bldall.bat	temp\bldall.bat
copy env.bat	temp\env.bat
rem copy ver.bat	temp\ver.bat

@echo ****************** VERSIONING ****************************************************
copy Apps\StUpdater.exe\version.h version.h
%PERL%\bin\perl.exe "Customization\bin\version.pl" release
call ver.bat

@echo ****************** ZIP ****************************************************
rem call Customization\env.bat
if exist "FirwareUpdate.v%STMP_BUILD_VERSION%_src.zip" del "FirwareUpdate.v%STMP_BUILD_VERSION%_src.zip"
"%TOOLS%\pkzipc.exe" -silent -add -exclude=?svn -dir=relative "FirwareUpdate.v%STMP_BUILD_VERSION%_src.zip"  "temp\*.*"


CD Apps\StUpdater.exe\bin