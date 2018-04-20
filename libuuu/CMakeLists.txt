cmake_minimum_required (VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0.16)
find_package(Threads)

set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wstrict-aliasing -Wextra")

set(SOURCES
	error.cpp
)

add_library( uuc SHARED ${SOURCES} )
add_library( uuc_s STATIC ${SOURCES} )
