#!/bin/bash 

SELF=`basename $0`
RELEASE_DIR=temp.Linux


function do_version() {

	local i
	local version
	
	VERSION_FILE=Apps/MfgTool.exe/version.h
	if [ ! -e ${VERSION_FILE} ] ; then
		myecho "${FUNCNAME}: error: could not find version file ${VERSION_FILE}" ${LOG}
		exit
	fi

	i=1
	version=`grep \#define ${VERSION_FILE} | grep \" | awk ' { printf $3 " " } '| sed -e 's/"//g' `
	echo ${version}
	return
}


VERSION=`do_version`
LOG=mfgtool_${VERSION}.log


function myecho() {
	MESSAGE=$1
	LOG=$2

	if [ -z ${LOG} ] ; then
		echo -e "${SELF}: ${MESSAGE}"
	else
		echo -e "${SELF}: ${MESSAGE}" | tee -a ${LOG}
	fi
	return
}

function remove_stuff() {
	DIR=$1
	STUFF="$2"

	for stuff in ${STUFF} ; do
		rm -f ${DIR}/${stuff}
	done

	return
}

function clean_up_by_extension() {
	DIR=$1
	EXTS="$2"

	for ext in ${EXTS} ; do
		find ${DIR} -type f | egrep -e '\.'${ext}'$' | xargs rm -f
	done
}

function clean_up() {

	rm -rf Apps/MfgTool.exe/{Release,Debug} ${LOG} ${RELEASE_DIR}
	for ext in aps bak user ncb suo ; do
		find Apps/MfgTool.exe -type f | egrep -e '\.'${ext}'$' | xargs rm -f
	done

	return
}


function copy_apps_files() {

	mkdir -p ${RELEASE_DIR}/Apps
	cp -rpa Apps/MfgTool.exe ${RELEASE_DIR}/Apps/
	if [ $? -ne 0 ] ; then
		myecho "${FUNCNAME}: error: could not copy files" ${LOG} 
		exit
	fi
	return
}

function copy_common_files() {
	
	DELETE_LIST="scsidefs.h wnaspi32.h"
	cp -rpa Common ${RELEASE_DIR}/
	for file in ${DELETE_LIST} ; do
		rm ${RELEASE_DIR}/Common/${file}
		if [ $? -ne 0 ] ; then
			myecho "${FUNCNAME}: error: could not delete $file" ${LOG}
		fi
	done
	return
}

function copy_drivers_files() {

	return
	
}

function copy_resources_files() {

	mkdir -p ${RELEASE_DIR}/Resources
	for file in closedfolder.bmp openfolder.bmp resourcefile.bmp freescale_logo.bmp ; do
		cp Resources/${file} ${RELEASE_DIR}/Resources/${file}
		if [ $? -ne 0 ] ; then
			myecho "${FUNCNAME}: error: could not copy ${file}" ${LOG}
			exit
		fi
	done	
}

function copy_libs_files() {

	# DevSupport
	mkdir -p ${RELEASE_DIR}/Libs/{,DevSupport}
	remove_stuff Libs/ "Release Debug"

	clean_up_by_extension Libs/DevSupport "bak user ncb suo"

	cp -rpa Libs/DevSupport/[A-z]* ${RELEASE_DIR}/Libs/DevSupport
	if [ $? -ne 0 ] ; then
		myecho "${FUNCNAME}: error: could not copy DevSupport"
		exit
	fi

	# Loki
	mkdir -p ${RELEASE_DIR}/Libs/Loki
	cp -rpa Libs/Loki/[A-z]* ${RELEASE_DIR}/Libs/Loki

	# WinSupport
	mkdir -p ${RELEASE_DIR}/Libs/WinSupport
	remove_stuff Libs/WinSupport "Release Debug"	
	clean_up_by_extension Libs/WinSupport "bak user ncb suo"

	cp -rpa Libs/WinSupport/* ${RELEASE_DIR}/Libs/WinSupport

	# Remainder
	mkdir -p ${RELEASE_DIR}/Libs/OtpAccessPitc/bin/Debug
	cp Libs/OtpAccessPitc/StOtpAccessPitc.h ${RELEASE_DIR}/Libs/OtpAccessPitc/StOtpAccessPitc.h
	cp Libs/OtpAccessPitc/bin/OtpAccess.* ${RELEASE_DIR}/Libs/OtpAccessPitc/bin
	cp Libs/OtpAccessPitc/bin/Debug/OtpAccess.* ${RELEASE_DIR}/Libs/OtpAccessPitc/bin/Debug

	mkdir -p ${RELEASE_DIR}/Libs/OtpAccessPitc/bin/VS2005{,/Debug}
	cp Libs/OtpAccessPitc/bin/VS2005/OtpAccess.* ${RELEASE_DIR}/Libs/OtpAccessPitc/bin/VS2005
	cp Libs/OtpAccessPitc/bin/VS2005/Debug/OtpAccess.* \
		${RELEASE_DIR}/Libs/OtpAccessPitc/bin/VS2005/Debug
	
	return
}


# the real stuff is here
myecho "status: preparing release package version ${VERSION}" ${LOG}
myecho "status: clean up" ${LOG}
clean_up

myecho "status: copy files" ${LOG}
for foo in apps common drivers resources libs ; do
	myecho "status: running copy_${foo}_files" ${LOG}
	copy_${foo}_files
done



