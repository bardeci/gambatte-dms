#!/bin/sh

BDAT=$(date +"%Y%m%d-%H%M%S")

echo '#define BUILDDATE "'$BDAT'"' >./gambatte_sdl/builddate.h

echo "cd libgambatte && scons"
(cd libgambatte && scons -Q target=bittboy) || exit

echo "cd gambatte_sdl && scons"
(cd gambatte_sdl && scons -Q target=bittboy)
