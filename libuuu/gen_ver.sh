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
	echo "Warning: Expected APPVEYOR_BUILD_VERSION variable (usually defined via appveyor CI). The repo will not be tagged."
else
	git tag -m"uuu ${APPVEYOR_BUILD_VERSION}" uuu_${APPVEYOR_BUILD_VERSION}
fi

# Test if we are in a repo
if [ "$(git rev-parse --is-inside-work-tree 2>/dev/null)" = "true" ];
then
	#echo "In a repo"
	# Get the version of the last commit of the repo
	set +e
	version=`git describe --long`
	if [ $? -ne 0 ]; then
		version="uuu_0.0.0_dev"
		echo "Warning: Using default version: '$version'"
	else
		echo "Version from git tag: '$version'"
	fi
	set -e
	echo "Writing version '$version' to file '$file_to_write'"
	echo "#define GIT_VERSION \"lib$version\"" > $file_to_write
fi
