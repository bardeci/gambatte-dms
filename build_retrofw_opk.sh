#!/bin/sh

BDAT=$(date +"%Y%m%d-%H%M%S")
echo '#define BUILDDATE "'$BDAT'"' >./gambatte_sdl/builddate.h

echo "cd libgambatte && scons"
(cd libgambatte && scons -Q target=retrofw) || exit
echo "cd gambatte_sdl && scons"
(cd gambatte_sdl && scons -Q target=retrofw)
mv gambatte_sdl/gambatte_sdl gambatte_sdl/gambatte-dms.retrofw

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

rm -f gambatte-dms-retrofw-r572u4-$BDAT.opk
mksquashfs ./dist/retrofw/default.retrofw.desktop ./dist/retrofw/gb.retrofw.desktop ./dist/retrofw/gbc.retrofw.desktop ./gambatte_sdl/gambatte-dms.retrofw ./dist/gambatte_dms.png ./dist/gambatte_dmg.png ./dist/gambatte_gbc.png ./dist/manual.txt gambatte-dms-retrofw-r572u4-$BDAT.opk -all-root -no-xattrs -noappend -no-exports

find . -type f -iname gambatte-dms.retrofw -delete
