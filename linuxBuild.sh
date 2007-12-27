#!/bin/bash
mkdir -p ~/.doom3/darkmod
scons -j2 BUILD_GAMEPAK=1 && cp game01-base.pk4 ~/.doom3/darkmod/
mv gamex86-base.so gamex86.so
strip gamex86.so
