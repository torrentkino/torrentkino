#!/bin/bash

cd src
for D in $(ls); do
	cd $D
	for F in $(ls *.[ch]); do
		indent -linux $F
	done
	cd - 1>/dev/null
done
