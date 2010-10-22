rem @echo off

if exist ..\DeviceApi_src rmdir /S /Q ..\DeviceApi_src
mkdir ..\DeviceApi_src

@echo ****************** APPS ***************************************************
mkdir ..\DeviceApi_src\Utilities
rem mkdir ..\DeviceApi_src\Utilities\DeviceApi
if exist  Release rmdir /S /Q Release
if exist  Debug   rmdir /S /Q Debug
xcopy /S /I . ..\DeviceApi_src\Utilities\DeviceApi

pause

if exist ..\DeviceApi_src\Utilities\DeviceApi\DeviceApi.ncb del ..\DeviceApi_src\Utilities\DeviceApi\DeviceApi.ncb
if exist ..\DeviceApi_src\Utilities\DeviceApi\DeviceApi.aps del ..\DeviceApi_src\Utilities\DeviceApi\DeviceApi.aps
if exist ..\DeviceApi_src\Utilities\DeviceApi\DeviceApi.suo del ..\DeviceApi_src\Utilities\DeviceApi\DeviceApi.suo
del /Q ..\DeviceApi_src\Utilities\DeviceApi\*.vs*
del /Q ..\DeviceApi_src\Utilities\DeviceApi\*.user

@echo ****************** COMMON *************************************************
mkdir ..\DeviceApi_src\Common
copy ..\..\Common\StdInt.h	..\DeviceApi_src\Common\StdInt.h
copy ..\..\Common\StdString.h	..\DeviceApi_src\Common\StdString.h

@echo ****************** LIBS ***************************************************
mkdir ..\DeviceApi_src\Libs
if exist  ..\..\Libs\DevSupport\Release rmdir /S /Q ..\..\Libs\DevSupport\Release
if exist  ..\..\Libs\DevSupport\Debug   rmdir /S /Q ..\..\Libs\DevSupport\Debug
xcopy /S /I ..\..\Libs\DevSupport	..\DeviceApi_src\Libs\DevSupport
del /Q ..\DeviceApi_src\Libs\DevSupport\*.vs*
del /Q ..\DeviceApi_src\Libs\DevSupport\*.user

mkdir ..\DeviceApi_src\Libs\WinSupport
if exist  ..\..\Libs\WinSupport\Release rmdir /S /Q ..\..\Libs\WinSupport\Release
if exist  ..\..\Libs\WinSupport\Debug   rmdir /S /Q ..\..\Libs\WinSupport\Debug
xcopy /S /I ..\..\Libs\WinSupport	..\DeviceApi_src\Libs\WinSupport
del /Q ..\DeviceApi_src\Libs\WinSupport\*.vs*
del /Q ..\DeviceApi_src\Libs\WinSupport\*.user

mkdir ..\DeviceApi_src\Libs\Loki
if exist  ..\..\Libs\Loki\Release rmdir /S /Q ..\..\Libs\Loki\Release
if exist  ..\..\Libs\Loki\Debug   rmdir /S /Q ..\..\Libs\Loki\Debug
xcopy /S /I ..\..\Libs\Loki	..\DeviceApi_src\Libs\Loki
del /Q ..\DeviceApi_src\Libs\Loki\*.vs*
del /Q ..\DeviceApi_src\Libs\Loki\*.user

@echo ****************** RESOURCES **********************************************
mkdir ..\DeviceApi_src\Resources
xcopy /S /I ..\..\Resources	..\DeviceApi_src\Resources

@echo ****************** ZIP ****************************************************
call env.bat
if exist "..\DeviceApi_src\%ST_DESCRIPTION%.v%ST_VERSION%_src.zip" del "..\DeviceApi_src\%ST_DESCRIPTION%.v%ST_VERSION%_src.zip"
"%TOOLS%\pkzipc.exe" -silent -add -dir=relative "..\DeviceApi_src\%ST_DESCRIPTION%.v%ST_VERSION%_src.zip"  "..\DeviceApi_src\*.*"
