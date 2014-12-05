#!/bin/sh
# $FreeBSD: head/tools/regression/pjdfstest/tests/chmod/05.t 211352 2010-08-15 21:24:17Z pjd $

desc="chmod returns EACCES when search permission is denied for a component of the path prefix"

dir=`dirname $0`
. ${dir}/../misc.sh

if supported lchmod; then
	echo "1..19"
else
	echo "1..14"
fi

n0=`namegen`
n1=`namegen`
n2=`namegen`

expect 0 mkdir ${n0} 0755
cdir=`pwd`
cd ${n0}
expect 0 mkdir ${n1} 0755
expect 0 chown ${n1} 65534 65534
expect 0 -u 65534 -g 65534 create ${n1}/${n2} 0644
expect 0 -u 65534 -g 65534 chmod ${n1}/${n2} 0642
expect 0642 -u 65534 -g 65534 stat ${n1}/${n2} mode
expect 0 chmod ${n1} 0644
expect EACCES -u 65534 -g 65534 chmod ${n1}/${n2} 0620
expect 0 chmod ${n1} 0755
expect 0 -u 65534 -g 65534 chmod ${n1}/${n2} 0420
expect 0420 -u 65534 -g 65534 stat ${n1}/${n2} mode
if supported lchmod; then
	expect 0 chmod ${n1} 0644
	expect EACCES -u 65534 -g 65534 lchmod ${n1}/${n2} 0410
	expect 0 chmod ${n1} 0755
	expect 0 -u 65534 -g 65534 lchmod ${n1}/${n2} 0710
	expect 0710 -u 65534 -g 65534 stat ${n1}/${n2} mode
fi
expect 0 -u 65534 -g 65534 unlink ${n1}/${n2}
expect 0 rmdir ${n1}
cd ${cdir}
expect 0 rmdir ${n0}
