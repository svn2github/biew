#!/bin/sh

PACKAGE="beye"

version=$(echo `cat <../version.sh | grep version= | cut -d '=' -f2 | cut -d '"' -f2`)
rpm_src=$(rpm --eval '%{_sourcedir}')
rpm_home="$rpm_src/../RPMS"
srcdir=$(pwd)
destdir="$srcdir/upload"

check_gcc() {
flag=$1
echo "int x;" | gcc $flag - -E
return $?
}

make_rpm() {
target_arch=$1
dest_rpm=$rpm_home/$target_arch/$PACKAGE-$version-1_linux.$target_arch.rpm
rpmbuild -bb -v --define "version $version" --define "_target_cpu $target_arch" --target $target_arch-linux $PACKAGE.spec
if ! test -f $dest_rpm ; then
echo "$dest_rpm was not found. Exiting..."
exit 1
fi
rpm_os=$(rpm -qp --queryformat "%{os}\n" $dest_rpm)
rpm_arch=$(rpm -qp --queryformat "%{arch}\n" $dest_rpm)
if test "$rpm_os" != "linux" ; then
echo "$dest_rpm has wrong OS: $rpm_os. Exiting..."
exit 1
fi
if test "$rpm_arch" != "$target_arch" ; then
echo "$dest_rpm has wrong ARCH: $rpm_arch. Exiting..."
exit 1
fi
echo "$dest_rpm has been built OK"
}

rpm2tgz() {
target_arch=$1
cd 
echo `pwd`
mkdir tmp
local srpm="$PACKAGE-$version-1_linux.$target_arch.rpm"
rpm --install --root $rpm_home/$target_arch/tmp --nodeps --ignorearch --ignoreos $srpm
local stgz="$PACKAGE-$version-1_linux.$target_arch.tgz"
cd tmp
tar c usr | gzip -9 >$stgz
mv $stgz $destdir
mv $rpm_home/$target_arch/$srpm $destdir
cd ..
rm -rf tmp
cd $srcdir
}

if ! test -f "$rpm_src/$PACKAGE-$version-src.tar.bz2" ; then
echo "Warning: Source file: $rpm_src/$PACKAGE-$version-src.tar.bz2 was not found! Build it"
svn export ../ ./$PACKAGE-$version
if [ $? != 0 ] ; then
echo "SVN export failure"
exit
fi
tar c $PACKAGE-$version | bzip2 -9 >$PACKAGE-$version-src.tar.bz2
rm -fr $PACKAGE-$version
mv $PACKAGE-$version-src.tar.bz2 $rpm_src
fi


install -p -d $destdir

if check_gcc "-m32"; then
# build ia32 binaries
make_rpm i686
rpm2tgz i686
fi
if check_gcc "-m64"; then
# build x86_64 binaries
make_rpm x86_64
rpm2tgz x86_64
fi
#build sources
rpmbuild -bs --define "version $version" $PACKAGE.spec
if ! test -f $rpm_home/../SRPMS/$PACKAGE-$version-1_linux.src.rpm ; then
echo "$PACKAGE-$version-1_linux.src.rpm was not found. Exiting..."
exit 1
fi
cd $rpm_home/../SRPMS
mv $PACKAGE-$version-1_linux.src.rpm $destdir
cd $rpm_src
mv $PACKAGE-$version-src.tar.bz2 $destdir

rpmbuild --clean

echo "Everything seems OK"
