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

dpkg -s libmagic-dev > /dev/null
if [ $? != "0" ]; then
	sudo apt-get install libmagic-dev
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
DEB=$(ls -1 ../torrentkino_*.deb | sort | tail -n 1)
sudo dpkg -i $DEB


cat <<EOF
###
### Finish
###
EOF
dpkg -l | grep torrentkino
