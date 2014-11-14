#!/bin/sh

cat <<EOF
###
### Check build environment
###
EOF
dpkg -s build-essential > /dev/null
if [ $? != "0" ]; then
	apt-cache show build-essential
	echo "# sudo apt-get install build-essential"
	exit
fi

dpkg -s debhelper > /dev/null
if [ $? != "0" ]; then
	apt-cache show debhelper
	echo "# sudo apt-get install debhelper"
	exit
fi

dpkg -s libmagic-dev > /dev/null
if [ $? != "0" ]; then
	apt-cache show libmagic-dev
	echo "# sudo apt-get install libmagic-dev"
	exit
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
DEB=$(ls -tr ../torrentkino_*.deb | tail -n 1)
echo "# sudo dpkg -i $DEB"
