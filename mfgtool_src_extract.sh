#!/bin/bash 

SELF=`basename $0`

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
LOG=mfgtool.v${VERSION}.log
RELEASE_DIR=mfgtool.v${VERSION}.SRC


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

	mkdir -p ${RELEASE_DIR}/Apps/MfgTool.exe
#	FILE_LIST={ls ./Apps/MfgTool.exe --ignore='Release' --ignore='Debug'}
#	for file in ${FILE_LIST} ; do
#		cp -rpa Apps/MfgTool.exe/${file} ./${RELEASE_DIR}/Apps/MfgTool.exe
#	done
	cp -d --preserve=all Apps/MfgTool.exe/* ${RELEASE_DIR}/Apps/MfgTool.exe
#	cpret=$?
#	if [ $cpret -ne 0 ] ; then
#		echo $cpret
#		myecho "${FUNCNAME}: error: could not copy files" ${LOG} 
#		exit
#	fi
	clean_up_by_extension ${RELEASE_DIR}/Apps/MfgTool.exe "aps bak bat user ncb suo log"
	
	mkdir ${RELEASE_DIR}/Apps/MfgTool.exe/res
	cp -d --preserve=all Apps/MfgTool.exe/res/* ${RELEASE_DIR}/Apps/MfgTool.exe/res/

	mkdir ${RELEASE_DIR}/Apps/MfgTool.exe/docs
#	DOCS_LIST="'Build Requirements.pdf' changelog.txt"
	cp -d --preserve=all Apps/MfgTool.exe/docs/"Build Requirements.pdf" ${RELEASE_DIR}/Apps/MfgTool.exe/docs
	cp -d --preserve=all Apps/MfgTool.exe/docs/changelog.txt ${RELEASE_DIR}/Apps/MfgTool.exe/docs
#	for file in ${DOCS_LIST} ; do
#		if [ $? -ne 0 ] ; then
#			myecho "${FUNCNAME}: error: could not copy $file" ${LOG}
#		fi
#	done
	
#	local LICENSE_LIST="EULA Third' 'Party' 'Components.txt"
#	for file in ${LICENSE_LIST} ; do
#		cp -d --preserve=all Apps/MfgTool.exe/docs/${file} ${RELEASE_DIR}/${file}
#		if [ $? -ne 0 ] ; then
#			myecho "${FUNCNAME}: error: could not copy $file" ${LOG}
#		fi
#	done
	cp -d --preserve=all Apps/MfgTool.exe/docs/EULA ${RELEASE_DIR}
	cp -d --preserve=all Apps/MfgTool.exe/docs/"Third Party Components.txt" ${RELEASE_DIR}

	return
}

function copy_common_files() {
	
#	DELETE_LIST="scsidefs.h wnaspi32.h"
	cp -rpa Common ${RELEASE_DIR}/
#	for file in ${DELETE_LIST} ; do
#		rm ${RELEASE_DIR}/Common/${file}
#		if [ $? -ne 0 ] ; then
#			myecho "${FUNCNAME}: error: could not delete $file" ${LOG}
#		fi
#	done
	return
}

function copy_drivers_files() {

	mkdir -p ${RELEASE_DIR}/Drivers/iMX_BulkIO_Driver/sys

	cp -rpa Drivers/iMX_BulkIO_Driver/sys/public.h ${RELEASE_DIR}/Drivers/iMX_BulkIO_Driver/sys
	if [ $? -ne 0 ] ; then
		myecho "${FUNCNAME}: error: could not copy Drivers/iMX_BulkIO_Driver/sys/public.h" ${LOG}
	fi
	return
	
}

function copy_libs_files() {

	# DevSupport
	mkdir -p ${RELEASE_DIR}/Libs/DevSupport

#	FILE_LIST={ls ./Libs/DevSupport --ignore='Release' --ignore='Debug'}
#	for file in ${FILE_LIST} ; do
#		cp -rpa Libs/DevSupport/${file} ./${RELEASE_DIR}/Libs/DevSupport
#	done
	cp -d --preserve=all Libs/DevSupport/* ${RELEASE_DIR}/Libs/DevSupport
#	cpret=$?
#	if [ $cpret -ne 0 ] ; then
#		echo $cpret
#		myecho "${FUNCNAME}: error: could not copy DevSupport" ${LOG} 
#		exit
#	fi
	clean_up_by_extension ${RELEASE_DIR}/Libs/DevSupport "aps bak bat user ncb suo log"

	# Loki
	mkdir -p ${RELEASE_DIR}/Libs/Loki
	cp -rpa Libs/Loki/[A-z]* ${RELEASE_DIR}/Libs/Loki

	# Public
	mkdir -p ${RELEASE_DIR}/Libs/Public
	cp -rpa Libs/Public/[A-z]* ${RELEASE_DIR}/Libs/Public

	return
}

# the real stuff is here
rm -f ${LOG}
myecho "status: preparing release package version ${VERSION}" ${LOG}
myecho "status: remove existing release directory ${RELEASE_DIR}" ${LOG}
rm -rf ${RELEASE_DIR}

for foo in apps common drivers libs ; do
	myecho "status: running copy_${foo}_files" ${LOG}
	copy_${foo}_files
done

LICENSE_LIST="COPYING.CPOL COPYING.MIT"
for file in ${LICENSE_LIST} ; do
	cp -d --preserve=all ${file} ${RELEASE_DIR}/${file}
	if [ $? -ne 0 ] ; then
		myecho "${FUNCNAME}: error: could not copy $file" ${LOG}
	fi
done



