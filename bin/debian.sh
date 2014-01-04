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
if [ "x$1" = "xtorrentkino" ]; then
	DEB=$(ls -tr ../torrentkino_*.deb | tail -n 1)
elif [ "x$1" = "xtumbleweed" ]; then
	DEB=$(ls -tr ../tumbleweed_*.deb | tail -n 1)
else
	exit
fi
sudo dpkg -i $DEB
