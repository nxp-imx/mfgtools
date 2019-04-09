cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0.16)
pkg_check_modules(LIBZIP REQUIRED libzip)
find_package(Threads)

include_directories(${LIBUSB_INCLUDE_DIRS} include)

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wstrict-aliasing -Wextra")
set(CMAKE_CXX_FLAGS "-O2")

set(SOURCES
	error.cpp
	buffer.cpp
	cmd.cpp
	config.cpp
	notify.cpp
	sdps.cpp
	trans.cpp
	usbhotplug.cpp
	version.cpp
	sdp.cpp
	gitversion.h
	fastboot.cpp
	zip.cpp
	fat.cpp
	rominfo.cpp
)

set(generated_files_dir "${CMAKE_BINARY_DIR}/libuuu/gen")

add_custom_command(
	OUTPUT gitversion.h
	PRE_BUILD
	COMMAND mkdir -p ${generated_files_dir}
	COMMAND sh -c 'cd ${CMAKE_CURRENT_SOURCE_DIR} && ./gen_ver.sh ${generated_files_dir}/gitversion.h'
)
include_directories(${generated_files_dir})

#add_library( uuc SHARED ${SOURCES} ))
add_library( uuc_s STATIC ${SOURCES} )
