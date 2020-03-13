#!/bin/sh

# Input parameters
file_to_write="$1"

set -e

if [ -f ../.tarball-version ]
then
	echo "#define GIT_VERSION \"lib$(cat ../.tarball-version)\"" > "$file_to_write"
	exit 0
fi

if [ "${APPVEYOR_BUILD_VERSION}" = "" ];
then
	echo build not in appveyor
else
	git tag uuu_${APPVEYOR_BUILD_VERSION}
fi

# Test if we are in a repo
if [ "$(git rev-parse --is-inside-work-tree 2>/dev/null)" = "true" ];
then
	#echo "In a repo"
	# Get the version of the last commit of the repo
	version=`git describe --tags --long`
	echo "#define GIT_VERSION \"lib$version\"" > $file_to_write
fi
