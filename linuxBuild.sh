#!/bin/bash

mkdir -p ~/games/tdm/darkmod

# make sure this file exists
touch scons.signatures.dblite

# FAST=true
# --debug=explain

# $@ = pass along the flags like "BUILD=profile" or "BUILD=debug"
time scons -j2 BUILD_GAMEPAK=1 NO_GCH=0 --debug=explain "$@"
mv gamex86-base.so gamex86.so
#strip gamex86.so
#strip thedarkmod.x86

cp thedarkmod.x86 ~/games/tdm/darkmod/
cp thedarkmod.x86 ~/games/tdm/

cp tdm_game02.pk4 ~/games/tdm/darkmod/tdm_game02.pk4
if [ -f ~/games/tdm/darkmod/gamex86.so ]; then
  rm ~/games/tdm/darkmod/gamex86.so
fi

