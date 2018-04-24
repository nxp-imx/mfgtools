cmake_minimum_required (VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0.16)
find_package(Threads)

set(CMAKE_CXX_FLAGS "-std=c++11 -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wstrict-aliasing -Wextra")

set(SOURCES
	uuu.cpp
)

link_directories(../libuuu)

add_executable(uuu ${SOURCES})
target_link_libraries(uuu uuc_s ${LIBUSB_LIBRARIES})
