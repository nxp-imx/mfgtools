cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_RPATH ON)

find_package(BZip2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0.16)
pkg_check_modules(LIBZSTD REQUIRED libzstd)
find_package(Threads)
pkg_check_modules(TINYXML2 REQUIRED tinyxml2)

if (STATIC)
set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

find_package(OpenSSL)

if(OPENSSL_FOUND)
set(UUUSSL "-DUUUSSL")
set(UUUOPENSLL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR})
endif()

include_directories(${LIBUSB_INCLUDE_DIRS} ${LIBZSTD_INCLUDE_DIRS} ${UUUOPENSLL_INCLUDE_DIR} ${TINYXML2_INCLUDE_DIRS} include)


if (FORCE_OLD)
set(FORCE_OLDLIBUSB "-DFORCE_OLDLIBUSB")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wstrict-aliasing -Wextra ${UUUSSL} ${FORCE_OLDLIBUSB}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${UUUSSL} ${FORCE_OLDLIBUSB}")

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
	hidreport.cpp
	sparse.cpp
	bmap.cpp
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
