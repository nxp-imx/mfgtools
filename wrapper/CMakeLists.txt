cmake_minimum_required(VERSION 3.5)
project(libuuu)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_RPATH ON)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# Set the directory where generated files will be placed
set(generated_files_dir "${CMAKE_BINARY_DIR}/libuuu/gen")

# Add the generated files directory to the include path
include_directories(${generated_files_dir})

# Set the path for gitversion.h in the generated files directory
set(definitions_h "${generated_files_dir}/definitions.h")

# Run git describe
execute_process(COMMAND git describe --long --always OUTPUT_VARIABLE version OUTPUT_STRIP_TRAILING_WHITESPACE)

# Add a custom command to generate definitions.h
add_custom_command(
    OUTPUT ${definitions_h}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_files_dir}
    COMMAND ${CMAKE_COMMAND} -E echo "#include \"libuuu.h\"" >> ${definitions_h};
    COMMAND ${CMAKE_COMMAND} -E echo "#define GIT_VERSION \"${version}\"" >> ${definitions_h};
    COMMAND ${CMAKE_COMMAND} -E echo "int g_verbose = 1;" >> ${definitions_h}
    COMMAND ${CMAKE_COMMAND} -E echo "bmap_mode g_bmap_mode;" >> ${definitions_h}
    COMMAND ${CMAKE_COMMAND} -E echo "#define PLATFORM_WINDOWS 1" >> ${definitions_h}
    COMMAND ${CMAKE_COMMAND} -E echo "#define ENABLE_LOGGING 1" >> ${definitions_h}
    COMMAND ${CMAKE_COMMAND} -E echo "#define DEFAULT_VISIBILITY /**/" >> ${definitions_h}
    COMMENT "Generating definitions.h"
    VERBATIM
)

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
    ../libuuu/globals.cpp
    ${definitions_h}
)

if (WIN32)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)
pkg_check_modules(tinyxml2 REQUIRED IMPORTED_TARGET tinyxml2)
pkg_check_modules(bzip2 REQUIRED IMPORTED_TARGET bzip2)
pkg_check_modules(zstd REQUIRED IMPORTED_TARGET libzstd)
pkg_check_modules(zlib REQUIRED IMPORTED_TARGET ZLIB)
add_definitions(-D_WIN32 -DWIN32 -DDLL_EXPORT_LIBUUU)
endif(WIN32)

set(CMAKE_CXX_FLAGS "-g -Wall -Wstrict-aliasing -Wall -Wextra")

target_link_libraries(libuuu PkgConfig::libusb PkgConfig::tinyxml2 PkgConfig::bzip2 PkgConfig::zstd PkgConfig::zlib)
