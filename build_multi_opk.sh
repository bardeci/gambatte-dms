#!/bin/sh

BDAT=$(date +"%Y%m%d-%H%M%S")
echo '#define BUILDDATE "'$BDAT'"' >./gambatte_sdl/builddate.h

echo "cd libgambatte && scons"
(cd libgambatte && scons -Q target=retrofw) || exit
echo "cd gambatte_sdl && scons"
(cd gambatte_sdl && scons -Q target=retrofw)
mv gambatte_sdl/gambatte_sdl gambatte_sdl/gambatte.retrofw

echo "cd gambatte_sdl && scons -c"
(cd gambatte_sdl && scons -c)
echo "cd libgambatte && scons -c"
(cd libgambatte && scons -c)
echo "rm -f *gambatte*/config.log"
rm -f *gambatte*/config.log
echo "rm -rf *gambatte*/.scon*"
rm -rf *gambatte*/.scon*
find . -type f -iname \*.o -delete
find . -type f -iname gambatte_sdl -delete

echo "cd libgambatte && scons"
(cd libgambatte && scons -Q target=opendingux) || exit
echo "cd gambatte_sdl && scons"
(cd gambatte_sdl && scons -Q target=opendingux)
mv gambatte_sdl/gambatte_sdl gambatte_sdl/gambatte.opendingux

rm -f gambatte-multi-r572u4-$BDAT.opk

mksquashfs ./dist/gcw0/default.gcw0.desktop ./dist/retrofw/default.retrofw.desktop ./dist/retrofw/gb.retrofw.desktop ./dist/retrofw/gbc.retrofw.desktop ./gambatte_sdl/gambatte.opendingux ./gambatte_sdl/gambatte.retrofw ./dist/gcw0/gambatte.png ./dist/gcw0/manual.txt ./dist/retrofw/gambatte.man.txt gambatte-multi-r572u4-$BDAT.opk -all-root -no-xattrs -noappend -no-exports

echo "cd gambatte_sdl && scons -c"
(cd gambatte_sdl && scons -c)
echo "cd libgambatte && scons -c"
(cd libgambatte && scons -c)
echo "rm -f *gambatte*/config.log"
rm -f *gambatte*/config.log
echo "rm -rf *gambatte*/.scon*"
rm -rf *gambatte*/.scon*
find . -type f -iname \*.o -delete
find . -type f -iname gambatte_sdl -delete

find . -type f -iname gambatte.retrofw -delete
find . -type f -iname gambatte.opendingux -delete
