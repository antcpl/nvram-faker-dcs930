#!/bin/sh

export ARCH=mipsel
TARGET=$1

# Sets up toolchain environment variables for various mips toolchain
# Here we used buildroot toolchain so you must change the name if you use a different toolchain

warn()
{
	echo "$1" >&2
}

if [ ! -z $(which mipsel-buildroot-linux-uclibc-gcc) ];
then
	export CC=$(which mipsel-buildroot-linux-uclibc-gcc)
else
	warn "Not setting CC: can't locate mipsel-buildroot-linux-uclibc-gcc."
fi

if [ ! -z $(which mipsel-buildroot-linux-uclibc-ld) ];
then
	export LD=$(which mipsel-buildroot-linux-uclibc-ld)
else
	warn "Not setting LD: can't locate mipsel-buildroot-linux-uclibc-ld."
fi

if [ ! -z $(which mipsel-buildroot-linux-uclibc-ar) ];
then
	export AR=$(which mipsel-buildroot-linux-uclibc-ar)
else
	warn "Not setting AR: can't locate mipsel-buildroot-linux-uclibc-ld."
fi


if [ ! -z $(which mipsel-buildroot-linux-uclibc-strip) ];
then
	export STRIP=$(which mipsel-buildroot-linux-uclibc-strip)
else
	warn "Not setting STRIP: can't locate mipsel-buildroot-linux-uclibc-strip."
fi

if [ ! -z $(which mipsel-buildroot-linux-uclibc-nm) ];
then
	export NM=$(which mipsel-buildroot-linux-uclibc-nm)
else
	warn "Not setting NM: can't lcoate mipsel-buildroot-linux-uclibc-nm."
fi


make $TARGET || exit $?


