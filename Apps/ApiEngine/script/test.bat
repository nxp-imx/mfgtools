@echo off

ApiEngine %1

if ERRORLEVEL 1 goto failed
if ERRORLEVEL 0 goto success

:failed
echo FAILED
goto end

:success
echo SUCCESS

:end