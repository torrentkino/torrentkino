#!/bin/sh

if [ ! $(which ronn) ]; then
	sudo apt-get install ruby-ronn
fi

cp -f docs/torrentkino.md README.md
ronn < docs/torrentkino.md > debian/docs/torrentkino6.1
ronn < docs/torrentkino.md > debian/docs/torrentkino4.1
ronn < docs/torrentkino.md > debian/docs/tk.1
ronn < docs/tumbleweed.md > debian/docs/tumbleweed.1
