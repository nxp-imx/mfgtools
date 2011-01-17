#!/bin/bash 

SELF=`basename $0`
LICENSE_LIST="COPYING.CPOL \
              COPYING.MIT  \
              Apps/MfgTool.exe/docs/EULA \
              Apps/MfgTool.exe/docs/Third_Party_Components.txt"

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

function clean_up_by_extension() {
    DIR=$1
    EXTS="$2"

    for ext in ${EXTS} ; do
        find ${DIR} -type f | egrep -e '\.'${ext}'$' | xargs rm -f
    done
}

function copy_apps_files() {

    # Apps/MfgTool.exe/{FILES_ONLY}
    mkdir -p ${RELEASE_DIR}/Apps/MfgTool.exe/
    find Apps/MfgTool.exe/ -maxdepth 1 -type f | xargs -i%% cp -d --preserve=all %% ${RELEASE_DIR}/Apps/MfgTool.exe/
    if [ $? -ne 0 ] ; then
        myecho "${FUNCNAME}: error: could not copy Apps/MfgTool.exe/files" ${LOG} 
        exit
    fi
    clean_up_by_extension ${RELEASE_DIR}/Apps/MfgTool.exe "aps bak bat user ncb suo log"
    
    # Apps/MfgTool.exe/res/{ALL}
    mkdir ${RELEASE_DIR}/Apps/MfgTool.exe/res/
    cp -d --preserve=all Apps/MfgTool.exe/res/[A-z]* ${RELEASE_DIR}/Apps/MfgTool.exe/res/
    if [ $? -ne 0 ] ; then
        myecho "${FUNCNAME}: error: could not copy Apps/MfgTool.exe/res/files" ${LOG} 
        exit
    fi

    # Apps/MfgTool.exe/docs/{LIST}
    DOCS_LIST="Build_Requirements.pdf changelog.txt"
    mkdir ${RELEASE_DIR}/Apps/MfgTool.exe/docs/
    for file in ${DOCS_LIST} ; do
        cp -d --preserve=all Apps/MfgTool.exe/docs/${file} ${RELEASE_DIR}/Apps/MfgTool.exe/docs/
        if [ $? -ne 0 ] ; then
            myecho "${FUNCNAME}: error: could not copy $file" ${LOG}
            exit
        fi
    done
    
    return
}

function copy_common_files() {
    
    # Common/{ALL}
    cp -rpa Common ${RELEASE_DIR}/
    if [ $? -ne 0 ] ; then
        myecho "${FUNCNAME}: error: could not copy files" ${LOG}
        exit
    fi

    return
}

function copy_drivers_files() {

    # Drivers/iMX_BulkIO_Driver/sys/driver_ioctl.h
    mkdir -p ${RELEASE_DIR}/Drivers/iMX_BulkIO_Driver/sys

    cp -rpa Drivers/iMX_BulkIO_Driver/sys/driver_ioctl.h ${RELEASE_DIR}/Drivers/iMX_BulkIO_Driver/sys/
    if [ $? -ne 0 ] ; then
        myecho "${FUNCNAME}: error: could not copy Drivers/iMX_BulkIO_Driver/sys/driver_ioctl.h" ${LOG}
        exit
    fi
    return
    
}

function copy_libs_files() {

    # Libs/DevSupport/{FILES_ONLY}
    mkdir -p ${RELEASE_DIR}/Libs/DevSupport/
    find Libs/DevSupport/ -maxdepth 1 -type f | xargs -i%% cp -d --preserve=all %% ${RELEASE_DIR}/Libs/DevSupport/
    if [ $? -ne 0 ] ; then
        myecho "${FUNCNAME}: error: could not copy Libs/DevSupport/files" ${LOG} 
        exit
    fi
    clean_up_by_extension ${RELEASE_DIR}/Libs/DevSupport/ "aps bak bat user ncb suo log"

    # Libs/Loki/{ALL}
    mkdir -p ${RELEASE_DIR}/Libs/Loki
    cp -rpa Libs/Loki/[A-z]* ${RELEASE_DIR}/Libs/Loki
    if [ $? -ne 0 ] ; then
        myecho "${FUNCNAME}: error: could not copy Libs/Loki/files" ${LOG} 
        exit
    fi

    # Libs/Public/{ALL}
    mkdir -p ${RELEASE_DIR}/Libs/Public
    cp -rpa Libs/Public/[A-z]* ${RELEASE_DIR}/Libs/Public
    if [ $? -ne 0 ] ; then
        myecho "${FUNCNAME}: error: could not copy Libs/Public/files" ${LOG} 
        exit
    fi

    return
}

# the real stuff is here
rm -f ${LOG}
myecho "status: preparing release package version ${VERSION}" ${LOG}
myecho "status: remove existing release directory ${RELEASE_DIR}" ${LOG}
rm -rf ${RELEASE_DIR}

# Get all the MfgTool.exe source code.
for foo in apps common drivers libs ; do
    myecho "status: running copy_${foo}_files" ${LOG}
    copy_${foo}_files
done

# Get the license files.
for file in ${LICENSE_LIST} ; do
    cp -d --preserve=all ${file} ${RELEASE_DIR}/
    if [ $? -ne 0 ] ; then
        myecho "main(): error: could not copy $file" ${LOG}
        exit
    fi
done

myecho "success"


