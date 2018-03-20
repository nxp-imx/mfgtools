call git --version
IF ERRORLEVEL 1 (

echo #define GIT_VERSION "unknown" > %1/gitversion.h

) ELSE (

call git log -n1 HEAD --pretty=format:"#define GIT_VERSION \"-g%%%%h\"%%%%n"> %1/gitversion.h

)