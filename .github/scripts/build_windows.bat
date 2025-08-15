@echo off
setlocal enabledelayedexpansion

echo [INFO] Starting Windows build process

call :install_dependencies || goto :error
call :setup_environment || goto :error
call :build_project || goto :error
call :copy_artifacts || goto :error

echo [SUCCESS] Windows build completed successfully
exit /b 0

:install_dependencies
echo [INFO] Installing build dependencies via Chocolatey
choco install cmake pkgconfiglite ninja --yes --no-progress
if errorlevel 1 (
    echo [ERROR] Failed to install build dependencies
    exit /b 1
)
echo [SUCCESS] Build dependencies installed
pip install pytest
if errorlevel 1 (
    echo [ERROR] Failed to install pytest
    exit /b 1
)
echo [SUCCESS] pytest installed
exit /b 0

:setup_environment
echo [INFO] Setting up build environment
cd wrapper
set PROJECT_DIR=%CD%

if exist "vcpkg" (
    echo [INFO] Removing existing vcpkg directory
    rmdir /s /q vcpkg
)

echo [INFO] Cloning vcpkg repository
git clone https://github.com/microsoft/vcpkg.git
if errorlevel 1 (
    echo [ERROR] Failed to clone vcpkg repository
    exit /b 1
)

cd vcpkg
echo [INFO] Bootstrapping vcpkg
call bootstrap-vcpkg.bat
if errorlevel 1 (
    echo [ERROR] Failed to bootstrap vcpkg
    exit /b 1
)

set VCPKG_ROOT=%CD%
set PATH=%VCPKG_ROOT%;%PATH%
cd %PROJECT_DIR%
echo [SUCCESS] Build environment setup completed
exit /b 0

:build_project
echo [INFO] Configuring and building project
cmake --preset=windows
if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    call :debug_info
    exit /b 1
)

cmake --build build
if errorlevel 1 (
    echo [ERROR] Build failed
    call :debug_info
    exit /b 1
)
echo [SUCCESS] Project built successfully
exit /b 0

:copy_artifacts
echo [INFO] Copying built libraries
set TARGET_DIR=libuuu\lib\windows\x86_64
mkdir "%TARGET_DIR%" 2>nul

REM Try different build output locations
for %%D in (Debug Release .) do (
    if exist "build\%%D\*.dll" (
        echo [INFO] Copying DLLs from build\%%D\
        copy "build\%%D\*.dll" "%TARGET_DIR%\" >nul
        if not errorlevel 1 (
            echo [SUCCESS] DLL files copied successfully
            exit /b 0
        )
    )
)

echo [ERROR] No DLL files found in any expected location
call :debug_info
exit /b 1

:debug_info
echo [DEBUG] Current directory: %CD%
if exist "build" (
    echo [DEBUG] Build directory contents:
    dir build /s
) else (
    echo [DEBUG] Build directory does not exist
)
exit /b 0

:error
echo [ERROR] Build script failed
exit /b 1