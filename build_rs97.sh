#!/bin/sh

BDAT=$(date +"%Y%m%d-%H%M%S")

echo '#define BUILDDATE "'$BDAT'"' >./gambatte_sdl/builddate.h

echo "cd libgambatte && scons"
(cd libgambatte && scons -Q target=rs97) || exit

echo "cd gambatte_sdl && scons"
(cd gambatte_sdl && scons -Q target=rs97)

rm -rf /tmp/.gambatte-ipk/ && mkdir -p /tmp/.gambatte-ipk/root/home/retrofw/emus/gambatte /tmp/.gambatte-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators /tmp/.gambatte-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
cp dist/rs97/gambatte.man.txt dist/rs97/gambatte.png /tmp/.gambatte-ipk/root/home/retrofw/emus/gambatte
cp gambatte_sdl/gambatte_sdl /tmp/.gambatte-ipk/root/home/retrofw/emus/gambatte/gambatte.dge
cp dist/rs97/gambatte.lnk /tmp/.gambatte-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators
cp dist/rs97/gb.gambatte.lnk dist/rs97/gbc.gambatte.lnk /tmp/.gambatte-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
sed "s/^Version:.*/Version: `date +%Y%m%d`/" dist/rs97/control > /tmp/.gambatte-ipk/control
tar --owner=0 --group=0 -czvf /tmp/.gambatte-ipk/control.tar.gz -C /tmp/.gambatte-ipk/ control
tar --owner=0 --group=0 -czvf /tmp/.gambatte-ipk/data.tar.gz -C /tmp/.gambatte-ipk/root/ .
echo 2.0 > /tmp/.gambatte-ipk/debian-binary
ar r gambatte-rs97-r572u3-$BDAT.ipk /tmp/.gambatte-ipk/control.tar.gz /tmp/.gambatte-ipk/data.tar.gz /tmp/.gambatte-ipk/debian-binary
