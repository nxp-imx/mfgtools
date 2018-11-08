find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0.16)
pkg_check_modules(LIBZIP REQUIRED libzip)
pkg_check_modules(LIBZ REQUIRED zlib)
find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -static-libstdc++ -static-libgcc")

set(LSTS
	uuu.lst
	emmc_burn_loader.lst
	emmc_burn_all.lst
	qspi_burn_loader.lst
	sd_burn_loader.lst
	spl_boot.lst
	sd_burn_all.lst
)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libuuu)

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
target_link_libraries(uuu uuc_s  ${LIBUSB_LIBRARIES} ${LIBZIP_LIBRARIES} ${LIBZ_LIBRARIES} bz2)

install(TARGETS uuu DESTINATION bin)
