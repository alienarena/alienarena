#!/bin/sh

USAGE="\
Wrapper script for use in build Debian packages.\n\
This script is intended to make calling the commands to build\n\
packages much easier. To use this script, you must be in the \n\
source tree directory, the same as where you need to call dpkg-buildpackage.\n\
Usage: debian/build-package.sh [OPTION] [PARAMETERS]...\n\
\n\
 --help\t\tDisplay this help\n\
 --base\t\tBuild the base packages.\n\
 \t\tBase packages are alienarena2007 and alienarena2007-server\n\
 --data\t\tBuild the game data package, alienarena2007-data\n\
 --clean\tRun debian/rules clean for cleanup.\n\
 --all\t\tBuild all packages available for Alien Arena 2007.\n\
 [PARAMETERS]\tPass extra parameters to be used by dpkg-buildpackage.\n"

BASE=0
DATA=0
CLEAN=0
ALL=0

case "$1" in
	--help)
		echo ${USAGE}
		exit 0
		;;
	--base)
		BASE=1
		shift
		;;
	--data)
		DATA=1
		shift
		;;
	--clean)
		CLEAN=1
		shift
		;;
	--all)
		ALL=1
		shift
		;;
esac

if [ ${BASE} = 1 ]; then
	exec `BUILD_PACKAGE=alienarena2007 dpkg-buildpackage -rfakeroot -I*/.svn/* -I.svn -I*.ico -I*.dll -I*.exe -I*.bat -I*/debian/* -Idebian "$@"`
elif [ ${DATA} = 1 ]; then
	exec `BUILD_PACKAGE=alienarena2007-data dpkg-buildpackage -rfakeroot -I*/.svn/* -I.svn -I*.ico -I*.dll -I*.exe -I*.bat -I*/debian/* -Idebian "$@"`
elif [ ${ALL} = 1 ]; then
	exec `BUILD_PACKAGE=all dpkg-buildpackage -rfakeroot -I*/.svn/* -I.svn -I*.ico -I*.dll -I*.exe -I*.bat -I*/debian/* -Idebian "$@"`
elif [ ${CLEAN} = 1 ]; then
	exec fakeroot debian/rules clean
else
	echo ${USAGE}
fi
