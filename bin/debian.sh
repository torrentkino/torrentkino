#!/bin/sh

cat <<EOF
###
### Check build environment
###
EOF
dpkg -s build-essential > /dev/null
if [ $? != "0" ]; then
	sudo apt-get install build-essential
fi

dpkg -s debhelper > /dev/null
if [ $? != "0" ]; then
	sudo apt-get install debhelper
fi

if [ "x$1" = "xtumbleweed" ]; then
	dpkg -s libmagic-dev > /dev/null
	if [ $? != "0" ]; then
		sudo apt-get install libmagic-dev
	fi
fi

cat <<EOF
###
### Build package
###
EOF
dpkg-buildpackage -us -uc

cat <<EOF
###
### Install package
###
EOF
DEB=$(ls -tr ../*.deb | tail -n 1)
echo "sudo dpkg -i $DEB"


cat <<EOF
###
### Done
###
EOF
