#!/bin/sh

BDAT=$(date +"%Y%m%d-%H%M%S")

echo '#define BUILDDATE "'$BDAT'"' >./gambatte_sdl/builddate.h

echo "cd libgambatte && scons"
(cd libgambatte && scons -Q target=gcw0) || exit

echo "cd gambatte_sdl && scons"
(cd gambatte_sdl && scons -Q target=gcw0)

rm -f gambatte-gcw0-r572u3-$BDAT.opk

mksquashfs ./dist/gcw0/default.gcw0.desktop ./gambatte_sdl/gambatte_sdl ./dist/gcw0/gambatte.png ./dist/gcw0/manual.txt gambatte-gcw0-r572u3-$BDAT.opk -all-root -no-xattrs -noappend -no-exports