#!/bin/sh -x

GITVER=`git log --dense --pretty=oneline -n1 | head -c8`

DATE=`date +%Y%m%d`

if [ ${RELEASE_MAJOR_VERSION} ]
then
MAJOR_VERSION=${RELEASE_MAJOR_VERSION}
else
MAJOR_VERSION=0
fi

if [ $RELEASE_MINOR_VERSION ]
then
MINOR_VERSION=${RELEASE_MINOR_VERSION}
else
MINOR_VERSION=5
fi

echo "#ifndef _VERSION_H_" 
echo "#define _VERSION_H_"
echo "// BSP release version"
echo "#define RELEASE_DATE ${DATE}"
echo "#define RELEASE_TYPE CYGDBG_RELEASE_TYPE"
echo "#define RELEASE_GIT 0x${GITVER}"
echo "#define MAJOR_VERSION $MAJOR_VERSION"
echo "#define MINOR_VERSION $MINOR_VERSION"
echo "#endif  /* _VERSION_H_ */"
