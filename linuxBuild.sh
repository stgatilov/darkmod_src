#!/bin/bash

mkdir -p ~/.doom3/darkmod

# $@ = pass along the flags like "BUILD=profile" or "BUILD=debug"
time scons -j2 BUILD_GAMEPAK=1 "$@" && cp tdm_game02.pk4 ~/.doom3/darkmod/
mv gamex86-base.so gamex86.so
strip gamex86.so
