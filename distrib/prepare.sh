#!/bin/sh

version=$(echo `cat <../version.sh | grep version= | cut -d '=' -f2 | cut -d '"' -f2`)
rpm_src=$(rpm --eval '%{_sourcedir}')
rpm_home="$rpm_src/../RPMS"
srcdir=$(pwd)
destdir="$srcdir/upload"

make_rpm() {
target_arch=$1
dest_rpm=$rpm_home/$target_arch/beye-$version-1_linux.$target_arch.rpm
rpmbuild -bb --define "version $version" --define "_target_cpu $target_arch" --target $target_arch-linux beye.spec
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
cd $rpm_home/$target_arch
echo `pwd`
mkdir tmp
local srpm="beye-$version-1_linux.$target_arch.rpm"
cp $srpm $destdir
rpm --install --root $rpm_home/$target_arch/tmp --nodeps --ignorearch --ignoreos $srpm
local stgz="beye-$version-1_linux.$target_arch.tgz"
cd tmp
tar c usr | gzip -9 >$stgz
cp $stgz $destdir
cd ..
rm -rf tmp
cd $srcdir
}

if ! test -f "$rpm_src/beye-$version-src.tar.bz2" ; then
echo "Error: Source file: $rpm_src/beye-$version-src.tar.bz2 was not found!"
exit 1
fi

install -p -d $destdir

# build ia32 binaries
make_rpm i686
rpm2tgz i686

# build x86_64 binaries
make_rpm x86_64
rpm2tgz x86_64

#build sources
rpmbuild -bs --define "version $version" mplayerxp.spec
if ! test -f $rpm_home/../SRPMS/mplayerxp-$version-1_linux.src.rpm ; then
echo "beye-$version-1_linux.src.rpm was not found. Exiting..."
exit 1
fi
cd $rpm_home/../SRPMS
cp beye-$version-1_linux.src.rpm $destdir
cd $rpm_src
cp beye-$version-src.tar.bz2 $destdir

echo "Everything seems OK"
