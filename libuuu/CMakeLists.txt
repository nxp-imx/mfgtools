cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(BZip2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0.16)
pkg_check_modules(LIBZIP REQUIRED libzip)
find_package(Threads)

if (STATIC)
set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

find_package(OpenSSL)

if(OPENSSL_FOUND)
set(UUUSSL "-DUUUSSL")
set(UUUOPENSLL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR})
endif()

include_directories(${LIBUSB_INCLUDE_DIRS} ${UUUOPENSLL_INCLUDE_DIR} include)


set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wstrict-aliasing -Wextra ${UUUSSL}")
set(CMAKE_CXX_FLAGS "-O2 ${UUUSSL}")

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
	tar.cpp
	rominfo.cpp
	http.cpp
)

set(generated_files_dir "${CMAKE_BINARY_DIR}/libuuu/gen")
set(gitversion_h "${generated_files_dir}/gitversion.h")

add_custom_command(
	OUTPUT gitversion.h
	PRE_BUILD
	COMMAND mkdir -p ${generated_files_dir}
	COMMAND sh -c 'cd ${CMAKE_CURRENT_SOURCE_DIR} && rm -f ${gitversion_h} && ./gen_ver.sh "${gitversion_h}.tmp" && mv -f "${gitversion_h}.tmp" "${gitversion_h}"'

)
include_directories(${generated_files_dir})

#add_library( uuc SHARED ${SOURCES} ))
add_library( uuc_s STATIC ${SOURCES} )
