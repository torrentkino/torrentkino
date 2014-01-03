#!/bin/sh

if [ ! $(which ronn) ]; then
	sudo apt-get install ruby-ronn
fi

if [ "x$1" = "xtorrentkino" ]; then

	ronn < README.md > debian/docs/torrentkino6.1
	ronn < README.md > debian/docs/torrentkino4.1
	ronn < README.md > debian/docs/tk.1

	cat > debian/torrentkino.manpages <<EOF
./debian/docs/torrentkino6.1
./debian/docs/torrentkino4.1
./debian/docs/tk.1
EOF

elif [ "x$1" = "xtumbleweed" ]; then

	ronn < README.md > debian/docs/tumbleweed.1

	cat > debian/tumbleweed.manpages <<EOF
./debian/docs/tumbleweed.1
EOF

else

	exit

fi
