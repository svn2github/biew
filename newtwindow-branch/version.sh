#!/bin/sh

# define here official release digits. Example: "1.0"
version="1.0.0"

svn_exec=$(LC_ALL=C which svn)
svn_revision=
test -n "$svn_exec" && svn_revision=$(LC_ALL=C svn info 2>/dev/null | grep Revision | cut -d' ' -f2)
test -z $svn_revision && svn_revision=$version || svn_revision="svn.$svn_revision"

cat << EOF > version.h
#ifndef __VERSION_H
#define __VERSION_H

#define VERSION "$svn_revision"

#endif
EOF
