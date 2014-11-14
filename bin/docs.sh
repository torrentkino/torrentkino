#!/bin/sh

if [ ! $(which ronn) ]; then
	apt-cache show ruby-ronn
	echo "# sudo apt-get install ruby-ronn"
	exit
fi

ronn < README.md > debian/docs/tk6.1
ronn < README.md > debian/docs/tk4.1
ronn < README.md > debian/docs/tumbleweed.1

cat > debian/torrentkino.manpages <<EOF
./debian/docs/tk6.1
./debian/docs/tk4.1
./debian/docs/tumbleweed.1
EOF
