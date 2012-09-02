#!/bin/bash

mkdir -p ~/.doom3/darkmod

# make sure this file exists
touch scons.signatures.dblite

# FAST=true
# --debug=explain

# $@ = pass along the flags like "BUILD=profile" or "BUILD=debug"
time scons -j2 BUILD_GAMEPAK=1 NO_GCH=0 --debug=explain "$@"
mv gamex86-base.so gamex86.so
#strip gamex86.so
#strip thedarkmod.x86

cp thedarkmod.x86 ~/.doom3/darkmod/

# Only if this directory exists
if [ -d ~/games/doom3/ ]; then
  cp thedarkmod.x86 ~/games/doom3/thedarkmod.x86
fi

cp tdm_game02.pk4 ~/.doom3/darkmod/tdm_game02.pk4
if [ -f ~/.doom3/darkmod/gamex86.so ]; then
  rm ~/.doom3/darkmod/gamex86.so
fi

