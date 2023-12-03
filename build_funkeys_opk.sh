#!/bin/sh

BDAT=$(date +"%Y%m%d-%H%M%S")
echo '#define BUILDDATE "'$BDAT'"' >./gambatte_sdl/builddate.h

make -f Makefile.funkeys
BDAT=$(date +"%Y%m%d")
rm -f gambatte-dms-funkey-s-r572u4-$BDAT.opk
mksquashfs ./dist/funkeys/default.funkey-s.desktop gambatte-dms.funkeys ./dist/gambatte_dms.png ./dist/manual.txt gambatte-dms-funkey-s-r572u4-$BDAT.opk -all-root -no-xattrs -noappend -no-exports
make -f Makefile.funkeys clean
