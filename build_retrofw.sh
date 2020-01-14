#!/bin/sh

BDAT=$(date +"%Y%m%d-%H%M%S")
echo '#define BUILDDATE "'$BDAT'"' >./gambatte_sdl/builddate.h

echo "cd libgambatte && scons"
(cd libgambatte && scons -Q target=retrofw) || exit
echo "cd gambatte_sdl && scons"
(cd gambatte_sdl && scons -Q target=retrofw)

rm -rf /tmp/.gambatte-ipk/ && mkdir -p /tmp/.gambatte-ipk/root/home/retrofw/emus/gambatte /tmp/.gambatte-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators /tmp/.gambatte-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
cp dist/retrofw/gambatte.man.txt dist/retrofw/gambatte.png /tmp/.gambatte-ipk/root/home/retrofw/emus/gambatte
cp gambatte_sdl/gambatte_sdl /tmp/.gambatte-ipk/root/home/retrofw/emus/gambatte/gambatte.dge
cp dist/retrofw/gambatte.lnk /tmp/.gambatte-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators
cp dist/retrofw/gb.gambatte.lnk dist/retrofw/gbc.gambatte.lnk /tmp/.gambatte-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
sed "s/^Version:.*/Version: `date +%Y%m%d`/" dist/retrofw/control > /tmp/.gambatte-ipk/control
tar --owner=0 --group=0 -czvf /tmp/.gambatte-ipk/control.tar.gz -C /tmp/.gambatte-ipk/ control
tar --owner=0 --group=0 -czvf /tmp/.gambatte-ipk/data.tar.gz -C /tmp/.gambatte-ipk/root/ .
echo 2.0 > /tmp/.gambatte-ipk/debian-binary
ar r gambatte-retrofw-r572u3-$BDAT.ipk /tmp/.gambatte-ipk/control.tar.gz /tmp/.gambatte-ipk/data.tar.gz /tmp/.gambatte-ipk/debian-binary

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
