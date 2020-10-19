cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_RPATH ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0.16)
pkg_check_modules(LIBZIP REQUIRED libzip)
pkg_check_modules(LIBZ REQUIRED zlib)
find_package(Threads)

if (STATIC)
set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

find_package(OpenSSL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -O2")

if (STATIC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
endif()

set(LSTS
	uuu.lst
	emmc_burn_loader.lst
	emmc_burn_all.lst
	fat_write.lst
	qspi_burn_loader.lst
	sd_burn_loader.lst
	spl_boot.lst
	sd_burn_all.lst
	nand_burn_loader.lst
)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libuuu ${LIBUSB_LIBRARY_DIRS} ${LIBZIP_LIBRARY_DIRS} ${LIBZ_LIBRARY_DIRS})

set(CLIST_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/gen_txt_include.sh)
set(generated_files_dir "${CMAKE_BINARY_DIR}/uuu/gen")

function(preprocess_clst out_var)
	set(result)
	foreach(in_f ${ARGN})
		set(out_f "${generated_files_dir}/${in_f}")
		string(REPLACE ".lst" ".clst" out_f ${out_f})
		add_custom_command(OUTPUT ${out_f}
			PRE_BUILD
				COMMAND mkdir -p ${generated_files_dir}
			COMMAND ${CLIST_EXECUTABLE} ${in_f} ${out_f}
			DEPENDS ${in_f}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMENT "Creating preprocessed clst file ${out_f}"
			VERBATIM
		)
		list(APPEND result ${out_f})
	endforeach()
	set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

preprocess_clst(CLSTS ${LSTS})

include_directories(${generated_files_dir})

set(SOURCES
	uuu.cpp
	buildincmd.cpp
	autocomplete.cpp
	${CLSTS}
)

add_executable(uuu ${SOURCES})
target_link_libraries(uuu uuc_s ${OPENSSL_LIBRARIES} ${LIBUSB_LIBRARIES} ${LIBZIP_LIBRARIES} ${LIBZ_LIBRARIES} dl bz2)

install(TARGETS uuu DESTINATION bin)
add_compile_definitions(TARGET_PATH="${CMAKE_INSTALL_PREFIX}/bin/uuu")
