#!/bin/sh

# Input parameters
file_to_write="$1"

# Test if we are in a repo
if [ "$(git rev-parse --is-inside-work-tree 2>/dev/null)" = "true" ];
then
	#echo "In a repo"
	# Get the version of the last commit of the repo
	version=`git log -n1 HEAD --pretty=format:%h`
	tag=`git describe --tags`
else
	#echo "Not in a repo"
	version="-unknown"
fi

#echo "version: [$version]"

# Create the C definition
definition="#define GIT_VERSION \"-g$version\""

#echo "definition: [$definition]"

# Write definition to file
echo "$definition" > "$file_to_write"

if [ "${APPVEYOR_BUILD_VERSION}" = "" ];
then
	echo "not build from appveror"
	echo "#define BUILD_VER \"${tag#*_}\"" >> "$file_to_write"
else
	echo "build from appveryor"
	echo "#define BUILD_VER \"${APPVEYOR_BUILD_VERSION}\"" >> "$file_to_write"
fi


