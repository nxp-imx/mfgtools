cmake_minimum_required (VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0.16)
pkg_check_modules(LIBZIP REQUIRED libzip)
pkg_check_modules(LIBZ REQUIRED zlib)
find_package(Threads)

set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -static-libstdc++ -static-libgcc")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wstrict-aliasing -Wextra")

set(SOURCES
	uuu.cpp
	buildincmd.cpp
	uuu.clst
	emmc_burn_loader.clst
	qspi_burn_loader.clst
)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libuuu)


set(generated_files_dir "${CMAKE_BINARY_DIR}/uuu/gen")

add_custom_command(
        OUTPUT uuu.clst emmc_burn_loader.clst qspi_burn_loader.clst
        DEPENDS uuu.lst emmc_burn_loader.lst qspi_burn_loader.lst
	PRE_BUILD
		COMMAND mkdir -p ${generated_files_dir}
        COMMAND sh -c 'cd ${CMAKE_CURRENT_SOURCE_DIR} && ./gen_txt_include.sh uuu.lst ${generated_files_dir}/uuu.clst'
	COMMAND sh -c 'cd ${CMAKE_CURRENT_SOURCE_DIR} && ./gen_txt_include.sh emmc_burn_loader.lst ${generated_files_dir}/emmc_burn_loader.clst'
	COMMAND sh -c 'cd ${CMAKE_CURRENT_SOURCE_DIR} && ./gen_txt_include.sh qspi_burn_loader.lst ${generated_files_dir}/qspi_burn_loader.clst'

)
include_directories(${generated_files_dir})

add_executable(uuu ${SOURCES})
target_link_libraries(uuu uuc_s  ${LIBUSB_LIBRARIES} ${LIBZIP_LIBRARIES} ${LIBZ_LIBRARIES})
