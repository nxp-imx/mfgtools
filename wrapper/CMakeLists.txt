cmake_minimum_required(VERSION 3.5)
project(libuuu)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_RPATH ON)

# Prefix all shared libraries with 'lib'.
set(CMAKE_SHARED_LIBRARY_PREFIX "")
# Prefix all static libraries with 'lib'.
set(CMAKE_STATIC_LIBRARY_PREFIX "")

# Set the directory where generated files will be placed
set(generated_files_dir "${CMAKE_BINARY_DIR}/libuuu/gen")

# Add the generated files directory to the include path
include_directories(${generated_files_dir})

# Set the path for gitversion.h in the generated files directory
set(gitversion_h "${generated_files_dir}/gitversion.h")

# Run git describe
execute_process(COMMAND git describe --long --always OUTPUT_VARIABLE version OUTPUT_STRIP_TRAILING_WHITESPACE)

# Add a custom command to generate definitions.h
if (WIN32)
add_custom_command(
    OUTPUT ${gitversion_h}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_files_dir}
    COMMAND ${CMAKE_COMMAND} -E echo "#define GIT_VERSION \"${version}\"" >> ${gitversion_h};
    COMMAND ${CMAKE_COMMAND} -E echo "#define PLATFORM_WINDOWS 1" >> ${gitversion_h}
    COMMAND ${CMAKE_COMMAND} -E echo "#define ENABLE_LOGGING 1" >> ${gitversion_h}
    COMMAND ${CMAKE_COMMAND} -E echo "#define DEFAULT_VISIBILITY /**/" >> ${gitversion_h}
    COMMENT "Generating gitversion.h"
    VERBATIM
)
else ()
add_custom_command(
    OUTPUT ${gitversion_h}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_files_dir}
    COMMAND ${CMAKE_COMMAND} -E echo "#define GIT_VERSION \"${version}\"" >> ${gitversion_h};
    COMMENT "Generating gitversion.h"
    VERBATIM
)
endif (WIN32)

add_library(
    libuuu 
    SHARED
    ../libuuu/error.cpp
    ../libuuu/buffer.cpp
    ../libuuu/cmd.cpp
    ../libuuu/config.cpp
    ../libuuu/notify.cpp
    ../libuuu/sdps.cpp
    ../libuuu/trans.cpp
    ../libuuu/usbhotplug.cpp
    ../libuuu/version.cpp
    ../libuuu/sdp.cpp
    ../libuuu/fastboot.cpp
    ../libuuu/zip.cpp
    ../libuuu/fat.cpp
    ../libuuu/tar.cpp
    ../libuuu/rominfo.cpp
    ../libuuu/http.cpp
    ../libuuu/hidreport.cpp
    ../libuuu/sparse.cpp
    ../libuuu/bmap.cpp
    ${gitversion_h}
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)
pkg_check_modules(tinyxml2 REQUIRED IMPORTED_TARGET tinyxml2)
pkg_check_modules(bzip2 REQUIRED IMPORTED_TARGET bzip2)
pkg_check_modules(zstd REQUIRED IMPORTED_TARGET libzstd)
pkg_check_modules(openssl REQUIRED IMPORTED_TARGET openssl)
pkg_check_modules(zlib REQUIRED IMPORTED_TARGET zlib)


if (DARWIN)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
endif(DARWIN)    

if (WIN32)
add_definitions(-D_WIN32 -DWIN32 -DDLL_EXPORT_LIBUUU)
set(CMAKE_CXX_FLAGS_RELEASE "/MT")
set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif(WIN32)

add_definitions(-DUUUSSL)

if (UNIX)
set(CMAKE_CXX_FLAGS "-g -Wall -Wstrict-aliasing -Wall -Wextra")
endif(UNIX)



target_link_libraries(libuuu PkgConfig::libusb PkgConfig::tinyxml2 PkgConfig::bzip2 PkgConfig::zstd PkgConfig::zlib PkgConfig::openssl)
