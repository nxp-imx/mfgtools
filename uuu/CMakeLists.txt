cmake_minimum_required (VERSION 2.6)

set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wstrict-aliasing -Wextra")

set(SOURCES
	uuu.cpp
)

link_directories(../libuuu)

add_executable(uuu ${SOURCES})
target_link_libraries(uuu uuc_s)
