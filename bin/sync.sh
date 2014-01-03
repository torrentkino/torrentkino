#!/bin/sh

if [ "x$1" = "xtorrentkino" ]; then
	TGT="../tumbleweed"
elif [ "x$1" = "xtumbleweed" ]; then
	TGT="../torrentkino"
else
	exit
fi

cp -f bin/sync.sh $TGT/bin/
cp -f bin/docs.sh $TGT/bin/
cp -f bin/debian.sh $TGT/bin/

cp -f debian/compat $TGT/debian/
cp -f debian/dirs $TGT/debian/
cp -f debian/gbp.conf $TGT/debian/
cp -f debian/rules $TGT/debian/
cp -f debian/source/format $TGT/debian/source/

cp -f src/ben.c $TGT/src/
cp -f src/ben.h $TGT/src/
cp -f src/conf.c $TGT/src/
cp -f src/conf.h $TGT/src/
cp -f src/fail.c $TGT/src/
cp -f src/fail.h $TGT/src/
cp -f src/file.c $TGT/src/
cp -f src/file.h $TGT/src/
cp -f src/ip.c $TGT/src/
cp -f src/ip.h $TGT/src/
cp -f src/list.c $TGT/src/
cp -f src/list.h $TGT/src/
cp -f src/log.c $TGT/src/
cp -f src/log.h $TGT/src/
cp -f src/main.h $TGT/src/
cp -f src/malloc.c $TGT/src/
cp -f src/malloc.h $TGT/src/
cp -f src/opts.c $TGT/src/
cp -f src/opts.h $TGT/src/
cp -f src/str.c $TGT/src/
cp -f src/str.h $TGT/src/
cp -f src/thrd.c $TGT/src/
cp -f src/thrd.h $TGT/src/
cp -f src/unix.c $TGT/src/
cp -f src/unix.h $TGT/src/
cp -f src/worker.c $TGT/src/
cp -f src/worker.h $TGT/src/
