call git --version
IF ERRORLEVEL 1 (

echo #define GIT_VERSION "-unknown" > %1/gitversion.h

) ELSE (

call git log -n1 HEAD --pretty=format:"#define GIT_VERSION \"-g%%%%h\"%%%%n"> %1/gitversion.h

IF "%APPVEYOR_BUILD_VERSION%" == "" (
echo "not build in appveyor" 
) ELSE (
echo #define BUILD_VER "%APPVEYOR_BUILD_VERSION%" >> %1/gitversion.h
)	

)