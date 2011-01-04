#!/bin/sh

# create distribution file
make dist

# create tar archive and signature
tarArchive=encfs-1.5-2.tgz
mv encfs-1.5.tar.gz $tarArchive
# let the user know why they're being asked for a passpharse
echo "Signing tar archive - enter GPG password";
gpg --detach-sign -a $tarArchive

# create rpms
#cp $tarArchive /usr/src/packages/SOURCES
#echo "Building signed RPM files - enter GPG password";
#rpmbuild -ba --sign encfs.spec

# move all distribution files to dist directory
mkdir dist
mv $tarArchive dist
mv $tarArchive.asc dist
#mv /usr/src/packages/SRPMS/encfs-1.5-2.src.rpm dist
#mv /usr/src/packages/RPMS/i586/encfs-1.5-2.i586.rpm dist

# cleanup
#rm /usr/src/packages/SOURCES/$tarArchive

